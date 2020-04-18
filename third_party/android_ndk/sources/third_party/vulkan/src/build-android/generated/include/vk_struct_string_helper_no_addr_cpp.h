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
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <string>
#include "vk_enum_string_helper.h"
namespace StreamControl
{
bool writeAddress = true;
template <typename T>
std::ostream& operator<< (std::ostream &out, T const* pointer)
{
    if(writeAddress)
    {
        out.operator<<(pointer);
    }
    else
    {
        std::operator<<(out, "address");
    }
    return out;
}
std::ostream& operator<<(std::ostream &out, char const*const s)
{
    return std::operator<<(out, s);
}
}

std::string dynamic_display(const void* pStruct, const std::string prefix);
// CODEGEN : file ../vk_helper.py line #889
std::string vk_print_vkallocationcallbacks(const VkAllocationCallbacks* pStruct, const std::string prefix);
#ifdef VK_USE_PLATFORM_ANDROID_KHR
std::string vk_print_vkandroidsurfacecreateinfokhr(const VkAndroidSurfaceCreateInfoKHR* pStruct, const std::string prefix);
#endif //VK_USE_PLATFORM_ANDROID_KHR
std::string vk_print_vkapplicationinfo(const VkApplicationInfo* pStruct, const std::string prefix);
std::string vk_print_vkattachmentdescription(const VkAttachmentDescription* pStruct, const std::string prefix);
std::string vk_print_vkattachmentreference(const VkAttachmentReference* pStruct, const std::string prefix);
std::string vk_print_vkbindsparseinfo(const VkBindSparseInfo* pStruct, const std::string prefix);
std::string vk_print_vkbuffercopy(const VkBufferCopy* pStruct, const std::string prefix);
std::string vk_print_vkbuffercreateinfo(const VkBufferCreateInfo* pStruct, const std::string prefix);
std::string vk_print_vkbufferimagecopy(const VkBufferImageCopy* pStruct, const std::string prefix);
std::string vk_print_vkbuffermemorybarrier(const VkBufferMemoryBarrier* pStruct, const std::string prefix);
std::string vk_print_vkbufferviewcreateinfo(const VkBufferViewCreateInfo* pStruct, const std::string prefix);
std::string vk_print_vkclearattachment(const VkClearAttachment* pStruct, const std::string prefix);
std::string vk_print_vkclearcolorvalue(const VkClearColorValue* pStruct, const std::string prefix);
std::string vk_print_vkcleardepthstencilvalue(const VkClearDepthStencilValue* pStruct, const std::string prefix);
std::string vk_print_vkclearrect(const VkClearRect* pStruct, const std::string prefix);
std::string vk_print_vkclearvalue(const VkClearValue* pStruct, const std::string prefix);
std::string vk_print_vkcommandbufferallocateinfo(const VkCommandBufferAllocateInfo* pStruct, const std::string prefix);
std::string vk_print_vkcommandbufferbegininfo(const VkCommandBufferBeginInfo* pStruct, const std::string prefix);
std::string vk_print_vkcommandbufferinheritanceinfo(const VkCommandBufferInheritanceInfo* pStruct, const std::string prefix);
std::string vk_print_vkcommandpoolcreateinfo(const VkCommandPoolCreateInfo* pStruct, const std::string prefix);
std::string vk_print_vkcomponentmapping(const VkComponentMapping* pStruct, const std::string prefix);
std::string vk_print_vkcomputepipelinecreateinfo(const VkComputePipelineCreateInfo* pStruct, const std::string prefix);
std::string vk_print_vkcopydescriptorset(const VkCopyDescriptorSet* pStruct, const std::string prefix);
std::string vk_print_vkdebugmarkermarkerinfoext(const VkDebugMarkerMarkerInfoEXT* pStruct, const std::string prefix);
std::string vk_print_vkdebugmarkerobjectnameinfoext(const VkDebugMarkerObjectNameInfoEXT* pStruct, const std::string prefix);
std::string vk_print_vkdebugmarkerobjecttaginfoext(const VkDebugMarkerObjectTagInfoEXT* pStruct, const std::string prefix);
std::string vk_print_vkdebugreportcallbackcreateinfoext(const VkDebugReportCallbackCreateInfoEXT* pStruct, const std::string prefix);
std::string vk_print_vkdedicatedallocationbuffercreateinfonv(const VkDedicatedAllocationBufferCreateInfoNV* pStruct, const std::string prefix);
std::string vk_print_vkdedicatedallocationimagecreateinfonv(const VkDedicatedAllocationImageCreateInfoNV* pStruct, const std::string prefix);
std::string vk_print_vkdedicatedallocationmemoryallocateinfonv(const VkDedicatedAllocationMemoryAllocateInfoNV* pStruct, const std::string prefix);
std::string vk_print_vkdescriptorbufferinfo(const VkDescriptorBufferInfo* pStruct, const std::string prefix);
std::string vk_print_vkdescriptorimageinfo(const VkDescriptorImageInfo* pStruct, const std::string prefix);
std::string vk_print_vkdescriptorpoolcreateinfo(const VkDescriptorPoolCreateInfo* pStruct, const std::string prefix);
std::string vk_print_vkdescriptorpoolsize(const VkDescriptorPoolSize* pStruct, const std::string prefix);
std::string vk_print_vkdescriptorsetallocateinfo(const VkDescriptorSetAllocateInfo* pStruct, const std::string prefix);
std::string vk_print_vkdescriptorsetlayoutbinding(const VkDescriptorSetLayoutBinding* pStruct, const std::string prefix);
std::string vk_print_vkdescriptorsetlayoutcreateinfo(const VkDescriptorSetLayoutCreateInfo* pStruct, const std::string prefix);
std::string vk_print_vkdevicecreateinfo(const VkDeviceCreateInfo* pStruct, const std::string prefix);
std::string vk_print_vkdevicequeuecreateinfo(const VkDeviceQueueCreateInfo* pStruct, const std::string prefix);
std::string vk_print_vkdispatchindirectcommand(const VkDispatchIndirectCommand* pStruct, const std::string prefix);
std::string vk_print_vkdisplaymodecreateinfokhr(const VkDisplayModeCreateInfoKHR* pStruct, const std::string prefix);
std::string vk_print_vkdisplaymodeparameterskhr(const VkDisplayModeParametersKHR* pStruct, const std::string prefix);
std::string vk_print_vkdisplaymodepropertieskhr(const VkDisplayModePropertiesKHR* pStruct, const std::string prefix);
std::string vk_print_vkdisplayplanecapabilitieskhr(const VkDisplayPlaneCapabilitiesKHR* pStruct, const std::string prefix);
std::string vk_print_vkdisplayplanepropertieskhr(const VkDisplayPlanePropertiesKHR* pStruct, const std::string prefix);
std::string vk_print_vkdisplaypresentinfokhr(const VkDisplayPresentInfoKHR* pStruct, const std::string prefix);
std::string vk_print_vkdisplaypropertieskhr(const VkDisplayPropertiesKHR* pStruct, const std::string prefix);
std::string vk_print_vkdisplaysurfacecreateinfokhr(const VkDisplaySurfaceCreateInfoKHR* pStruct, const std::string prefix);
std::string vk_print_vkdrawindexedindirectcommand(const VkDrawIndexedIndirectCommand* pStruct, const std::string prefix);
std::string vk_print_vkdrawindirectcommand(const VkDrawIndirectCommand* pStruct, const std::string prefix);
std::string vk_print_vkeventcreateinfo(const VkEventCreateInfo* pStruct, const std::string prefix);
std::string vk_print_vkexportmemoryallocateinfonv(const VkExportMemoryAllocateInfoNV* pStruct, const std::string prefix);
#ifdef VK_USE_PLATFORM_WIN32_KHR
std::string vk_print_vkexportmemorywin32handleinfonv(const VkExportMemoryWin32HandleInfoNV* pStruct, const std::string prefix);
#endif //VK_USE_PLATFORM_WIN32_KHR
std::string vk_print_vkextensionproperties(const VkExtensionProperties* pStruct, const std::string prefix);
std::string vk_print_vkextent2d(const VkExtent2D* pStruct, const std::string prefix);
std::string vk_print_vkextent3d(const VkExtent3D* pStruct, const std::string prefix);
std::string vk_print_vkexternalimageformatpropertiesnv(const VkExternalImageFormatPropertiesNV* pStruct, const std::string prefix);
std::string vk_print_vkexternalmemoryimagecreateinfonv(const VkExternalMemoryImageCreateInfoNV* pStruct, const std::string prefix);
std::string vk_print_vkfencecreateinfo(const VkFenceCreateInfo* pStruct, const std::string prefix);
std::string vk_print_vkformatproperties(const VkFormatProperties* pStruct, const std::string prefix);
std::string vk_print_vkframebuffercreateinfo(const VkFramebufferCreateInfo* pStruct, const std::string prefix);
std::string vk_print_vkgraphicspipelinecreateinfo(const VkGraphicsPipelineCreateInfo* pStruct, const std::string prefix);
std::string vk_print_vkimageblit(const VkImageBlit* pStruct, const std::string prefix);
std::string vk_print_vkimagecopy(const VkImageCopy* pStruct, const std::string prefix);
std::string vk_print_vkimagecreateinfo(const VkImageCreateInfo* pStruct, const std::string prefix);
std::string vk_print_vkimageformatproperties(const VkImageFormatProperties* pStruct, const std::string prefix);
std::string vk_print_vkimagememorybarrier(const VkImageMemoryBarrier* pStruct, const std::string prefix);
std::string vk_print_vkimageresolve(const VkImageResolve* pStruct, const std::string prefix);
std::string vk_print_vkimagesubresource(const VkImageSubresource* pStruct, const std::string prefix);
std::string vk_print_vkimagesubresourcelayers(const VkImageSubresourceLayers* pStruct, const std::string prefix);
std::string vk_print_vkimagesubresourcerange(const VkImageSubresourceRange* pStruct, const std::string prefix);
std::string vk_print_vkimageviewcreateinfo(const VkImageViewCreateInfo* pStruct, const std::string prefix);
#ifdef VK_USE_PLATFORM_WIN32_KHR
std::string vk_print_vkimportmemorywin32handleinfonv(const VkImportMemoryWin32HandleInfoNV* pStruct, const std::string prefix);
#endif //VK_USE_PLATFORM_WIN32_KHR
std::string vk_print_vkinstancecreateinfo(const VkInstanceCreateInfo* pStruct, const std::string prefix);
std::string vk_print_vklayerproperties(const VkLayerProperties* pStruct, const std::string prefix);
std::string vk_print_vkmappedmemoryrange(const VkMappedMemoryRange* pStruct, const std::string prefix);
std::string vk_print_vkmemoryallocateinfo(const VkMemoryAllocateInfo* pStruct, const std::string prefix);
std::string vk_print_vkmemorybarrier(const VkMemoryBarrier* pStruct, const std::string prefix);
std::string vk_print_vkmemoryheap(const VkMemoryHeap* pStruct, const std::string prefix);
std::string vk_print_vkmemoryrequirements(const VkMemoryRequirements* pStruct, const std::string prefix);
std::string vk_print_vkmemorytype(const VkMemoryType* pStruct, const std::string prefix);
#ifdef VK_USE_PLATFORM_MIR_KHR
std::string vk_print_vkmirsurfacecreateinfokhr(const VkMirSurfaceCreateInfoKHR* pStruct, const std::string prefix);
#endif //VK_USE_PLATFORM_MIR_KHR
std::string vk_print_vkoffset2d(const VkOffset2D* pStruct, const std::string prefix);
std::string vk_print_vkoffset3d(const VkOffset3D* pStruct, const std::string prefix);
std::string vk_print_vkphysicaldevicefeatures(const VkPhysicalDeviceFeatures* pStruct, const std::string prefix);
std::string vk_print_vkphysicaldevicelimits(const VkPhysicalDeviceLimits* pStruct, const std::string prefix);
std::string vk_print_vkphysicaldevicememoryproperties(const VkPhysicalDeviceMemoryProperties* pStruct, const std::string prefix);
std::string vk_print_vkphysicaldeviceproperties(const VkPhysicalDeviceProperties* pStruct, const std::string prefix);
std::string vk_print_vkphysicaldevicesparseproperties(const VkPhysicalDeviceSparseProperties* pStruct, const std::string prefix);
std::string vk_print_vkpipelinecachecreateinfo(const VkPipelineCacheCreateInfo* pStruct, const std::string prefix);
std::string vk_print_vkpipelinecolorblendattachmentstate(const VkPipelineColorBlendAttachmentState* pStruct, const std::string prefix);
std::string vk_print_vkpipelinecolorblendstatecreateinfo(const VkPipelineColorBlendStateCreateInfo* pStruct, const std::string prefix);
std::string vk_print_vkpipelinedepthstencilstatecreateinfo(const VkPipelineDepthStencilStateCreateInfo* pStruct, const std::string prefix);
std::string vk_print_vkpipelinedynamicstatecreateinfo(const VkPipelineDynamicStateCreateInfo* pStruct, const std::string prefix);
std::string vk_print_vkpipelineinputassemblystatecreateinfo(const VkPipelineInputAssemblyStateCreateInfo* pStruct, const std::string prefix);
std::string vk_print_vkpipelinelayoutcreateinfo(const VkPipelineLayoutCreateInfo* pStruct, const std::string prefix);
std::string vk_print_vkpipelinemultisamplestatecreateinfo(const VkPipelineMultisampleStateCreateInfo* pStruct, const std::string prefix);
std::string vk_print_vkpipelinerasterizationstatecreateinfo(const VkPipelineRasterizationStateCreateInfo* pStruct, const std::string prefix);
std::string vk_print_vkpipelinerasterizationstaterasterizationorderamd(const VkPipelineRasterizationStateRasterizationOrderAMD* pStruct, const std::string prefix);
std::string vk_print_vkpipelineshaderstagecreateinfo(const VkPipelineShaderStageCreateInfo* pStruct, const std::string prefix);
std::string vk_print_vkpipelinetessellationstatecreateinfo(const VkPipelineTessellationStateCreateInfo* pStruct, const std::string prefix);
std::string vk_print_vkpipelinevertexinputstatecreateinfo(const VkPipelineVertexInputStateCreateInfo* pStruct, const std::string prefix);
std::string vk_print_vkpipelineviewportstatecreateinfo(const VkPipelineViewportStateCreateInfo* pStruct, const std::string prefix);
std::string vk_print_vkpresentinfokhr(const VkPresentInfoKHR* pStruct, const std::string prefix);
std::string vk_print_vkpushconstantrange(const VkPushConstantRange* pStruct, const std::string prefix);
std::string vk_print_vkquerypoolcreateinfo(const VkQueryPoolCreateInfo* pStruct, const std::string prefix);
std::string vk_print_vkqueuefamilyproperties(const VkQueueFamilyProperties* pStruct, const std::string prefix);
std::string vk_print_vkrect2d(const VkRect2D* pStruct, const std::string prefix);
std::string vk_print_vkrenderpassbegininfo(const VkRenderPassBeginInfo* pStruct, const std::string prefix);
std::string vk_print_vkrenderpasscreateinfo(const VkRenderPassCreateInfo* pStruct, const std::string prefix);
std::string vk_print_vksamplercreateinfo(const VkSamplerCreateInfo* pStruct, const std::string prefix);
std::string vk_print_vksemaphorecreateinfo(const VkSemaphoreCreateInfo* pStruct, const std::string prefix);
std::string vk_print_vkshadermodulecreateinfo(const VkShaderModuleCreateInfo* pStruct, const std::string prefix);
std::string vk_print_vksparsebuffermemorybindinfo(const VkSparseBufferMemoryBindInfo* pStruct, const std::string prefix);
std::string vk_print_vksparseimageformatproperties(const VkSparseImageFormatProperties* pStruct, const std::string prefix);
std::string vk_print_vksparseimagememorybind(const VkSparseImageMemoryBind* pStruct, const std::string prefix);
std::string vk_print_vksparseimagememorybindinfo(const VkSparseImageMemoryBindInfo* pStruct, const std::string prefix);
std::string vk_print_vksparseimagememoryrequirements(const VkSparseImageMemoryRequirements* pStruct, const std::string prefix);
std::string vk_print_vksparseimageopaquememorybindinfo(const VkSparseImageOpaqueMemoryBindInfo* pStruct, const std::string prefix);
std::string vk_print_vksparsememorybind(const VkSparseMemoryBind* pStruct, const std::string prefix);
std::string vk_print_vkspecializationinfo(const VkSpecializationInfo* pStruct, const std::string prefix);
std::string vk_print_vkspecializationmapentry(const VkSpecializationMapEntry* pStruct, const std::string prefix);
std::string vk_print_vkstencilopstate(const VkStencilOpState* pStruct, const std::string prefix);
std::string vk_print_vksubmitinfo(const VkSubmitInfo* pStruct, const std::string prefix);
std::string vk_print_vksubpassdependency(const VkSubpassDependency* pStruct, const std::string prefix);
std::string vk_print_vksubpassdescription(const VkSubpassDescription* pStruct, const std::string prefix);
std::string vk_print_vksubresourcelayout(const VkSubresourceLayout* pStruct, const std::string prefix);
std::string vk_print_vksurfacecapabilitieskhr(const VkSurfaceCapabilitiesKHR* pStruct, const std::string prefix);
std::string vk_print_vksurfaceformatkhr(const VkSurfaceFormatKHR* pStruct, const std::string prefix);
std::string vk_print_vkswapchaincreateinfokhr(const VkSwapchainCreateInfoKHR* pStruct, const std::string prefix);
std::string vk_print_vkvalidationflagsext(const VkValidationFlagsEXT* pStruct, const std::string prefix);
std::string vk_print_vkvertexinputattributedescription(const VkVertexInputAttributeDescription* pStruct, const std::string prefix);
std::string vk_print_vkvertexinputbindingdescription(const VkVertexInputBindingDescription* pStruct, const std::string prefix);
std::string vk_print_vkviewport(const VkViewport* pStruct, const std::string prefix);
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
std::string vk_print_vkwaylandsurfacecreateinfokhr(const VkWaylandSurfaceCreateInfoKHR* pStruct, const std::string prefix);
#endif //VK_USE_PLATFORM_WAYLAND_KHR
#ifdef VK_USE_PLATFORM_WIN32_KHR
std::string vk_print_vkwin32keyedmutexacquirereleaseinfonv(const VkWin32KeyedMutexAcquireReleaseInfoNV* pStruct, const std::string prefix);
#endif //VK_USE_PLATFORM_WIN32_KHR
#ifdef VK_USE_PLATFORM_WIN32_KHR
std::string vk_print_vkwin32surfacecreateinfokhr(const VkWin32SurfaceCreateInfoKHR* pStruct, const std::string prefix);
#endif //VK_USE_PLATFORM_WIN32_KHR
std::string vk_print_vkwritedescriptorset(const VkWriteDescriptorSet* pStruct, const std::string prefix);
#ifdef VK_USE_PLATFORM_XCB_KHR
std::string vk_print_vkxcbsurfacecreateinfokhr(const VkXcbSurfaceCreateInfoKHR* pStruct, const std::string prefix);
#endif //VK_USE_PLATFORM_XCB_KHR
#ifdef VK_USE_PLATFORM_XLIB_KHR
std::string vk_print_vkxlibsurfacecreateinfokhr(const VkXlibSurfaceCreateInfoKHR* pStruct, const std::string prefix);
#endif //VK_USE_PLATFORM_XLIB_KHR


// CODEGEN : file ../vk_helper.py line #897
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkallocationcallbacks(const VkAllocationCallbacks* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[6];
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pUserData;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->pfnAllocation;
// CODEGEN : file ../vk_helper.py line #1086
    ss[2] << "0x" << pStruct->pfnReallocation;
// CODEGEN : file ../vk_helper.py line #1086
    ss[3] << "0x" << pStruct->pfnFree;
// CODEGEN : file ../vk_helper.py line #1086
    ss[4] << "0x" << pStruct->pfnInternalAllocation;
// CODEGEN : file ../vk_helper.py line #1086
    ss[5] << "0x" << pStruct->pfnInternalFree;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "pUserData = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "pfnAllocation = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "pfnReallocation = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "pfnFree = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "pfnInternalAllocation = " + ss[4].str() + "\n";
    final_str = final_str + prefix + "pfnInternalFree = " + ss[5].str() + "\n";
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
#ifdef VK_USE_PLATFORM_ANDROID_KHR
std::string vk_print_vkandroidsurfacecreateinfokhr(const VkAndroidSurfaceCreateInfoKHR* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[3];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->flags;
// CODEGEN : file ../vk_helper.py line #1086
    ss[2] << "0x" << pStruct->window;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "flags = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "window = " + ss[2].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
#endif //VK_USE_PLATFORM_ANDROID_KHR
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkapplicationinfo(const VkApplicationInfo* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[6];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1076
    if (pStruct->pApplicationName != NULL) {
        ss[1] << pStruct->pApplicationName;
     } else {
        ss[1] << "";
     }
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[2] << pStruct->applicationVersion;
// CODEGEN : file ../vk_helper.py line #1076
    if (pStruct->pEngineName != NULL) {
        ss[3] << pStruct->pEngineName;
     } else {
        ss[3] << "";
     }
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[4] << pStruct->engineVersion;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[5] << pStruct->apiVersion;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "pApplicationName = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "applicationVersion = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "pEngineName = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "engineVersion = " + ss[4].str() + "\n";
    final_str = final_str + prefix + "apiVersion = " + ss[5].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkattachmentdescription(const VkAttachmentDescription* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[1];
// CODEGEN : file ../vk_helper.py line #1086
    ss[0] << "0x" << pStruct->flags;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "flags = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "format = " + string_VkFormat(pStruct->format) + "\n";
    final_str = final_str + prefix + "samples = " + string_VkSampleCountFlagBits(pStruct->samples) + "\n";
    final_str = final_str + prefix + "loadOp = " + string_VkAttachmentLoadOp(pStruct->loadOp) + "\n";
    final_str = final_str + prefix + "storeOp = " + string_VkAttachmentStoreOp(pStruct->storeOp) + "\n";
    final_str = final_str + prefix + "stencilLoadOp = " + string_VkAttachmentLoadOp(pStruct->stencilLoadOp) + "\n";
    final_str = final_str + prefix + "stencilStoreOp = " + string_VkAttachmentStoreOp(pStruct->stencilStoreOp) + "\n";
    final_str = final_str + prefix + "initialLayout = " + string_VkImageLayout(pStruct->initialLayout) + "\n";
    final_str = final_str + prefix + "finalLayout = " + string_VkImageLayout(pStruct->finalLayout) + "\n";
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkattachmentreference(const VkAttachmentReference* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[1];
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[0] << pStruct->attachment;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "attachment = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "layout = " + string_VkImageLayout(pStruct->layout) + "\n";
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkbindsparseinfo(const VkBindSparseInfo* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[11];
    string stp_strs[6];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
        stp_strs[1] = "";
        stringstream index_ss;
        if (pStruct->pWaitSemaphores) {
            for (uint32_t i = 0; i < pStruct->waitSemaphoreCount; i++) {
                index_ss.str("");
                index_ss << i;
// CODEGEN : file ../vk_helper.py line #990
                ss[1] << "0x" << pStruct->pWaitSemaphores[i];
                stp_strs[1] += " " + prefix + "pWaitSemaphores[" + index_ss.str() + "].handle = " + ss[1].str() + "\n";
// CODEGEN : file ../vk_helper.py line #1000
                ss[1].str("");
            }
        }
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
        stp_strs[2] = "";
        if (pStruct->pBufferBinds) {
            for (uint32_t i = 0; i < pStruct->bufferBindCount; i++) {
                index_ss.str("");
                index_ss << i;
// CODEGEN : file ../vk_helper.py line #980
                ss[2] << "0x" << &pStruct->pBufferBinds[i];
                tmp_str = vk_print_vksparsebuffermemorybindinfo(&pStruct->pBufferBinds[i], extra_indent);
// CODEGEN : file ../vk_helper.py line #984
                stp_strs[2] += " " + prefix + "pBufferBinds[" + index_ss.str() + "] (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1000
                ss[2].str("");
            }
        }
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
        stp_strs[3] = "";
        if (pStruct->pImageOpaqueBinds) {
            for (uint32_t i = 0; i < pStruct->imageOpaqueBindCount; i++) {
                index_ss.str("");
                index_ss << i;
// CODEGEN : file ../vk_helper.py line #980
                ss[3] << "0x" << &pStruct->pImageOpaqueBinds[i];
                tmp_str = vk_print_vksparseimageopaquememorybindinfo(&pStruct->pImageOpaqueBinds[i], extra_indent);
// CODEGEN : file ../vk_helper.py line #984
                stp_strs[3] += " " + prefix + "pImageOpaqueBinds[" + index_ss.str() + "] (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1000
                ss[3].str("");
            }
        }
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
        stp_strs[4] = "";
        if (pStruct->pImageBinds) {
            for (uint32_t i = 0; i < pStruct->imageBindCount; i++) {
                index_ss.str("");
                index_ss << i;
// CODEGEN : file ../vk_helper.py line #980
                ss[4] << "0x" << &pStruct->pImageBinds[i];
                tmp_str = vk_print_vksparseimagememorybindinfo(&pStruct->pImageBinds[i], extra_indent);
// CODEGEN : file ../vk_helper.py line #984
                stp_strs[4] += " " + prefix + "pImageBinds[" + index_ss.str() + "] (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1000
                ss[4].str("");
            }
        }
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
        stp_strs[5] = "";
        if (pStruct->pSignalSemaphores) {
            for (uint32_t i = 0; i < pStruct->signalSemaphoreCount; i++) {
                index_ss.str("");
                index_ss << i;
// CODEGEN : file ../vk_helper.py line #990
                ss[5] << "0x" << pStruct->pSignalSemaphores[i];
                stp_strs[5] += " " + prefix + "pSignalSemaphores[" + index_ss.str() + "].handle = " + ss[5].str() + "\n";
// CODEGEN : file ../vk_helper.py line #1000
                ss[5].str("");
            }
        }
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[1] << pStruct->waitSemaphoreCount;
// CODEGEN : file ../vk_helper.py line #1061
    ss[2] << "0x" << (void*)pStruct->pWaitSemaphores;
// CODEGEN : file ../vk_helper.py line #1090: NB: Edit here to choose hex vs dec output by variable name
    ss[3] << "0x" << pStruct->bufferBindCount;
// CODEGEN : file ../vk_helper.py line #1061
    ss[4] << "0x" << (void*)pStruct->pBufferBinds;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[5] << pStruct->imageOpaqueBindCount;
// CODEGEN : file ../vk_helper.py line #1061
    ss[6] << "0x" << (void*)pStruct->pImageOpaqueBinds;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[7] << pStruct->imageBindCount;
// CODEGEN : file ../vk_helper.py line #1061
    ss[8] << "0x" << (void*)pStruct->pImageBinds;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[9] << pStruct->signalSemaphoreCount;
// CODEGEN : file ../vk_helper.py line #1061
    ss[10] << "0x" << (void*)pStruct->pSignalSemaphores;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "waitSemaphoreCount = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "pWaitSemaphores = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "bufferBindCount = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "pBufferBinds = " + ss[4].str() + "\n";
    final_str = final_str + prefix + "imageOpaqueBindCount = " + ss[5].str() + "\n";
    final_str = final_str + prefix + "pImageOpaqueBinds = " + ss[6].str() + "\n";
    final_str = final_str + prefix + "imageBindCount = " + ss[7].str() + "\n";
    final_str = final_str + prefix + "pImageBinds = " + ss[8].str() + "\n";
    final_str = final_str + prefix + "signalSemaphoreCount = " + ss[9].str() + "\n";
    final_str = final_str + prefix + "pSignalSemaphores = " + ss[10].str() + "\n";
    final_str = final_str + stp_strs[5] + stp_strs[4] + stp_strs[3] + stp_strs[2] + stp_strs[1] + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkbuffercopy(const VkBufferCopy* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[3];
// CODEGEN : file ../vk_helper.py line #1086
    ss[0] << "0x" << pStruct->srcOffset;
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->dstOffset;
// CODEGEN : file ../vk_helper.py line #1086
    ss[2] << "0x" << pStruct->size;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "srcOffset = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "dstOffset = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "size = " + ss[2].str() + "\n";
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkbuffercreateinfo(const VkBufferCreateInfo* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[6];
    string stp_strs[2];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
        stp_strs[1] = "";
        stringstream index_ss;
        if (pStruct->sharingMode == VK_SHARING_MODE_CONCURRENT) {
            if (pStruct->pQueueFamilyIndices) {
                for (uint32_t i = 0; i < pStruct->queueFamilyIndexCount; i++) {
                    index_ss.str("");
                    index_ss << i;
// CODEGEN : file ../vk_helper.py line #990
                    ss[1] << "0x" << pStruct->pQueueFamilyIndices[i];
                    stp_strs[1] += " " + prefix + "pQueueFamilyIndices[" + index_ss.str() + "] = " + ss[1].str() + "\n";
// CODEGEN : file ../vk_helper.py line #1000
                    ss[1].str("");
                }
            }
        }
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->flags;
// CODEGEN : file ../vk_helper.py line #1086
    ss[2] << "0x" << pStruct->size;
// CODEGEN : file ../vk_helper.py line #1086
    ss[3] << "0x" << pStruct->usage;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[4] << pStruct->queueFamilyIndexCount;
// CODEGEN : file ../vk_helper.py line #1061
    ss[5] << "0x" << (void*)pStruct->pQueueFamilyIndices;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "flags = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "size = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "usage = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "sharingMode = " + string_VkSharingMode(pStruct->sharingMode) + "\n";
    final_str = final_str + prefix + "queueFamilyIndexCount = " + ss[4].str() + "\n";
    final_str = final_str + prefix + "pQueueFamilyIndices = " + ss[5].str() + "\n";
    final_str = final_str + stp_strs[1] + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkbufferimagecopy(const VkBufferImageCopy* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[6];
    string stp_strs[3];
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkimagesubresourcelayers(&pStruct->imageSubresource, extra_indent);
    ss[0] << "0x" << &pStruct->imageSubresource;
    stp_strs[0] = " " + prefix + "imageSubresource (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[0].str("");
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkoffset3d(&pStruct->imageOffset, extra_indent);
    ss[1] << "0x" << &pStruct->imageOffset;
    stp_strs[1] = " " + prefix + "imageOffset (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[1].str("");
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkextent3d(&pStruct->imageExtent, extra_indent);
    ss[2] << "0x" << &pStruct->imageExtent;
    stp_strs[2] = " " + prefix + "imageExtent (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[2].str("");
// CODEGEN : file ../vk_helper.py line #1086
    ss[0] << "0x" << pStruct->bufferOffset;
// CODEGEN : file ../vk_helper.py line #1090: NB: Edit here to choose hex vs dec output by variable name
    ss[1] << "0x" << pStruct->bufferRowLength;
// CODEGEN : file ../vk_helper.py line #1090: NB: Edit here to choose hex vs dec output by variable name
    ss[2] << "0x" << pStruct->bufferImageHeight;
// CODEGEN : file ../vk_helper.py line #1055
    ss[3].str("addr");
// CODEGEN : file ../vk_helper.py line #1055
    ss[4].str("addr");
// CODEGEN : file ../vk_helper.py line #1055
    ss[5].str("addr");
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "bufferOffset = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "bufferRowLength = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "bufferImageHeight = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "imageSubresource = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "imageOffset = " + ss[4].str() + "\n";
    final_str = final_str + prefix + "imageExtent = " + ss[5].str() + "\n";
    final_str = final_str + stp_strs[2] + stp_strs[1] + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkbuffermemorybarrier(const VkBufferMemoryBarrier* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[8];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->srcAccessMask;
// CODEGEN : file ../vk_helper.py line #1086
    ss[2] << "0x" << pStruct->dstAccessMask;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[3] << pStruct->srcQueueFamilyIndex;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[4] << pStruct->dstQueueFamilyIndex;
// CODEGEN : file ../vk_helper.py line #1086
    ss[5] << "0x" << pStruct->buffer;
// CODEGEN : file ../vk_helper.py line #1086
    ss[6] << "0x" << pStruct->offset;
// CODEGEN : file ../vk_helper.py line #1086
    ss[7] << "0x" << pStruct->size;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "srcAccessMask = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "dstAccessMask = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "srcQueueFamilyIndex = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "dstQueueFamilyIndex = " + ss[4].str() + "\n";
    final_str = final_str + prefix + "buffer = " + ss[5].str() + "\n";
    final_str = final_str + prefix + "offset = " + ss[6].str() + "\n";
    final_str = final_str + prefix + "size = " + ss[7].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkbufferviewcreateinfo(const VkBufferViewCreateInfo* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[5];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->flags;
// CODEGEN : file ../vk_helper.py line #1086
    ss[2] << "0x" << pStruct->buffer;
// CODEGEN : file ../vk_helper.py line #1086
    ss[3] << "0x" << pStruct->offset;
// CODEGEN : file ../vk_helper.py line #1086
    ss[4] << "0x" << pStruct->range;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "flags = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "buffer = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "format = " + string_VkFormat(pStruct->format) + "\n";
    final_str = final_str + prefix + "offset = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "range = " + ss[4].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkclearattachment(const VkClearAttachment* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[3];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkclearvalue(&pStruct->clearValue, extra_indent);
    ss[0] << "0x" << &pStruct->clearValue;
    stp_strs[0] = " " + prefix + "clearValue (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[0].str("");
// CODEGEN : file ../vk_helper.py line #1086
    ss[0] << "0x" << pStruct->aspectMask;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[1] << pStruct->colorAttachment;
// CODEGEN : file ../vk_helper.py line #1055
    ss[2].str("addr");
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "aspectMask = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "colorAttachment = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "clearValue = " + ss[2].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkclearcolorvalue(const VkClearColorValue* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[3];
    string stp_strs[3];
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #937
// CODEGEN : file ../vk_helper.py line #939
    stp_strs[0] = "";
    stringstream index_ss;
    for (uint32_t i = 0; i < 4; i++) {
        index_ss.str("");
        index_ss << i;
// CODEGEN : file ../vk_helper.py line #990
        ss[0] << pStruct->float32[i];
        stp_strs[0] += " " + prefix + "float32[" + index_ss.str() + "] = " + ss[0].str() + "\n";
// CODEGEN : file ../vk_helper.py line #1000
        ss[0].str("");
    }
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #937
// CODEGEN : file ../vk_helper.py line #939
    stp_strs[1] = "";
    for (uint32_t i = 0; i < 4; i++) {
        index_ss.str("");
        index_ss << i;
// CODEGEN : file ../vk_helper.py line #990
        ss[1] << pStruct->int32[i];
        stp_strs[1] += " " + prefix + "int32[" + index_ss.str() + "] = " + ss[1].str() + "\n";
// CODEGEN : file ../vk_helper.py line #1000
        ss[1].str("");
    }
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #937
// CODEGEN : file ../vk_helper.py line #939
    stp_strs[2] = "";
    for (uint32_t i = 0; i < 4; i++) {
        index_ss.str("");
        index_ss << i;
// CODEGEN : file ../vk_helper.py line #990
        ss[2] << pStruct->uint32[i];
        stp_strs[2] += " " + prefix + "uint32[" + index_ss.str() + "] = " + ss[2].str() + "\n";
// CODEGEN : file ../vk_helper.py line #1000
        ss[2].str("");
    }
// CODEGEN : file ../vk_helper.py line #1061
    ss[0] << "0x" << (void*)pStruct->float32;
// CODEGEN : file ../vk_helper.py line #1061
    ss[1] << "0x" << (void*)pStruct->int32;
// CODEGEN : file ../vk_helper.py line #1061
    ss[2] << "0x" << (void*)pStruct->uint32;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "float32 = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "int32 = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "uint32 = " + ss[2].str() + "\n";
    final_str = final_str + stp_strs[2] + stp_strs[1] + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkcleardepthstencilvalue(const VkClearDepthStencilValue* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[2];
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[0] << pStruct->depth;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[1] << pStruct->stencil;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "depth = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "stencil = " + ss[1].str() + "\n";
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkclearrect(const VkClearRect* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[3];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkrect2d(&pStruct->rect, extra_indent);
    ss[0] << "0x" << &pStruct->rect;
    stp_strs[0] = " " + prefix + "rect (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[0].str("");
// CODEGEN : file ../vk_helper.py line #1055
    ss[0].str("addr");
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[1] << pStruct->baseArrayLayer;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[2] << pStruct->layerCount;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "rect = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "baseArrayLayer = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "layerCount = " + ss[2].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkclearvalue(const VkClearValue* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[2];
    string stp_strs[2];
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkclearcolorvalue(&pStruct->color, extra_indent);
    ss[0] << "0x" << &pStruct->color;
    stp_strs[0] = " " + prefix + "color (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[0].str("");
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkcleardepthstencilvalue(&pStruct->depthStencil, extra_indent);
    ss[1] << "0x" << &pStruct->depthStencil;
    stp_strs[1] = " " + prefix + "depthStencil (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[1].str("");
// CODEGEN : file ../vk_helper.py line #1055
    ss[0].str("addr");
// CODEGEN : file ../vk_helper.py line #1055
    ss[1].str("addr");
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "color = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "depthStencil = " + ss[1].str() + "\n";
    final_str = final_str + stp_strs[1] + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkcommandbufferallocateinfo(const VkCommandBufferAllocateInfo* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[3];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->commandPool;
// CODEGEN : file ../vk_helper.py line #1090: NB: Edit here to choose hex vs dec output by variable name
    ss[2] << "0x" << pStruct->commandBufferCount;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "commandPool = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "level = " + string_VkCommandBufferLevel(pStruct->level) + "\n";
    final_str = final_str + prefix + "commandBufferCount = " + ss[2].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkcommandbufferbegininfo(const VkCommandBufferBeginInfo* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[3];
    string stp_strs[2];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1012
        if (pStruct->pInheritanceInfo) {
// CODEGEN : file ../vk_helper.py line #1024
        tmp_str = vk_print_vkcommandbufferinheritanceinfo(pStruct->pInheritanceInfo, extra_indent);
        ss[1] << "0x" << &pStruct->pInheritanceInfo;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[1] = " " + prefix + "pInheritanceInfo (addr)\n" + tmp_str;
        ss[1].str("");
    }
    else
        stp_strs[1] = "";
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->flags;
// CODEGEN : file ../vk_helper.py line #1086
    ss[2] << "0x" << pStruct->pInheritanceInfo;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "flags = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "pInheritanceInfo = " + ss[2].str() + "\n";
    final_str = final_str + stp_strs[1] + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkcommandbufferinheritanceinfo(const VkCommandBufferInheritanceInfo* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[7];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->renderPass;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[2] << pStruct->subpass;
// CODEGEN : file ../vk_helper.py line #1086
    ss[3] << "0x" << pStruct->framebuffer;
// CODEGEN : file ../vk_helper.py line #1064
    ss[4].str(pStruct->occlusionQueryEnable ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1086
    ss[5] << "0x" << pStruct->queryFlags;
// CODEGEN : file ../vk_helper.py line #1086
    ss[6] << "0x" << pStruct->pipelineStatistics;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "renderPass = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "subpass = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "framebuffer = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "occlusionQueryEnable = " + ss[4].str() + "\n";
    final_str = final_str + prefix + "queryFlags = " + ss[5].str() + "\n";
    final_str = final_str + prefix + "pipelineStatistics = " + ss[6].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkcommandpoolcreateinfo(const VkCommandPoolCreateInfo* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[3];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->flags;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[2] << pStruct->queueFamilyIndex;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "flags = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "queueFamilyIndex = " + ss[2].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkcomponentmapping(const VkComponentMapping* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "r = " + string_VkComponentSwizzle(pStruct->r) + "\n";
    final_str = final_str + prefix + "g = " + string_VkComponentSwizzle(pStruct->g) + "\n";
    final_str = final_str + prefix + "b = " + string_VkComponentSwizzle(pStruct->b) + "\n";
    final_str = final_str + prefix + "a = " + string_VkComponentSwizzle(pStruct->a) + "\n";
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkcomputepipelinecreateinfo(const VkComputePipelineCreateInfo* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[6];
    string stp_strs[2];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkpipelineshaderstagecreateinfo(&pStruct->stage, extra_indent);
    ss[1] << "0x" << &pStruct->stage;
    stp_strs[1] = " " + prefix + "stage (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[1].str("");
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->flags;
// CODEGEN : file ../vk_helper.py line #1055
    ss[2].str("addr");
// CODEGEN : file ../vk_helper.py line #1086
    ss[3] << "0x" << pStruct->layout;
// CODEGEN : file ../vk_helper.py line #1086
    ss[4] << "0x" << pStruct->basePipelineHandle;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[5] << pStruct->basePipelineIndex;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "flags = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "stage = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "layout = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "basePipelineHandle = " + ss[4].str() + "\n";
    final_str = final_str + prefix + "basePipelineIndex = " + ss[5].str() + "\n";
    final_str = final_str + stp_strs[1] + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkcopydescriptorset(const VkCopyDescriptorSet* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[8];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->srcSet;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[2] << pStruct->srcBinding;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[3] << pStruct->srcArrayElement;
// CODEGEN : file ../vk_helper.py line #1086
    ss[4] << "0x" << pStruct->dstSet;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[5] << pStruct->dstBinding;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[6] << pStruct->dstArrayElement;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[7] << pStruct->descriptorCount;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "srcSet = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "srcBinding = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "srcArrayElement = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "dstSet = " + ss[4].str() + "\n";
    final_str = final_str + prefix + "dstBinding = " + ss[5].str() + "\n";
    final_str = final_str + prefix + "dstArrayElement = " + ss[6].str() + "\n";
    final_str = final_str + prefix + "descriptorCount = " + ss[7].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkdebugmarkermarkerinfoext(const VkDebugMarkerMarkerInfoEXT* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[3];
    string stp_strs[2];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #937
// CODEGEN : file ../vk_helper.py line #939
        stp_strs[1] = "";
        stringstream index_ss;
        for (uint32_t i = 0; i < 4; i++) {
            index_ss.str("");
            index_ss << i;
// CODEGEN : file ../vk_helper.py line #990
            ss[1] << pStruct->color[i];
            stp_strs[1] += " " + prefix + "color[" + index_ss.str() + "] = " + ss[1].str() + "\n";
// CODEGEN : file ../vk_helper.py line #1000
            ss[1].str("");
        }
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1076
    if (pStruct->pMarkerName != NULL) {
        ss[1] << pStruct->pMarkerName;
     } else {
        ss[1] << "";
     }
// CODEGEN : file ../vk_helper.py line #1061
    ss[2] << "0x" << (void*)pStruct->color;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "pMarkerName = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "color = " + ss[2].str() + "\n";
    final_str = final_str + stp_strs[1] + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkdebugmarkerobjectnameinfoext(const VkDebugMarkerObjectNameInfoEXT* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[3];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1090: NB: Edit here to choose hex vs dec output by variable name
    ss[1] << "0x" << pStruct->object;
// CODEGEN : file ../vk_helper.py line #1076
    if (pStruct->pObjectName != NULL) {
        ss[2] << pStruct->pObjectName;
     } else {
        ss[2] << "";
     }
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "objectType = " + string_VkDebugReportObjectTypeEXT(pStruct->objectType) + "\n";
    final_str = final_str + prefix + "object = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "pObjectName = " + ss[2].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkdebugmarkerobjecttaginfoext(const VkDebugMarkerObjectTagInfoEXT* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[5];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1090: NB: Edit here to choose hex vs dec output by variable name
    ss[1] << "0x" << pStruct->object;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[2] << pStruct->tagName;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[3] << pStruct->tagSize;
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[4] << "0x" << pStruct->pTag;
    else
        ss[4].str("address");
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "objectType = " + string_VkDebugReportObjectTypeEXT(pStruct->objectType) + "\n";
    final_str = final_str + prefix + "object = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "tagName = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "tagSize = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "pTag = " + ss[4].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkdebugreportcallbackcreateinfoext(const VkDebugReportCallbackCreateInfoEXT* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[4];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->flags;
// CODEGEN : file ../vk_helper.py line #1086
    ss[2] << "0x" << pStruct->pfnCallback;
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[3] << "0x" << pStruct->pUserData;
    else
        ss[3].str("address");
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "flags = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "pfnCallback = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "pUserData = " + ss[3].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkdedicatedallocationbuffercreateinfonv(const VkDedicatedAllocationBufferCreateInfoNV* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[2];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1064
    ss[1].str(pStruct->dedicatedAllocation ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "dedicatedAllocation = " + ss[1].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkdedicatedallocationimagecreateinfonv(const VkDedicatedAllocationImageCreateInfoNV* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[2];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1064
    ss[1].str(pStruct->dedicatedAllocation ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "dedicatedAllocation = " + ss[1].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkdedicatedallocationmemoryallocateinfonv(const VkDedicatedAllocationMemoryAllocateInfoNV* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[3];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->image;
// CODEGEN : file ../vk_helper.py line #1086
    ss[2] << "0x" << pStruct->buffer;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "image = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "buffer = " + ss[2].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkdescriptorbufferinfo(const VkDescriptorBufferInfo* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[3];
// CODEGEN : file ../vk_helper.py line #1086
    ss[0] << "0x" << pStruct->buffer;
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->offset;
// CODEGEN : file ../vk_helper.py line #1086
    ss[2] << "0x" << pStruct->range;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "buffer = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "offset = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "range = " + ss[2].str() + "\n";
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkdescriptorimageinfo(const VkDescriptorImageInfo* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[2];
// CODEGEN : file ../vk_helper.py line #1086
    ss[0] << "0x" << pStruct->sampler;
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->imageView;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sampler = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "imageView = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "imageLayout = " + string_VkImageLayout(pStruct->imageLayout) + "\n";
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkdescriptorpoolcreateinfo(const VkDescriptorPoolCreateInfo* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[5];
    string stp_strs[2];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
        stp_strs[1] = "";
        stringstream index_ss;
        if (pStruct->pPoolSizes) {
            for (uint32_t i = 0; i < pStruct->poolSizeCount; i++) {
                index_ss.str("");
                index_ss << i;
// CODEGEN : file ../vk_helper.py line #980
                ss[1] << "0x" << &pStruct->pPoolSizes[i];
                tmp_str = vk_print_vkdescriptorpoolsize(&pStruct->pPoolSizes[i], extra_indent);
// CODEGEN : file ../vk_helper.py line #984
                stp_strs[1] += " " + prefix + "pPoolSizes[" + index_ss.str() + "] (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1000
                ss[1].str("");
            }
        }
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->flags;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[2] << pStruct->maxSets;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[3] << pStruct->poolSizeCount;
// CODEGEN : file ../vk_helper.py line #1061
    ss[4] << "0x" << (void*)pStruct->pPoolSizes;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "flags = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "maxSets = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "poolSizeCount = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "pPoolSizes = " + ss[4].str() + "\n";
    final_str = final_str + stp_strs[1] + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkdescriptorpoolsize(const VkDescriptorPoolSize* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[1];
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[0] << pStruct->descriptorCount;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "type = " + string_VkDescriptorType(pStruct->type) + "\n";
    final_str = final_str + prefix + "descriptorCount = " + ss[0].str() + "\n";
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkdescriptorsetallocateinfo(const VkDescriptorSetAllocateInfo* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[4];
    string stp_strs[2];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
        stp_strs[1] = "";
        stringstream index_ss;
        if (pStruct->pSetLayouts) {
            for (uint32_t i = 0; i < pStruct->descriptorSetCount; i++) {
                index_ss.str("");
                index_ss << i;
// CODEGEN : file ../vk_helper.py line #990
                ss[1] << "0x" << pStruct->pSetLayouts[i];
                stp_strs[1] += " " + prefix + "pSetLayouts[" + index_ss.str() + "].handle = " + ss[1].str() + "\n";
// CODEGEN : file ../vk_helper.py line #1000
                ss[1].str("");
            }
        }
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->descriptorPool;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[2] << pStruct->descriptorSetCount;
// CODEGEN : file ../vk_helper.py line #1061
    ss[3] << "0x" << (void*)pStruct->pSetLayouts;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "descriptorPool = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "descriptorSetCount = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "pSetLayouts = " + ss[3].str() + "\n";
    final_str = final_str + stp_strs[1] + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkdescriptorsetlayoutbinding(const VkDescriptorSetLayoutBinding* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[4];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
    stp_strs[0] = "";
    stringstream index_ss;
    if (pStruct->pImmutableSamplers) {
        for (uint32_t i = 0; i < pStruct->descriptorCount; i++) {
            index_ss.str("");
            index_ss << i;
// CODEGEN : file ../vk_helper.py line #990
            ss[0] << "0x" << pStruct->pImmutableSamplers[i];
            stp_strs[0] += " " + prefix + "pImmutableSamplers[" + index_ss.str() + "].handle = " + ss[0].str() + "\n";
// CODEGEN : file ../vk_helper.py line #1000
            ss[0].str("");
        }
    }
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[0] << pStruct->binding;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[1] << pStruct->descriptorCount;
// CODEGEN : file ../vk_helper.py line #1086
    ss[2] << "0x" << pStruct->stageFlags;
// CODEGEN : file ../vk_helper.py line #1061
    ss[3] << "0x" << (void*)pStruct->pImmutableSamplers;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "binding = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "descriptorType = " + string_VkDescriptorType(pStruct->descriptorType) + "\n";
    final_str = final_str + prefix + "descriptorCount = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "stageFlags = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "pImmutableSamplers = " + ss[3].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkdescriptorsetlayoutcreateinfo(const VkDescriptorSetLayoutCreateInfo* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[4];
    string stp_strs[2];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
        stp_strs[1] = "";
        stringstream index_ss;
        if (pStruct->pBindings) {
            for (uint32_t i = 0; i < pStruct->bindingCount; i++) {
                index_ss.str("");
                index_ss << i;
// CODEGEN : file ../vk_helper.py line #980
                ss[1] << "0x" << &pStruct->pBindings[i];
                tmp_str = vk_print_vkdescriptorsetlayoutbinding(&pStruct->pBindings[i], extra_indent);
// CODEGEN : file ../vk_helper.py line #984
                stp_strs[1] += " " + prefix + "pBindings[" + index_ss.str() + "] (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1000
                ss[1].str("");
            }
        }
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->flags;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[2] << pStruct->bindingCount;
// CODEGEN : file ../vk_helper.py line #1061
    ss[3] << "0x" << (void*)pStruct->pBindings;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "flags = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "bindingCount = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "pBindings = " + ss[3].str() + "\n";
    final_str = final_str + stp_strs[1] + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkdevicecreateinfo(const VkDeviceCreateInfo* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[9];
    string stp_strs[5];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
        stp_strs[1] = "";
        stringstream index_ss;
        if (pStruct->pQueueCreateInfos) {
            for (uint32_t i = 0; i < pStruct->queueCreateInfoCount; i++) {
                index_ss.str("");
                index_ss << i;
// CODEGEN : file ../vk_helper.py line #980
                ss[1] << "0x" << &pStruct->pQueueCreateInfos[i];
                tmp_str = vk_print_vkdevicequeuecreateinfo(&pStruct->pQueueCreateInfos[i], extra_indent);
// CODEGEN : file ../vk_helper.py line #984
                stp_strs[1] += " " + prefix + "pQueueCreateInfos[" + index_ss.str() + "] (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1000
                ss[1].str("");
            }
        }
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
        stp_strs[2] = "";
        if (pStruct->ppEnabledLayerNames) {
            for (uint32_t i = 0; i < pStruct->enabledLayerCount; i++) {
                index_ss.str("");
                index_ss << i;
// CODEGEN : file ../vk_helper.py line #990
                ss[2] << "0x" << pStruct->ppEnabledLayerNames[i];
                stp_strs[2] += " " + prefix + "ppEnabledLayerNames[" + index_ss.str() + "] = " + ss[2].str() + "\n";
// CODEGEN : file ../vk_helper.py line #1000
                ss[2].str("");
            }
        }
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
        stp_strs[3] = "";
        if (pStruct->ppEnabledExtensionNames) {
            for (uint32_t i = 0; i < pStruct->enabledExtensionCount; i++) {
                index_ss.str("");
                index_ss << i;
// CODEGEN : file ../vk_helper.py line #990
                ss[3] << "0x" << pStruct->ppEnabledExtensionNames[i];
                stp_strs[3] += " " + prefix + "ppEnabledExtensionNames[" + index_ss.str() + "] = " + ss[3].str() + "\n";
// CODEGEN : file ../vk_helper.py line #1000
                ss[3].str("");
            }
        }
// CODEGEN : file ../vk_helper.py line #1012
        if (pStruct->pEnabledFeatures) {
// CODEGEN : file ../vk_helper.py line #1024
        tmp_str = vk_print_vkphysicaldevicefeatures(pStruct->pEnabledFeatures, extra_indent);
        ss[4] << "0x" << &pStruct->pEnabledFeatures;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[4] = " " + prefix + "pEnabledFeatures (addr)\n" + tmp_str;
        ss[4].str("");
    }
    else
        stp_strs[4] = "";
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->flags;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[2] << pStruct->queueCreateInfoCount;
// CODEGEN : file ../vk_helper.py line #1061
    ss[3] << "0x" << (void*)pStruct->pQueueCreateInfos;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[4] << pStruct->enabledLayerCount;
// CODEGEN : file ../vk_helper.py line #1061
    ss[5] << "0x" << (void*)pStruct->ppEnabledLayerNames;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[6] << pStruct->enabledExtensionCount;
// CODEGEN : file ../vk_helper.py line #1061
    ss[7] << "0x" << (void*)pStruct->ppEnabledExtensionNames;
// CODEGEN : file ../vk_helper.py line #1086
    ss[8] << "0x" << pStruct->pEnabledFeatures;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "flags = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "queueCreateInfoCount = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "pQueueCreateInfos = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "enabledLayerCount = " + ss[4].str() + "\n";
    final_str = final_str + prefix + "ppEnabledLayerNames = " + ss[5].str() + "\n";
    final_str = final_str + prefix + "enabledExtensionCount = " + ss[6].str() + "\n";
    final_str = final_str + prefix + "ppEnabledExtensionNames = " + ss[7].str() + "\n";
    final_str = final_str + prefix + "pEnabledFeatures = " + ss[8].str() + "\n";
    final_str = final_str + stp_strs[4] + stp_strs[3] + stp_strs[2] + stp_strs[1] + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkdevicequeuecreateinfo(const VkDeviceQueueCreateInfo* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[5];
    string stp_strs[2];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
        stp_strs[1] = "";
        stringstream index_ss;
        if (pStruct->pQueuePriorities) {
            for (uint32_t i = 0; i < pStruct->queueCount; i++) {
                index_ss.str("");
                index_ss << i;
// CODEGEN : file ../vk_helper.py line #990
                ss[1] << "0x" << pStruct->pQueuePriorities[i];
                stp_strs[1] += " " + prefix + "pQueuePriorities[" + index_ss.str() + "] = " + ss[1].str() + "\n";
// CODEGEN : file ../vk_helper.py line #1000
                ss[1].str("");
            }
        }
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->flags;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[2] << pStruct->queueFamilyIndex;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[3] << pStruct->queueCount;
// CODEGEN : file ../vk_helper.py line #1061
    ss[4] << "0x" << (void*)pStruct->pQueuePriorities;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "flags = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "queueFamilyIndex = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "queueCount = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "pQueuePriorities = " + ss[4].str() + "\n";
    final_str = final_str + stp_strs[1] + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkdispatchindirectcommand(const VkDispatchIndirectCommand* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[3];
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[0] << pStruct->x;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[1] << pStruct->y;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[2] << pStruct->z;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "x = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "y = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "z = " + ss[2].str() + "\n";
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkdisplaymodecreateinfokhr(const VkDisplayModeCreateInfoKHR* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[3];
    string stp_strs[2];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkdisplaymodeparameterskhr(&pStruct->parameters, extra_indent);
    ss[1] << "0x" << &pStruct->parameters;
    stp_strs[1] = " " + prefix + "parameters (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[1].str("");
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->flags;
// CODEGEN : file ../vk_helper.py line #1055
    ss[2].str("addr");
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "flags = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "parameters = " + ss[2].str() + "\n";
    final_str = final_str + stp_strs[1] + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkdisplaymodeparameterskhr(const VkDisplayModeParametersKHR* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[2];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkextent2d(&pStruct->visibleRegion, extra_indent);
    ss[0] << "0x" << &pStruct->visibleRegion;
    stp_strs[0] = " " + prefix + "visibleRegion (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[0].str("");
// CODEGEN : file ../vk_helper.py line #1055
    ss[0].str("addr");
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[1] << pStruct->refreshRate;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "visibleRegion = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "refreshRate = " + ss[1].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkdisplaymodepropertieskhr(const VkDisplayModePropertiesKHR* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[2];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkdisplaymodeparameterskhr(&pStruct->parameters, extra_indent);
    ss[0] << "0x" << &pStruct->parameters;
    stp_strs[0] = " " + prefix + "parameters (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[0].str("");
// CODEGEN : file ../vk_helper.py line #1086
    ss[0] << "0x" << pStruct->displayMode;
// CODEGEN : file ../vk_helper.py line #1055
    ss[1].str("addr");
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "displayMode = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "parameters = " + ss[1].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkdisplayplanecapabilitieskhr(const VkDisplayPlaneCapabilitiesKHR* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[9];
    string stp_strs[8];
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkoffset2d(&pStruct->minSrcPosition, extra_indent);
    ss[0] << "0x" << &pStruct->minSrcPosition;
    stp_strs[0] = " " + prefix + "minSrcPosition (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[0].str("");
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkoffset2d(&pStruct->maxSrcPosition, extra_indent);
    ss[1] << "0x" << &pStruct->maxSrcPosition;
    stp_strs[1] = " " + prefix + "maxSrcPosition (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[1].str("");
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkextent2d(&pStruct->minSrcExtent, extra_indent);
    ss[2] << "0x" << &pStruct->minSrcExtent;
    stp_strs[2] = " " + prefix + "minSrcExtent (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[2].str("");
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkextent2d(&pStruct->maxSrcExtent, extra_indent);
    ss[3] << "0x" << &pStruct->maxSrcExtent;
    stp_strs[3] = " " + prefix + "maxSrcExtent (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[3].str("");
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkoffset2d(&pStruct->minDstPosition, extra_indent);
    ss[4] << "0x" << &pStruct->minDstPosition;
    stp_strs[4] = " " + prefix + "minDstPosition (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[4].str("");
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkoffset2d(&pStruct->maxDstPosition, extra_indent);
    ss[5] << "0x" << &pStruct->maxDstPosition;
    stp_strs[5] = " " + prefix + "maxDstPosition (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[5].str("");
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkextent2d(&pStruct->minDstExtent, extra_indent);
    ss[6] << "0x" << &pStruct->minDstExtent;
    stp_strs[6] = " " + prefix + "minDstExtent (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[6].str("");
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkextent2d(&pStruct->maxDstExtent, extra_indent);
    ss[7] << "0x" << &pStruct->maxDstExtent;
    stp_strs[7] = " " + prefix + "maxDstExtent (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[7].str("");
// CODEGEN : file ../vk_helper.py line #1086
    ss[0] << "0x" << pStruct->supportedAlpha;
// CODEGEN : file ../vk_helper.py line #1055
    ss[1].str("addr");
// CODEGEN : file ../vk_helper.py line #1055
    ss[2].str("addr");
// CODEGEN : file ../vk_helper.py line #1055
    ss[3].str("addr");
// CODEGEN : file ../vk_helper.py line #1055
    ss[4].str("addr");
// CODEGEN : file ../vk_helper.py line #1055
    ss[5].str("addr");
// CODEGEN : file ../vk_helper.py line #1055
    ss[6].str("addr");
// CODEGEN : file ../vk_helper.py line #1055
    ss[7].str("addr");
// CODEGEN : file ../vk_helper.py line #1055
    ss[8].str("addr");
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "supportedAlpha = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "minSrcPosition = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "maxSrcPosition = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "minSrcExtent = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "maxSrcExtent = " + ss[4].str() + "\n";
    final_str = final_str + prefix + "minDstPosition = " + ss[5].str() + "\n";
    final_str = final_str + prefix + "maxDstPosition = " + ss[6].str() + "\n";
    final_str = final_str + prefix + "minDstExtent = " + ss[7].str() + "\n";
    final_str = final_str + prefix + "maxDstExtent = " + ss[8].str() + "\n";
    final_str = final_str + stp_strs[7] + stp_strs[6] + stp_strs[5] + stp_strs[4] + stp_strs[3] + stp_strs[2] + stp_strs[1] + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkdisplayplanepropertieskhr(const VkDisplayPlanePropertiesKHR* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[2];
// CODEGEN : file ../vk_helper.py line #1086
    ss[0] << "0x" << pStruct->currentDisplay;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[1] << pStruct->currentStackIndex;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "currentDisplay = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "currentStackIndex = " + ss[1].str() + "\n";
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkdisplaypresentinfokhr(const VkDisplayPresentInfoKHR* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[4];
    string stp_strs[3];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkrect2d(&pStruct->srcRect, extra_indent);
    ss[1] << "0x" << &pStruct->srcRect;
    stp_strs[1] = " " + prefix + "srcRect (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[1].str("");
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkrect2d(&pStruct->dstRect, extra_indent);
    ss[2] << "0x" << &pStruct->dstRect;
    stp_strs[2] = " " + prefix + "dstRect (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[2].str("");
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1055
    ss[1].str("addr");
// CODEGEN : file ../vk_helper.py line #1055
    ss[2].str("addr");
// CODEGEN : file ../vk_helper.py line #1064
    ss[3].str(pStruct->persistent ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "srcRect = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "dstRect = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "persistent = " + ss[3].str() + "\n";
    final_str = final_str + stp_strs[2] + stp_strs[1] + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkdisplaypropertieskhr(const VkDisplayPropertiesKHR* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[7];
    string stp_strs[2];
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkextent2d(&pStruct->physicalDimensions, extra_indent);
    ss[0] << "0x" << &pStruct->physicalDimensions;
    stp_strs[0] = " " + prefix + "physicalDimensions (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[0].str("");
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkextent2d(&pStruct->physicalResolution, extra_indent);
    ss[1] << "0x" << &pStruct->physicalResolution;
    stp_strs[1] = " " + prefix + "physicalResolution (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[1].str("");
// CODEGEN : file ../vk_helper.py line #1086
    ss[0] << "0x" << pStruct->display;
// CODEGEN : file ../vk_helper.py line #1076
    if (pStruct->displayName != NULL) {
        ss[1] << pStruct->displayName;
     } else {
        ss[1] << "";
     }
// CODEGEN : file ../vk_helper.py line #1055
    ss[2].str("addr");
// CODEGEN : file ../vk_helper.py line #1055
    ss[3].str("addr");
// CODEGEN : file ../vk_helper.py line #1086
    ss[4] << "0x" << pStruct->supportedTransforms;
// CODEGEN : file ../vk_helper.py line #1064
    ss[5].str(pStruct->planeReorderPossible ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[6].str(pStruct->persistentContent ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "display = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "displayName = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "physicalDimensions = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "physicalResolution = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "supportedTransforms = " + ss[4].str() + "\n";
    final_str = final_str + prefix + "planeReorderPossible = " + ss[5].str() + "\n";
    final_str = final_str + prefix + "persistentContent = " + ss[6].str() + "\n";
    final_str = final_str + stp_strs[1] + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkdisplaysurfacecreateinfokhr(const VkDisplaySurfaceCreateInfoKHR* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[7];
    string stp_strs[2];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkextent2d(&pStruct->imageExtent, extra_indent);
    ss[1] << "0x" << &pStruct->imageExtent;
    stp_strs[1] = " " + prefix + "imageExtent (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[1].str("");
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->flags;
// CODEGEN : file ../vk_helper.py line #1086
    ss[2] << "0x" << pStruct->displayMode;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[3] << pStruct->planeIndex;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[4] << pStruct->planeStackIndex;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[5] << pStruct->globalAlpha;
// CODEGEN : file ../vk_helper.py line #1055
    ss[6].str("addr");
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "flags = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "displayMode = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "planeIndex = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "planeStackIndex = " + ss[4].str() + "\n";
    final_str = final_str + prefix + "transform = " + string_VkSurfaceTransformFlagBitsKHR(pStruct->transform) + "\n";
    final_str = final_str + prefix + "globalAlpha = " + ss[5].str() + "\n";
    final_str = final_str + prefix + "alphaMode = " + string_VkDisplayPlaneAlphaFlagBitsKHR(pStruct->alphaMode) + "\n";
    final_str = final_str + prefix + "imageExtent = " + ss[6].str() + "\n";
    final_str = final_str + stp_strs[1] + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkdrawindexedindirectcommand(const VkDrawIndexedIndirectCommand* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[5];
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[0] << pStruct->indexCount;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[1] << pStruct->instanceCount;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[2] << pStruct->firstIndex;
// CODEGEN : file ../vk_helper.py line #1090: NB: Edit here to choose hex vs dec output by variable name
    ss[3] << "0x" << pStruct->vertexOffset;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[4] << pStruct->firstInstance;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "indexCount = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "instanceCount = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "firstIndex = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "vertexOffset = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "firstInstance = " + ss[4].str() + "\n";
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkdrawindirectcommand(const VkDrawIndirectCommand* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[4];
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[0] << pStruct->vertexCount;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[1] << pStruct->instanceCount;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[2] << pStruct->firstVertex;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[3] << pStruct->firstInstance;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "vertexCount = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "instanceCount = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "firstVertex = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "firstInstance = " + ss[3].str() + "\n";
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkeventcreateinfo(const VkEventCreateInfo* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[2];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->flags;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "flags = " + ss[1].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkexportmemoryallocateinfonv(const VkExportMemoryAllocateInfoNV* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[2];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->handleTypes;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "handleTypes = " + ss[1].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
#ifdef VK_USE_PLATFORM_WIN32_KHR
std::string vk_print_vkexportmemorywin32handleinfonv(const VkExportMemoryWin32HandleInfoNV* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[3];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->pAttributes;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[2] << pStruct->dwAccess;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "pAttributes = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "dwAccess = " + ss[2].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
#endif //VK_USE_PLATFORM_WIN32_KHR
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkextensionproperties(const VkExtensionProperties* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[2];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #937
// CODEGEN : file ../vk_helper.py line #939
    stp_strs[0] = "";
    stringstream index_ss;
    for (uint32_t i = 0; i < VK_MAX_EXTENSION_NAME_SIZE; i++) {
        index_ss.str("");
        index_ss << i;
// CODEGEN : file ../vk_helper.py line #990
        ss[0] << pStruct->extensionName[i];
        stp_strs[0] += " " + prefix + "extensionName[" + index_ss.str() + "] = " + ss[0].str() + "\n";
// CODEGEN : file ../vk_helper.py line #1000
        ss[0].str("");
    }
// CODEGEN : file ../vk_helper.py line #1061
    ss[0] << "0x" << (void*)pStruct->extensionName;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[1] << pStruct->specVersion;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "extensionName = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "specVersion = " + ss[1].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkextent2d(const VkExtent2D* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[2];
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[0] << pStruct->width;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[1] << pStruct->height;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "width = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "height = " + ss[1].str() + "\n";
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkextent3d(const VkExtent3D* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[3];
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[0] << pStruct->width;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[1] << pStruct->height;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[2] << pStruct->depth;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "width = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "height = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "depth = " + ss[2].str() + "\n";
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkexternalimageformatpropertiesnv(const VkExternalImageFormatPropertiesNV* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[4];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkimageformatproperties(&pStruct->imageFormatProperties, extra_indent);
    ss[0] << "0x" << &pStruct->imageFormatProperties;
    stp_strs[0] = " " + prefix + "imageFormatProperties (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[0].str("");
// CODEGEN : file ../vk_helper.py line #1055
    ss[0].str("addr");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->externalMemoryFeatures;
// CODEGEN : file ../vk_helper.py line #1086
    ss[2] << "0x" << pStruct->exportFromImportedHandleTypes;
// CODEGEN : file ../vk_helper.py line #1086
    ss[3] << "0x" << pStruct->compatibleHandleTypes;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "imageFormatProperties = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "externalMemoryFeatures = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "exportFromImportedHandleTypes = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "compatibleHandleTypes = " + ss[3].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkexternalmemoryimagecreateinfonv(const VkExternalMemoryImageCreateInfoNV* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[2];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->handleTypes;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "handleTypes = " + ss[1].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkfencecreateinfo(const VkFenceCreateInfo* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[2];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->flags;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "flags = " + ss[1].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkformatproperties(const VkFormatProperties* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[3];
// CODEGEN : file ../vk_helper.py line #1086
    ss[0] << "0x" << pStruct->linearTilingFeatures;
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->optimalTilingFeatures;
// CODEGEN : file ../vk_helper.py line #1086
    ss[2] << "0x" << pStruct->bufferFeatures;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "linearTilingFeatures = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "optimalTilingFeatures = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "bufferFeatures = " + ss[2].str() + "\n";
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkframebuffercreateinfo(const VkFramebufferCreateInfo* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[8];
    string stp_strs[2];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
        stp_strs[1] = "";
        stringstream index_ss;
        if (pStruct->pAttachments) {
            for (uint32_t i = 0; i < pStruct->attachmentCount; i++) {
                index_ss.str("");
                index_ss << i;
// CODEGEN : file ../vk_helper.py line #990
                ss[1] << "0x" << pStruct->pAttachments[i];
                stp_strs[1] += " " + prefix + "pAttachments[" + index_ss.str() + "].handle = " + ss[1].str() + "\n";
// CODEGEN : file ../vk_helper.py line #1000
                ss[1].str("");
            }
        }
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->flags;
// CODEGEN : file ../vk_helper.py line #1086
    ss[2] << "0x" << pStruct->renderPass;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[3] << pStruct->attachmentCount;
// CODEGEN : file ../vk_helper.py line #1061
    ss[4] << "0x" << (void*)pStruct->pAttachments;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[5] << pStruct->width;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[6] << pStruct->height;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[7] << pStruct->layers;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "flags = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "renderPass = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "attachmentCount = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "pAttachments = " + ss[4].str() + "\n";
    final_str = final_str + prefix + "width = " + ss[5].str() + "\n";
    final_str = final_str + prefix + "height = " + ss[6].str() + "\n";
    final_str = final_str + prefix + "layers = " + ss[7].str() + "\n";
    final_str = final_str + stp_strs[1] + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkgraphicspipelinecreateinfo(const VkGraphicsPipelineCreateInfo* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[18];
    string stp_strs[11];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
        stp_strs[1] = "";
        stringstream index_ss;
        if (pStruct->pStages) {
            for (uint32_t i = 0; i < pStruct->stageCount; i++) {
                index_ss.str("");
                index_ss << i;
// CODEGEN : file ../vk_helper.py line #980
                ss[1] << "0x" << &pStruct->pStages[i];
                tmp_str = vk_print_vkpipelineshaderstagecreateinfo(&pStruct->pStages[i], extra_indent);
// CODEGEN : file ../vk_helper.py line #984
                stp_strs[1] += " " + prefix + "pStages[" + index_ss.str() + "] (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1000
                ss[1].str("");
            }
        }
// CODEGEN : file ../vk_helper.py line #1012
        if (pStruct->pVertexInputState) {
// CODEGEN : file ../vk_helper.py line #1024
        tmp_str = vk_print_vkpipelinevertexinputstatecreateinfo(pStruct->pVertexInputState, extra_indent);
        ss[2] << "0x" << &pStruct->pVertexInputState;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[2] = " " + prefix + "pVertexInputState (addr)\n" + tmp_str;
        ss[2].str("");
    }
    else
        stp_strs[2] = "";
// CODEGEN : file ../vk_helper.py line #1012
            if (pStruct->pInputAssemblyState) {
// CODEGEN : file ../vk_helper.py line #1024
        tmp_str = vk_print_vkpipelineinputassemblystatecreateinfo(pStruct->pInputAssemblyState, extra_indent);
        ss[3] << "0x" << &pStruct->pInputAssemblyState;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[3] = " " + prefix + "pInputAssemblyState (addr)\n" + tmp_str;
        ss[3].str("");
    }
    else
        stp_strs[3] = "";
// CODEGEN : file ../vk_helper.py line #1012
                if (pStruct->pTessellationState) {
// CODEGEN : file ../vk_helper.py line #1024
        tmp_str = vk_print_vkpipelinetessellationstatecreateinfo(pStruct->pTessellationState, extra_indent);
        ss[4] << "0x" << &pStruct->pTessellationState;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[4] = " " + prefix + "pTessellationState (addr)\n" + tmp_str;
        ss[4].str("");
    }
    else
        stp_strs[4] = "";
// CODEGEN : file ../vk_helper.py line #1012
                    if (pStruct->pViewportState) {
// CODEGEN : file ../vk_helper.py line #1024
        tmp_str = vk_print_vkpipelineviewportstatecreateinfo(pStruct->pViewportState, extra_indent);
        ss[5] << "0x" << &pStruct->pViewportState;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[5] = " " + prefix + "pViewportState (addr)\n" + tmp_str;
        ss[5].str("");
    }
    else
        stp_strs[5] = "";
// CODEGEN : file ../vk_helper.py line #1012
                        if (pStruct->pRasterizationState) {
// CODEGEN : file ../vk_helper.py line #1024
        tmp_str = vk_print_vkpipelinerasterizationstatecreateinfo(pStruct->pRasterizationState, extra_indent);
        ss[6] << "0x" << &pStruct->pRasterizationState;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[6] = " " + prefix + "pRasterizationState (addr)\n" + tmp_str;
        ss[6].str("");
    }
    else
        stp_strs[6] = "";
// CODEGEN : file ../vk_helper.py line #1012
                            if (pStruct->pMultisampleState) {
// CODEGEN : file ../vk_helper.py line #1024
        tmp_str = vk_print_vkpipelinemultisamplestatecreateinfo(pStruct->pMultisampleState, extra_indent);
        ss[7] << "0x" << &pStruct->pMultisampleState;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[7] = " " + prefix + "pMultisampleState (addr)\n" + tmp_str;
        ss[7].str("");
    }
    else
        stp_strs[7] = "";
// CODEGEN : file ../vk_helper.py line #1012
                                if (pStruct->pDepthStencilState) {
// CODEGEN : file ../vk_helper.py line #1024
        tmp_str = vk_print_vkpipelinedepthstencilstatecreateinfo(pStruct->pDepthStencilState, extra_indent);
        ss[8] << "0x" << &pStruct->pDepthStencilState;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[8] = " " + prefix + "pDepthStencilState (addr)\n" + tmp_str;
        ss[8].str("");
    }
    else
        stp_strs[8] = "";
// CODEGEN : file ../vk_helper.py line #1012
                                    if (pStruct->pColorBlendState) {
// CODEGEN : file ../vk_helper.py line #1024
        tmp_str = vk_print_vkpipelinecolorblendstatecreateinfo(pStruct->pColorBlendState, extra_indent);
        ss[9] << "0x" << &pStruct->pColorBlendState;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[9] = " " + prefix + "pColorBlendState (addr)\n" + tmp_str;
        ss[9].str("");
    }
    else
        stp_strs[9] = "";
// CODEGEN : file ../vk_helper.py line #1012
                                        if (pStruct->pDynamicState) {
// CODEGEN : file ../vk_helper.py line #1024
        tmp_str = vk_print_vkpipelinedynamicstatecreateinfo(pStruct->pDynamicState, extra_indent);
        ss[10] << "0x" << &pStruct->pDynamicState;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[10] = " " + prefix + "pDynamicState (addr)\n" + tmp_str;
        ss[10].str("");
    }
    else
        stp_strs[10] = "";
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->flags;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[2] << pStruct->stageCount;
// CODEGEN : file ../vk_helper.py line #1061
    ss[3] << "0x" << (void*)pStruct->pStages;
// CODEGEN : file ../vk_helper.py line #1086
    ss[4] << "0x" << pStruct->pVertexInputState;
// CODEGEN : file ../vk_helper.py line #1086
    ss[5] << "0x" << pStruct->pInputAssemblyState;
// CODEGEN : file ../vk_helper.py line #1086
    ss[6] << "0x" << pStruct->pTessellationState;
// CODEGEN : file ../vk_helper.py line #1086
    ss[7] << "0x" << pStruct->pViewportState;
// CODEGEN : file ../vk_helper.py line #1086
    ss[8] << "0x" << pStruct->pRasterizationState;
// CODEGEN : file ../vk_helper.py line #1086
    ss[9] << "0x" << pStruct->pMultisampleState;
// CODEGEN : file ../vk_helper.py line #1086
    ss[10] << "0x" << pStruct->pDepthStencilState;
// CODEGEN : file ../vk_helper.py line #1086
    ss[11] << "0x" << pStruct->pColorBlendState;
// CODEGEN : file ../vk_helper.py line #1086
    ss[12] << "0x" << pStruct->pDynamicState;
// CODEGEN : file ../vk_helper.py line #1086
    ss[13] << "0x" << pStruct->layout;
// CODEGEN : file ../vk_helper.py line #1086
    ss[14] << "0x" << pStruct->renderPass;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[15] << pStruct->subpass;
// CODEGEN : file ../vk_helper.py line #1086
    ss[16] << "0x" << pStruct->basePipelineHandle;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[17] << pStruct->basePipelineIndex;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "flags = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "stageCount = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "pStages = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "pVertexInputState = " + ss[4].str() + "\n";
    final_str = final_str + prefix + "pInputAssemblyState = " + ss[5].str() + "\n";
    final_str = final_str + prefix + "pTessellationState = " + ss[6].str() + "\n";
    final_str = final_str + prefix + "pViewportState = " + ss[7].str() + "\n";
    final_str = final_str + prefix + "pRasterizationState = " + ss[8].str() + "\n";
    final_str = final_str + prefix + "pMultisampleState = " + ss[9].str() + "\n";
    final_str = final_str + prefix + "pDepthStencilState = " + ss[10].str() + "\n";
    final_str = final_str + prefix + "pColorBlendState = " + ss[11].str() + "\n";
    final_str = final_str + prefix + "pDynamicState = " + ss[12].str() + "\n";
    final_str = final_str + prefix + "layout = " + ss[13].str() + "\n";
    final_str = final_str + prefix + "renderPass = " + ss[14].str() + "\n";
    final_str = final_str + prefix + "subpass = " + ss[15].str() + "\n";
    final_str = final_str + prefix + "basePipelineHandle = " + ss[16].str() + "\n";
    final_str = final_str + prefix + "basePipelineIndex = " + ss[17].str() + "\n";
    final_str = final_str + stp_strs[10] + stp_strs[9] + stp_strs[8] + stp_strs[7] + stp_strs[6] + stp_strs[5] + stp_strs[4] + stp_strs[3] + stp_strs[2] + stp_strs[1] + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkimageblit(const VkImageBlit* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[4];
    string stp_strs[4];
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkimagesubresourcelayers(&pStruct->srcSubresource, extra_indent);
    ss[0] << "0x" << &pStruct->srcSubresource;
    stp_strs[0] = " " + prefix + "srcSubresource (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[0].str("");
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #937
// CODEGEN : file ../vk_helper.py line #939
    stp_strs[1] = "";
    stringstream index_ss;
    for (uint32_t i = 0; i < 2; i++) {
        index_ss.str("");
        index_ss << i;
// CODEGEN : file ../vk_helper.py line #980
        ss[1] << "0x" << &pStruct->srcOffsets[i];
        tmp_str = vk_print_vkoffset3d(&pStruct->srcOffsets[i], extra_indent);
// CODEGEN : file ../vk_helper.py line #984
        stp_strs[1] += " " + prefix + "srcOffsets[" + index_ss.str() + "] (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1000
        ss[1].str("");
    }
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkimagesubresourcelayers(&pStruct->dstSubresource, extra_indent);
    ss[2] << "0x" << &pStruct->dstSubresource;
    stp_strs[2] = " " + prefix + "dstSubresource (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[2].str("");
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #937
// CODEGEN : file ../vk_helper.py line #939
    stp_strs[3] = "";
    for (uint32_t i = 0; i < 2; i++) {
        index_ss.str("");
        index_ss << i;
// CODEGEN : file ../vk_helper.py line #980
        ss[3] << "0x" << &pStruct->dstOffsets[i];
        tmp_str = vk_print_vkoffset3d(&pStruct->dstOffsets[i], extra_indent);
// CODEGEN : file ../vk_helper.py line #984
        stp_strs[3] += " " + prefix + "dstOffsets[" + index_ss.str() + "] (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1000
        ss[3].str("");
    }
// CODEGEN : file ../vk_helper.py line #1055
    ss[0].str("addr");
// CODEGEN : file ../vk_helper.py line #1055
    ss[1].str("addr");
// CODEGEN : file ../vk_helper.py line #1055
    ss[2].str("addr");
// CODEGEN : file ../vk_helper.py line #1055
    ss[3].str("addr");
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "srcSubresource = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "srcOffsets = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "dstSubresource = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "dstOffsets = " + ss[3].str() + "\n";
    final_str = final_str + stp_strs[3] + stp_strs[2] + stp_strs[1] + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkimagecopy(const VkImageCopy* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[5];
    string stp_strs[5];
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkimagesubresourcelayers(&pStruct->srcSubresource, extra_indent);
    ss[0] << "0x" << &pStruct->srcSubresource;
    stp_strs[0] = " " + prefix + "srcSubresource (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[0].str("");
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkoffset3d(&pStruct->srcOffset, extra_indent);
    ss[1] << "0x" << &pStruct->srcOffset;
    stp_strs[1] = " " + prefix + "srcOffset (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[1].str("");
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkimagesubresourcelayers(&pStruct->dstSubresource, extra_indent);
    ss[2] << "0x" << &pStruct->dstSubresource;
    stp_strs[2] = " " + prefix + "dstSubresource (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[2].str("");
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkoffset3d(&pStruct->dstOffset, extra_indent);
    ss[3] << "0x" << &pStruct->dstOffset;
    stp_strs[3] = " " + prefix + "dstOffset (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[3].str("");
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkextent3d(&pStruct->extent, extra_indent);
    ss[4] << "0x" << &pStruct->extent;
    stp_strs[4] = " " + prefix + "extent (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[4].str("");
// CODEGEN : file ../vk_helper.py line #1055
    ss[0].str("addr");
// CODEGEN : file ../vk_helper.py line #1055
    ss[1].str("addr");
// CODEGEN : file ../vk_helper.py line #1055
    ss[2].str("addr");
// CODEGEN : file ../vk_helper.py line #1055
    ss[3].str("addr");
// CODEGEN : file ../vk_helper.py line #1055
    ss[4].str("addr");
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "srcSubresource = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "srcOffset = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "dstSubresource = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "dstOffset = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "extent = " + ss[4].str() + "\n";
    final_str = final_str + stp_strs[4] + stp_strs[3] + stp_strs[2] + stp_strs[1] + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkimagecreateinfo(const VkImageCreateInfo* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[8];
    string stp_strs[3];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkextent3d(&pStruct->extent, extra_indent);
    ss[1] << "0x" << &pStruct->extent;
    stp_strs[1] = " " + prefix + "extent (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[1].str("");
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
        stp_strs[2] = "";
        stringstream index_ss;
        if (pStruct->sharingMode == VK_SHARING_MODE_CONCURRENT) {
            if (pStruct->pQueueFamilyIndices) {
                for (uint32_t i = 0; i < pStruct->queueFamilyIndexCount; i++) {
                    index_ss.str("");
                    index_ss << i;
// CODEGEN : file ../vk_helper.py line #990
                    ss[2] << "0x" << pStruct->pQueueFamilyIndices[i];
                    stp_strs[2] += " " + prefix + "pQueueFamilyIndices[" + index_ss.str() + "] = " + ss[2].str() + "\n";
// CODEGEN : file ../vk_helper.py line #1000
                    ss[2].str("");
                }
            }
        }
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->flags;
// CODEGEN : file ../vk_helper.py line #1055
    ss[2].str("addr");
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[3] << pStruct->mipLevels;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[4] << pStruct->arrayLayers;
// CODEGEN : file ../vk_helper.py line #1086
    ss[5] << "0x" << pStruct->usage;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[6] << pStruct->queueFamilyIndexCount;
// CODEGEN : file ../vk_helper.py line #1061
    ss[7] << "0x" << (void*)pStruct->pQueueFamilyIndices;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "flags = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "imageType = " + string_VkImageType(pStruct->imageType) + "\n";
    final_str = final_str + prefix + "format = " + string_VkFormat(pStruct->format) + "\n";
    final_str = final_str + prefix + "extent = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "mipLevels = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "arrayLayers = " + ss[4].str() + "\n";
    final_str = final_str + prefix + "samples = " + string_VkSampleCountFlagBits(pStruct->samples) + "\n";
    final_str = final_str + prefix + "tiling = " + string_VkImageTiling(pStruct->tiling) + "\n";
    final_str = final_str + prefix + "usage = " + ss[5].str() + "\n";
    final_str = final_str + prefix + "sharingMode = " + string_VkSharingMode(pStruct->sharingMode) + "\n";
    final_str = final_str + prefix + "queueFamilyIndexCount = " + ss[6].str() + "\n";
    final_str = final_str + prefix + "pQueueFamilyIndices = " + ss[7].str() + "\n";
    final_str = final_str + prefix + "initialLayout = " + string_VkImageLayout(pStruct->initialLayout) + "\n";
    final_str = final_str + stp_strs[2] + stp_strs[1] + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkimageformatproperties(const VkImageFormatProperties* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[5];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkextent3d(&pStruct->maxExtent, extra_indent);
    ss[0] << "0x" << &pStruct->maxExtent;
    stp_strs[0] = " " + prefix + "maxExtent (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[0].str("");
// CODEGEN : file ../vk_helper.py line #1055
    ss[0].str("addr");
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[1] << pStruct->maxMipLevels;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[2] << pStruct->maxArrayLayers;
// CODEGEN : file ../vk_helper.py line #1086
    ss[3] << "0x" << pStruct->sampleCounts;
// CODEGEN : file ../vk_helper.py line #1086
    ss[4] << "0x" << pStruct->maxResourceSize;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "maxExtent = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "maxMipLevels = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "maxArrayLayers = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "sampleCounts = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "maxResourceSize = " + ss[4].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkimagememorybarrier(const VkImageMemoryBarrier* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[7];
    string stp_strs[2];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkimagesubresourcerange(&pStruct->subresourceRange, extra_indent);
    ss[1] << "0x" << &pStruct->subresourceRange;
    stp_strs[1] = " " + prefix + "subresourceRange (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[1].str("");
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->srcAccessMask;
// CODEGEN : file ../vk_helper.py line #1086
    ss[2] << "0x" << pStruct->dstAccessMask;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[3] << pStruct->srcQueueFamilyIndex;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[4] << pStruct->dstQueueFamilyIndex;
// CODEGEN : file ../vk_helper.py line #1086
    ss[5] << "0x" << pStruct->image;
// CODEGEN : file ../vk_helper.py line #1055
    ss[6].str("addr");
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "srcAccessMask = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "dstAccessMask = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "oldLayout = " + string_VkImageLayout(pStruct->oldLayout) + "\n";
    final_str = final_str + prefix + "newLayout = " + string_VkImageLayout(pStruct->newLayout) + "\n";
    final_str = final_str + prefix + "srcQueueFamilyIndex = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "dstQueueFamilyIndex = " + ss[4].str() + "\n";
    final_str = final_str + prefix + "image = " + ss[5].str() + "\n";
    final_str = final_str + prefix + "subresourceRange = " + ss[6].str() + "\n";
    final_str = final_str + stp_strs[1] + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkimageresolve(const VkImageResolve* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[5];
    string stp_strs[5];
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkimagesubresourcelayers(&pStruct->srcSubresource, extra_indent);
    ss[0] << "0x" << &pStruct->srcSubresource;
    stp_strs[0] = " " + prefix + "srcSubresource (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[0].str("");
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkoffset3d(&pStruct->srcOffset, extra_indent);
    ss[1] << "0x" << &pStruct->srcOffset;
    stp_strs[1] = " " + prefix + "srcOffset (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[1].str("");
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkimagesubresourcelayers(&pStruct->dstSubresource, extra_indent);
    ss[2] << "0x" << &pStruct->dstSubresource;
    stp_strs[2] = " " + prefix + "dstSubresource (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[2].str("");
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkoffset3d(&pStruct->dstOffset, extra_indent);
    ss[3] << "0x" << &pStruct->dstOffset;
    stp_strs[3] = " " + prefix + "dstOffset (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[3].str("");
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkextent3d(&pStruct->extent, extra_indent);
    ss[4] << "0x" << &pStruct->extent;
    stp_strs[4] = " " + prefix + "extent (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[4].str("");
// CODEGEN : file ../vk_helper.py line #1055
    ss[0].str("addr");
// CODEGEN : file ../vk_helper.py line #1055
    ss[1].str("addr");
// CODEGEN : file ../vk_helper.py line #1055
    ss[2].str("addr");
// CODEGEN : file ../vk_helper.py line #1055
    ss[3].str("addr");
// CODEGEN : file ../vk_helper.py line #1055
    ss[4].str("addr");
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "srcSubresource = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "srcOffset = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "dstSubresource = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "dstOffset = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "extent = " + ss[4].str() + "\n";
    final_str = final_str + stp_strs[4] + stp_strs[3] + stp_strs[2] + stp_strs[1] + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkimagesubresource(const VkImageSubresource* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[3];
// CODEGEN : file ../vk_helper.py line #1086
    ss[0] << "0x" << pStruct->aspectMask;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[1] << pStruct->mipLevel;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[2] << pStruct->arrayLayer;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "aspectMask = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "mipLevel = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "arrayLayer = " + ss[2].str() + "\n";
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkimagesubresourcelayers(const VkImageSubresourceLayers* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[4];
// CODEGEN : file ../vk_helper.py line #1086
    ss[0] << "0x" << pStruct->aspectMask;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[1] << pStruct->mipLevel;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[2] << pStruct->baseArrayLayer;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[3] << pStruct->layerCount;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "aspectMask = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "mipLevel = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "baseArrayLayer = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "layerCount = " + ss[3].str() + "\n";
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkimagesubresourcerange(const VkImageSubresourceRange* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[5];
// CODEGEN : file ../vk_helper.py line #1086
    ss[0] << "0x" << pStruct->aspectMask;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[1] << pStruct->baseMipLevel;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[2] << pStruct->levelCount;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[3] << pStruct->baseArrayLayer;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[4] << pStruct->layerCount;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "aspectMask = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "baseMipLevel = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "levelCount = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "baseArrayLayer = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "layerCount = " + ss[4].str() + "\n";
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkimageviewcreateinfo(const VkImageViewCreateInfo* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[5];
    string stp_strs[3];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkcomponentmapping(&pStruct->components, extra_indent);
    ss[1] << "0x" << &pStruct->components;
    stp_strs[1] = " " + prefix + "components (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[1].str("");
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkimagesubresourcerange(&pStruct->subresourceRange, extra_indent);
    ss[2] << "0x" << &pStruct->subresourceRange;
    stp_strs[2] = " " + prefix + "subresourceRange (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[2].str("");
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->flags;
// CODEGEN : file ../vk_helper.py line #1086
    ss[2] << "0x" << pStruct->image;
// CODEGEN : file ../vk_helper.py line #1055
    ss[3].str("addr");
// CODEGEN : file ../vk_helper.py line #1055
    ss[4].str("addr");
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "flags = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "image = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "viewType = " + string_VkImageViewType(pStruct->viewType) + "\n";
    final_str = final_str + prefix + "format = " + string_VkFormat(pStruct->format) + "\n";
    final_str = final_str + prefix + "components = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "subresourceRange = " + ss[4].str() + "\n";
    final_str = final_str + stp_strs[2] + stp_strs[1] + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
#ifdef VK_USE_PLATFORM_WIN32_KHR
std::string vk_print_vkimportmemorywin32handleinfonv(const VkImportMemoryWin32HandleInfoNV* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[3];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->handleType;
// CODEGEN : file ../vk_helper.py line #1090: NB: Edit here to choose hex vs dec output by variable name
    ss[2] << "0x" << pStruct->handle;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "handleType = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "handle = " + ss[2].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
#endif //VK_USE_PLATFORM_WIN32_KHR
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkinstancecreateinfo(const VkInstanceCreateInfo* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[7];
    string stp_strs[4];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1012
        if (pStruct->pApplicationInfo) {
// CODEGEN : file ../vk_helper.py line #1024
        tmp_str = vk_print_vkapplicationinfo(pStruct->pApplicationInfo, extra_indent);
        ss[1] << "0x" << &pStruct->pApplicationInfo;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[1] = " " + prefix + "pApplicationInfo (addr)\n" + tmp_str;
        ss[1].str("");
    }
    else
        stp_strs[1] = "";
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
            stp_strs[2] = "";
            stringstream index_ss;
            if (pStruct->ppEnabledLayerNames) {
                for (uint32_t i = 0; i < pStruct->enabledLayerCount; i++) {
                    index_ss.str("");
                    index_ss << i;
// CODEGEN : file ../vk_helper.py line #990
                    ss[2] << "0x" << pStruct->ppEnabledLayerNames[i];
                    stp_strs[2] += " " + prefix + "ppEnabledLayerNames[" + index_ss.str() + "] = " + ss[2].str() + "\n";
// CODEGEN : file ../vk_helper.py line #1000
                    ss[2].str("");
                }
            }
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
            stp_strs[3] = "";
            if (pStruct->ppEnabledExtensionNames) {
                for (uint32_t i = 0; i < pStruct->enabledExtensionCount; i++) {
                    index_ss.str("");
                    index_ss << i;
// CODEGEN : file ../vk_helper.py line #990
                    ss[3] << "0x" << pStruct->ppEnabledExtensionNames[i];
                    stp_strs[3] += " " + prefix + "ppEnabledExtensionNames[" + index_ss.str() + "] = " + ss[3].str() + "\n";
// CODEGEN : file ../vk_helper.py line #1000
                    ss[3].str("");
                }
            }
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->flags;
// CODEGEN : file ../vk_helper.py line #1086
    ss[2] << "0x" << pStruct->pApplicationInfo;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[3] << pStruct->enabledLayerCount;
// CODEGEN : file ../vk_helper.py line #1061
    ss[4] << "0x" << (void*)pStruct->ppEnabledLayerNames;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[5] << pStruct->enabledExtensionCount;
// CODEGEN : file ../vk_helper.py line #1061
    ss[6] << "0x" << (void*)pStruct->ppEnabledExtensionNames;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "flags = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "pApplicationInfo = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "enabledLayerCount = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "ppEnabledLayerNames = " + ss[4].str() + "\n";
    final_str = final_str + prefix + "enabledExtensionCount = " + ss[5].str() + "\n";
    final_str = final_str + prefix + "ppEnabledExtensionNames = " + ss[6].str() + "\n";
    final_str = final_str + stp_strs[3] + stp_strs[2] + stp_strs[1] + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vklayerproperties(const VkLayerProperties* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[4];
    string stp_strs[2];
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #937
// CODEGEN : file ../vk_helper.py line #939
    stp_strs[0] = "";
    stringstream index_ss;
    for (uint32_t i = 0; i < VK_MAX_EXTENSION_NAME_SIZE; i++) {
        index_ss.str("");
        index_ss << i;
// CODEGEN : file ../vk_helper.py line #990
        ss[0] << pStruct->layerName[i];
        stp_strs[0] += " " + prefix + "layerName[" + index_ss.str() + "] = " + ss[0].str() + "\n";
// CODEGEN : file ../vk_helper.py line #1000
        ss[0].str("");
    }
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #937
// CODEGEN : file ../vk_helper.py line #939
    stp_strs[1] = "";
    for (uint32_t i = 0; i < VK_MAX_DESCRIPTION_SIZE; i++) {
        index_ss.str("");
        index_ss << i;
// CODEGEN : file ../vk_helper.py line #990
        ss[1] << pStruct->description[i];
        stp_strs[1] += " " + prefix + "description[" + index_ss.str() + "] = " + ss[1].str() + "\n";
// CODEGEN : file ../vk_helper.py line #1000
        ss[1].str("");
    }
// CODEGEN : file ../vk_helper.py line #1061
    ss[0] << "0x" << (void*)pStruct->layerName;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[1] << pStruct->specVersion;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[2] << pStruct->implementationVersion;
// CODEGEN : file ../vk_helper.py line #1061
    ss[3] << "0x" << (void*)pStruct->description;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "layerName = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "specVersion = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "implementationVersion = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "description = " + ss[3].str() + "\n";
    final_str = final_str + stp_strs[1] + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkmappedmemoryrange(const VkMappedMemoryRange* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[4];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->memory;
// CODEGEN : file ../vk_helper.py line #1086
    ss[2] << "0x" << pStruct->offset;
// CODEGEN : file ../vk_helper.py line #1086
    ss[3] << "0x" << pStruct->size;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "memory = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "offset = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "size = " + ss[3].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkmemoryallocateinfo(const VkMemoryAllocateInfo* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[3];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->allocationSize;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[2] << pStruct->memoryTypeIndex;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "allocationSize = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "memoryTypeIndex = " + ss[2].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkmemorybarrier(const VkMemoryBarrier* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[3];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->srcAccessMask;
// CODEGEN : file ../vk_helper.py line #1086
    ss[2] << "0x" << pStruct->dstAccessMask;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "srcAccessMask = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "dstAccessMask = " + ss[2].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkmemoryheap(const VkMemoryHeap* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[2];
// CODEGEN : file ../vk_helper.py line #1086
    ss[0] << "0x" << pStruct->size;
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->flags;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "size = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "flags = " + ss[1].str() + "\n";
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkmemoryrequirements(const VkMemoryRequirements* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[3];
// CODEGEN : file ../vk_helper.py line #1086
    ss[0] << "0x" << pStruct->size;
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->alignment;
// CODEGEN : file ../vk_helper.py line #1090: NB: Edit here to choose hex vs dec output by variable name
    ss[2] << "0x" << pStruct->memoryTypeBits;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "size = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "alignment = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "memoryTypeBits = " + ss[2].str() + "\n";
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkmemorytype(const VkMemoryType* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[2];
// CODEGEN : file ../vk_helper.py line #1086
    ss[0] << "0x" << pStruct->propertyFlags;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[1] << pStruct->heapIndex;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "propertyFlags = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "heapIndex = " + ss[1].str() + "\n";
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
#ifdef VK_USE_PLATFORM_MIR_KHR
std::string vk_print_vkmirsurfacecreateinfokhr(const VkMirSurfaceCreateInfoKHR* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[4];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->flags;
// CODEGEN : file ../vk_helper.py line #1086
    ss[2] << "0x" << pStruct->connection;
// CODEGEN : file ../vk_helper.py line #1086
    ss[3] << "0x" << pStruct->mirSurface;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "flags = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "connection = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "mirSurface = " + ss[3].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
#endif //VK_USE_PLATFORM_MIR_KHR
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkoffset2d(const VkOffset2D* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[2];
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[0] << pStruct->x;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[1] << pStruct->y;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "x = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "y = " + ss[1].str() + "\n";
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkoffset3d(const VkOffset3D* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[3];
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[0] << pStruct->x;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[1] << pStruct->y;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[2] << pStruct->z;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "x = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "y = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "z = " + ss[2].str() + "\n";
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkphysicaldevicefeatures(const VkPhysicalDeviceFeatures* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[55];
// CODEGEN : file ../vk_helper.py line #1064
    ss[0].str(pStruct->robustBufferAccess ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[1].str(pStruct->fullDrawIndexUint32 ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[2].str(pStruct->imageCubeArray ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[3].str(pStruct->independentBlend ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[4].str(pStruct->geometryShader ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[5].str(pStruct->tessellationShader ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[6].str(pStruct->sampleRateShading ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[7].str(pStruct->dualSrcBlend ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[8].str(pStruct->logicOp ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[9].str(pStruct->multiDrawIndirect ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[10].str(pStruct->drawIndirectFirstInstance ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[11].str(pStruct->depthClamp ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[12].str(pStruct->depthBiasClamp ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[13].str(pStruct->fillModeNonSolid ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[14].str(pStruct->depthBounds ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[15].str(pStruct->wideLines ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[16].str(pStruct->largePoints ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[17].str(pStruct->alphaToOne ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[18].str(pStruct->multiViewport ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[19].str(pStruct->samplerAnisotropy ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[20].str(pStruct->textureCompressionETC2 ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[21].str(pStruct->textureCompressionASTC_LDR ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[22].str(pStruct->textureCompressionBC ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[23].str(pStruct->occlusionQueryPrecise ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[24].str(pStruct->pipelineStatisticsQuery ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[25].str(pStruct->vertexPipelineStoresAndAtomics ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[26].str(pStruct->fragmentStoresAndAtomics ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[27].str(pStruct->shaderTessellationAndGeometryPointSize ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[28].str(pStruct->shaderImageGatherExtended ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[29].str(pStruct->shaderStorageImageExtendedFormats ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[30].str(pStruct->shaderStorageImageMultisample ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[31].str(pStruct->shaderStorageImageReadWithoutFormat ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[32].str(pStruct->shaderStorageImageWriteWithoutFormat ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[33].str(pStruct->shaderUniformBufferArrayDynamicIndexing ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[34].str(pStruct->shaderSampledImageArrayDynamicIndexing ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[35].str(pStruct->shaderStorageBufferArrayDynamicIndexing ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[36].str(pStruct->shaderStorageImageArrayDynamicIndexing ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[37].str(pStruct->shaderClipDistance ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[38].str(pStruct->shaderCullDistance ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[39].str(pStruct->shaderFloat64 ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[40].str(pStruct->shaderInt64 ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[41].str(pStruct->shaderInt16 ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[42].str(pStruct->shaderResourceResidency ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[43].str(pStruct->shaderResourceMinLod ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[44].str(pStruct->sparseBinding ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[45].str(pStruct->sparseResidencyBuffer ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[46].str(pStruct->sparseResidencyImage2D ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[47].str(pStruct->sparseResidencyImage3D ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[48].str(pStruct->sparseResidency2Samples ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[49].str(pStruct->sparseResidency4Samples ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[50].str(pStruct->sparseResidency8Samples ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[51].str(pStruct->sparseResidency16Samples ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[52].str(pStruct->sparseResidencyAliased ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[53].str(pStruct->variableMultisampleRate ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[54].str(pStruct->inheritedQueries ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "robustBufferAccess = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "fullDrawIndexUint32 = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "imageCubeArray = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "independentBlend = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "geometryShader = " + ss[4].str() + "\n";
    final_str = final_str + prefix + "tessellationShader = " + ss[5].str() + "\n";
    final_str = final_str + prefix + "sampleRateShading = " + ss[6].str() + "\n";
    final_str = final_str + prefix + "dualSrcBlend = " + ss[7].str() + "\n";
    final_str = final_str + prefix + "logicOp = " + ss[8].str() + "\n";
    final_str = final_str + prefix + "multiDrawIndirect = " + ss[9].str() + "\n";
    final_str = final_str + prefix + "drawIndirectFirstInstance = " + ss[10].str() + "\n";
    final_str = final_str + prefix + "depthClamp = " + ss[11].str() + "\n";
    final_str = final_str + prefix + "depthBiasClamp = " + ss[12].str() + "\n";
    final_str = final_str + prefix + "fillModeNonSolid = " + ss[13].str() + "\n";
    final_str = final_str + prefix + "depthBounds = " + ss[14].str() + "\n";
    final_str = final_str + prefix + "wideLines = " + ss[15].str() + "\n";
    final_str = final_str + prefix + "largePoints = " + ss[16].str() + "\n";
    final_str = final_str + prefix + "alphaToOne = " + ss[17].str() + "\n";
    final_str = final_str + prefix + "multiViewport = " + ss[18].str() + "\n";
    final_str = final_str + prefix + "samplerAnisotropy = " + ss[19].str() + "\n";
    final_str = final_str + prefix + "textureCompressionETC2 = " + ss[20].str() + "\n";
    final_str = final_str + prefix + "textureCompressionASTC_LDR = " + ss[21].str() + "\n";
    final_str = final_str + prefix + "textureCompressionBC = " + ss[22].str() + "\n";
    final_str = final_str + prefix + "occlusionQueryPrecise = " + ss[23].str() + "\n";
    final_str = final_str + prefix + "pipelineStatisticsQuery = " + ss[24].str() + "\n";
    final_str = final_str + prefix + "vertexPipelineStoresAndAtomics = " + ss[25].str() + "\n";
    final_str = final_str + prefix + "fragmentStoresAndAtomics = " + ss[26].str() + "\n";
    final_str = final_str + prefix + "shaderTessellationAndGeometryPointSize = " + ss[27].str() + "\n";
    final_str = final_str + prefix + "shaderImageGatherExtended = " + ss[28].str() + "\n";
    final_str = final_str + prefix + "shaderStorageImageExtendedFormats = " + ss[29].str() + "\n";
    final_str = final_str + prefix + "shaderStorageImageMultisample = " + ss[30].str() + "\n";
    final_str = final_str + prefix + "shaderStorageImageReadWithoutFormat = " + ss[31].str() + "\n";
    final_str = final_str + prefix + "shaderStorageImageWriteWithoutFormat = " + ss[32].str() + "\n";
    final_str = final_str + prefix + "shaderUniformBufferArrayDynamicIndexing = " + ss[33].str() + "\n";
    final_str = final_str + prefix + "shaderSampledImageArrayDynamicIndexing = " + ss[34].str() + "\n";
    final_str = final_str + prefix + "shaderStorageBufferArrayDynamicIndexing = " + ss[35].str() + "\n";
    final_str = final_str + prefix + "shaderStorageImageArrayDynamicIndexing = " + ss[36].str() + "\n";
    final_str = final_str + prefix + "shaderClipDistance = " + ss[37].str() + "\n";
    final_str = final_str + prefix + "shaderCullDistance = " + ss[38].str() + "\n";
    final_str = final_str + prefix + "shaderFloat64 = " + ss[39].str() + "\n";
    final_str = final_str + prefix + "shaderInt64 = " + ss[40].str() + "\n";
    final_str = final_str + prefix + "shaderInt16 = " + ss[41].str() + "\n";
    final_str = final_str + prefix + "shaderResourceResidency = " + ss[42].str() + "\n";
    final_str = final_str + prefix + "shaderResourceMinLod = " + ss[43].str() + "\n";
    final_str = final_str + prefix + "sparseBinding = " + ss[44].str() + "\n";
    final_str = final_str + prefix + "sparseResidencyBuffer = " + ss[45].str() + "\n";
    final_str = final_str + prefix + "sparseResidencyImage2D = " + ss[46].str() + "\n";
    final_str = final_str + prefix + "sparseResidencyImage3D = " + ss[47].str() + "\n";
    final_str = final_str + prefix + "sparseResidency2Samples = " + ss[48].str() + "\n";
    final_str = final_str + prefix + "sparseResidency4Samples = " + ss[49].str() + "\n";
    final_str = final_str + prefix + "sparseResidency8Samples = " + ss[50].str() + "\n";
    final_str = final_str + prefix + "sparseResidency16Samples = " + ss[51].str() + "\n";
    final_str = final_str + prefix + "sparseResidencyAliased = " + ss[52].str() + "\n";
    final_str = final_str + prefix + "variableMultisampleRate = " + ss[53].str() + "\n";
    final_str = final_str + prefix + "inheritedQueries = " + ss[54].str() + "\n";
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkphysicaldevicelimits(const VkPhysicalDeviceLimits* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[106];
    string stp_strs[6];
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #937
// CODEGEN : file ../vk_helper.py line #939
    stp_strs[0] = "";
    stringstream index_ss;
    for (uint32_t i = 0; i < 3; i++) {
        index_ss.str("");
        index_ss << i;
// CODEGEN : file ../vk_helper.py line #990
        ss[0] << pStruct->maxComputeWorkGroupCount[i];
        stp_strs[0] += " " + prefix + "maxComputeWorkGroupCount[" + index_ss.str() + "] = " + ss[0].str() + "\n";
// CODEGEN : file ../vk_helper.py line #1000
        ss[0].str("");
    }
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #937
// CODEGEN : file ../vk_helper.py line #939
    stp_strs[1] = "";
    for (uint32_t i = 0; i < 3; i++) {
        index_ss.str("");
        index_ss << i;
// CODEGEN : file ../vk_helper.py line #990
        ss[1] << pStruct->maxComputeWorkGroupSize[i];
        stp_strs[1] += " " + prefix + "maxComputeWorkGroupSize[" + index_ss.str() + "] = " + ss[1].str() + "\n";
// CODEGEN : file ../vk_helper.py line #1000
        ss[1].str("");
    }
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #937
// CODEGEN : file ../vk_helper.py line #939
    stp_strs[2] = "";
    for (uint32_t i = 0; i < 2; i++) {
        index_ss.str("");
        index_ss << i;
// CODEGEN : file ../vk_helper.py line #990
        ss[2] << pStruct->maxViewportDimensions[i];
        stp_strs[2] += " " + prefix + "maxViewportDimensions[" + index_ss.str() + "] = " + ss[2].str() + "\n";
// CODEGEN : file ../vk_helper.py line #1000
        ss[2].str("");
    }
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #937
// CODEGEN : file ../vk_helper.py line #939
    stp_strs[3] = "";
    for (uint32_t i = 0; i < 2; i++) {
        index_ss.str("");
        index_ss << i;
// CODEGEN : file ../vk_helper.py line #990
        ss[3] << pStruct->viewportBoundsRange[i];
        stp_strs[3] += " " + prefix + "viewportBoundsRange[" + index_ss.str() + "] = " + ss[3].str() + "\n";
// CODEGEN : file ../vk_helper.py line #1000
        ss[3].str("");
    }
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #937
// CODEGEN : file ../vk_helper.py line #939
    stp_strs[4] = "";
    for (uint32_t i = 0; i < 2; i++) {
        index_ss.str("");
        index_ss << i;
// CODEGEN : file ../vk_helper.py line #990
        ss[4] << pStruct->pointSizeRange[i];
        stp_strs[4] += " " + prefix + "pointSizeRange[" + index_ss.str() + "] = " + ss[4].str() + "\n";
// CODEGEN : file ../vk_helper.py line #1000
        ss[4].str("");
    }
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #937
// CODEGEN : file ../vk_helper.py line #939
    stp_strs[5] = "";
    for (uint32_t i = 0; i < 2; i++) {
        index_ss.str("");
        index_ss << i;
// CODEGEN : file ../vk_helper.py line #990
        ss[5] << pStruct->lineWidthRange[i];
        stp_strs[5] += " " + prefix + "lineWidthRange[" + index_ss.str() + "] = " + ss[5].str() + "\n";
// CODEGEN : file ../vk_helper.py line #1000
        ss[5].str("");
    }
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[0] << pStruct->maxImageDimension1D;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[1] << pStruct->maxImageDimension2D;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[2] << pStruct->maxImageDimension3D;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[3] << pStruct->maxImageDimensionCube;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[4] << pStruct->maxImageArrayLayers;
// CODEGEN : file ../vk_helper.py line #1090: NB: Edit here to choose hex vs dec output by variable name
    ss[5] << "0x" << pStruct->maxTexelBufferElements;
// CODEGEN : file ../vk_helper.py line #1090: NB: Edit here to choose hex vs dec output by variable name
    ss[6] << "0x" << pStruct->maxUniformBufferRange;
// CODEGEN : file ../vk_helper.py line #1090: NB: Edit here to choose hex vs dec output by variable name
    ss[7] << "0x" << pStruct->maxStorageBufferRange;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[8] << pStruct->maxPushConstantsSize;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[9] << pStruct->maxMemoryAllocationCount;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[10] << pStruct->maxSamplerAllocationCount;
// CODEGEN : file ../vk_helper.py line #1086
    ss[11] << "0x" << pStruct->bufferImageGranularity;
// CODEGEN : file ../vk_helper.py line #1086
    ss[12] << "0x" << pStruct->sparseAddressSpaceSize;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[13] << pStruct->maxBoundDescriptorSets;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[14] << pStruct->maxPerStageDescriptorSamplers;
// CODEGEN : file ../vk_helper.py line #1090: NB: Edit here to choose hex vs dec output by variable name
    ss[15] << "0x" << pStruct->maxPerStageDescriptorUniformBuffers;
// CODEGEN : file ../vk_helper.py line #1090: NB: Edit here to choose hex vs dec output by variable name
    ss[16] << "0x" << pStruct->maxPerStageDescriptorStorageBuffers;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[17] << pStruct->maxPerStageDescriptorSampledImages;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[18] << pStruct->maxPerStageDescriptorStorageImages;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[19] << pStruct->maxPerStageDescriptorInputAttachments;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[20] << pStruct->maxPerStageResources;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[21] << pStruct->maxDescriptorSetSamplers;
// CODEGEN : file ../vk_helper.py line #1090: NB: Edit here to choose hex vs dec output by variable name
    ss[22] << "0x" << pStruct->maxDescriptorSetUniformBuffers;
// CODEGEN : file ../vk_helper.py line #1090: NB: Edit here to choose hex vs dec output by variable name
    ss[23] << "0x" << pStruct->maxDescriptorSetUniformBuffersDynamic;
// CODEGEN : file ../vk_helper.py line #1090: NB: Edit here to choose hex vs dec output by variable name
    ss[24] << "0x" << pStruct->maxDescriptorSetStorageBuffers;
// CODEGEN : file ../vk_helper.py line #1090: NB: Edit here to choose hex vs dec output by variable name
    ss[25] << "0x" << pStruct->maxDescriptorSetStorageBuffersDynamic;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[26] << pStruct->maxDescriptorSetSampledImages;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[27] << pStruct->maxDescriptorSetStorageImages;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[28] << pStruct->maxDescriptorSetInputAttachments;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[29] << pStruct->maxVertexInputAttributes;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[30] << pStruct->maxVertexInputBindings;
// CODEGEN : file ../vk_helper.py line #1090: NB: Edit here to choose hex vs dec output by variable name
    ss[31] << "0x" << pStruct->maxVertexInputAttributeOffset;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[32] << pStruct->maxVertexInputBindingStride;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[33] << pStruct->maxVertexOutputComponents;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[34] << pStruct->maxTessellationGenerationLevel;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[35] << pStruct->maxTessellationPatchSize;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[36] << pStruct->maxTessellationControlPerVertexInputComponents;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[37] << pStruct->maxTessellationControlPerVertexOutputComponents;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[38] << pStruct->maxTessellationControlPerPatchOutputComponents;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[39] << pStruct->maxTessellationControlTotalOutputComponents;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[40] << pStruct->maxTessellationEvaluationInputComponents;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[41] << pStruct->maxTessellationEvaluationOutputComponents;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[42] << pStruct->maxGeometryShaderInvocations;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[43] << pStruct->maxGeometryInputComponents;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[44] << pStruct->maxGeometryOutputComponents;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[45] << pStruct->maxGeometryOutputVertices;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[46] << pStruct->maxGeometryTotalOutputComponents;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[47] << pStruct->maxFragmentInputComponents;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[48] << pStruct->maxFragmentOutputAttachments;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[49] << pStruct->maxFragmentDualSrcAttachments;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[50] << pStruct->maxFragmentCombinedOutputResources;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[51] << pStruct->maxComputeSharedMemorySize;
// CODEGEN : file ../vk_helper.py line #1061
    ss[52] << "0x" << (void*)pStruct->maxComputeWorkGroupCount;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[53] << pStruct->maxComputeWorkGroupInvocations;
// CODEGEN : file ../vk_helper.py line #1061
    ss[54] << "0x" << (void*)pStruct->maxComputeWorkGroupSize;
// CODEGEN : file ../vk_helper.py line #1090: NB: Edit here to choose hex vs dec output by variable name
    ss[55] << "0x" << pStruct->subPixelPrecisionBits;
// CODEGEN : file ../vk_helper.py line #1090: NB: Edit here to choose hex vs dec output by variable name
    ss[56] << "0x" << pStruct->subTexelPrecisionBits;
// CODEGEN : file ../vk_helper.py line #1090: NB: Edit here to choose hex vs dec output by variable name
    ss[57] << "0x" << pStruct->mipmapPrecisionBits;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[58] << pStruct->maxDrawIndexedIndexValue;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[59] << pStruct->maxDrawIndirectCount;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[60] << pStruct->maxSamplerLodBias;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[61] << pStruct->maxSamplerAnisotropy;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[62] << pStruct->maxViewports;
// CODEGEN : file ../vk_helper.py line #1061
    ss[63] << "0x" << (void*)pStruct->maxViewportDimensions;
// CODEGEN : file ../vk_helper.py line #1061
    ss[64] << "0x" << (void*)pStruct->viewportBoundsRange;
// CODEGEN : file ../vk_helper.py line #1090: NB: Edit here to choose hex vs dec output by variable name
    ss[65] << "0x" << pStruct->viewportSubPixelBits;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[66] << pStruct->minMemoryMapAlignment;
// CODEGEN : file ../vk_helper.py line #1086
    ss[67] << "0x" << pStruct->minTexelBufferOffsetAlignment;
// CODEGEN : file ../vk_helper.py line #1086
    ss[68] << "0x" << pStruct->minUniformBufferOffsetAlignment;
// CODEGEN : file ../vk_helper.py line #1086
    ss[69] << "0x" << pStruct->minStorageBufferOffsetAlignment;
// CODEGEN : file ../vk_helper.py line #1090: NB: Edit here to choose hex vs dec output by variable name
    ss[70] << "0x" << pStruct->minTexelOffset;
// CODEGEN : file ../vk_helper.py line #1090: NB: Edit here to choose hex vs dec output by variable name
    ss[71] << "0x" << pStruct->maxTexelOffset;
// CODEGEN : file ../vk_helper.py line #1090: NB: Edit here to choose hex vs dec output by variable name
    ss[72] << "0x" << pStruct->minTexelGatherOffset;
// CODEGEN : file ../vk_helper.py line #1090: NB: Edit here to choose hex vs dec output by variable name
    ss[73] << "0x" << pStruct->maxTexelGatherOffset;
// CODEGEN : file ../vk_helper.py line #1090: NB: Edit here to choose hex vs dec output by variable name
    ss[74] << "0x" << pStruct->minInterpolationOffset;
// CODEGEN : file ../vk_helper.py line #1090: NB: Edit here to choose hex vs dec output by variable name
    ss[75] << "0x" << pStruct->maxInterpolationOffset;
// CODEGEN : file ../vk_helper.py line #1090: NB: Edit here to choose hex vs dec output by variable name
    ss[76] << "0x" << pStruct->subPixelInterpolationOffsetBits;
// CODEGEN : file ../vk_helper.py line #1090: NB: Edit here to choose hex vs dec output by variable name
    ss[77] << "0x" << pStruct->maxFramebufferWidth;
// CODEGEN : file ../vk_helper.py line #1090: NB: Edit here to choose hex vs dec output by variable name
    ss[78] << "0x" << pStruct->maxFramebufferHeight;
// CODEGEN : file ../vk_helper.py line #1090: NB: Edit here to choose hex vs dec output by variable name
    ss[79] << "0x" << pStruct->maxFramebufferLayers;
// CODEGEN : file ../vk_helper.py line #1086
    ss[80] << "0x" << pStruct->framebufferColorSampleCounts;
// CODEGEN : file ../vk_helper.py line #1086
    ss[81] << "0x" << pStruct->framebufferDepthSampleCounts;
// CODEGEN : file ../vk_helper.py line #1086
    ss[82] << "0x" << pStruct->framebufferStencilSampleCounts;
// CODEGEN : file ../vk_helper.py line #1086
    ss[83] << "0x" << pStruct->framebufferNoAttachmentsSampleCounts;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[84] << pStruct->maxColorAttachments;
// CODEGEN : file ../vk_helper.py line #1086
    ss[85] << "0x" << pStruct->sampledImageColorSampleCounts;
// CODEGEN : file ../vk_helper.py line #1086
    ss[86] << "0x" << pStruct->sampledImageIntegerSampleCounts;
// CODEGEN : file ../vk_helper.py line #1086
    ss[87] << "0x" << pStruct->sampledImageDepthSampleCounts;
// CODEGEN : file ../vk_helper.py line #1086
    ss[88] << "0x" << pStruct->sampledImageStencilSampleCounts;
// CODEGEN : file ../vk_helper.py line #1086
    ss[89] << "0x" << pStruct->storageImageSampleCounts;
// CODEGEN : file ../vk_helper.py line #1090: NB: Edit here to choose hex vs dec output by variable name
    ss[90] << "0x" << pStruct->maxSampleMaskWords;
// CODEGEN : file ../vk_helper.py line #1064
    ss[91].str(pStruct->timestampComputeAndGraphics ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[92] << pStruct->timestampPeriod;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[93] << pStruct->maxClipDistances;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[94] << pStruct->maxCullDistances;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[95] << pStruct->maxCombinedClipAndCullDistances;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[96] << pStruct->discreteQueuePriorities;
// CODEGEN : file ../vk_helper.py line #1061
    ss[97] << "0x" << (void*)pStruct->pointSizeRange;
// CODEGEN : file ../vk_helper.py line #1061
    ss[98] << "0x" << (void*)pStruct->lineWidthRange;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[99] << pStruct->pointSizeGranularity;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[100] << pStruct->lineWidthGranularity;
// CODEGEN : file ../vk_helper.py line #1064
    ss[101].str(pStruct->strictLines ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[102].str(pStruct->standardSampleLocations ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1086
    ss[103] << "0x" << pStruct->optimalBufferCopyOffsetAlignment;
// CODEGEN : file ../vk_helper.py line #1086
    ss[104] << "0x" << pStruct->optimalBufferCopyRowPitchAlignment;
// CODEGEN : file ../vk_helper.py line #1086
    ss[105] << "0x" << pStruct->nonCoherentAtomSize;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "maxImageDimension1D = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "maxImageDimension2D = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "maxImageDimension3D = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "maxImageDimensionCube = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "maxImageArrayLayers = " + ss[4].str() + "\n";
    final_str = final_str + prefix + "maxTexelBufferElements = " + ss[5].str() + "\n";
    final_str = final_str + prefix + "maxUniformBufferRange = " + ss[6].str() + "\n";
    final_str = final_str + prefix + "maxStorageBufferRange = " + ss[7].str() + "\n";
    final_str = final_str + prefix + "maxPushConstantsSize = " + ss[8].str() + "\n";
    final_str = final_str + prefix + "maxMemoryAllocationCount = " + ss[9].str() + "\n";
    final_str = final_str + prefix + "maxSamplerAllocationCount = " + ss[10].str() + "\n";
    final_str = final_str + prefix + "bufferImageGranularity = " + ss[11].str() + "\n";
    final_str = final_str + prefix + "sparseAddressSpaceSize = " + ss[12].str() + "\n";
    final_str = final_str + prefix + "maxBoundDescriptorSets = " + ss[13].str() + "\n";
    final_str = final_str + prefix + "maxPerStageDescriptorSamplers = " + ss[14].str() + "\n";
    final_str = final_str + prefix + "maxPerStageDescriptorUniformBuffers = " + ss[15].str() + "\n";
    final_str = final_str + prefix + "maxPerStageDescriptorStorageBuffers = " + ss[16].str() + "\n";
    final_str = final_str + prefix + "maxPerStageDescriptorSampledImages = " + ss[17].str() + "\n";
    final_str = final_str + prefix + "maxPerStageDescriptorStorageImages = " + ss[18].str() + "\n";
    final_str = final_str + prefix + "maxPerStageDescriptorInputAttachments = " + ss[19].str() + "\n";
    final_str = final_str + prefix + "maxPerStageResources = " + ss[20].str() + "\n";
    final_str = final_str + prefix + "maxDescriptorSetSamplers = " + ss[21].str() + "\n";
    final_str = final_str + prefix + "maxDescriptorSetUniformBuffers = " + ss[22].str() + "\n";
    final_str = final_str + prefix + "maxDescriptorSetUniformBuffersDynamic = " + ss[23].str() + "\n";
    final_str = final_str + prefix + "maxDescriptorSetStorageBuffers = " + ss[24].str() + "\n";
    final_str = final_str + prefix + "maxDescriptorSetStorageBuffersDynamic = " + ss[25].str() + "\n";
    final_str = final_str + prefix + "maxDescriptorSetSampledImages = " + ss[26].str() + "\n";
    final_str = final_str + prefix + "maxDescriptorSetStorageImages = " + ss[27].str() + "\n";
    final_str = final_str + prefix + "maxDescriptorSetInputAttachments = " + ss[28].str() + "\n";
    final_str = final_str + prefix + "maxVertexInputAttributes = " + ss[29].str() + "\n";
    final_str = final_str + prefix + "maxVertexInputBindings = " + ss[30].str() + "\n";
    final_str = final_str + prefix + "maxVertexInputAttributeOffset = " + ss[31].str() + "\n";
    final_str = final_str + prefix + "maxVertexInputBindingStride = " + ss[32].str() + "\n";
    final_str = final_str + prefix + "maxVertexOutputComponents = " + ss[33].str() + "\n";
    final_str = final_str + prefix + "maxTessellationGenerationLevel = " + ss[34].str() + "\n";
    final_str = final_str + prefix + "maxTessellationPatchSize = " + ss[35].str() + "\n";
    final_str = final_str + prefix + "maxTessellationControlPerVertexInputComponents = " + ss[36].str() + "\n";
    final_str = final_str + prefix + "maxTessellationControlPerVertexOutputComponents = " + ss[37].str() + "\n";
    final_str = final_str + prefix + "maxTessellationControlPerPatchOutputComponents = " + ss[38].str() + "\n";
    final_str = final_str + prefix + "maxTessellationControlTotalOutputComponents = " + ss[39].str() + "\n";
    final_str = final_str + prefix + "maxTessellationEvaluationInputComponents = " + ss[40].str() + "\n";
    final_str = final_str + prefix + "maxTessellationEvaluationOutputComponents = " + ss[41].str() + "\n";
    final_str = final_str + prefix + "maxGeometryShaderInvocations = " + ss[42].str() + "\n";
    final_str = final_str + prefix + "maxGeometryInputComponents = " + ss[43].str() + "\n";
    final_str = final_str + prefix + "maxGeometryOutputComponents = " + ss[44].str() + "\n";
    final_str = final_str + prefix + "maxGeometryOutputVertices = " + ss[45].str() + "\n";
    final_str = final_str + prefix + "maxGeometryTotalOutputComponents = " + ss[46].str() + "\n";
    final_str = final_str + prefix + "maxFragmentInputComponents = " + ss[47].str() + "\n";
    final_str = final_str + prefix + "maxFragmentOutputAttachments = " + ss[48].str() + "\n";
    final_str = final_str + prefix + "maxFragmentDualSrcAttachments = " + ss[49].str() + "\n";
    final_str = final_str + prefix + "maxFragmentCombinedOutputResources = " + ss[50].str() + "\n";
    final_str = final_str + prefix + "maxComputeSharedMemorySize = " + ss[51].str() + "\n";
    final_str = final_str + prefix + "maxComputeWorkGroupCount = " + ss[52].str() + "\n";
    final_str = final_str + prefix + "maxComputeWorkGroupInvocations = " + ss[53].str() + "\n";
    final_str = final_str + prefix + "maxComputeWorkGroupSize = " + ss[54].str() + "\n";
    final_str = final_str + prefix + "subPixelPrecisionBits = " + ss[55].str() + "\n";
    final_str = final_str + prefix + "subTexelPrecisionBits = " + ss[56].str() + "\n";
    final_str = final_str + prefix + "mipmapPrecisionBits = " + ss[57].str() + "\n";
    final_str = final_str + prefix + "maxDrawIndexedIndexValue = " + ss[58].str() + "\n";
    final_str = final_str + prefix + "maxDrawIndirectCount = " + ss[59].str() + "\n";
    final_str = final_str + prefix + "maxSamplerLodBias = " + ss[60].str() + "\n";
    final_str = final_str + prefix + "maxSamplerAnisotropy = " + ss[61].str() + "\n";
    final_str = final_str + prefix + "maxViewports = " + ss[62].str() + "\n";
    final_str = final_str + prefix + "maxViewportDimensions = " + ss[63].str() + "\n";
    final_str = final_str + prefix + "viewportBoundsRange = " + ss[64].str() + "\n";
    final_str = final_str + prefix + "viewportSubPixelBits = " + ss[65].str() + "\n";
    final_str = final_str + prefix + "minMemoryMapAlignment = " + ss[66].str() + "\n";
    final_str = final_str + prefix + "minTexelBufferOffsetAlignment = " + ss[67].str() + "\n";
    final_str = final_str + prefix + "minUniformBufferOffsetAlignment = " + ss[68].str() + "\n";
    final_str = final_str + prefix + "minStorageBufferOffsetAlignment = " + ss[69].str() + "\n";
    final_str = final_str + prefix + "minTexelOffset = " + ss[70].str() + "\n";
    final_str = final_str + prefix + "maxTexelOffset = " + ss[71].str() + "\n";
    final_str = final_str + prefix + "minTexelGatherOffset = " + ss[72].str() + "\n";
    final_str = final_str + prefix + "maxTexelGatherOffset = " + ss[73].str() + "\n";
    final_str = final_str + prefix + "minInterpolationOffset = " + ss[74].str() + "\n";
    final_str = final_str + prefix + "maxInterpolationOffset = " + ss[75].str() + "\n";
    final_str = final_str + prefix + "subPixelInterpolationOffsetBits = " + ss[76].str() + "\n";
    final_str = final_str + prefix + "maxFramebufferWidth = " + ss[77].str() + "\n";
    final_str = final_str + prefix + "maxFramebufferHeight = " + ss[78].str() + "\n";
    final_str = final_str + prefix + "maxFramebufferLayers = " + ss[79].str() + "\n";
    final_str = final_str + prefix + "framebufferColorSampleCounts = " + ss[80].str() + "\n";
    final_str = final_str + prefix + "framebufferDepthSampleCounts = " + ss[81].str() + "\n";
    final_str = final_str + prefix + "framebufferStencilSampleCounts = " + ss[82].str() + "\n";
    final_str = final_str + prefix + "framebufferNoAttachmentsSampleCounts = " + ss[83].str() + "\n";
    final_str = final_str + prefix + "maxColorAttachments = " + ss[84].str() + "\n";
    final_str = final_str + prefix + "sampledImageColorSampleCounts = " + ss[85].str() + "\n";
    final_str = final_str + prefix + "sampledImageIntegerSampleCounts = " + ss[86].str() + "\n";
    final_str = final_str + prefix + "sampledImageDepthSampleCounts = " + ss[87].str() + "\n";
    final_str = final_str + prefix + "sampledImageStencilSampleCounts = " + ss[88].str() + "\n";
    final_str = final_str + prefix + "storageImageSampleCounts = " + ss[89].str() + "\n";
    final_str = final_str + prefix + "maxSampleMaskWords = " + ss[90].str() + "\n";
    final_str = final_str + prefix + "timestampComputeAndGraphics = " + ss[91].str() + "\n";
    final_str = final_str + prefix + "timestampPeriod = " + ss[92].str() + "\n";
    final_str = final_str + prefix + "maxClipDistances = " + ss[93].str() + "\n";
    final_str = final_str + prefix + "maxCullDistances = " + ss[94].str() + "\n";
    final_str = final_str + prefix + "maxCombinedClipAndCullDistances = " + ss[95].str() + "\n";
    final_str = final_str + prefix + "discreteQueuePriorities = " + ss[96].str() + "\n";
    final_str = final_str + prefix + "pointSizeRange = " + ss[97].str() + "\n";
    final_str = final_str + prefix + "lineWidthRange = " + ss[98].str() + "\n";
    final_str = final_str + prefix + "pointSizeGranularity = " + ss[99].str() + "\n";
    final_str = final_str + prefix + "lineWidthGranularity = " + ss[100].str() + "\n";
    final_str = final_str + prefix + "strictLines = " + ss[101].str() + "\n";
    final_str = final_str + prefix + "standardSampleLocations = " + ss[102].str() + "\n";
    final_str = final_str + prefix + "optimalBufferCopyOffsetAlignment = " + ss[103].str() + "\n";
    final_str = final_str + prefix + "optimalBufferCopyRowPitchAlignment = " + ss[104].str() + "\n";
    final_str = final_str + prefix + "nonCoherentAtomSize = " + ss[105].str() + "\n";
    final_str = final_str + stp_strs[5] + stp_strs[4] + stp_strs[3] + stp_strs[2] + stp_strs[1] + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkphysicaldevicememoryproperties(const VkPhysicalDeviceMemoryProperties* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[4];
    string stp_strs[2];
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #937
// CODEGEN : file ../vk_helper.py line #939
    stp_strs[0] = "";
    stringstream index_ss;
    for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++) {
        index_ss.str("");
        index_ss << i;
// CODEGEN : file ../vk_helper.py line #980
        ss[0] << "0x" << &pStruct->memoryTypes[i];
        tmp_str = vk_print_vkmemorytype(&pStruct->memoryTypes[i], extra_indent);
// CODEGEN : file ../vk_helper.py line #984
        stp_strs[0] += " " + prefix + "memoryTypes[" + index_ss.str() + "] (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1000
        ss[0].str("");
    }
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #937
// CODEGEN : file ../vk_helper.py line #939
    stp_strs[1] = "";
    for (uint32_t i = 0; i < VK_MAX_MEMORY_HEAPS; i++) {
        index_ss.str("");
        index_ss << i;
// CODEGEN : file ../vk_helper.py line #980
        ss[1] << "0x" << &pStruct->memoryHeaps[i];
        tmp_str = vk_print_vkmemoryheap(&pStruct->memoryHeaps[i], extra_indent);
// CODEGEN : file ../vk_helper.py line #984
        stp_strs[1] += " " + prefix + "memoryHeaps[" + index_ss.str() + "] (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1000
        ss[1].str("");
    }
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[0] << pStruct->memoryTypeCount;
// CODEGEN : file ../vk_helper.py line #1055
    ss[1].str("addr");
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[2] << pStruct->memoryHeapCount;
// CODEGEN : file ../vk_helper.py line #1055
    ss[3].str("addr");
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "memoryTypeCount = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "memoryTypes = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "memoryHeapCount = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "memoryHeaps = " + ss[3].str() + "\n";
    final_str = final_str + stp_strs[1] + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkphysicaldeviceproperties(const VkPhysicalDeviceProperties* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[8];
    string stp_strs[4];
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #937
// CODEGEN : file ../vk_helper.py line #939
    stp_strs[0] = "";
    stringstream index_ss;
    for (uint32_t i = 0; i < VK_MAX_PHYSICAL_DEVICE_NAME_SIZE; i++) {
        index_ss.str("");
        index_ss << i;
// CODEGEN : file ../vk_helper.py line #990
        ss[0] << pStruct->deviceName[i];
        stp_strs[0] += " " + prefix + "deviceName[" + index_ss.str() + "] = " + ss[0].str() + "\n";
// CODEGEN : file ../vk_helper.py line #1000
        ss[0].str("");
    }
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #937
// CODEGEN : file ../vk_helper.py line #939
    stp_strs[1] = "";
    for (uint32_t i = 0; i < VK_UUID_SIZE; i++) {
        index_ss.str("");
        index_ss << i;
// CODEGEN : file ../vk_helper.py line #990
        ss[1] << "0x" << pStruct->pipelineCacheUUID[i];
        stp_strs[1] += " " + prefix + "pipelineCacheUUID[" + index_ss.str() + "] = " + ss[1].str() + "\n";
// CODEGEN : file ../vk_helper.py line #1000
        ss[1].str("");
    }
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkphysicaldevicelimits(&pStruct->limits, extra_indent);
    ss[2] << "0x" << &pStruct->limits;
    stp_strs[2] = " " + prefix + "limits (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[2].str("");
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkphysicaldevicesparseproperties(&pStruct->sparseProperties, extra_indent);
    ss[3] << "0x" << &pStruct->sparseProperties;
    stp_strs[3] = " " + prefix + "sparseProperties (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[3].str("");
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[0] << pStruct->apiVersion;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[1] << pStruct->driverVersion;
// CODEGEN : file ../vk_helper.py line #1090: NB: Edit here to choose hex vs dec output by variable name
    ss[2] << "0x" << pStruct->vendorID;
// CODEGEN : file ../vk_helper.py line #1090: NB: Edit here to choose hex vs dec output by variable name
    ss[3] << "0x" << pStruct->deviceID;
// CODEGEN : file ../vk_helper.py line #1061
    ss[4] << "0x" << (void*)pStruct->deviceName;
// CODEGEN : file ../vk_helper.py line #1061
    ss[5] << "0x" << (void*)pStruct->pipelineCacheUUID;
// CODEGEN : file ../vk_helper.py line #1055
    ss[6].str("addr");
// CODEGEN : file ../vk_helper.py line #1055
    ss[7].str("addr");
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "apiVersion = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "driverVersion = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "vendorID = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "deviceID = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "deviceType = " + string_VkPhysicalDeviceType(pStruct->deviceType) + "\n";
    final_str = final_str + prefix + "deviceName = " + ss[4].str() + "\n";
    final_str = final_str + prefix + "pipelineCacheUUID = " + ss[5].str() + "\n";
    final_str = final_str + prefix + "limits = " + ss[6].str() + "\n";
    final_str = final_str + prefix + "sparseProperties = " + ss[7].str() + "\n";
    final_str = final_str + stp_strs[3] + stp_strs[2] + stp_strs[1] + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkphysicaldevicesparseproperties(const VkPhysicalDeviceSparseProperties* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[5];
// CODEGEN : file ../vk_helper.py line #1064
    ss[0].str(pStruct->residencyStandard2DBlockShape ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[1].str(pStruct->residencyStandard2DMultisampleBlockShape ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[2].str(pStruct->residencyStandard3DBlockShape ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[3].str(pStruct->residencyAlignedMipSize ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[4].str(pStruct->residencyNonResidentStrict ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "residencyStandard2DBlockShape = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "residencyStandard2DMultisampleBlockShape = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "residencyStandard3DBlockShape = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "residencyAlignedMipSize = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "residencyNonResidentStrict = " + ss[4].str() + "\n";
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkpipelinecachecreateinfo(const VkPipelineCacheCreateInfo* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[4];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->flags;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[2] << pStruct->initialDataSize;
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[3] << "0x" << pStruct->pInitialData;
    else
        ss[3].str("address");
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "flags = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "initialDataSize = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "pInitialData = " + ss[3].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkpipelinecolorblendattachmentstate(const VkPipelineColorBlendAttachmentState* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[2];
// CODEGEN : file ../vk_helper.py line #1064
    ss[0].str(pStruct->blendEnable ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->colorWriteMask;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "blendEnable = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "srcColorBlendFactor = " + string_VkBlendFactor(pStruct->srcColorBlendFactor) + "\n";
    final_str = final_str + prefix + "dstColorBlendFactor = " + string_VkBlendFactor(pStruct->dstColorBlendFactor) + "\n";
    final_str = final_str + prefix + "colorBlendOp = " + string_VkBlendOp(pStruct->colorBlendOp) + "\n";
    final_str = final_str + prefix + "srcAlphaBlendFactor = " + string_VkBlendFactor(pStruct->srcAlphaBlendFactor) + "\n";
    final_str = final_str + prefix + "dstAlphaBlendFactor = " + string_VkBlendFactor(pStruct->dstAlphaBlendFactor) + "\n";
    final_str = final_str + prefix + "alphaBlendOp = " + string_VkBlendOp(pStruct->alphaBlendOp) + "\n";
    final_str = final_str + prefix + "colorWriteMask = " + ss[1].str() + "\n";
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkpipelinecolorblendstatecreateinfo(const VkPipelineColorBlendStateCreateInfo* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[6];
    string stp_strs[3];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
        stp_strs[1] = "";
        stringstream index_ss;
        if (pStruct->pAttachments) {
            for (uint32_t i = 0; i < pStruct->attachmentCount; i++) {
                index_ss.str("");
                index_ss << i;
// CODEGEN : file ../vk_helper.py line #980
                ss[1] << "0x" << &pStruct->pAttachments[i];
                tmp_str = vk_print_vkpipelinecolorblendattachmentstate(&pStruct->pAttachments[i], extra_indent);
// CODEGEN : file ../vk_helper.py line #984
                stp_strs[1] += " " + prefix + "pAttachments[" + index_ss.str() + "] (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1000
                ss[1].str("");
            }
        }
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #937
// CODEGEN : file ../vk_helper.py line #939
        stp_strs[2] = "";
        for (uint32_t i = 0; i < 4; i++) {
            index_ss.str("");
            index_ss << i;
// CODEGEN : file ../vk_helper.py line #990
            ss[2] << pStruct->blendConstants[i];
            stp_strs[2] += " " + prefix + "blendConstants[" + index_ss.str() + "] = " + ss[2].str() + "\n";
// CODEGEN : file ../vk_helper.py line #1000
            ss[2].str("");
        }
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->flags;
// CODEGEN : file ../vk_helper.py line #1064
    ss[2].str(pStruct->logicOpEnable ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[3] << pStruct->attachmentCount;
// CODEGEN : file ../vk_helper.py line #1061
    ss[4] << "0x" << (void*)pStruct->pAttachments;
// CODEGEN : file ../vk_helper.py line #1061
    ss[5] << "0x" << (void*)pStruct->blendConstants;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "flags = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "logicOpEnable = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "logicOp = " + string_VkLogicOp(pStruct->logicOp) + "\n";
    final_str = final_str + prefix + "attachmentCount = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "pAttachments = " + ss[4].str() + "\n";
    final_str = final_str + prefix + "blendConstants = " + ss[5].str() + "\n";
    final_str = final_str + stp_strs[2] + stp_strs[1] + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkpipelinedepthstencilstatecreateinfo(const VkPipelineDepthStencilStateCreateInfo* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[10];
    string stp_strs[3];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkstencilopstate(&pStruct->front, extra_indent);
    ss[1] << "0x" << &pStruct->front;
    stp_strs[1] = " " + prefix + "front (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[1].str("");
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkstencilopstate(&pStruct->back, extra_indent);
    ss[2] << "0x" << &pStruct->back;
    stp_strs[2] = " " + prefix + "back (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[2].str("");
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->flags;
// CODEGEN : file ../vk_helper.py line #1064
    ss[2].str(pStruct->depthTestEnable ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[3].str(pStruct->depthWriteEnable ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[4].str(pStruct->depthBoundsTestEnable ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[5].str(pStruct->stencilTestEnable ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1055
    ss[6].str("addr");
// CODEGEN : file ../vk_helper.py line #1055
    ss[7].str("addr");
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[8] << pStruct->minDepthBounds;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[9] << pStruct->maxDepthBounds;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "flags = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "depthTestEnable = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "depthWriteEnable = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "depthCompareOp = " + string_VkCompareOp(pStruct->depthCompareOp) + "\n";
    final_str = final_str + prefix + "depthBoundsTestEnable = " + ss[4].str() + "\n";
    final_str = final_str + prefix + "stencilTestEnable = " + ss[5].str() + "\n";
    final_str = final_str + prefix + "front = " + ss[6].str() + "\n";
    final_str = final_str + prefix + "back = " + ss[7].str() + "\n";
    final_str = final_str + prefix + "minDepthBounds = " + ss[8].str() + "\n";
    final_str = final_str + prefix + "maxDepthBounds = " + ss[9].str() + "\n";
    final_str = final_str + stp_strs[2] + stp_strs[1] + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkpipelinedynamicstatecreateinfo(const VkPipelineDynamicStateCreateInfo* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[4];
    string stp_strs[2];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
        stp_strs[1] = "";
        stringstream index_ss;
        if (pStruct->pDynamicStates) {
            for (uint32_t i = 0; i < pStruct->dynamicStateCount; i++) {
                index_ss.str("");
                index_ss << i;
// CODEGEN : file ../vk_helper.py line #974
                ss[1] << string_VkDynamicState(pStruct->pDynamicStates[i]);
                stp_strs[1] += " " + prefix + "pDynamicStates[" + index_ss.str() + "] = " + ss[1].str() + "\n";
// CODEGEN : file ../vk_helper.py line #1000
                ss[1].str("");
            }
        }
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->flags;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[2] << pStruct->dynamicStateCount;
// CODEGEN : file ../vk_helper.py line #1100
    if (pStruct->pDynamicStates)
        ss[3] << "0x" << pStruct->pDynamicStates << " (See individual array values below)";
    else
        ss[3].str("NULL");
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "flags = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "dynamicStateCount = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "pDynamicStates = " + ss[3].str() + "\n";
    final_str = final_str + stp_strs[1] + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkpipelineinputassemblystatecreateinfo(const VkPipelineInputAssemblyStateCreateInfo* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[3];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->flags;
// CODEGEN : file ../vk_helper.py line #1064
    ss[2].str(pStruct->primitiveRestartEnable ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "flags = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "topology = " + string_VkPrimitiveTopology(pStruct->topology) + "\n";
    final_str = final_str + prefix + "primitiveRestartEnable = " + ss[2].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkpipelinelayoutcreateinfo(const VkPipelineLayoutCreateInfo* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[6];
    string stp_strs[3];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
        stp_strs[1] = "";
        stringstream index_ss;
        if (pStruct->pSetLayouts) {
            for (uint32_t i = 0; i < pStruct->setLayoutCount; i++) {
                index_ss.str("");
                index_ss << i;
// CODEGEN : file ../vk_helper.py line #990
                ss[1] << "0x" << pStruct->pSetLayouts[i];
                stp_strs[1] += " " + prefix + "pSetLayouts[" + index_ss.str() + "].handle = " + ss[1].str() + "\n";
// CODEGEN : file ../vk_helper.py line #1000
                ss[1].str("");
            }
        }
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
        stp_strs[2] = "";
        if (pStruct->pPushConstantRanges) {
            for (uint32_t i = 0; i < pStruct->pushConstantRangeCount; i++) {
                index_ss.str("");
                index_ss << i;
// CODEGEN : file ../vk_helper.py line #980
                ss[2] << "0x" << &pStruct->pPushConstantRanges[i];
                tmp_str = vk_print_vkpushconstantrange(&pStruct->pPushConstantRanges[i], extra_indent);
// CODEGEN : file ../vk_helper.py line #984
                stp_strs[2] += " " + prefix + "pPushConstantRanges[" + index_ss.str() + "] (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1000
                ss[2].str("");
            }
        }
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->flags;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[2] << pStruct->setLayoutCount;
// CODEGEN : file ../vk_helper.py line #1061
    ss[3] << "0x" << (void*)pStruct->pSetLayouts;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[4] << pStruct->pushConstantRangeCount;
// CODEGEN : file ../vk_helper.py line #1061
    ss[5] << "0x" << (void*)pStruct->pPushConstantRanges;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "flags = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "setLayoutCount = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "pSetLayouts = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "pushConstantRangeCount = " + ss[4].str() + "\n";
    final_str = final_str + prefix + "pPushConstantRanges = " + ss[5].str() + "\n";
    final_str = final_str + stp_strs[2] + stp_strs[1] + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkpipelinemultisamplestatecreateinfo(const VkPipelineMultisampleStateCreateInfo* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[7];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->flags;
// CODEGEN : file ../vk_helper.py line #1064
    ss[2].str(pStruct->sampleShadingEnable ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[3] << pStruct->minSampleShading;
// CODEGEN : file ../vk_helper.py line #1086
    ss[4] << "0x" << pStruct->pSampleMask;
// CODEGEN : file ../vk_helper.py line #1064
    ss[5].str(pStruct->alphaToCoverageEnable ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[6].str(pStruct->alphaToOneEnable ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "flags = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "rasterizationSamples = " + string_VkSampleCountFlagBits(pStruct->rasterizationSamples) + "\n";
    final_str = final_str + prefix + "sampleShadingEnable = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "minSampleShading = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "pSampleMask = " + ss[4].str() + "\n";
    final_str = final_str + prefix + "alphaToCoverageEnable = " + ss[5].str() + "\n";
    final_str = final_str + prefix + "alphaToOneEnable = " + ss[6].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkpipelinerasterizationstatecreateinfo(const VkPipelineRasterizationStateCreateInfo* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[10];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->flags;
// CODEGEN : file ../vk_helper.py line #1064
    ss[2].str(pStruct->depthClampEnable ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1064
    ss[3].str(pStruct->rasterizerDiscardEnable ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1086
    ss[4] << "0x" << pStruct->cullMode;
// CODEGEN : file ../vk_helper.py line #1064
    ss[5].str(pStruct->depthBiasEnable ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[6] << pStruct->depthBiasConstantFactor;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[7] << pStruct->depthBiasClamp;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[8] << pStruct->depthBiasSlopeFactor;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[9] << pStruct->lineWidth;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "flags = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "depthClampEnable = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "rasterizerDiscardEnable = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "polygonMode = " + string_VkPolygonMode(pStruct->polygonMode) + "\n";
    final_str = final_str + prefix + "cullMode = " + ss[4].str() + "\n";
    final_str = final_str + prefix + "frontFace = " + string_VkFrontFace(pStruct->frontFace) + "\n";
    final_str = final_str + prefix + "depthBiasEnable = " + ss[5].str() + "\n";
    final_str = final_str + prefix + "depthBiasConstantFactor = " + ss[6].str() + "\n";
    final_str = final_str + prefix + "depthBiasClamp = " + ss[7].str() + "\n";
    final_str = final_str + prefix + "depthBiasSlopeFactor = " + ss[8].str() + "\n";
    final_str = final_str + prefix + "lineWidth = " + ss[9].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkpipelinerasterizationstaterasterizationorderamd(const VkPipelineRasterizationStateRasterizationOrderAMD* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[1];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "rasterizationOrder = " + string_VkRasterizationOrderAMD(pStruct->rasterizationOrder) + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkpipelineshaderstagecreateinfo(const VkPipelineShaderStageCreateInfo* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[5];
    string stp_strs[2];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1012
        if (pStruct->pSpecializationInfo) {
// CODEGEN : file ../vk_helper.py line #1024
        tmp_str = vk_print_vkspecializationinfo(pStruct->pSpecializationInfo, extra_indent);
        ss[1] << "0x" << &pStruct->pSpecializationInfo;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[1] = " " + prefix + "pSpecializationInfo (addr)\n" + tmp_str;
        ss[1].str("");
    }
    else
        stp_strs[1] = "";
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->flags;
// CODEGEN : file ../vk_helper.py line #1086
    ss[2] << "0x" << pStruct->module;
// CODEGEN : file ../vk_helper.py line #1076
    if (pStruct->pName != NULL) {
        ss[3] << pStruct->pName;
     } else {
        ss[3] << "";
     }
// CODEGEN : file ../vk_helper.py line #1086
    ss[4] << "0x" << pStruct->pSpecializationInfo;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "flags = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "stage = " + string_VkShaderStageFlagBits(pStruct->stage) + "\n";
    final_str = final_str + prefix + "module = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "pName = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "pSpecializationInfo = " + ss[4].str() + "\n";
    final_str = final_str + stp_strs[1] + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkpipelinetessellationstatecreateinfo(const VkPipelineTessellationStateCreateInfo* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[3];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->flags;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[2] << pStruct->patchControlPoints;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "flags = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "patchControlPoints = " + ss[2].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkpipelinevertexinputstatecreateinfo(const VkPipelineVertexInputStateCreateInfo* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[6];
    string stp_strs[3];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
        stp_strs[1] = "";
        stringstream index_ss;
        if (pStruct->pVertexBindingDescriptions) {
            for (uint32_t i = 0; i < pStruct->vertexBindingDescriptionCount; i++) {
                index_ss.str("");
                index_ss << i;
// CODEGEN : file ../vk_helper.py line #980
                ss[1] << "0x" << &pStruct->pVertexBindingDescriptions[i];
                tmp_str = vk_print_vkvertexinputbindingdescription(&pStruct->pVertexBindingDescriptions[i], extra_indent);
// CODEGEN : file ../vk_helper.py line #984
                stp_strs[1] += " " + prefix + "pVertexBindingDescriptions[" + index_ss.str() + "] (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1000
                ss[1].str("");
            }
        }
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
        stp_strs[2] = "";
        if (pStruct->pVertexAttributeDescriptions) {
            for (uint32_t i = 0; i < pStruct->vertexAttributeDescriptionCount; i++) {
                index_ss.str("");
                index_ss << i;
// CODEGEN : file ../vk_helper.py line #980
                ss[2] << "0x" << &pStruct->pVertexAttributeDescriptions[i];
                tmp_str = vk_print_vkvertexinputattributedescription(&pStruct->pVertexAttributeDescriptions[i], extra_indent);
// CODEGEN : file ../vk_helper.py line #984
                stp_strs[2] += " " + prefix + "pVertexAttributeDescriptions[" + index_ss.str() + "] (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1000
                ss[2].str("");
            }
        }
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->flags;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[2] << pStruct->vertexBindingDescriptionCount;
// CODEGEN : file ../vk_helper.py line #1061
    ss[3] << "0x" << (void*)pStruct->pVertexBindingDescriptions;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[4] << pStruct->vertexAttributeDescriptionCount;
// CODEGEN : file ../vk_helper.py line #1061
    ss[5] << "0x" << (void*)pStruct->pVertexAttributeDescriptions;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "flags = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "vertexBindingDescriptionCount = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "pVertexBindingDescriptions = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "vertexAttributeDescriptionCount = " + ss[4].str() + "\n";
    final_str = final_str + prefix + "pVertexAttributeDescriptions = " + ss[5].str() + "\n";
    final_str = final_str + stp_strs[2] + stp_strs[1] + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkpipelineviewportstatecreateinfo(const VkPipelineViewportStateCreateInfo* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[6];
    string stp_strs[3];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
        stp_strs[1] = "";
        stringstream index_ss;
        if (pStruct->pViewports) {
            for (uint32_t i = 0; i < pStruct->viewportCount; i++) {
                index_ss.str("");
                index_ss << i;
// CODEGEN : file ../vk_helper.py line #980
                ss[1] << "0x" << &pStruct->pViewports[i];
                tmp_str = vk_print_vkviewport(&pStruct->pViewports[i], extra_indent);
// CODEGEN : file ../vk_helper.py line #984
                stp_strs[1] += " " + prefix + "pViewports[" + index_ss.str() + "] (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1000
                ss[1].str("");
            }
        }
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
        stp_strs[2] = "";
        if (pStruct->pScissors) {
            for (uint32_t i = 0; i < pStruct->scissorCount; i++) {
                index_ss.str("");
                index_ss << i;
// CODEGEN : file ../vk_helper.py line #980
                ss[2] << "0x" << &pStruct->pScissors[i];
                tmp_str = vk_print_vkrect2d(&pStruct->pScissors[i], extra_indent);
// CODEGEN : file ../vk_helper.py line #984
                stp_strs[2] += " " + prefix + "pScissors[" + index_ss.str() + "] (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1000
                ss[2].str("");
            }
        }
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->flags;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[2] << pStruct->viewportCount;
// CODEGEN : file ../vk_helper.py line #1061
    ss[3] << "0x" << (void*)pStruct->pViewports;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[4] << pStruct->scissorCount;
// CODEGEN : file ../vk_helper.py line #1061
    ss[5] << "0x" << (void*)pStruct->pScissors;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "flags = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "viewportCount = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "pViewports = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "scissorCount = " + ss[4].str() + "\n";
    final_str = final_str + prefix + "pScissors = " + ss[5].str() + "\n";
    final_str = final_str + stp_strs[2] + stp_strs[1] + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkpresentinfokhr(const VkPresentInfoKHR* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[7];
    string stp_strs[4];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
        stp_strs[1] = "";
        stringstream index_ss;
        if (pStruct->pWaitSemaphores) {
            for (uint32_t i = 0; i < pStruct->waitSemaphoreCount; i++) {
                index_ss.str("");
                index_ss << i;
// CODEGEN : file ../vk_helper.py line #990
                ss[1] << "0x" << pStruct->pWaitSemaphores[i];
                stp_strs[1] += " " + prefix + "pWaitSemaphores[" + index_ss.str() + "].handle = " + ss[1].str() + "\n";
// CODEGEN : file ../vk_helper.py line #1000
                ss[1].str("");
            }
        }
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
        stp_strs[2] = "";
        if (pStruct->pSwapchains) {
            for (uint32_t i = 0; i < pStruct->swapchainCount; i++) {
                index_ss.str("");
                index_ss << i;
// CODEGEN : file ../vk_helper.py line #990
                ss[2] << "0x" << pStruct->pSwapchains[i];
                stp_strs[2] += " " + prefix + "pSwapchains[" + index_ss.str() + "] = " + ss[2].str() + "\n";
// CODEGEN : file ../vk_helper.py line #1000
                ss[2].str("");
            }
        }
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
        stp_strs[3] = "";
        if (pStruct->pImageIndices) {
            for (uint32_t i = 0; i < pStruct->swapchainCount; i++) {
                index_ss.str("");
                index_ss << i;
// CODEGEN : file ../vk_helper.py line #990
                ss[3] << "0x" << pStruct->pImageIndices[i];
                stp_strs[3] += " " + prefix + "pImageIndices[" + index_ss.str() + "] = " + ss[3].str() + "\n";
// CODEGEN : file ../vk_helper.py line #1000
                ss[3].str("");
            }
        }
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[1] << pStruct->waitSemaphoreCount;
// CODEGEN : file ../vk_helper.py line #1061
    ss[2] << "0x" << (void*)pStruct->pWaitSemaphores;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[3] << pStruct->swapchainCount;
// CODEGEN : file ../vk_helper.py line #1061
    ss[4] << "0x" << (void*)pStruct->pSwapchains;
// CODEGEN : file ../vk_helper.py line #1061
    ss[5] << "0x" << (void*)pStruct->pImageIndices;
// CODEGEN : file ../vk_helper.py line #1100
    if (pStruct->pResults)
        ss[6] << "0x" << pStruct->pResults << " (See individual array values below)";
    else
        ss[6].str("NULL");
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "waitSemaphoreCount = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "pWaitSemaphores = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "swapchainCount = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "pSwapchains = " + ss[4].str() + "\n";
    final_str = final_str + prefix + "pImageIndices = " + ss[5].str() + "\n";
    final_str = final_str + prefix + "pResults = " + ss[6].str() + "\n";
    final_str = final_str + stp_strs[3] + stp_strs[2] + stp_strs[1] + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkpushconstantrange(const VkPushConstantRange* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[3];
// CODEGEN : file ../vk_helper.py line #1086
    ss[0] << "0x" << pStruct->stageFlags;
// CODEGEN : file ../vk_helper.py line #1090: NB: Edit here to choose hex vs dec output by variable name
    ss[1] << "0x" << pStruct->offset;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[2] << pStruct->size;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "stageFlags = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "offset = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "size = " + ss[2].str() + "\n";
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkquerypoolcreateinfo(const VkQueryPoolCreateInfo* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[4];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->flags;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[2] << pStruct->queryCount;
// CODEGEN : file ../vk_helper.py line #1086
    ss[3] << "0x" << pStruct->pipelineStatistics;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "flags = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "queryType = " + string_VkQueryType(pStruct->queryType) + "\n";
    final_str = final_str + prefix + "queryCount = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "pipelineStatistics = " + ss[3].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkqueuefamilyproperties(const VkQueueFamilyProperties* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[4];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkextent3d(&pStruct->minImageTransferGranularity, extra_indent);
    ss[0] << "0x" << &pStruct->minImageTransferGranularity;
    stp_strs[0] = " " + prefix + "minImageTransferGranularity (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[0].str("");
// CODEGEN : file ../vk_helper.py line #1086
    ss[0] << "0x" << pStruct->queueFlags;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[1] << pStruct->queueCount;
// CODEGEN : file ../vk_helper.py line #1090: NB: Edit here to choose hex vs dec output by variable name
    ss[2] << "0x" << pStruct->timestampValidBits;
// CODEGEN : file ../vk_helper.py line #1055
    ss[3].str("addr");
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "queueFlags = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "queueCount = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "timestampValidBits = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "minImageTransferGranularity = " + ss[3].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkrect2d(const VkRect2D* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[2];
    string stp_strs[2];
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkoffset2d(&pStruct->offset, extra_indent);
    ss[0] << "0x" << &pStruct->offset;
    stp_strs[0] = " " + prefix + "offset (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[0].str("");
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkextent2d(&pStruct->extent, extra_indent);
    ss[1] << "0x" << &pStruct->extent;
    stp_strs[1] = " " + prefix + "extent (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[1].str("");
// CODEGEN : file ../vk_helper.py line #1055
    ss[0].str("addr");
// CODEGEN : file ../vk_helper.py line #1055
    ss[1].str("addr");
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "offset = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "extent = " + ss[1].str() + "\n";
    final_str = final_str + stp_strs[1] + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkrenderpassbegininfo(const VkRenderPassBeginInfo* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[6];
    string stp_strs[3];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkrect2d(&pStruct->renderArea, extra_indent);
    ss[1] << "0x" << &pStruct->renderArea;
    stp_strs[1] = " " + prefix + "renderArea (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[1].str("");
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
        stp_strs[2] = "";
        stringstream index_ss;
        if (pStruct->pClearValues) {
            for (uint32_t i = 0; i < pStruct->clearValueCount; i++) {
                index_ss.str("");
                index_ss << i;
// CODEGEN : file ../vk_helper.py line #980
                ss[2] << "0x" << &pStruct->pClearValues[i];
                tmp_str = vk_print_vkclearvalue(&pStruct->pClearValues[i], extra_indent);
// CODEGEN : file ../vk_helper.py line #984
                stp_strs[2] += " " + prefix + "pClearValues[" + index_ss.str() + "] (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1000
                ss[2].str("");
            }
        }
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->renderPass;
// CODEGEN : file ../vk_helper.py line #1086
    ss[2] << "0x" << pStruct->framebuffer;
// CODEGEN : file ../vk_helper.py line #1055
    ss[3].str("addr");
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[4] << pStruct->clearValueCount;
// CODEGEN : file ../vk_helper.py line #1061
    ss[5] << "0x" << (void*)pStruct->pClearValues;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "renderPass = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "framebuffer = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "renderArea = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "clearValueCount = " + ss[4].str() + "\n";
    final_str = final_str + prefix + "pClearValues = " + ss[5].str() + "\n";
    final_str = final_str + stp_strs[2] + stp_strs[1] + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkrenderpasscreateinfo(const VkRenderPassCreateInfo* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[8];
    string stp_strs[4];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
        stp_strs[1] = "";
        stringstream index_ss;
        if (pStruct->pAttachments) {
            for (uint32_t i = 0; i < pStruct->attachmentCount; i++) {
                index_ss.str("");
                index_ss << i;
// CODEGEN : file ../vk_helper.py line #980
                ss[1] << "0x" << &pStruct->pAttachments[i];
                tmp_str = vk_print_vkattachmentdescription(&pStruct->pAttachments[i], extra_indent);
// CODEGEN : file ../vk_helper.py line #984
                stp_strs[1] += " " + prefix + "pAttachments[" + index_ss.str() + "] (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1000
                ss[1].str("");
            }
        }
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
        stp_strs[2] = "";
        if (pStruct->pSubpasses) {
            for (uint32_t i = 0; i < pStruct->subpassCount; i++) {
                index_ss.str("");
                index_ss << i;
// CODEGEN : file ../vk_helper.py line #980
                ss[2] << "0x" << &pStruct->pSubpasses[i];
                tmp_str = vk_print_vksubpassdescription(&pStruct->pSubpasses[i], extra_indent);
// CODEGEN : file ../vk_helper.py line #984
                stp_strs[2] += " " + prefix + "pSubpasses[" + index_ss.str() + "] (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1000
                ss[2].str("");
            }
        }
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
        stp_strs[3] = "";
        if (pStruct->pDependencies) {
            for (uint32_t i = 0; i < pStruct->dependencyCount; i++) {
                index_ss.str("");
                index_ss << i;
// CODEGEN : file ../vk_helper.py line #980
                ss[3] << "0x" << &pStruct->pDependencies[i];
                tmp_str = vk_print_vksubpassdependency(&pStruct->pDependencies[i], extra_indent);
// CODEGEN : file ../vk_helper.py line #984
                stp_strs[3] += " " + prefix + "pDependencies[" + index_ss.str() + "] (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1000
                ss[3].str("");
            }
        }
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->flags;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[2] << pStruct->attachmentCount;
// CODEGEN : file ../vk_helper.py line #1061
    ss[3] << "0x" << (void*)pStruct->pAttachments;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[4] << pStruct->subpassCount;
// CODEGEN : file ../vk_helper.py line #1061
    ss[5] << "0x" << (void*)pStruct->pSubpasses;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[6] << pStruct->dependencyCount;
// CODEGEN : file ../vk_helper.py line #1061
    ss[7] << "0x" << (void*)pStruct->pDependencies;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "flags = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "attachmentCount = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "pAttachments = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "subpassCount = " + ss[4].str() + "\n";
    final_str = final_str + prefix + "pSubpasses = " + ss[5].str() + "\n";
    final_str = final_str + prefix + "dependencyCount = " + ss[6].str() + "\n";
    final_str = final_str + prefix + "pDependencies = " + ss[7].str() + "\n";
    final_str = final_str + stp_strs[3] + stp_strs[2] + stp_strs[1] + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vksamplercreateinfo(const VkSamplerCreateInfo* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[9];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->flags;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[2] << pStruct->mipLodBias;
// CODEGEN : file ../vk_helper.py line #1064
    ss[3].str(pStruct->anisotropyEnable ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[4] << pStruct->maxAnisotropy;
// CODEGEN : file ../vk_helper.py line #1064
    ss[5].str(pStruct->compareEnable ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[6] << pStruct->minLod;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[7] << pStruct->maxLod;
// CODEGEN : file ../vk_helper.py line #1064
    ss[8].str(pStruct->unnormalizedCoordinates ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "flags = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "magFilter = " + string_VkFilter(pStruct->magFilter) + "\n";
    final_str = final_str + prefix + "minFilter = " + string_VkFilter(pStruct->minFilter) + "\n";
    final_str = final_str + prefix + "mipmapMode = " + string_VkSamplerMipmapMode(pStruct->mipmapMode) + "\n";
    final_str = final_str + prefix + "addressModeU = " + string_VkSamplerAddressMode(pStruct->addressModeU) + "\n";
    final_str = final_str + prefix + "addressModeV = " + string_VkSamplerAddressMode(pStruct->addressModeV) + "\n";
    final_str = final_str + prefix + "addressModeW = " + string_VkSamplerAddressMode(pStruct->addressModeW) + "\n";
    final_str = final_str + prefix + "mipLodBias = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "anisotropyEnable = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "maxAnisotropy = " + ss[4].str() + "\n";
    final_str = final_str + prefix + "compareEnable = " + ss[5].str() + "\n";
    final_str = final_str + prefix + "compareOp = " + string_VkCompareOp(pStruct->compareOp) + "\n";
    final_str = final_str + prefix + "minLod = " + ss[6].str() + "\n";
    final_str = final_str + prefix + "maxLod = " + ss[7].str() + "\n";
    final_str = final_str + prefix + "borderColor = " + string_VkBorderColor(pStruct->borderColor) + "\n";
    final_str = final_str + prefix + "unnormalizedCoordinates = " + ss[8].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vksemaphorecreateinfo(const VkSemaphoreCreateInfo* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[2];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->flags;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "flags = " + ss[1].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkshadermodulecreateinfo(const VkShaderModuleCreateInfo* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[4];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->flags;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[2] << pStruct->codeSize;
// CODEGEN : file ../vk_helper.py line #1086
    ss[3] << "0x" << pStruct->pCode;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "flags = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "codeSize = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "pCode = " + ss[3].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vksparsebuffermemorybindinfo(const VkSparseBufferMemoryBindInfo* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[3];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
    stp_strs[0] = "";
    stringstream index_ss;
    if (pStruct->pBinds) {
        for (uint32_t i = 0; i < pStruct->bindCount; i++) {
            index_ss.str("");
            index_ss << i;
// CODEGEN : file ../vk_helper.py line #980
            ss[0] << "0x" << &pStruct->pBinds[i];
            tmp_str = vk_print_vksparsememorybind(&pStruct->pBinds[i], extra_indent);
// CODEGEN : file ../vk_helper.py line #984
            stp_strs[0] += " " + prefix + "pBinds[" + index_ss.str() + "] (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1000
            ss[0].str("");
        }
    }
// CODEGEN : file ../vk_helper.py line #1086
    ss[0] << "0x" << pStruct->buffer;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[1] << pStruct->bindCount;
// CODEGEN : file ../vk_helper.py line #1061
    ss[2] << "0x" << (void*)pStruct->pBinds;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "buffer = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "bindCount = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "pBinds = " + ss[2].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vksparseimageformatproperties(const VkSparseImageFormatProperties* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[3];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkextent3d(&pStruct->imageGranularity, extra_indent);
    ss[0] << "0x" << &pStruct->imageGranularity;
    stp_strs[0] = " " + prefix + "imageGranularity (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[0].str("");
// CODEGEN : file ../vk_helper.py line #1086
    ss[0] << "0x" << pStruct->aspectMask;
// CODEGEN : file ../vk_helper.py line #1055
    ss[1].str("addr");
// CODEGEN : file ../vk_helper.py line #1086
    ss[2] << "0x" << pStruct->flags;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "aspectMask = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "imageGranularity = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "flags = " + ss[2].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vksparseimagememorybind(const VkSparseImageMemoryBind* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[6];
    string stp_strs[3];
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkimagesubresource(&pStruct->subresource, extra_indent);
    ss[0] << "0x" << &pStruct->subresource;
    stp_strs[0] = " " + prefix + "subresource (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[0].str("");
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkoffset3d(&pStruct->offset, extra_indent);
    ss[1] << "0x" << &pStruct->offset;
    stp_strs[1] = " " + prefix + "offset (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[1].str("");
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkextent3d(&pStruct->extent, extra_indent);
    ss[2] << "0x" << &pStruct->extent;
    stp_strs[2] = " " + prefix + "extent (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[2].str("");
// CODEGEN : file ../vk_helper.py line #1055
    ss[0].str("addr");
// CODEGEN : file ../vk_helper.py line #1055
    ss[1].str("addr");
// CODEGEN : file ../vk_helper.py line #1055
    ss[2].str("addr");
// CODEGEN : file ../vk_helper.py line #1086
    ss[3] << "0x" << pStruct->memory;
// CODEGEN : file ../vk_helper.py line #1086
    ss[4] << "0x" << pStruct->memoryOffset;
// CODEGEN : file ../vk_helper.py line #1086
    ss[5] << "0x" << pStruct->flags;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "subresource = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "offset = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "extent = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "memory = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "memoryOffset = " + ss[4].str() + "\n";
    final_str = final_str + prefix + "flags = " + ss[5].str() + "\n";
    final_str = final_str + stp_strs[2] + stp_strs[1] + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vksparseimagememorybindinfo(const VkSparseImageMemoryBindInfo* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[3];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
    stp_strs[0] = "";
    stringstream index_ss;
    if (pStruct->pBinds) {
        for (uint32_t i = 0; i < pStruct->bindCount; i++) {
            index_ss.str("");
            index_ss << i;
// CODEGEN : file ../vk_helper.py line #980
            ss[0] << "0x" << &pStruct->pBinds[i];
            tmp_str = vk_print_vksparseimagememorybind(&pStruct->pBinds[i], extra_indent);
// CODEGEN : file ../vk_helper.py line #984
            stp_strs[0] += " " + prefix + "pBinds[" + index_ss.str() + "] (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1000
            ss[0].str("");
        }
    }
// CODEGEN : file ../vk_helper.py line #1086
    ss[0] << "0x" << pStruct->image;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[1] << pStruct->bindCount;
// CODEGEN : file ../vk_helper.py line #1061
    ss[2] << "0x" << (void*)pStruct->pBinds;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "image = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "bindCount = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "pBinds = " + ss[2].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vksparseimagememoryrequirements(const VkSparseImageMemoryRequirements* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[5];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vksparseimageformatproperties(&pStruct->formatProperties, extra_indent);
    ss[0] << "0x" << &pStruct->formatProperties;
    stp_strs[0] = " " + prefix + "formatProperties (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[0].str("");
// CODEGEN : file ../vk_helper.py line #1055
    ss[0].str("addr");
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[1] << pStruct->imageMipTailFirstLod;
// CODEGEN : file ../vk_helper.py line #1086
    ss[2] << "0x" << pStruct->imageMipTailSize;
// CODEGEN : file ../vk_helper.py line #1086
    ss[3] << "0x" << pStruct->imageMipTailOffset;
// CODEGEN : file ../vk_helper.py line #1086
    ss[4] << "0x" << pStruct->imageMipTailStride;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "formatProperties = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "imageMipTailFirstLod = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "imageMipTailSize = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "imageMipTailOffset = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "imageMipTailStride = " + ss[4].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vksparseimageopaquememorybindinfo(const VkSparseImageOpaqueMemoryBindInfo* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[3];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
    stp_strs[0] = "";
    stringstream index_ss;
    if (pStruct->pBinds) {
        for (uint32_t i = 0; i < pStruct->bindCount; i++) {
            index_ss.str("");
            index_ss << i;
// CODEGEN : file ../vk_helper.py line #980
            ss[0] << "0x" << &pStruct->pBinds[i];
            tmp_str = vk_print_vksparsememorybind(&pStruct->pBinds[i], extra_indent);
// CODEGEN : file ../vk_helper.py line #984
            stp_strs[0] += " " + prefix + "pBinds[" + index_ss.str() + "] (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1000
            ss[0].str("");
        }
    }
// CODEGEN : file ../vk_helper.py line #1086
    ss[0] << "0x" << pStruct->image;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[1] << pStruct->bindCount;
// CODEGEN : file ../vk_helper.py line #1061
    ss[2] << "0x" << (void*)pStruct->pBinds;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "image = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "bindCount = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "pBinds = " + ss[2].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vksparsememorybind(const VkSparseMemoryBind* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[5];
// CODEGEN : file ../vk_helper.py line #1086
    ss[0] << "0x" << pStruct->resourceOffset;
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->size;
// CODEGEN : file ../vk_helper.py line #1086
    ss[2] << "0x" << pStruct->memory;
// CODEGEN : file ../vk_helper.py line #1086
    ss[3] << "0x" << pStruct->memoryOffset;
// CODEGEN : file ../vk_helper.py line #1086
    ss[4] << "0x" << pStruct->flags;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "resourceOffset = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "size = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "memory = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "memoryOffset = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "flags = " + ss[4].str() + "\n";
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkspecializationinfo(const VkSpecializationInfo* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[4];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
    stp_strs[0] = "";
    stringstream index_ss;
    if (pStruct->pMapEntries) {
        for (uint32_t i = 0; i < pStruct->mapEntryCount; i++) {
            index_ss.str("");
            index_ss << i;
// CODEGEN : file ../vk_helper.py line #980
            ss[0] << "0x" << &pStruct->pMapEntries[i];
            tmp_str = vk_print_vkspecializationmapentry(&pStruct->pMapEntries[i], extra_indent);
// CODEGEN : file ../vk_helper.py line #984
            stp_strs[0] += " " + prefix + "pMapEntries[" + index_ss.str() + "] (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1000
            ss[0].str("");
        }
    }
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[0] << pStruct->mapEntryCount;
// CODEGEN : file ../vk_helper.py line #1061
    ss[1] << "0x" << (void*)pStruct->pMapEntries;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[2] << pStruct->dataSize;
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[3] << "0x" << pStruct->pData;
    else
        ss[3].str("address");
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "mapEntryCount = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "pMapEntries = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "dataSize = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "pData = " + ss[3].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkspecializationmapentry(const VkSpecializationMapEntry* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[3];
// CODEGEN : file ../vk_helper.py line #1090: NB: Edit here to choose hex vs dec output by variable name
    ss[0] << "0x" << pStruct->constantID;
// CODEGEN : file ../vk_helper.py line #1090: NB: Edit here to choose hex vs dec output by variable name
    ss[1] << "0x" << pStruct->offset;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[2] << pStruct->size;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "constantID = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "offset = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "size = " + ss[2].str() + "\n";
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkstencilopstate(const VkStencilOpState* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[3];
// CODEGEN : file ../vk_helper.py line #1090: NB: Edit here to choose hex vs dec output by variable name
    ss[0] << "0x" << pStruct->compareMask;
// CODEGEN : file ../vk_helper.py line #1090: NB: Edit here to choose hex vs dec output by variable name
    ss[1] << "0x" << pStruct->writeMask;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[2] << pStruct->reference;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "failOp = " + string_VkStencilOp(pStruct->failOp) + "\n";
    final_str = final_str + prefix + "passOp = " + string_VkStencilOp(pStruct->passOp) + "\n";
    final_str = final_str + prefix + "depthFailOp = " + string_VkStencilOp(pStruct->depthFailOp) + "\n";
    final_str = final_str + prefix + "compareOp = " + string_VkCompareOp(pStruct->compareOp) + "\n";
    final_str = final_str + prefix + "compareMask = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "writeMask = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "reference = " + ss[2].str() + "\n";
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vksubmitinfo(const VkSubmitInfo* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[8];
    string stp_strs[4];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
        stp_strs[1] = "";
        stringstream index_ss;
        if (pStruct->pWaitSemaphores) {
            for (uint32_t i = 0; i < pStruct->waitSemaphoreCount; i++) {
                index_ss.str("");
                index_ss << i;
// CODEGEN : file ../vk_helper.py line #990
                ss[1] << "0x" << pStruct->pWaitSemaphores[i];
                stp_strs[1] += " " + prefix + "pWaitSemaphores[" + index_ss.str() + "].handle = " + ss[1].str() + "\n";
// CODEGEN : file ../vk_helper.py line #1000
                ss[1].str("");
            }
        }
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
        stp_strs[2] = "";
        if (pStruct->pCommandBuffers) {
            for (uint32_t i = 0; i < pStruct->commandBufferCount; i++) {
                index_ss.str("");
                index_ss << i;
// CODEGEN : file ../vk_helper.py line #990
                ss[2] << "0x" << pStruct->pCommandBuffers[i];
                stp_strs[2] += " " + prefix + "pCommandBuffers[" + index_ss.str() + "].handle = " + ss[2].str() + "\n";
// CODEGEN : file ../vk_helper.py line #1000
                ss[2].str("");
            }
        }
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
        stp_strs[3] = "";
        if (pStruct->pSignalSemaphores) {
            for (uint32_t i = 0; i < pStruct->signalSemaphoreCount; i++) {
                index_ss.str("");
                index_ss << i;
// CODEGEN : file ../vk_helper.py line #990
                ss[3] << "0x" << pStruct->pSignalSemaphores[i];
                stp_strs[3] += " " + prefix + "pSignalSemaphores[" + index_ss.str() + "].handle = " + ss[3].str() + "\n";
// CODEGEN : file ../vk_helper.py line #1000
                ss[3].str("");
            }
        }
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[1] << pStruct->waitSemaphoreCount;
// CODEGEN : file ../vk_helper.py line #1061
    ss[2] << "0x" << (void*)pStruct->pWaitSemaphores;
// CODEGEN : file ../vk_helper.py line #1086
    ss[3] << "0x" << pStruct->pWaitDstStageMask;
// CODEGEN : file ../vk_helper.py line #1090: NB: Edit here to choose hex vs dec output by variable name
    ss[4] << "0x" << pStruct->commandBufferCount;
// CODEGEN : file ../vk_helper.py line #1061
    ss[5] << "0x" << (void*)pStruct->pCommandBuffers;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[6] << pStruct->signalSemaphoreCount;
// CODEGEN : file ../vk_helper.py line #1061
    ss[7] << "0x" << (void*)pStruct->pSignalSemaphores;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "waitSemaphoreCount = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "pWaitSemaphores = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "pWaitDstStageMask = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "commandBufferCount = " + ss[4].str() + "\n";
    final_str = final_str + prefix + "pCommandBuffers = " + ss[5].str() + "\n";
    final_str = final_str + prefix + "signalSemaphoreCount = " + ss[6].str() + "\n";
    final_str = final_str + prefix + "pSignalSemaphores = " + ss[7].str() + "\n";
    final_str = final_str + stp_strs[3] + stp_strs[2] + stp_strs[1] + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vksubpassdependency(const VkSubpassDependency* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[7];
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[0] << pStruct->srcSubpass;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[1] << pStruct->dstSubpass;
// CODEGEN : file ../vk_helper.py line #1086
    ss[2] << "0x" << pStruct->srcStageMask;
// CODEGEN : file ../vk_helper.py line #1086
    ss[3] << "0x" << pStruct->dstStageMask;
// CODEGEN : file ../vk_helper.py line #1086
    ss[4] << "0x" << pStruct->srcAccessMask;
// CODEGEN : file ../vk_helper.py line #1086
    ss[5] << "0x" << pStruct->dstAccessMask;
// CODEGEN : file ../vk_helper.py line #1086
    ss[6] << "0x" << pStruct->dependencyFlags;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "srcSubpass = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "dstSubpass = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "srcStageMask = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "dstStageMask = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "srcAccessMask = " + ss[4].str() + "\n";
    final_str = final_str + prefix + "dstAccessMask = " + ss[5].str() + "\n";
    final_str = final_str + prefix + "dependencyFlags = " + ss[6].str() + "\n";
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vksubpassdescription(const VkSubpassDescription* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[9];
    string stp_strs[5];
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
    stp_strs[0] = "";
    stringstream index_ss;
    if (pStruct->pInputAttachments) {
        for (uint32_t i = 0; i < pStruct->inputAttachmentCount; i++) {
            index_ss.str("");
            index_ss << i;
// CODEGEN : file ../vk_helper.py line #980
            ss[0] << "0x" << &pStruct->pInputAttachments[i];
            tmp_str = vk_print_vkattachmentreference(&pStruct->pInputAttachments[i], extra_indent);
// CODEGEN : file ../vk_helper.py line #984
            stp_strs[0] += " " + prefix + "pInputAttachments[" + index_ss.str() + "] (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1000
            ss[0].str("");
        }
    }
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
    stp_strs[1] = "";
    if (pStruct->pColorAttachments) {
        for (uint32_t i = 0; i < pStruct->colorAttachmentCount; i++) {
            index_ss.str("");
            index_ss << i;
// CODEGEN : file ../vk_helper.py line #980
            ss[1] << "0x" << &pStruct->pColorAttachments[i];
            tmp_str = vk_print_vkattachmentreference(&pStruct->pColorAttachments[i], extra_indent);
// CODEGEN : file ../vk_helper.py line #984
            stp_strs[1] += " " + prefix + "pColorAttachments[" + index_ss.str() + "] (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1000
            ss[1].str("");
        }
    }
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
    stp_strs[2] = "";
    if (pStruct->pResolveAttachments) {
        for (uint32_t i = 0; i < pStruct->colorAttachmentCount; i++) {
            index_ss.str("");
            index_ss << i;
// CODEGEN : file ../vk_helper.py line #980
            ss[2] << "0x" << &pStruct->pResolveAttachments[i];
            tmp_str = vk_print_vkattachmentreference(&pStruct->pResolveAttachments[i], extra_indent);
// CODEGEN : file ../vk_helper.py line #984
            stp_strs[2] += " " + prefix + "pResolveAttachments[" + index_ss.str() + "] (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1000
            ss[2].str("");
        }
    }
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pDepthStencilAttachment) {
// CODEGEN : file ../vk_helper.py line #1024
        tmp_str = vk_print_vkattachmentreference(pStruct->pDepthStencilAttachment, extra_indent);
        ss[3] << "0x" << &pStruct->pDepthStencilAttachment;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[3] = " " + prefix + "pDepthStencilAttachment (addr)\n" + tmp_str;
        ss[3].str("");
    }
    else
        stp_strs[3] = "";
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
        stp_strs[4] = "";
        if (pStruct->pPreserveAttachments) {
            for (uint32_t i = 0; i < pStruct->preserveAttachmentCount; i++) {
                index_ss.str("");
                index_ss << i;
// CODEGEN : file ../vk_helper.py line #990
                ss[4] << "0x" << pStruct->pPreserveAttachments[i];
                stp_strs[4] += " " + prefix + "pPreserveAttachments[" + index_ss.str() + "] = " + ss[4].str() + "\n";
// CODEGEN : file ../vk_helper.py line #1000
                ss[4].str("");
            }
        }
// CODEGEN : file ../vk_helper.py line #1086
    ss[0] << "0x" << pStruct->flags;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[1] << pStruct->inputAttachmentCount;
// CODEGEN : file ../vk_helper.py line #1061
    ss[2] << "0x" << (void*)pStruct->pInputAttachments;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[3] << pStruct->colorAttachmentCount;
// CODEGEN : file ../vk_helper.py line #1061
    ss[4] << "0x" << (void*)pStruct->pColorAttachments;
// CODEGEN : file ../vk_helper.py line #1061
    ss[5] << "0x" << (void*)pStruct->pResolveAttachments;
// CODEGEN : file ../vk_helper.py line #1086
    ss[6] << "0x" << pStruct->pDepthStencilAttachment;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[7] << pStruct->preserveAttachmentCount;
// CODEGEN : file ../vk_helper.py line #1061
    ss[8] << "0x" << (void*)pStruct->pPreserveAttachments;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "flags = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "pipelineBindPoint = " + string_VkPipelineBindPoint(pStruct->pipelineBindPoint) + "\n";
    final_str = final_str + prefix + "inputAttachmentCount = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "pInputAttachments = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "colorAttachmentCount = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "pColorAttachments = " + ss[4].str() + "\n";
    final_str = final_str + prefix + "pResolveAttachments = " + ss[5].str() + "\n";
    final_str = final_str + prefix + "pDepthStencilAttachment = " + ss[6].str() + "\n";
    final_str = final_str + prefix + "preserveAttachmentCount = " + ss[7].str() + "\n";
    final_str = final_str + prefix + "pPreserveAttachments = " + ss[8].str() + "\n";
    final_str = final_str + stp_strs[4] + stp_strs[3] + stp_strs[2] + stp_strs[1] + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vksubresourcelayout(const VkSubresourceLayout* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[5];
// CODEGEN : file ../vk_helper.py line #1086
    ss[0] << "0x" << pStruct->offset;
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->size;
// CODEGEN : file ../vk_helper.py line #1086
    ss[2] << "0x" << pStruct->rowPitch;
// CODEGEN : file ../vk_helper.py line #1086
    ss[3] << "0x" << pStruct->arrayPitch;
// CODEGEN : file ../vk_helper.py line #1086
    ss[4] << "0x" << pStruct->depthPitch;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "offset = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "size = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "rowPitch = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "arrayPitch = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "depthPitch = " + ss[4].str() + "\n";
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vksurfacecapabilitieskhr(const VkSurfaceCapabilitiesKHR* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[9];
    string stp_strs[3];
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkextent2d(&pStruct->currentExtent, extra_indent);
    ss[0] << "0x" << &pStruct->currentExtent;
    stp_strs[0] = " " + prefix + "currentExtent (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[0].str("");
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkextent2d(&pStruct->minImageExtent, extra_indent);
    ss[1] << "0x" << &pStruct->minImageExtent;
    stp_strs[1] = " " + prefix + "minImageExtent (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[1].str("");
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkextent2d(&pStruct->maxImageExtent, extra_indent);
    ss[2] << "0x" << &pStruct->maxImageExtent;
    stp_strs[2] = " " + prefix + "maxImageExtent (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[2].str("");
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[0] << pStruct->minImageCount;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[1] << pStruct->maxImageCount;
// CODEGEN : file ../vk_helper.py line #1055
    ss[2].str("addr");
// CODEGEN : file ../vk_helper.py line #1055
    ss[3].str("addr");
// CODEGEN : file ../vk_helper.py line #1055
    ss[4].str("addr");
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[5] << pStruct->maxImageArrayLayers;
// CODEGEN : file ../vk_helper.py line #1086
    ss[6] << "0x" << pStruct->supportedTransforms;
// CODEGEN : file ../vk_helper.py line #1086
    ss[7] << "0x" << pStruct->supportedCompositeAlpha;
// CODEGEN : file ../vk_helper.py line #1086
    ss[8] << "0x" << pStruct->supportedUsageFlags;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "minImageCount = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "maxImageCount = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "currentExtent = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "minImageExtent = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "maxImageExtent = " + ss[4].str() + "\n";
    final_str = final_str + prefix + "maxImageArrayLayers = " + ss[5].str() + "\n";
    final_str = final_str + prefix + "supportedTransforms = " + ss[6].str() + "\n";
    final_str = final_str + prefix + "currentTransform = " + string_VkSurfaceTransformFlagBitsKHR(pStruct->currentTransform) + "\n";
    final_str = final_str + prefix + "supportedCompositeAlpha = " + ss[7].str() + "\n";
    final_str = final_str + prefix + "supportedUsageFlags = " + ss[8].str() + "\n";
    final_str = final_str + stp_strs[2] + stp_strs[1] + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vksurfaceformatkhr(const VkSurfaceFormatKHR* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "format = " + string_VkFormat(pStruct->format) + "\n";
    final_str = final_str + prefix + "colorSpace = " + string_VkColorSpaceKHR(pStruct->colorSpace) + "\n";
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkswapchaincreateinfokhr(const VkSwapchainCreateInfoKHR* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[11];
    string stp_strs[3];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1038
    tmp_str = vk_print_vkextent2d(&pStruct->imageExtent, extra_indent);
    ss[1] << "0x" << &pStruct->imageExtent;
    stp_strs[1] = " " + prefix + "imageExtent (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1043
    ss[1].str("");
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
        stp_strs[2] = "";
        stringstream index_ss;
        if (pStruct->imageSharingMode == VK_SHARING_MODE_CONCURRENT) {
            if (pStruct->pQueueFamilyIndices) {
                for (uint32_t i = 0; i < pStruct->queueFamilyIndexCount; i++) {
                    index_ss.str("");
                    index_ss << i;
// CODEGEN : file ../vk_helper.py line #990
                    ss[2] << "0x" << pStruct->pQueueFamilyIndices[i];
                    stp_strs[2] += " " + prefix + "pQueueFamilyIndices[" + index_ss.str() + "] = " + ss[2].str() + "\n";
// CODEGEN : file ../vk_helper.py line #1000
                    ss[2].str("");
                }
            }
        }
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->flags;
// CODEGEN : file ../vk_helper.py line #1086
    ss[2] << "0x" << pStruct->surface;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[3] << pStruct->minImageCount;
// CODEGEN : file ../vk_helper.py line #1055
    ss[4].str("addr");
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[5] << pStruct->imageArrayLayers;
// CODEGEN : file ../vk_helper.py line #1086
    ss[6] << "0x" << pStruct->imageUsage;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[7] << pStruct->queueFamilyIndexCount;
// CODEGEN : file ../vk_helper.py line #1061
    ss[8] << "0x" << (void*)pStruct->pQueueFamilyIndices;
// CODEGEN : file ../vk_helper.py line #1064
    ss[9].str(pStruct->clipped ? "TRUE" : "FALSE");
// CODEGEN : file ../vk_helper.py line #1086
    ss[10] << "0x" << pStruct->oldSwapchain;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "flags = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "surface = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "minImageCount = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "imageFormat = " + string_VkFormat(pStruct->imageFormat) + "\n";
    final_str = final_str + prefix + "imageColorSpace = " + string_VkColorSpaceKHR(pStruct->imageColorSpace) + "\n";
    final_str = final_str + prefix + "imageExtent = " + ss[4].str() + "\n";
    final_str = final_str + prefix + "imageArrayLayers = " + ss[5].str() + "\n";
    final_str = final_str + prefix + "imageUsage = " + ss[6].str() + "\n";
    final_str = final_str + prefix + "imageSharingMode = " + string_VkSharingMode(pStruct->imageSharingMode) + "\n";
    final_str = final_str + prefix + "queueFamilyIndexCount = " + ss[7].str() + "\n";
    final_str = final_str + prefix + "pQueueFamilyIndices = " + ss[8].str() + "\n";
    final_str = final_str + prefix + "preTransform = " + string_VkSurfaceTransformFlagBitsKHR(pStruct->preTransform) + "\n";
    final_str = final_str + prefix + "compositeAlpha = " + string_VkCompositeAlphaFlagBitsKHR(pStruct->compositeAlpha) + "\n";
    final_str = final_str + prefix + "presentMode = " + string_VkPresentModeKHR(pStruct->presentMode) + "\n";
    final_str = final_str + prefix + "clipped = " + ss[9].str() + "\n";
    final_str = final_str + prefix + "oldSwapchain = " + ss[10].str() + "\n";
    final_str = final_str + stp_strs[2] + stp_strs[1] + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkvalidationflagsext(const VkValidationFlagsEXT* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[3];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[1] << pStruct->disabledValidationCheckCount;
// CODEGEN : file ../vk_helper.py line #1100
    if (pStruct->pDisabledValidationChecks)
        ss[2] << "0x" << pStruct->pDisabledValidationChecks << " (See individual array values below)";
    else
        ss[2].str("NULL");
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "disabledValidationCheckCount = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "pDisabledValidationChecks = " + ss[2].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkvertexinputattributedescription(const VkVertexInputAttributeDescription* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[3];
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[0] << pStruct->location;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[1] << pStruct->binding;
// CODEGEN : file ../vk_helper.py line #1090: NB: Edit here to choose hex vs dec output by variable name
    ss[2] << "0x" << pStruct->offset;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "location = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "binding = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "format = " + string_VkFormat(pStruct->format) + "\n";
    final_str = final_str + prefix + "offset = " + ss[2].str() + "\n";
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkvertexinputbindingdescription(const VkVertexInputBindingDescription* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[2];
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[0] << pStruct->binding;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[1] << pStruct->stride;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "binding = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "stride = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "inputRate = " + string_VkVertexInputRate(pStruct->inputRate) + "\n";
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkviewport(const VkViewport* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[6];
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[0] << pStruct->x;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[1] << pStruct->y;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[2] << pStruct->width;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[3] << pStruct->height;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[4] << pStruct->minDepth;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[5] << pStruct->maxDepth;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "x = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "y = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "width = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "height = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "minDepth = " + ss[4].str() + "\n";
    final_str = final_str + prefix + "maxDepth = " + ss[5].str() + "\n";
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
std::string vk_print_vkwaylandsurfacecreateinfokhr(const VkWaylandSurfaceCreateInfoKHR* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[4];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->flags;
// CODEGEN : file ../vk_helper.py line #1086
    ss[2] << "0x" << pStruct->display;
// CODEGEN : file ../vk_helper.py line #1086
    ss[3] << "0x" << pStruct->surface;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "flags = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "display = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "surface = " + ss[3].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
#endif //VK_USE_PLATFORM_WAYLAND_KHR
// CODEGEN : file ../vk_helper.py line #907
#ifdef VK_USE_PLATFORM_WIN32_KHR
std::string vk_print_vkwin32keyedmutexacquirereleaseinfonv(const VkWin32KeyedMutexAcquireReleaseInfoNV* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[8];
    string stp_strs[6];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
        stp_strs[1] = "";
        stringstream index_ss;
        if (pStruct->pAcquireSyncs) {
            for (uint32_t i = 0; i < pStruct->acquireCount; i++) {
                index_ss.str("");
                index_ss << i;
// CODEGEN : file ../vk_helper.py line #990
                ss[1] << "0x" << pStruct->pAcquireSyncs[i];
                stp_strs[1] += " " + prefix + "pAcquireSyncs[" + index_ss.str() + "].handle = " + ss[1].str() + "\n";
// CODEGEN : file ../vk_helper.py line #1000
                ss[1].str("");
            }
        }
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
        stp_strs[2] = "";
        if (pStruct->pAcquireKeys) {
            for (uint32_t i = 0; i < pStruct->acquireCount; i++) {
                index_ss.str("");
                index_ss << i;
// CODEGEN : file ../vk_helper.py line #990
                ss[2] << "0x" << pStruct->pAcquireKeys[i];
                stp_strs[2] += " " + prefix + "pAcquireKeys[" + index_ss.str() + "] = " + ss[2].str() + "\n";
// CODEGEN : file ../vk_helper.py line #1000
                ss[2].str("");
            }
        }
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
        stp_strs[3] = "";
        if (pStruct->pAcquireTimeoutMilliseconds) {
            for (uint32_t i = 0; i < pStruct->acquireCount; i++) {
                index_ss.str("");
                index_ss << i;
// CODEGEN : file ../vk_helper.py line #990
                ss[3] << "0x" << pStruct->pAcquireTimeoutMilliseconds[i];
                stp_strs[3] += " " + prefix + "pAcquireTimeoutMilliseconds[" + index_ss.str() + "] = " + ss[3].str() + "\n";
// CODEGEN : file ../vk_helper.py line #1000
                ss[3].str("");
            }
        }
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
        stp_strs[4] = "";
        if (pStruct->pReleaseSyncs) {
            for (uint32_t i = 0; i < pStruct->releaseCount; i++) {
                index_ss.str("");
                index_ss << i;
// CODEGEN : file ../vk_helper.py line #990
                ss[4] << "0x" << pStruct->pReleaseSyncs[i];
                stp_strs[4] += " " + prefix + "pReleaseSyncs[" + index_ss.str() + "].handle = " + ss[4].str() + "\n";
// CODEGEN : file ../vk_helper.py line #1000
                ss[4].str("");
            }
        }
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
        stp_strs[5] = "";
        if (pStruct->pReleaseKeys) {
            for (uint32_t i = 0; i < pStruct->releaseCount; i++) {
                index_ss.str("");
                index_ss << i;
// CODEGEN : file ../vk_helper.py line #990
                ss[5] << "0x" << pStruct->pReleaseKeys[i];
                stp_strs[5] += " " + prefix + "pReleaseKeys[" + index_ss.str() + "] = " + ss[5].str() + "\n";
// CODEGEN : file ../vk_helper.py line #1000
                ss[5].str("");
            }
        }
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[1] << pStruct->acquireCount;
// CODEGEN : file ../vk_helper.py line #1061
    ss[2] << "0x" << (void*)pStruct->pAcquireSyncs;
// CODEGEN : file ../vk_helper.py line #1061
    ss[3] << "0x" << (void*)pStruct->pAcquireKeys;
// CODEGEN : file ../vk_helper.py line #1061
    ss[4] << "0x" << (void*)pStruct->pAcquireTimeoutMilliseconds;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[5] << pStruct->releaseCount;
// CODEGEN : file ../vk_helper.py line #1061
    ss[6] << "0x" << (void*)pStruct->pReleaseSyncs;
// CODEGEN : file ../vk_helper.py line #1061
    ss[7] << "0x" << (void*)pStruct->pReleaseKeys;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "acquireCount = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "pAcquireSyncs = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "pAcquireKeys = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "pAcquireTimeoutMilliseconds = " + ss[4].str() + "\n";
    final_str = final_str + prefix + "releaseCount = " + ss[5].str() + "\n";
    final_str = final_str + prefix + "pReleaseSyncs = " + ss[6].str() + "\n";
    final_str = final_str + prefix + "pReleaseKeys = " + ss[7].str() + "\n";
    final_str = final_str + stp_strs[5] + stp_strs[4] + stp_strs[3] + stp_strs[2] + stp_strs[1] + stp_strs[0];
    return final_str;
}
#endif //VK_USE_PLATFORM_WIN32_KHR
// CODEGEN : file ../vk_helper.py line #907
#ifdef VK_USE_PLATFORM_WIN32_KHR
std::string vk_print_vkwin32surfacecreateinfokhr(const VkWin32SurfaceCreateInfoKHR* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[4];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->flags;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[2] << pStruct->hinstance;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[3] << pStruct->hwnd;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "flags = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "hinstance = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "hwnd = " + ss[3].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
#endif //VK_USE_PLATFORM_WIN32_KHR
// CODEGEN : file ../vk_helper.py line #907
std::string vk_print_vkwritedescriptorset(const VkWriteDescriptorSet* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[8];
    string stp_strs[4];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
        stp_strs[1] = "";
        stringstream index_ss;
        if ((pStruct->descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER)                ||
            (pStruct->descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) ||
            (pStruct->descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)          ||
            (pStruct->descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE))           {
            if (pStruct->pImageInfo) {
                for (uint32_t i = 0; i < pStruct->descriptorCount; i++) {
                    index_ss.str("");
                    index_ss << i;
// CODEGEN : file ../vk_helper.py line #980
                    ss[1] << "0x" << &pStruct->pImageInfo[i];
                    tmp_str = vk_print_vkdescriptorimageinfo(&pStruct->pImageInfo[i], extra_indent);
// CODEGEN : file ../vk_helper.py line #984
                    stp_strs[1] += " " + prefix + "pImageInfo[" + index_ss.str() + "] (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1000
                    ss[1].str("");
                }
            }
        }
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
        stp_strs[2] = "";
        if ((pStruct->descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)         ||
            (pStruct->descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)         ||
            (pStruct->descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) ||
            (pStruct->descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC))  {
            if (pStruct->pBufferInfo) {
                for (uint32_t i = 0; i < pStruct->descriptorCount; i++) {
                    index_ss.str("");
                    index_ss << i;
// CODEGEN : file ../vk_helper.py line #980
                    ss[2] << "0x" << &pStruct->pBufferInfo[i];
                    tmp_str = vk_print_vkdescriptorbufferinfo(&pStruct->pBufferInfo[i], extra_indent);
// CODEGEN : file ../vk_helper.py line #984
                    stp_strs[2] += " " + prefix + "pBufferInfo[" + index_ss.str() + "] (addr)\n" + tmp_str;
// CODEGEN : file ../vk_helper.py line #1000
                    ss[2].str("");
                }
            }
        }
// CODEGEN : file ../vk_helper.py line #932
// CODEGEN : file ../vk_helper.py line #934
// CODEGEN : file ../vk_helper.py line #939
        stp_strs[3] = "";
        if ((pStruct->descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER) ||
            (pStruct->descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER))  {
            if (pStruct->pTexelBufferView) {
                for (uint32_t i = 0; i < pStruct->descriptorCount; i++) {
                    index_ss.str("");
                    index_ss << i;
// CODEGEN : file ../vk_helper.py line #990
                    ss[3] << "0x" << pStruct->pTexelBufferView[i];
                    stp_strs[3] += " " + prefix + "pTexelBufferView[" + index_ss.str() + "].handle = " + ss[3].str() + "\n";
// CODEGEN : file ../vk_helper.py line #1000
                    ss[3].str("");
                }
            }
        }
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->dstSet;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[2] << pStruct->dstBinding;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[3] << pStruct->dstArrayElement;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[4] << pStruct->descriptorCount;
// CODEGEN : file ../vk_helper.py line #1061
    ss[5] << "0x" << (void*)pStruct->pImageInfo;
// CODEGEN : file ../vk_helper.py line #1061
    ss[6] << "0x" << (void*)pStruct->pBufferInfo;
// CODEGEN : file ../vk_helper.py line #1061
    ss[7] << "0x" << (void*)pStruct->pTexelBufferView;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "dstSet = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "dstBinding = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "dstArrayElement = " + ss[3].str() + "\n";
    final_str = final_str + prefix + "descriptorCount = " + ss[4].str() + "\n";
    final_str = final_str + prefix + "descriptorType = " + string_VkDescriptorType(pStruct->descriptorType) + "\n";
    final_str = final_str + prefix + "pImageInfo = " + ss[5].str() + "\n";
    final_str = final_str + prefix + "pBufferInfo = " + ss[6].str() + "\n";
    final_str = final_str + prefix + "pTexelBufferView = " + ss[7].str() + "\n";
    final_str = final_str + stp_strs[3] + stp_strs[2] + stp_strs[1] + stp_strs[0];
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #907
#ifdef VK_USE_PLATFORM_XCB_KHR
std::string vk_print_vkxcbsurfacecreateinfokhr(const VkXcbSurfaceCreateInfoKHR* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[4];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->flags;
// CODEGEN : file ../vk_helper.py line #1086
    ss[2] << "0x" << pStruct->connection;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[3] << pStruct->window;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "flags = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "connection = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "window = " + ss[3].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
#endif //VK_USE_PLATFORM_XCB_KHR
// CODEGEN : file ../vk_helper.py line #907
#ifdef VK_USE_PLATFORM_XLIB_KHR
std::string vk_print_vkxlibsurfacecreateinfokhr(const VkXlibSurfaceCreateInfoKHR* pStruct, const std::string prefix)
{
// CODEGEN : file ../vk_helper.py line #913
    using namespace StreamControl;
    using namespace std;
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[4];
    string stp_strs[1];
// CODEGEN : file ../vk_helper.py line #1012
    if (pStruct->pNext) {
// CODEGEN : file ../vk_helper.py line #1016
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << "0x" << &pStruct->pNext;
// CODEGEN : file ../vk_helper.py line #1028
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
// CODEGEN : file ../vk_helper.py line #1070
    if (StreamControl::writeAddress)
        ss[0] << "0x" << pStruct->pNext;
    else
        ss[0].str("address");
// CODEGEN : file ../vk_helper.py line #1086
    ss[1] << "0x" << pStruct->flags;
// CODEGEN : file ../vk_helper.py line #1086
    ss[2] << "0x" << pStruct->dpy;
// CODEGEN : file ../vk_helper.py line #1093: NB Edit this section to choose hex vs dec output by variable name
    ss[3] << pStruct->window;
// CODEGEN : file ../vk_helper.py line #1113
    final_str = final_str + prefix + "sType = " + string_VkStructureType(pStruct->sType) + "\n";
    final_str = final_str + prefix + "pNext = " + ss[0].str() + "\n";
    final_str = final_str + prefix + "flags = " + ss[1].str() + "\n";
    final_str = final_str + prefix + "dpy = " + ss[2].str() + "\n";
    final_str = final_str + prefix + "window = " + ss[3].str() + "\n";
    final_str = final_str + stp_strs[0];
    return final_str;
}
#endif //VK_USE_PLATFORM_XLIB_KHR
// CODEGEN : file ../vk_helper.py line #1122
std::string string_convert_helper(const void* toString, const std::string prefix)
{
    using namespace StreamControl;
    using namespace std;
    stringstream ss;
    ss << toString;
    string final_str = prefix + ss.str();
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #1131
std::string string_convert_helper(const uint64_t toString, const std::string prefix)
{
    using namespace StreamControl;
    using namespace std;
    stringstream ss;
    ss << toString;
    string final_str = prefix + ss.str();
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #1141
std::string string_convert_helper(VkSurfaceFormatKHR toString, const std::string prefix)
{
    using namespace std;
    string final_str = prefix + "format = " + string_VkFormat(toString.format) + "format = " + string_VkColorSpaceKHR(toString.colorSpace);
    return final_str;
}
// CODEGEN : file ../vk_helper.py line #1148
std::string dynamic_display(const void* pStruct, const std::string prefix)
{
    using namespace std;
    // Cast to APP_INFO ptr initially just to pull sType off struct
    if (pStruct == NULL) {

        return string();
    }

    VkStructureType sType = ((VkApplicationInfo*)pStruct)->sType;
    string indent = "    ";
    indent += prefix;
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
        return string();
// CODEGEN : file ../vk_helper.py line #1174
    }
}