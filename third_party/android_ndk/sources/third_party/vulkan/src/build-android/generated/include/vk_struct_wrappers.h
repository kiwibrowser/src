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

//class declaration
class vkallocationcallbacks_struct_wrapper
{
public:
    vkallocationcallbacks_struct_wrapper();
    vkallocationcallbacks_struct_wrapper(VkAllocationCallbacks* pInStruct);
    vkallocationcallbacks_struct_wrapper(const VkAllocationCallbacks* pInStruct);

    virtual ~vkallocationcallbacks_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    void* get_pUserData() { return m_struct.pUserData; }
    void set_pUserData(void* inValue) { m_struct.pUserData = inValue; }
    PFN_vkAllocationFunction get_pfnAllocation() { return m_struct.pfnAllocation; }
    void set_pfnAllocation(PFN_vkAllocationFunction inValue) { m_struct.pfnAllocation = inValue; }
    PFN_vkReallocationFunction get_pfnReallocation() { return m_struct.pfnReallocation; }
    void set_pfnReallocation(PFN_vkReallocationFunction inValue) { m_struct.pfnReallocation = inValue; }
    PFN_vkFreeFunction get_pfnFree() { return m_struct.pfnFree; }
    void set_pfnFree(PFN_vkFreeFunction inValue) { m_struct.pfnFree = inValue; }
    PFN_vkInternalAllocationNotification get_pfnInternalAllocation() { return m_struct.pfnInternalAllocation; }
    void set_pfnInternalAllocation(PFN_vkInternalAllocationNotification inValue) { m_struct.pfnInternalAllocation = inValue; }
    PFN_vkInternalFreeNotification get_pfnInternalFree() { return m_struct.pfnInternalFree; }
    void set_pfnInternalFree(PFN_vkInternalFreeNotification inValue) { m_struct.pfnInternalFree = inValue; }


private:
    VkAllocationCallbacks m_struct;
    const VkAllocationCallbacks* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkandroidsurfacecreateinfokhr_struct_wrapper
{
public:
    vkandroidsurfacecreateinfokhr_struct_wrapper();
    vkandroidsurfacecreateinfokhr_struct_wrapper(VkAndroidSurfaceCreateInfoKHR* pInStruct);
    vkandroidsurfacecreateinfokhr_struct_wrapper(const VkAndroidSurfaceCreateInfoKHR* pInStruct);

    virtual ~vkandroidsurfacecreateinfokhr_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkAndroidSurfaceCreateFlagsKHR get_flags() { return m_struct.flags; }
    void set_flags(VkAndroidSurfaceCreateFlagsKHR inValue) { m_struct.flags = inValue; }
    ANativeWindow* get_window() { return m_struct.window; }
    void set_window(ANativeWindow* inValue) { m_struct.window = inValue; }


private:
    VkAndroidSurfaceCreateInfoKHR m_struct;
    const VkAndroidSurfaceCreateInfoKHR* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkapplicationinfo_struct_wrapper
{
public:
    vkapplicationinfo_struct_wrapper();
    vkapplicationinfo_struct_wrapper(VkApplicationInfo* pInStruct);
    vkapplicationinfo_struct_wrapper(const VkApplicationInfo* pInStruct);

    virtual ~vkapplicationinfo_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    const char* get_pApplicationName() { return m_struct.pApplicationName; }
    uint32_t get_applicationVersion() { return m_struct.applicationVersion; }
    void set_applicationVersion(uint32_t inValue) { m_struct.applicationVersion = inValue; }
    const char* get_pEngineName() { return m_struct.pEngineName; }
    uint32_t get_engineVersion() { return m_struct.engineVersion; }
    void set_engineVersion(uint32_t inValue) { m_struct.engineVersion = inValue; }
    uint32_t get_apiVersion() { return m_struct.apiVersion; }
    void set_apiVersion(uint32_t inValue) { m_struct.apiVersion = inValue; }


private:
    VkApplicationInfo m_struct;
    const VkApplicationInfo* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkattachmentdescription_struct_wrapper
{
public:
    vkattachmentdescription_struct_wrapper();
    vkattachmentdescription_struct_wrapper(VkAttachmentDescription* pInStruct);
    vkattachmentdescription_struct_wrapper(const VkAttachmentDescription* pInStruct);

    virtual ~vkattachmentdescription_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkAttachmentDescriptionFlags get_flags() { return m_struct.flags; }
    void set_flags(VkAttachmentDescriptionFlags inValue) { m_struct.flags = inValue; }
    VkFormat get_format() { return m_struct.format; }
    void set_format(VkFormat inValue) { m_struct.format = inValue; }
    VkSampleCountFlagBits get_samples() { return m_struct.samples; }
    void set_samples(VkSampleCountFlagBits inValue) { m_struct.samples = inValue; }
    VkAttachmentLoadOp get_loadOp() { return m_struct.loadOp; }
    void set_loadOp(VkAttachmentLoadOp inValue) { m_struct.loadOp = inValue; }
    VkAttachmentStoreOp get_storeOp() { return m_struct.storeOp; }
    void set_storeOp(VkAttachmentStoreOp inValue) { m_struct.storeOp = inValue; }
    VkAttachmentLoadOp get_stencilLoadOp() { return m_struct.stencilLoadOp; }
    void set_stencilLoadOp(VkAttachmentLoadOp inValue) { m_struct.stencilLoadOp = inValue; }
    VkAttachmentStoreOp get_stencilStoreOp() { return m_struct.stencilStoreOp; }
    void set_stencilStoreOp(VkAttachmentStoreOp inValue) { m_struct.stencilStoreOp = inValue; }
    VkImageLayout get_initialLayout() { return m_struct.initialLayout; }
    void set_initialLayout(VkImageLayout inValue) { m_struct.initialLayout = inValue; }
    VkImageLayout get_finalLayout() { return m_struct.finalLayout; }
    void set_finalLayout(VkImageLayout inValue) { m_struct.finalLayout = inValue; }


private:
    VkAttachmentDescription m_struct;
    const VkAttachmentDescription* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkattachmentreference_struct_wrapper
{
public:
    vkattachmentreference_struct_wrapper();
    vkattachmentreference_struct_wrapper(VkAttachmentReference* pInStruct);
    vkattachmentreference_struct_wrapper(const VkAttachmentReference* pInStruct);

    virtual ~vkattachmentreference_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    uint32_t get_attachment() { return m_struct.attachment; }
    void set_attachment(uint32_t inValue) { m_struct.attachment = inValue; }
    VkImageLayout get_layout() { return m_struct.layout; }
    void set_layout(VkImageLayout inValue) { m_struct.layout = inValue; }


private:
    VkAttachmentReference m_struct;
    const VkAttachmentReference* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkbindsparseinfo_struct_wrapper
{
public:
    vkbindsparseinfo_struct_wrapper();
    vkbindsparseinfo_struct_wrapper(VkBindSparseInfo* pInStruct);
    vkbindsparseinfo_struct_wrapper(const VkBindSparseInfo* pInStruct);

    virtual ~vkbindsparseinfo_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    uint32_t get_waitSemaphoreCount() { return m_struct.waitSemaphoreCount; }
    void set_waitSemaphoreCount(uint32_t inValue) { m_struct.waitSemaphoreCount = inValue; }
    uint32_t get_bufferBindCount() { return m_struct.bufferBindCount; }
    void set_bufferBindCount(uint32_t inValue) { m_struct.bufferBindCount = inValue; }
    uint32_t get_imageOpaqueBindCount() { return m_struct.imageOpaqueBindCount; }
    void set_imageOpaqueBindCount(uint32_t inValue) { m_struct.imageOpaqueBindCount = inValue; }
    uint32_t get_imageBindCount() { return m_struct.imageBindCount; }
    void set_imageBindCount(uint32_t inValue) { m_struct.imageBindCount = inValue; }
    uint32_t get_signalSemaphoreCount() { return m_struct.signalSemaphoreCount; }
    void set_signalSemaphoreCount(uint32_t inValue) { m_struct.signalSemaphoreCount = inValue; }


private:
    VkBindSparseInfo m_struct;
    const VkBindSparseInfo* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkbuffercopy_struct_wrapper
{
public:
    vkbuffercopy_struct_wrapper();
    vkbuffercopy_struct_wrapper(VkBufferCopy* pInStruct);
    vkbuffercopy_struct_wrapper(const VkBufferCopy* pInStruct);

    virtual ~vkbuffercopy_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkDeviceSize get_srcOffset() { return m_struct.srcOffset; }
    void set_srcOffset(VkDeviceSize inValue) { m_struct.srcOffset = inValue; }
    VkDeviceSize get_dstOffset() { return m_struct.dstOffset; }
    void set_dstOffset(VkDeviceSize inValue) { m_struct.dstOffset = inValue; }
    VkDeviceSize get_size() { return m_struct.size; }
    void set_size(VkDeviceSize inValue) { m_struct.size = inValue; }


private:
    VkBufferCopy m_struct;
    const VkBufferCopy* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkbuffercreateinfo_struct_wrapper
{
public:
    vkbuffercreateinfo_struct_wrapper();
    vkbuffercreateinfo_struct_wrapper(VkBufferCreateInfo* pInStruct);
    vkbuffercreateinfo_struct_wrapper(const VkBufferCreateInfo* pInStruct);

    virtual ~vkbuffercreateinfo_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkBufferCreateFlags get_flags() { return m_struct.flags; }
    void set_flags(VkBufferCreateFlags inValue) { m_struct.flags = inValue; }
    VkDeviceSize get_size() { return m_struct.size; }
    void set_size(VkDeviceSize inValue) { m_struct.size = inValue; }
    VkBufferUsageFlags get_usage() { return m_struct.usage; }
    void set_usage(VkBufferUsageFlags inValue) { m_struct.usage = inValue; }
    VkSharingMode get_sharingMode() { return m_struct.sharingMode; }
    void set_sharingMode(VkSharingMode inValue) { m_struct.sharingMode = inValue; }
    uint32_t get_queueFamilyIndexCount() { return m_struct.queueFamilyIndexCount; }
    void set_queueFamilyIndexCount(uint32_t inValue) { m_struct.queueFamilyIndexCount = inValue; }


private:
    VkBufferCreateInfo m_struct;
    const VkBufferCreateInfo* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkbufferimagecopy_struct_wrapper
{
public:
    vkbufferimagecopy_struct_wrapper();
    vkbufferimagecopy_struct_wrapper(VkBufferImageCopy* pInStruct);
    vkbufferimagecopy_struct_wrapper(const VkBufferImageCopy* pInStruct);

    virtual ~vkbufferimagecopy_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkDeviceSize get_bufferOffset() { return m_struct.bufferOffset; }
    void set_bufferOffset(VkDeviceSize inValue) { m_struct.bufferOffset = inValue; }
    uint32_t get_bufferRowLength() { return m_struct.bufferRowLength; }
    void set_bufferRowLength(uint32_t inValue) { m_struct.bufferRowLength = inValue; }
    uint32_t get_bufferImageHeight() { return m_struct.bufferImageHeight; }
    void set_bufferImageHeight(uint32_t inValue) { m_struct.bufferImageHeight = inValue; }
    VkImageSubresourceLayers get_imageSubresource() { return m_struct.imageSubresource; }
    void set_imageSubresource(VkImageSubresourceLayers inValue) { m_struct.imageSubresource = inValue; }
    VkOffset3D get_imageOffset() { return m_struct.imageOffset; }
    void set_imageOffset(VkOffset3D inValue) { m_struct.imageOffset = inValue; }
    VkExtent3D get_imageExtent() { return m_struct.imageExtent; }
    void set_imageExtent(VkExtent3D inValue) { m_struct.imageExtent = inValue; }


private:
    VkBufferImageCopy m_struct;
    const VkBufferImageCopy* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkbuffermemorybarrier_struct_wrapper
{
public:
    vkbuffermemorybarrier_struct_wrapper();
    vkbuffermemorybarrier_struct_wrapper(VkBufferMemoryBarrier* pInStruct);
    vkbuffermemorybarrier_struct_wrapper(const VkBufferMemoryBarrier* pInStruct);

    virtual ~vkbuffermemorybarrier_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkAccessFlags get_srcAccessMask() { return m_struct.srcAccessMask; }
    void set_srcAccessMask(VkAccessFlags inValue) { m_struct.srcAccessMask = inValue; }
    VkAccessFlags get_dstAccessMask() { return m_struct.dstAccessMask; }
    void set_dstAccessMask(VkAccessFlags inValue) { m_struct.dstAccessMask = inValue; }
    uint32_t get_srcQueueFamilyIndex() { return m_struct.srcQueueFamilyIndex; }
    void set_srcQueueFamilyIndex(uint32_t inValue) { m_struct.srcQueueFamilyIndex = inValue; }
    uint32_t get_dstQueueFamilyIndex() { return m_struct.dstQueueFamilyIndex; }
    void set_dstQueueFamilyIndex(uint32_t inValue) { m_struct.dstQueueFamilyIndex = inValue; }
    VkBuffer get_buffer() { return m_struct.buffer; }
    void set_buffer(VkBuffer inValue) { m_struct.buffer = inValue; }
    VkDeviceSize get_offset() { return m_struct.offset; }
    void set_offset(VkDeviceSize inValue) { m_struct.offset = inValue; }
    VkDeviceSize get_size() { return m_struct.size; }
    void set_size(VkDeviceSize inValue) { m_struct.size = inValue; }


private:
    VkBufferMemoryBarrier m_struct;
    const VkBufferMemoryBarrier* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkbufferviewcreateinfo_struct_wrapper
{
public:
    vkbufferviewcreateinfo_struct_wrapper();
    vkbufferviewcreateinfo_struct_wrapper(VkBufferViewCreateInfo* pInStruct);
    vkbufferviewcreateinfo_struct_wrapper(const VkBufferViewCreateInfo* pInStruct);

    virtual ~vkbufferviewcreateinfo_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkBufferViewCreateFlags get_flags() { return m_struct.flags; }
    void set_flags(VkBufferViewCreateFlags inValue) { m_struct.flags = inValue; }
    VkBuffer get_buffer() { return m_struct.buffer; }
    void set_buffer(VkBuffer inValue) { m_struct.buffer = inValue; }
    VkFormat get_format() { return m_struct.format; }
    void set_format(VkFormat inValue) { m_struct.format = inValue; }
    VkDeviceSize get_offset() { return m_struct.offset; }
    void set_offset(VkDeviceSize inValue) { m_struct.offset = inValue; }
    VkDeviceSize get_range() { return m_struct.range; }
    void set_range(VkDeviceSize inValue) { m_struct.range = inValue; }


private:
    VkBufferViewCreateInfo m_struct;
    const VkBufferViewCreateInfo* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkclearattachment_struct_wrapper
{
public:
    vkclearattachment_struct_wrapper();
    vkclearattachment_struct_wrapper(VkClearAttachment* pInStruct);
    vkclearattachment_struct_wrapper(const VkClearAttachment* pInStruct);

    virtual ~vkclearattachment_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkImageAspectFlags get_aspectMask() { return m_struct.aspectMask; }
    void set_aspectMask(VkImageAspectFlags inValue) { m_struct.aspectMask = inValue; }
    uint32_t get_colorAttachment() { return m_struct.colorAttachment; }
    void set_colorAttachment(uint32_t inValue) { m_struct.colorAttachment = inValue; }
    VkClearValue get_clearValue() { return m_struct.clearValue; }
    void set_clearValue(VkClearValue inValue) { m_struct.clearValue = inValue; }


private:
    VkClearAttachment m_struct;
    const VkClearAttachment* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkclearcolorvalue_struct_wrapper
{
public:
    vkclearcolorvalue_struct_wrapper();
    vkclearcolorvalue_struct_wrapper(VkClearColorValue* pInStruct);
    vkclearcolorvalue_struct_wrapper(const VkClearColorValue* pInStruct);

    virtual ~vkclearcolorvalue_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }


private:
    VkClearColorValue m_struct;
    const VkClearColorValue* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkcleardepthstencilvalue_struct_wrapper
{
public:
    vkcleardepthstencilvalue_struct_wrapper();
    vkcleardepthstencilvalue_struct_wrapper(VkClearDepthStencilValue* pInStruct);
    vkcleardepthstencilvalue_struct_wrapper(const VkClearDepthStencilValue* pInStruct);

    virtual ~vkcleardepthstencilvalue_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    float get_depth() { return m_struct.depth; }
    void set_depth(float inValue) { m_struct.depth = inValue; }
    uint32_t get_stencil() { return m_struct.stencil; }
    void set_stencil(uint32_t inValue) { m_struct.stencil = inValue; }


private:
    VkClearDepthStencilValue m_struct;
    const VkClearDepthStencilValue* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkclearrect_struct_wrapper
{
public:
    vkclearrect_struct_wrapper();
    vkclearrect_struct_wrapper(VkClearRect* pInStruct);
    vkclearrect_struct_wrapper(const VkClearRect* pInStruct);

    virtual ~vkclearrect_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkRect2D get_rect() { return m_struct.rect; }
    void set_rect(VkRect2D inValue) { m_struct.rect = inValue; }
    uint32_t get_baseArrayLayer() { return m_struct.baseArrayLayer; }
    void set_baseArrayLayer(uint32_t inValue) { m_struct.baseArrayLayer = inValue; }
    uint32_t get_layerCount() { return m_struct.layerCount; }
    void set_layerCount(uint32_t inValue) { m_struct.layerCount = inValue; }


private:
    VkClearRect m_struct;
    const VkClearRect* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkclearvalue_struct_wrapper
{
public:
    vkclearvalue_struct_wrapper();
    vkclearvalue_struct_wrapper(VkClearValue* pInStruct);
    vkclearvalue_struct_wrapper(const VkClearValue* pInStruct);

    virtual ~vkclearvalue_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkClearColorValue get_color() { return m_struct.color; }
    void set_color(VkClearColorValue inValue) { m_struct.color = inValue; }
    VkClearDepthStencilValue get_depthStencil() { return m_struct.depthStencil; }
    void set_depthStencil(VkClearDepthStencilValue inValue) { m_struct.depthStencil = inValue; }


private:
    VkClearValue m_struct;
    const VkClearValue* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkcommandbufferallocateinfo_struct_wrapper
{
public:
    vkcommandbufferallocateinfo_struct_wrapper();
    vkcommandbufferallocateinfo_struct_wrapper(VkCommandBufferAllocateInfo* pInStruct);
    vkcommandbufferallocateinfo_struct_wrapper(const VkCommandBufferAllocateInfo* pInStruct);

    virtual ~vkcommandbufferallocateinfo_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkCommandPool get_commandPool() { return m_struct.commandPool; }
    void set_commandPool(VkCommandPool inValue) { m_struct.commandPool = inValue; }
    VkCommandBufferLevel get_level() { return m_struct.level; }
    void set_level(VkCommandBufferLevel inValue) { m_struct.level = inValue; }
    uint32_t get_commandBufferCount() { return m_struct.commandBufferCount; }
    void set_commandBufferCount(uint32_t inValue) { m_struct.commandBufferCount = inValue; }


private:
    VkCommandBufferAllocateInfo m_struct;
    const VkCommandBufferAllocateInfo* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkcommandbufferbegininfo_struct_wrapper
{
public:
    vkcommandbufferbegininfo_struct_wrapper();
    vkcommandbufferbegininfo_struct_wrapper(VkCommandBufferBeginInfo* pInStruct);
    vkcommandbufferbegininfo_struct_wrapper(const VkCommandBufferBeginInfo* pInStruct);

    virtual ~vkcommandbufferbegininfo_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkCommandBufferUsageFlags get_flags() { return m_struct.flags; }
    void set_flags(VkCommandBufferUsageFlags inValue) { m_struct.flags = inValue; }
    const VkCommandBufferInheritanceInfo* get_pInheritanceInfo() { return m_struct.pInheritanceInfo; }


private:
    VkCommandBufferBeginInfo m_struct;
    const VkCommandBufferBeginInfo* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkcommandbufferinheritanceinfo_struct_wrapper
{
public:
    vkcommandbufferinheritanceinfo_struct_wrapper();
    vkcommandbufferinheritanceinfo_struct_wrapper(VkCommandBufferInheritanceInfo* pInStruct);
    vkcommandbufferinheritanceinfo_struct_wrapper(const VkCommandBufferInheritanceInfo* pInStruct);

    virtual ~vkcommandbufferinheritanceinfo_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkRenderPass get_renderPass() { return m_struct.renderPass; }
    void set_renderPass(VkRenderPass inValue) { m_struct.renderPass = inValue; }
    uint32_t get_subpass() { return m_struct.subpass; }
    void set_subpass(uint32_t inValue) { m_struct.subpass = inValue; }
    VkFramebuffer get_framebuffer() { return m_struct.framebuffer; }
    void set_framebuffer(VkFramebuffer inValue) { m_struct.framebuffer = inValue; }
    VkBool32 get_occlusionQueryEnable() { return m_struct.occlusionQueryEnable; }
    void set_occlusionQueryEnable(VkBool32 inValue) { m_struct.occlusionQueryEnable = inValue; }
    VkQueryControlFlags get_queryFlags() { return m_struct.queryFlags; }
    void set_queryFlags(VkQueryControlFlags inValue) { m_struct.queryFlags = inValue; }
    VkQueryPipelineStatisticFlags get_pipelineStatistics() { return m_struct.pipelineStatistics; }
    void set_pipelineStatistics(VkQueryPipelineStatisticFlags inValue) { m_struct.pipelineStatistics = inValue; }


private:
    VkCommandBufferInheritanceInfo m_struct;
    const VkCommandBufferInheritanceInfo* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkcommandpoolcreateinfo_struct_wrapper
{
public:
    vkcommandpoolcreateinfo_struct_wrapper();
    vkcommandpoolcreateinfo_struct_wrapper(VkCommandPoolCreateInfo* pInStruct);
    vkcommandpoolcreateinfo_struct_wrapper(const VkCommandPoolCreateInfo* pInStruct);

    virtual ~vkcommandpoolcreateinfo_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkCommandPoolCreateFlags get_flags() { return m_struct.flags; }
    void set_flags(VkCommandPoolCreateFlags inValue) { m_struct.flags = inValue; }
    uint32_t get_queueFamilyIndex() { return m_struct.queueFamilyIndex; }
    void set_queueFamilyIndex(uint32_t inValue) { m_struct.queueFamilyIndex = inValue; }


private:
    VkCommandPoolCreateInfo m_struct;
    const VkCommandPoolCreateInfo* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkcomponentmapping_struct_wrapper
{
public:
    vkcomponentmapping_struct_wrapper();
    vkcomponentmapping_struct_wrapper(VkComponentMapping* pInStruct);
    vkcomponentmapping_struct_wrapper(const VkComponentMapping* pInStruct);

    virtual ~vkcomponentmapping_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkComponentSwizzle get_r() { return m_struct.r; }
    void set_r(VkComponentSwizzle inValue) { m_struct.r = inValue; }
    VkComponentSwizzle get_g() { return m_struct.g; }
    void set_g(VkComponentSwizzle inValue) { m_struct.g = inValue; }
    VkComponentSwizzle get_b() { return m_struct.b; }
    void set_b(VkComponentSwizzle inValue) { m_struct.b = inValue; }
    VkComponentSwizzle get_a() { return m_struct.a; }
    void set_a(VkComponentSwizzle inValue) { m_struct.a = inValue; }


private:
    VkComponentMapping m_struct;
    const VkComponentMapping* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkcomputepipelinecreateinfo_struct_wrapper
{
public:
    vkcomputepipelinecreateinfo_struct_wrapper();
    vkcomputepipelinecreateinfo_struct_wrapper(VkComputePipelineCreateInfo* pInStruct);
    vkcomputepipelinecreateinfo_struct_wrapper(const VkComputePipelineCreateInfo* pInStruct);

    virtual ~vkcomputepipelinecreateinfo_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkPipelineCreateFlags get_flags() { return m_struct.flags; }
    void set_flags(VkPipelineCreateFlags inValue) { m_struct.flags = inValue; }
    VkPipelineShaderStageCreateInfo get_stage() { return m_struct.stage; }
    void set_stage(VkPipelineShaderStageCreateInfo inValue) { m_struct.stage = inValue; }
    VkPipelineLayout get_layout() { return m_struct.layout; }
    void set_layout(VkPipelineLayout inValue) { m_struct.layout = inValue; }
    VkPipeline get_basePipelineHandle() { return m_struct.basePipelineHandle; }
    void set_basePipelineHandle(VkPipeline inValue) { m_struct.basePipelineHandle = inValue; }
    int32_t get_basePipelineIndex() { return m_struct.basePipelineIndex; }
    void set_basePipelineIndex(int32_t inValue) { m_struct.basePipelineIndex = inValue; }


private:
    VkComputePipelineCreateInfo m_struct;
    const VkComputePipelineCreateInfo* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkcopydescriptorset_struct_wrapper
{
public:
    vkcopydescriptorset_struct_wrapper();
    vkcopydescriptorset_struct_wrapper(VkCopyDescriptorSet* pInStruct);
    vkcopydescriptorset_struct_wrapper(const VkCopyDescriptorSet* pInStruct);

    virtual ~vkcopydescriptorset_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkDescriptorSet get_srcSet() { return m_struct.srcSet; }
    void set_srcSet(VkDescriptorSet inValue) { m_struct.srcSet = inValue; }
    uint32_t get_srcBinding() { return m_struct.srcBinding; }
    void set_srcBinding(uint32_t inValue) { m_struct.srcBinding = inValue; }
    uint32_t get_srcArrayElement() { return m_struct.srcArrayElement; }
    void set_srcArrayElement(uint32_t inValue) { m_struct.srcArrayElement = inValue; }
    VkDescriptorSet get_dstSet() { return m_struct.dstSet; }
    void set_dstSet(VkDescriptorSet inValue) { m_struct.dstSet = inValue; }
    uint32_t get_dstBinding() { return m_struct.dstBinding; }
    void set_dstBinding(uint32_t inValue) { m_struct.dstBinding = inValue; }
    uint32_t get_dstArrayElement() { return m_struct.dstArrayElement; }
    void set_dstArrayElement(uint32_t inValue) { m_struct.dstArrayElement = inValue; }
    uint32_t get_descriptorCount() { return m_struct.descriptorCount; }
    void set_descriptorCount(uint32_t inValue) { m_struct.descriptorCount = inValue; }


private:
    VkCopyDescriptorSet m_struct;
    const VkCopyDescriptorSet* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkdebugmarkermarkerinfoext_struct_wrapper
{
public:
    vkdebugmarkermarkerinfoext_struct_wrapper();
    vkdebugmarkermarkerinfoext_struct_wrapper(VkDebugMarkerMarkerInfoEXT* pInStruct);
    vkdebugmarkermarkerinfoext_struct_wrapper(const VkDebugMarkerMarkerInfoEXT* pInStruct);

    virtual ~vkdebugmarkermarkerinfoext_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    const char* get_pMarkerName() { return m_struct.pMarkerName; }


private:
    VkDebugMarkerMarkerInfoEXT m_struct;
    const VkDebugMarkerMarkerInfoEXT* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkdebugmarkerobjectnameinfoext_struct_wrapper
{
public:
    vkdebugmarkerobjectnameinfoext_struct_wrapper();
    vkdebugmarkerobjectnameinfoext_struct_wrapper(VkDebugMarkerObjectNameInfoEXT* pInStruct);
    vkdebugmarkerobjectnameinfoext_struct_wrapper(const VkDebugMarkerObjectNameInfoEXT* pInStruct);

    virtual ~vkdebugmarkerobjectnameinfoext_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkDebugReportObjectTypeEXT get_objectType() { return m_struct.objectType; }
    void set_objectType(VkDebugReportObjectTypeEXT inValue) { m_struct.objectType = inValue; }
    uint64_t get_object() { return m_struct.object; }
    void set_object(uint64_t inValue) { m_struct.object = inValue; }
    const char* get_pObjectName() { return m_struct.pObjectName; }


private:
    VkDebugMarkerObjectNameInfoEXT m_struct;
    const VkDebugMarkerObjectNameInfoEXT* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkdebugmarkerobjecttaginfoext_struct_wrapper
{
public:
    vkdebugmarkerobjecttaginfoext_struct_wrapper();
    vkdebugmarkerobjecttaginfoext_struct_wrapper(VkDebugMarkerObjectTagInfoEXT* pInStruct);
    vkdebugmarkerobjecttaginfoext_struct_wrapper(const VkDebugMarkerObjectTagInfoEXT* pInStruct);

    virtual ~vkdebugmarkerobjecttaginfoext_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkDebugReportObjectTypeEXT get_objectType() { return m_struct.objectType; }
    void set_objectType(VkDebugReportObjectTypeEXT inValue) { m_struct.objectType = inValue; }
    uint64_t get_object() { return m_struct.object; }
    void set_object(uint64_t inValue) { m_struct.object = inValue; }
    uint64_t get_tagName() { return m_struct.tagName; }
    void set_tagName(uint64_t inValue) { m_struct.tagName = inValue; }
    size_t get_tagSize() { return m_struct.tagSize; }
    void set_tagSize(size_t inValue) { m_struct.tagSize = inValue; }
    const void* get_pTag() { return m_struct.pTag; }


private:
    VkDebugMarkerObjectTagInfoEXT m_struct;
    const VkDebugMarkerObjectTagInfoEXT* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkdebugreportcallbackcreateinfoext_struct_wrapper
{
public:
    vkdebugreportcallbackcreateinfoext_struct_wrapper();
    vkdebugreportcallbackcreateinfoext_struct_wrapper(VkDebugReportCallbackCreateInfoEXT* pInStruct);
    vkdebugreportcallbackcreateinfoext_struct_wrapper(const VkDebugReportCallbackCreateInfoEXT* pInStruct);

    virtual ~vkdebugreportcallbackcreateinfoext_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkDebugReportFlagsEXT get_flags() { return m_struct.flags; }
    void set_flags(VkDebugReportFlagsEXT inValue) { m_struct.flags = inValue; }
    PFN_vkDebugReportCallbackEXT get_pfnCallback() { return m_struct.pfnCallback; }
    void set_pfnCallback(PFN_vkDebugReportCallbackEXT inValue) { m_struct.pfnCallback = inValue; }
    void* get_pUserData() { return m_struct.pUserData; }
    void set_pUserData(void* inValue) { m_struct.pUserData = inValue; }


private:
    VkDebugReportCallbackCreateInfoEXT m_struct;
    const VkDebugReportCallbackCreateInfoEXT* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkdedicatedallocationbuffercreateinfonv_struct_wrapper
{
public:
    vkdedicatedallocationbuffercreateinfonv_struct_wrapper();
    vkdedicatedallocationbuffercreateinfonv_struct_wrapper(VkDedicatedAllocationBufferCreateInfoNV* pInStruct);
    vkdedicatedallocationbuffercreateinfonv_struct_wrapper(const VkDedicatedAllocationBufferCreateInfoNV* pInStruct);

    virtual ~vkdedicatedallocationbuffercreateinfonv_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkBool32 get_dedicatedAllocation() { return m_struct.dedicatedAllocation; }
    void set_dedicatedAllocation(VkBool32 inValue) { m_struct.dedicatedAllocation = inValue; }


private:
    VkDedicatedAllocationBufferCreateInfoNV m_struct;
    const VkDedicatedAllocationBufferCreateInfoNV* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkdedicatedallocationimagecreateinfonv_struct_wrapper
{
public:
    vkdedicatedallocationimagecreateinfonv_struct_wrapper();
    vkdedicatedallocationimagecreateinfonv_struct_wrapper(VkDedicatedAllocationImageCreateInfoNV* pInStruct);
    vkdedicatedallocationimagecreateinfonv_struct_wrapper(const VkDedicatedAllocationImageCreateInfoNV* pInStruct);

    virtual ~vkdedicatedallocationimagecreateinfonv_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkBool32 get_dedicatedAllocation() { return m_struct.dedicatedAllocation; }
    void set_dedicatedAllocation(VkBool32 inValue) { m_struct.dedicatedAllocation = inValue; }


private:
    VkDedicatedAllocationImageCreateInfoNV m_struct;
    const VkDedicatedAllocationImageCreateInfoNV* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkdedicatedallocationmemoryallocateinfonv_struct_wrapper
{
public:
    vkdedicatedallocationmemoryallocateinfonv_struct_wrapper();
    vkdedicatedallocationmemoryallocateinfonv_struct_wrapper(VkDedicatedAllocationMemoryAllocateInfoNV* pInStruct);
    vkdedicatedallocationmemoryallocateinfonv_struct_wrapper(const VkDedicatedAllocationMemoryAllocateInfoNV* pInStruct);

    virtual ~vkdedicatedallocationmemoryallocateinfonv_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkImage get_image() { return m_struct.image; }
    void set_image(VkImage inValue) { m_struct.image = inValue; }
    VkBuffer get_buffer() { return m_struct.buffer; }
    void set_buffer(VkBuffer inValue) { m_struct.buffer = inValue; }


private:
    VkDedicatedAllocationMemoryAllocateInfoNV m_struct;
    const VkDedicatedAllocationMemoryAllocateInfoNV* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkdescriptorbufferinfo_struct_wrapper
{
public:
    vkdescriptorbufferinfo_struct_wrapper();
    vkdescriptorbufferinfo_struct_wrapper(VkDescriptorBufferInfo* pInStruct);
    vkdescriptorbufferinfo_struct_wrapper(const VkDescriptorBufferInfo* pInStruct);

    virtual ~vkdescriptorbufferinfo_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkBuffer get_buffer() { return m_struct.buffer; }
    void set_buffer(VkBuffer inValue) { m_struct.buffer = inValue; }
    VkDeviceSize get_offset() { return m_struct.offset; }
    void set_offset(VkDeviceSize inValue) { m_struct.offset = inValue; }
    VkDeviceSize get_range() { return m_struct.range; }
    void set_range(VkDeviceSize inValue) { m_struct.range = inValue; }


private:
    VkDescriptorBufferInfo m_struct;
    const VkDescriptorBufferInfo* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkdescriptorimageinfo_struct_wrapper
{
public:
    vkdescriptorimageinfo_struct_wrapper();
    vkdescriptorimageinfo_struct_wrapper(VkDescriptorImageInfo* pInStruct);
    vkdescriptorimageinfo_struct_wrapper(const VkDescriptorImageInfo* pInStruct);

    virtual ~vkdescriptorimageinfo_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkSampler get_sampler() { return m_struct.sampler; }
    void set_sampler(VkSampler inValue) { m_struct.sampler = inValue; }
    VkImageView get_imageView() { return m_struct.imageView; }
    void set_imageView(VkImageView inValue) { m_struct.imageView = inValue; }
    VkImageLayout get_imageLayout() { return m_struct.imageLayout; }
    void set_imageLayout(VkImageLayout inValue) { m_struct.imageLayout = inValue; }


private:
    VkDescriptorImageInfo m_struct;
    const VkDescriptorImageInfo* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkdescriptorpoolcreateinfo_struct_wrapper
{
public:
    vkdescriptorpoolcreateinfo_struct_wrapper();
    vkdescriptorpoolcreateinfo_struct_wrapper(VkDescriptorPoolCreateInfo* pInStruct);
    vkdescriptorpoolcreateinfo_struct_wrapper(const VkDescriptorPoolCreateInfo* pInStruct);

    virtual ~vkdescriptorpoolcreateinfo_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkDescriptorPoolCreateFlags get_flags() { return m_struct.flags; }
    void set_flags(VkDescriptorPoolCreateFlags inValue) { m_struct.flags = inValue; }
    uint32_t get_maxSets() { return m_struct.maxSets; }
    void set_maxSets(uint32_t inValue) { m_struct.maxSets = inValue; }
    uint32_t get_poolSizeCount() { return m_struct.poolSizeCount; }
    void set_poolSizeCount(uint32_t inValue) { m_struct.poolSizeCount = inValue; }


private:
    VkDescriptorPoolCreateInfo m_struct;
    const VkDescriptorPoolCreateInfo* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkdescriptorpoolsize_struct_wrapper
{
public:
    vkdescriptorpoolsize_struct_wrapper();
    vkdescriptorpoolsize_struct_wrapper(VkDescriptorPoolSize* pInStruct);
    vkdescriptorpoolsize_struct_wrapper(const VkDescriptorPoolSize* pInStruct);

    virtual ~vkdescriptorpoolsize_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkDescriptorType get_type() { return m_struct.type; }
    void set_type(VkDescriptorType inValue) { m_struct.type = inValue; }
    uint32_t get_descriptorCount() { return m_struct.descriptorCount; }
    void set_descriptorCount(uint32_t inValue) { m_struct.descriptorCount = inValue; }


private:
    VkDescriptorPoolSize m_struct;
    const VkDescriptorPoolSize* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkdescriptorsetallocateinfo_struct_wrapper
{
public:
    vkdescriptorsetallocateinfo_struct_wrapper();
    vkdescriptorsetallocateinfo_struct_wrapper(VkDescriptorSetAllocateInfo* pInStruct);
    vkdescriptorsetallocateinfo_struct_wrapper(const VkDescriptorSetAllocateInfo* pInStruct);

    virtual ~vkdescriptorsetallocateinfo_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkDescriptorPool get_descriptorPool() { return m_struct.descriptorPool; }
    void set_descriptorPool(VkDescriptorPool inValue) { m_struct.descriptorPool = inValue; }
    uint32_t get_descriptorSetCount() { return m_struct.descriptorSetCount; }
    void set_descriptorSetCount(uint32_t inValue) { m_struct.descriptorSetCount = inValue; }


private:
    VkDescriptorSetAllocateInfo m_struct;
    const VkDescriptorSetAllocateInfo* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkdescriptorsetlayoutbinding_struct_wrapper
{
public:
    vkdescriptorsetlayoutbinding_struct_wrapper();
    vkdescriptorsetlayoutbinding_struct_wrapper(VkDescriptorSetLayoutBinding* pInStruct);
    vkdescriptorsetlayoutbinding_struct_wrapper(const VkDescriptorSetLayoutBinding* pInStruct);

    virtual ~vkdescriptorsetlayoutbinding_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    uint32_t get_binding() { return m_struct.binding; }
    void set_binding(uint32_t inValue) { m_struct.binding = inValue; }
    VkDescriptorType get_descriptorType() { return m_struct.descriptorType; }
    void set_descriptorType(VkDescriptorType inValue) { m_struct.descriptorType = inValue; }
    uint32_t get_descriptorCount() { return m_struct.descriptorCount; }
    void set_descriptorCount(uint32_t inValue) { m_struct.descriptorCount = inValue; }
    VkShaderStageFlags get_stageFlags() { return m_struct.stageFlags; }
    void set_stageFlags(VkShaderStageFlags inValue) { m_struct.stageFlags = inValue; }


private:
    VkDescriptorSetLayoutBinding m_struct;
    const VkDescriptorSetLayoutBinding* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkdescriptorsetlayoutcreateinfo_struct_wrapper
{
public:
    vkdescriptorsetlayoutcreateinfo_struct_wrapper();
    vkdescriptorsetlayoutcreateinfo_struct_wrapper(VkDescriptorSetLayoutCreateInfo* pInStruct);
    vkdescriptorsetlayoutcreateinfo_struct_wrapper(const VkDescriptorSetLayoutCreateInfo* pInStruct);

    virtual ~vkdescriptorsetlayoutcreateinfo_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkDescriptorSetLayoutCreateFlags get_flags() { return m_struct.flags; }
    void set_flags(VkDescriptorSetLayoutCreateFlags inValue) { m_struct.flags = inValue; }
    uint32_t get_bindingCount() { return m_struct.bindingCount; }
    void set_bindingCount(uint32_t inValue) { m_struct.bindingCount = inValue; }


private:
    VkDescriptorSetLayoutCreateInfo m_struct;
    const VkDescriptorSetLayoutCreateInfo* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkdevicecreateinfo_struct_wrapper
{
public:
    vkdevicecreateinfo_struct_wrapper();
    vkdevicecreateinfo_struct_wrapper(VkDeviceCreateInfo* pInStruct);
    vkdevicecreateinfo_struct_wrapper(const VkDeviceCreateInfo* pInStruct);

    virtual ~vkdevicecreateinfo_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkDeviceCreateFlags get_flags() { return m_struct.flags; }
    void set_flags(VkDeviceCreateFlags inValue) { m_struct.flags = inValue; }
    uint32_t get_queueCreateInfoCount() { return m_struct.queueCreateInfoCount; }
    void set_queueCreateInfoCount(uint32_t inValue) { m_struct.queueCreateInfoCount = inValue; }
    uint32_t get_enabledLayerCount() { return m_struct.enabledLayerCount; }
    void set_enabledLayerCount(uint32_t inValue) { m_struct.enabledLayerCount = inValue; }
    uint32_t get_enabledExtensionCount() { return m_struct.enabledExtensionCount; }
    void set_enabledExtensionCount(uint32_t inValue) { m_struct.enabledExtensionCount = inValue; }
    const VkPhysicalDeviceFeatures* get_pEnabledFeatures() { return m_struct.pEnabledFeatures; }


private:
    VkDeviceCreateInfo m_struct;
    const VkDeviceCreateInfo* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkdevicequeuecreateinfo_struct_wrapper
{
public:
    vkdevicequeuecreateinfo_struct_wrapper();
    vkdevicequeuecreateinfo_struct_wrapper(VkDeviceQueueCreateInfo* pInStruct);
    vkdevicequeuecreateinfo_struct_wrapper(const VkDeviceQueueCreateInfo* pInStruct);

    virtual ~vkdevicequeuecreateinfo_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkDeviceQueueCreateFlags get_flags() { return m_struct.flags; }
    void set_flags(VkDeviceQueueCreateFlags inValue) { m_struct.flags = inValue; }
    uint32_t get_queueFamilyIndex() { return m_struct.queueFamilyIndex; }
    void set_queueFamilyIndex(uint32_t inValue) { m_struct.queueFamilyIndex = inValue; }
    uint32_t get_queueCount() { return m_struct.queueCount; }
    void set_queueCount(uint32_t inValue) { m_struct.queueCount = inValue; }


private:
    VkDeviceQueueCreateInfo m_struct;
    const VkDeviceQueueCreateInfo* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkdispatchindirectcommand_struct_wrapper
{
public:
    vkdispatchindirectcommand_struct_wrapper();
    vkdispatchindirectcommand_struct_wrapper(VkDispatchIndirectCommand* pInStruct);
    vkdispatchindirectcommand_struct_wrapper(const VkDispatchIndirectCommand* pInStruct);

    virtual ~vkdispatchindirectcommand_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    uint32_t get_x() { return m_struct.x; }
    void set_x(uint32_t inValue) { m_struct.x = inValue; }
    uint32_t get_y() { return m_struct.y; }
    void set_y(uint32_t inValue) { m_struct.y = inValue; }
    uint32_t get_z() { return m_struct.z; }
    void set_z(uint32_t inValue) { m_struct.z = inValue; }


private:
    VkDispatchIndirectCommand m_struct;
    const VkDispatchIndirectCommand* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkdisplaymodecreateinfokhr_struct_wrapper
{
public:
    vkdisplaymodecreateinfokhr_struct_wrapper();
    vkdisplaymodecreateinfokhr_struct_wrapper(VkDisplayModeCreateInfoKHR* pInStruct);
    vkdisplaymodecreateinfokhr_struct_wrapper(const VkDisplayModeCreateInfoKHR* pInStruct);

    virtual ~vkdisplaymodecreateinfokhr_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkDisplayModeCreateFlagsKHR get_flags() { return m_struct.flags; }
    void set_flags(VkDisplayModeCreateFlagsKHR inValue) { m_struct.flags = inValue; }
    VkDisplayModeParametersKHR get_parameters() { return m_struct.parameters; }
    void set_parameters(VkDisplayModeParametersKHR inValue) { m_struct.parameters = inValue; }


private:
    VkDisplayModeCreateInfoKHR m_struct;
    const VkDisplayModeCreateInfoKHR* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkdisplaymodeparameterskhr_struct_wrapper
{
public:
    vkdisplaymodeparameterskhr_struct_wrapper();
    vkdisplaymodeparameterskhr_struct_wrapper(VkDisplayModeParametersKHR* pInStruct);
    vkdisplaymodeparameterskhr_struct_wrapper(const VkDisplayModeParametersKHR* pInStruct);

    virtual ~vkdisplaymodeparameterskhr_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkExtent2D get_visibleRegion() { return m_struct.visibleRegion; }
    void set_visibleRegion(VkExtent2D inValue) { m_struct.visibleRegion = inValue; }
    uint32_t get_refreshRate() { return m_struct.refreshRate; }
    void set_refreshRate(uint32_t inValue) { m_struct.refreshRate = inValue; }


private:
    VkDisplayModeParametersKHR m_struct;
    const VkDisplayModeParametersKHR* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkdisplaymodepropertieskhr_struct_wrapper
{
public:
    vkdisplaymodepropertieskhr_struct_wrapper();
    vkdisplaymodepropertieskhr_struct_wrapper(VkDisplayModePropertiesKHR* pInStruct);
    vkdisplaymodepropertieskhr_struct_wrapper(const VkDisplayModePropertiesKHR* pInStruct);

    virtual ~vkdisplaymodepropertieskhr_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkDisplayModeKHR get_displayMode() { return m_struct.displayMode; }
    void set_displayMode(VkDisplayModeKHR inValue) { m_struct.displayMode = inValue; }
    VkDisplayModeParametersKHR get_parameters() { return m_struct.parameters; }
    void set_parameters(VkDisplayModeParametersKHR inValue) { m_struct.parameters = inValue; }


private:
    VkDisplayModePropertiesKHR m_struct;
    const VkDisplayModePropertiesKHR* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkdisplayplanecapabilitieskhr_struct_wrapper
{
public:
    vkdisplayplanecapabilitieskhr_struct_wrapper();
    vkdisplayplanecapabilitieskhr_struct_wrapper(VkDisplayPlaneCapabilitiesKHR* pInStruct);
    vkdisplayplanecapabilitieskhr_struct_wrapper(const VkDisplayPlaneCapabilitiesKHR* pInStruct);

    virtual ~vkdisplayplanecapabilitieskhr_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkDisplayPlaneAlphaFlagsKHR get_supportedAlpha() { return m_struct.supportedAlpha; }
    void set_supportedAlpha(VkDisplayPlaneAlphaFlagsKHR inValue) { m_struct.supportedAlpha = inValue; }
    VkOffset2D get_minSrcPosition() { return m_struct.minSrcPosition; }
    void set_minSrcPosition(VkOffset2D inValue) { m_struct.minSrcPosition = inValue; }
    VkOffset2D get_maxSrcPosition() { return m_struct.maxSrcPosition; }
    void set_maxSrcPosition(VkOffset2D inValue) { m_struct.maxSrcPosition = inValue; }
    VkExtent2D get_minSrcExtent() { return m_struct.minSrcExtent; }
    void set_minSrcExtent(VkExtent2D inValue) { m_struct.minSrcExtent = inValue; }
    VkExtent2D get_maxSrcExtent() { return m_struct.maxSrcExtent; }
    void set_maxSrcExtent(VkExtent2D inValue) { m_struct.maxSrcExtent = inValue; }
    VkOffset2D get_minDstPosition() { return m_struct.minDstPosition; }
    void set_minDstPosition(VkOffset2D inValue) { m_struct.minDstPosition = inValue; }
    VkOffset2D get_maxDstPosition() { return m_struct.maxDstPosition; }
    void set_maxDstPosition(VkOffset2D inValue) { m_struct.maxDstPosition = inValue; }
    VkExtent2D get_minDstExtent() { return m_struct.minDstExtent; }
    void set_minDstExtent(VkExtent2D inValue) { m_struct.minDstExtent = inValue; }
    VkExtent2D get_maxDstExtent() { return m_struct.maxDstExtent; }
    void set_maxDstExtent(VkExtent2D inValue) { m_struct.maxDstExtent = inValue; }


private:
    VkDisplayPlaneCapabilitiesKHR m_struct;
    const VkDisplayPlaneCapabilitiesKHR* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkdisplayplanepropertieskhr_struct_wrapper
{
public:
    vkdisplayplanepropertieskhr_struct_wrapper();
    vkdisplayplanepropertieskhr_struct_wrapper(VkDisplayPlanePropertiesKHR* pInStruct);
    vkdisplayplanepropertieskhr_struct_wrapper(const VkDisplayPlanePropertiesKHR* pInStruct);

    virtual ~vkdisplayplanepropertieskhr_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkDisplayKHR get_currentDisplay() { return m_struct.currentDisplay; }
    void set_currentDisplay(VkDisplayKHR inValue) { m_struct.currentDisplay = inValue; }
    uint32_t get_currentStackIndex() { return m_struct.currentStackIndex; }
    void set_currentStackIndex(uint32_t inValue) { m_struct.currentStackIndex = inValue; }


private:
    VkDisplayPlanePropertiesKHR m_struct;
    const VkDisplayPlanePropertiesKHR* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkdisplaypresentinfokhr_struct_wrapper
{
public:
    vkdisplaypresentinfokhr_struct_wrapper();
    vkdisplaypresentinfokhr_struct_wrapper(VkDisplayPresentInfoKHR* pInStruct);
    vkdisplaypresentinfokhr_struct_wrapper(const VkDisplayPresentInfoKHR* pInStruct);

    virtual ~vkdisplaypresentinfokhr_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkRect2D get_srcRect() { return m_struct.srcRect; }
    void set_srcRect(VkRect2D inValue) { m_struct.srcRect = inValue; }
    VkRect2D get_dstRect() { return m_struct.dstRect; }
    void set_dstRect(VkRect2D inValue) { m_struct.dstRect = inValue; }
    VkBool32 get_persistent() { return m_struct.persistent; }
    void set_persistent(VkBool32 inValue) { m_struct.persistent = inValue; }


private:
    VkDisplayPresentInfoKHR m_struct;
    const VkDisplayPresentInfoKHR* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkdisplaypropertieskhr_struct_wrapper
{
public:
    vkdisplaypropertieskhr_struct_wrapper();
    vkdisplaypropertieskhr_struct_wrapper(VkDisplayPropertiesKHR* pInStruct);
    vkdisplaypropertieskhr_struct_wrapper(const VkDisplayPropertiesKHR* pInStruct);

    virtual ~vkdisplaypropertieskhr_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkDisplayKHR get_display() { return m_struct.display; }
    void set_display(VkDisplayKHR inValue) { m_struct.display = inValue; }
    const char* get_displayName() { return m_struct.displayName; }
    VkExtent2D get_physicalDimensions() { return m_struct.physicalDimensions; }
    void set_physicalDimensions(VkExtent2D inValue) { m_struct.physicalDimensions = inValue; }
    VkExtent2D get_physicalResolution() { return m_struct.physicalResolution; }
    void set_physicalResolution(VkExtent2D inValue) { m_struct.physicalResolution = inValue; }
    VkSurfaceTransformFlagsKHR get_supportedTransforms() { return m_struct.supportedTransforms; }
    void set_supportedTransforms(VkSurfaceTransformFlagsKHR inValue) { m_struct.supportedTransforms = inValue; }
    VkBool32 get_planeReorderPossible() { return m_struct.planeReorderPossible; }
    void set_planeReorderPossible(VkBool32 inValue) { m_struct.planeReorderPossible = inValue; }
    VkBool32 get_persistentContent() { return m_struct.persistentContent; }
    void set_persistentContent(VkBool32 inValue) { m_struct.persistentContent = inValue; }


private:
    VkDisplayPropertiesKHR m_struct;
    const VkDisplayPropertiesKHR* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkdisplaysurfacecreateinfokhr_struct_wrapper
{
public:
    vkdisplaysurfacecreateinfokhr_struct_wrapper();
    vkdisplaysurfacecreateinfokhr_struct_wrapper(VkDisplaySurfaceCreateInfoKHR* pInStruct);
    vkdisplaysurfacecreateinfokhr_struct_wrapper(const VkDisplaySurfaceCreateInfoKHR* pInStruct);

    virtual ~vkdisplaysurfacecreateinfokhr_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkDisplaySurfaceCreateFlagsKHR get_flags() { return m_struct.flags; }
    void set_flags(VkDisplaySurfaceCreateFlagsKHR inValue) { m_struct.flags = inValue; }
    VkDisplayModeKHR get_displayMode() { return m_struct.displayMode; }
    void set_displayMode(VkDisplayModeKHR inValue) { m_struct.displayMode = inValue; }
    uint32_t get_planeIndex() { return m_struct.planeIndex; }
    void set_planeIndex(uint32_t inValue) { m_struct.planeIndex = inValue; }
    uint32_t get_planeStackIndex() { return m_struct.planeStackIndex; }
    void set_planeStackIndex(uint32_t inValue) { m_struct.planeStackIndex = inValue; }
    VkSurfaceTransformFlagBitsKHR get_transform() { return m_struct.transform; }
    void set_transform(VkSurfaceTransformFlagBitsKHR inValue) { m_struct.transform = inValue; }
    float get_globalAlpha() { return m_struct.globalAlpha; }
    void set_globalAlpha(float inValue) { m_struct.globalAlpha = inValue; }
    VkDisplayPlaneAlphaFlagBitsKHR get_alphaMode() { return m_struct.alphaMode; }
    void set_alphaMode(VkDisplayPlaneAlphaFlagBitsKHR inValue) { m_struct.alphaMode = inValue; }
    VkExtent2D get_imageExtent() { return m_struct.imageExtent; }
    void set_imageExtent(VkExtent2D inValue) { m_struct.imageExtent = inValue; }


private:
    VkDisplaySurfaceCreateInfoKHR m_struct;
    const VkDisplaySurfaceCreateInfoKHR* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkdrawindexedindirectcommand_struct_wrapper
{
public:
    vkdrawindexedindirectcommand_struct_wrapper();
    vkdrawindexedindirectcommand_struct_wrapper(VkDrawIndexedIndirectCommand* pInStruct);
    vkdrawindexedindirectcommand_struct_wrapper(const VkDrawIndexedIndirectCommand* pInStruct);

    virtual ~vkdrawindexedindirectcommand_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    uint32_t get_indexCount() { return m_struct.indexCount; }
    void set_indexCount(uint32_t inValue) { m_struct.indexCount = inValue; }
    uint32_t get_instanceCount() { return m_struct.instanceCount; }
    void set_instanceCount(uint32_t inValue) { m_struct.instanceCount = inValue; }
    uint32_t get_firstIndex() { return m_struct.firstIndex; }
    void set_firstIndex(uint32_t inValue) { m_struct.firstIndex = inValue; }
    int32_t get_vertexOffset() { return m_struct.vertexOffset; }
    void set_vertexOffset(int32_t inValue) { m_struct.vertexOffset = inValue; }
    uint32_t get_firstInstance() { return m_struct.firstInstance; }
    void set_firstInstance(uint32_t inValue) { m_struct.firstInstance = inValue; }


private:
    VkDrawIndexedIndirectCommand m_struct;
    const VkDrawIndexedIndirectCommand* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkdrawindirectcommand_struct_wrapper
{
public:
    vkdrawindirectcommand_struct_wrapper();
    vkdrawindirectcommand_struct_wrapper(VkDrawIndirectCommand* pInStruct);
    vkdrawindirectcommand_struct_wrapper(const VkDrawIndirectCommand* pInStruct);

    virtual ~vkdrawindirectcommand_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    uint32_t get_vertexCount() { return m_struct.vertexCount; }
    void set_vertexCount(uint32_t inValue) { m_struct.vertexCount = inValue; }
    uint32_t get_instanceCount() { return m_struct.instanceCount; }
    void set_instanceCount(uint32_t inValue) { m_struct.instanceCount = inValue; }
    uint32_t get_firstVertex() { return m_struct.firstVertex; }
    void set_firstVertex(uint32_t inValue) { m_struct.firstVertex = inValue; }
    uint32_t get_firstInstance() { return m_struct.firstInstance; }
    void set_firstInstance(uint32_t inValue) { m_struct.firstInstance = inValue; }


private:
    VkDrawIndirectCommand m_struct;
    const VkDrawIndirectCommand* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkeventcreateinfo_struct_wrapper
{
public:
    vkeventcreateinfo_struct_wrapper();
    vkeventcreateinfo_struct_wrapper(VkEventCreateInfo* pInStruct);
    vkeventcreateinfo_struct_wrapper(const VkEventCreateInfo* pInStruct);

    virtual ~vkeventcreateinfo_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkEventCreateFlags get_flags() { return m_struct.flags; }
    void set_flags(VkEventCreateFlags inValue) { m_struct.flags = inValue; }


private:
    VkEventCreateInfo m_struct;
    const VkEventCreateInfo* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkexportmemoryallocateinfonv_struct_wrapper
{
public:
    vkexportmemoryallocateinfonv_struct_wrapper();
    vkexportmemoryallocateinfonv_struct_wrapper(VkExportMemoryAllocateInfoNV* pInStruct);
    vkexportmemoryallocateinfonv_struct_wrapper(const VkExportMemoryAllocateInfoNV* pInStruct);

    virtual ~vkexportmemoryallocateinfonv_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkExternalMemoryHandleTypeFlagsNV get_handleTypes() { return m_struct.handleTypes; }
    void set_handleTypes(VkExternalMemoryHandleTypeFlagsNV inValue) { m_struct.handleTypes = inValue; }


private:
    VkExportMemoryAllocateInfoNV m_struct;
    const VkExportMemoryAllocateInfoNV* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkexportmemorywin32handleinfonv_struct_wrapper
{
public:
    vkexportmemorywin32handleinfonv_struct_wrapper();
    vkexportmemorywin32handleinfonv_struct_wrapper(VkExportMemoryWin32HandleInfoNV* pInStruct);
    vkexportmemorywin32handleinfonv_struct_wrapper(const VkExportMemoryWin32HandleInfoNV* pInStruct);

    virtual ~vkexportmemorywin32handleinfonv_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    const SECURITY_ATTRIBUTES* get_pAttributes() { return m_struct.pAttributes; }
    DWORD get_dwAccess() { return m_struct.dwAccess; }
    void set_dwAccess(DWORD inValue) { m_struct.dwAccess = inValue; }


private:
    VkExportMemoryWin32HandleInfoNV m_struct;
    const VkExportMemoryWin32HandleInfoNV* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkextensionproperties_struct_wrapper
{
public:
    vkextensionproperties_struct_wrapper();
    vkextensionproperties_struct_wrapper(VkExtensionProperties* pInStruct);
    vkextensionproperties_struct_wrapper(const VkExtensionProperties* pInStruct);

    virtual ~vkextensionproperties_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    uint32_t get_specVersion() { return m_struct.specVersion; }
    void set_specVersion(uint32_t inValue) { m_struct.specVersion = inValue; }


private:
    VkExtensionProperties m_struct;
    const VkExtensionProperties* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkextent2d_struct_wrapper
{
public:
    vkextent2d_struct_wrapper();
    vkextent2d_struct_wrapper(VkExtent2D* pInStruct);
    vkextent2d_struct_wrapper(const VkExtent2D* pInStruct);

    virtual ~vkextent2d_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    uint32_t get_width() { return m_struct.width; }
    void set_width(uint32_t inValue) { m_struct.width = inValue; }
    uint32_t get_height() { return m_struct.height; }
    void set_height(uint32_t inValue) { m_struct.height = inValue; }


private:
    VkExtent2D m_struct;
    const VkExtent2D* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkextent3d_struct_wrapper
{
public:
    vkextent3d_struct_wrapper();
    vkextent3d_struct_wrapper(VkExtent3D* pInStruct);
    vkextent3d_struct_wrapper(const VkExtent3D* pInStruct);

    virtual ~vkextent3d_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    uint32_t get_width() { return m_struct.width; }
    void set_width(uint32_t inValue) { m_struct.width = inValue; }
    uint32_t get_height() { return m_struct.height; }
    void set_height(uint32_t inValue) { m_struct.height = inValue; }
    uint32_t get_depth() { return m_struct.depth; }
    void set_depth(uint32_t inValue) { m_struct.depth = inValue; }


private:
    VkExtent3D m_struct;
    const VkExtent3D* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkexternalimageformatpropertiesnv_struct_wrapper
{
public:
    vkexternalimageformatpropertiesnv_struct_wrapper();
    vkexternalimageformatpropertiesnv_struct_wrapper(VkExternalImageFormatPropertiesNV* pInStruct);
    vkexternalimageformatpropertiesnv_struct_wrapper(const VkExternalImageFormatPropertiesNV* pInStruct);

    virtual ~vkexternalimageformatpropertiesnv_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkImageFormatProperties get_imageFormatProperties() { return m_struct.imageFormatProperties; }
    void set_imageFormatProperties(VkImageFormatProperties inValue) { m_struct.imageFormatProperties = inValue; }
    VkExternalMemoryFeatureFlagsNV get_externalMemoryFeatures() { return m_struct.externalMemoryFeatures; }
    void set_externalMemoryFeatures(VkExternalMemoryFeatureFlagsNV inValue) { m_struct.externalMemoryFeatures = inValue; }
    VkExternalMemoryHandleTypeFlagsNV get_exportFromImportedHandleTypes() { return m_struct.exportFromImportedHandleTypes; }
    void set_exportFromImportedHandleTypes(VkExternalMemoryHandleTypeFlagsNV inValue) { m_struct.exportFromImportedHandleTypes = inValue; }
    VkExternalMemoryHandleTypeFlagsNV get_compatibleHandleTypes() { return m_struct.compatibleHandleTypes; }
    void set_compatibleHandleTypes(VkExternalMemoryHandleTypeFlagsNV inValue) { m_struct.compatibleHandleTypes = inValue; }


private:
    VkExternalImageFormatPropertiesNV m_struct;
    const VkExternalImageFormatPropertiesNV* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkexternalmemoryimagecreateinfonv_struct_wrapper
{
public:
    vkexternalmemoryimagecreateinfonv_struct_wrapper();
    vkexternalmemoryimagecreateinfonv_struct_wrapper(VkExternalMemoryImageCreateInfoNV* pInStruct);
    vkexternalmemoryimagecreateinfonv_struct_wrapper(const VkExternalMemoryImageCreateInfoNV* pInStruct);

    virtual ~vkexternalmemoryimagecreateinfonv_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkExternalMemoryHandleTypeFlagsNV get_handleTypes() { return m_struct.handleTypes; }
    void set_handleTypes(VkExternalMemoryHandleTypeFlagsNV inValue) { m_struct.handleTypes = inValue; }


private:
    VkExternalMemoryImageCreateInfoNV m_struct;
    const VkExternalMemoryImageCreateInfoNV* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkfencecreateinfo_struct_wrapper
{
public:
    vkfencecreateinfo_struct_wrapper();
    vkfencecreateinfo_struct_wrapper(VkFenceCreateInfo* pInStruct);
    vkfencecreateinfo_struct_wrapper(const VkFenceCreateInfo* pInStruct);

    virtual ~vkfencecreateinfo_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkFenceCreateFlags get_flags() { return m_struct.flags; }
    void set_flags(VkFenceCreateFlags inValue) { m_struct.flags = inValue; }


private:
    VkFenceCreateInfo m_struct;
    const VkFenceCreateInfo* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkformatproperties_struct_wrapper
{
public:
    vkformatproperties_struct_wrapper();
    vkformatproperties_struct_wrapper(VkFormatProperties* pInStruct);
    vkformatproperties_struct_wrapper(const VkFormatProperties* pInStruct);

    virtual ~vkformatproperties_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkFormatFeatureFlags get_linearTilingFeatures() { return m_struct.linearTilingFeatures; }
    void set_linearTilingFeatures(VkFormatFeatureFlags inValue) { m_struct.linearTilingFeatures = inValue; }
    VkFormatFeatureFlags get_optimalTilingFeatures() { return m_struct.optimalTilingFeatures; }
    void set_optimalTilingFeatures(VkFormatFeatureFlags inValue) { m_struct.optimalTilingFeatures = inValue; }
    VkFormatFeatureFlags get_bufferFeatures() { return m_struct.bufferFeatures; }
    void set_bufferFeatures(VkFormatFeatureFlags inValue) { m_struct.bufferFeatures = inValue; }


private:
    VkFormatProperties m_struct;
    const VkFormatProperties* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkframebuffercreateinfo_struct_wrapper
{
public:
    vkframebuffercreateinfo_struct_wrapper();
    vkframebuffercreateinfo_struct_wrapper(VkFramebufferCreateInfo* pInStruct);
    vkframebuffercreateinfo_struct_wrapper(const VkFramebufferCreateInfo* pInStruct);

    virtual ~vkframebuffercreateinfo_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkFramebufferCreateFlags get_flags() { return m_struct.flags; }
    void set_flags(VkFramebufferCreateFlags inValue) { m_struct.flags = inValue; }
    VkRenderPass get_renderPass() { return m_struct.renderPass; }
    void set_renderPass(VkRenderPass inValue) { m_struct.renderPass = inValue; }
    uint32_t get_attachmentCount() { return m_struct.attachmentCount; }
    void set_attachmentCount(uint32_t inValue) { m_struct.attachmentCount = inValue; }
    uint32_t get_width() { return m_struct.width; }
    void set_width(uint32_t inValue) { m_struct.width = inValue; }
    uint32_t get_height() { return m_struct.height; }
    void set_height(uint32_t inValue) { m_struct.height = inValue; }
    uint32_t get_layers() { return m_struct.layers; }
    void set_layers(uint32_t inValue) { m_struct.layers = inValue; }


private:
    VkFramebufferCreateInfo m_struct;
    const VkFramebufferCreateInfo* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkgraphicspipelinecreateinfo_struct_wrapper
{
public:
    vkgraphicspipelinecreateinfo_struct_wrapper();
    vkgraphicspipelinecreateinfo_struct_wrapper(VkGraphicsPipelineCreateInfo* pInStruct);
    vkgraphicspipelinecreateinfo_struct_wrapper(const VkGraphicsPipelineCreateInfo* pInStruct);

    virtual ~vkgraphicspipelinecreateinfo_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkPipelineCreateFlags get_flags() { return m_struct.flags; }
    void set_flags(VkPipelineCreateFlags inValue) { m_struct.flags = inValue; }
    uint32_t get_stageCount() { return m_struct.stageCount; }
    void set_stageCount(uint32_t inValue) { m_struct.stageCount = inValue; }
    const VkPipelineVertexInputStateCreateInfo* get_pVertexInputState() { return m_struct.pVertexInputState; }
    const VkPipelineInputAssemblyStateCreateInfo* get_pInputAssemblyState() { return m_struct.pInputAssemblyState; }
    const VkPipelineTessellationStateCreateInfo* get_pTessellationState() { return m_struct.pTessellationState; }
    const VkPipelineViewportStateCreateInfo* get_pViewportState() { return m_struct.pViewportState; }
    const VkPipelineRasterizationStateCreateInfo* get_pRasterizationState() { return m_struct.pRasterizationState; }
    const VkPipelineMultisampleStateCreateInfo* get_pMultisampleState() { return m_struct.pMultisampleState; }
    const VkPipelineDepthStencilStateCreateInfo* get_pDepthStencilState() { return m_struct.pDepthStencilState; }
    const VkPipelineColorBlendStateCreateInfo* get_pColorBlendState() { return m_struct.pColorBlendState; }
    const VkPipelineDynamicStateCreateInfo* get_pDynamicState() { return m_struct.pDynamicState; }
    VkPipelineLayout get_layout() { return m_struct.layout; }
    void set_layout(VkPipelineLayout inValue) { m_struct.layout = inValue; }
    VkRenderPass get_renderPass() { return m_struct.renderPass; }
    void set_renderPass(VkRenderPass inValue) { m_struct.renderPass = inValue; }
    uint32_t get_subpass() { return m_struct.subpass; }
    void set_subpass(uint32_t inValue) { m_struct.subpass = inValue; }
    VkPipeline get_basePipelineHandle() { return m_struct.basePipelineHandle; }
    void set_basePipelineHandle(VkPipeline inValue) { m_struct.basePipelineHandle = inValue; }
    int32_t get_basePipelineIndex() { return m_struct.basePipelineIndex; }
    void set_basePipelineIndex(int32_t inValue) { m_struct.basePipelineIndex = inValue; }


private:
    VkGraphicsPipelineCreateInfo m_struct;
    const VkGraphicsPipelineCreateInfo* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkimageblit_struct_wrapper
{
public:
    vkimageblit_struct_wrapper();
    vkimageblit_struct_wrapper(VkImageBlit* pInStruct);
    vkimageblit_struct_wrapper(const VkImageBlit* pInStruct);

    virtual ~vkimageblit_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkImageSubresourceLayers get_srcSubresource() { return m_struct.srcSubresource; }
    void set_srcSubresource(VkImageSubresourceLayers inValue) { m_struct.srcSubresource = inValue; }
    VkImageSubresourceLayers get_dstSubresource() { return m_struct.dstSubresource; }
    void set_dstSubresource(VkImageSubresourceLayers inValue) { m_struct.dstSubresource = inValue; }


private:
    VkImageBlit m_struct;
    const VkImageBlit* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkimagecopy_struct_wrapper
{
public:
    vkimagecopy_struct_wrapper();
    vkimagecopy_struct_wrapper(VkImageCopy* pInStruct);
    vkimagecopy_struct_wrapper(const VkImageCopy* pInStruct);

    virtual ~vkimagecopy_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkImageSubresourceLayers get_srcSubresource() { return m_struct.srcSubresource; }
    void set_srcSubresource(VkImageSubresourceLayers inValue) { m_struct.srcSubresource = inValue; }
    VkOffset3D get_srcOffset() { return m_struct.srcOffset; }
    void set_srcOffset(VkOffset3D inValue) { m_struct.srcOffset = inValue; }
    VkImageSubresourceLayers get_dstSubresource() { return m_struct.dstSubresource; }
    void set_dstSubresource(VkImageSubresourceLayers inValue) { m_struct.dstSubresource = inValue; }
    VkOffset3D get_dstOffset() { return m_struct.dstOffset; }
    void set_dstOffset(VkOffset3D inValue) { m_struct.dstOffset = inValue; }
    VkExtent3D get_extent() { return m_struct.extent; }
    void set_extent(VkExtent3D inValue) { m_struct.extent = inValue; }


private:
    VkImageCopy m_struct;
    const VkImageCopy* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkimagecreateinfo_struct_wrapper
{
public:
    vkimagecreateinfo_struct_wrapper();
    vkimagecreateinfo_struct_wrapper(VkImageCreateInfo* pInStruct);
    vkimagecreateinfo_struct_wrapper(const VkImageCreateInfo* pInStruct);

    virtual ~vkimagecreateinfo_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkImageCreateFlags get_flags() { return m_struct.flags; }
    void set_flags(VkImageCreateFlags inValue) { m_struct.flags = inValue; }
    VkImageType get_imageType() { return m_struct.imageType; }
    void set_imageType(VkImageType inValue) { m_struct.imageType = inValue; }
    VkFormat get_format() { return m_struct.format; }
    void set_format(VkFormat inValue) { m_struct.format = inValue; }
    VkExtent3D get_extent() { return m_struct.extent; }
    void set_extent(VkExtent3D inValue) { m_struct.extent = inValue; }
    uint32_t get_mipLevels() { return m_struct.mipLevels; }
    void set_mipLevels(uint32_t inValue) { m_struct.mipLevels = inValue; }
    uint32_t get_arrayLayers() { return m_struct.arrayLayers; }
    void set_arrayLayers(uint32_t inValue) { m_struct.arrayLayers = inValue; }
    VkSampleCountFlagBits get_samples() { return m_struct.samples; }
    void set_samples(VkSampleCountFlagBits inValue) { m_struct.samples = inValue; }
    VkImageTiling get_tiling() { return m_struct.tiling; }
    void set_tiling(VkImageTiling inValue) { m_struct.tiling = inValue; }
    VkImageUsageFlags get_usage() { return m_struct.usage; }
    void set_usage(VkImageUsageFlags inValue) { m_struct.usage = inValue; }
    VkSharingMode get_sharingMode() { return m_struct.sharingMode; }
    void set_sharingMode(VkSharingMode inValue) { m_struct.sharingMode = inValue; }
    uint32_t get_queueFamilyIndexCount() { return m_struct.queueFamilyIndexCount; }
    void set_queueFamilyIndexCount(uint32_t inValue) { m_struct.queueFamilyIndexCount = inValue; }
    VkImageLayout get_initialLayout() { return m_struct.initialLayout; }
    void set_initialLayout(VkImageLayout inValue) { m_struct.initialLayout = inValue; }


private:
    VkImageCreateInfo m_struct;
    const VkImageCreateInfo* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkimageformatproperties_struct_wrapper
{
public:
    vkimageformatproperties_struct_wrapper();
    vkimageformatproperties_struct_wrapper(VkImageFormatProperties* pInStruct);
    vkimageformatproperties_struct_wrapper(const VkImageFormatProperties* pInStruct);

    virtual ~vkimageformatproperties_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkExtent3D get_maxExtent() { return m_struct.maxExtent; }
    void set_maxExtent(VkExtent3D inValue) { m_struct.maxExtent = inValue; }
    uint32_t get_maxMipLevels() { return m_struct.maxMipLevels; }
    void set_maxMipLevels(uint32_t inValue) { m_struct.maxMipLevels = inValue; }
    uint32_t get_maxArrayLayers() { return m_struct.maxArrayLayers; }
    void set_maxArrayLayers(uint32_t inValue) { m_struct.maxArrayLayers = inValue; }
    VkSampleCountFlags get_sampleCounts() { return m_struct.sampleCounts; }
    void set_sampleCounts(VkSampleCountFlags inValue) { m_struct.sampleCounts = inValue; }
    VkDeviceSize get_maxResourceSize() { return m_struct.maxResourceSize; }
    void set_maxResourceSize(VkDeviceSize inValue) { m_struct.maxResourceSize = inValue; }


private:
    VkImageFormatProperties m_struct;
    const VkImageFormatProperties* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkimagememorybarrier_struct_wrapper
{
public:
    vkimagememorybarrier_struct_wrapper();
    vkimagememorybarrier_struct_wrapper(VkImageMemoryBarrier* pInStruct);
    vkimagememorybarrier_struct_wrapper(const VkImageMemoryBarrier* pInStruct);

    virtual ~vkimagememorybarrier_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkAccessFlags get_srcAccessMask() { return m_struct.srcAccessMask; }
    void set_srcAccessMask(VkAccessFlags inValue) { m_struct.srcAccessMask = inValue; }
    VkAccessFlags get_dstAccessMask() { return m_struct.dstAccessMask; }
    void set_dstAccessMask(VkAccessFlags inValue) { m_struct.dstAccessMask = inValue; }
    VkImageLayout get_oldLayout() { return m_struct.oldLayout; }
    void set_oldLayout(VkImageLayout inValue) { m_struct.oldLayout = inValue; }
    VkImageLayout get_newLayout() { return m_struct.newLayout; }
    void set_newLayout(VkImageLayout inValue) { m_struct.newLayout = inValue; }
    uint32_t get_srcQueueFamilyIndex() { return m_struct.srcQueueFamilyIndex; }
    void set_srcQueueFamilyIndex(uint32_t inValue) { m_struct.srcQueueFamilyIndex = inValue; }
    uint32_t get_dstQueueFamilyIndex() { return m_struct.dstQueueFamilyIndex; }
    void set_dstQueueFamilyIndex(uint32_t inValue) { m_struct.dstQueueFamilyIndex = inValue; }
    VkImage get_image() { return m_struct.image; }
    void set_image(VkImage inValue) { m_struct.image = inValue; }
    VkImageSubresourceRange get_subresourceRange() { return m_struct.subresourceRange; }
    void set_subresourceRange(VkImageSubresourceRange inValue) { m_struct.subresourceRange = inValue; }


private:
    VkImageMemoryBarrier m_struct;
    const VkImageMemoryBarrier* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkimageresolve_struct_wrapper
{
public:
    vkimageresolve_struct_wrapper();
    vkimageresolve_struct_wrapper(VkImageResolve* pInStruct);
    vkimageresolve_struct_wrapper(const VkImageResolve* pInStruct);

    virtual ~vkimageresolve_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkImageSubresourceLayers get_srcSubresource() { return m_struct.srcSubresource; }
    void set_srcSubresource(VkImageSubresourceLayers inValue) { m_struct.srcSubresource = inValue; }
    VkOffset3D get_srcOffset() { return m_struct.srcOffset; }
    void set_srcOffset(VkOffset3D inValue) { m_struct.srcOffset = inValue; }
    VkImageSubresourceLayers get_dstSubresource() { return m_struct.dstSubresource; }
    void set_dstSubresource(VkImageSubresourceLayers inValue) { m_struct.dstSubresource = inValue; }
    VkOffset3D get_dstOffset() { return m_struct.dstOffset; }
    void set_dstOffset(VkOffset3D inValue) { m_struct.dstOffset = inValue; }
    VkExtent3D get_extent() { return m_struct.extent; }
    void set_extent(VkExtent3D inValue) { m_struct.extent = inValue; }


private:
    VkImageResolve m_struct;
    const VkImageResolve* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkimagesubresource_struct_wrapper
{
public:
    vkimagesubresource_struct_wrapper();
    vkimagesubresource_struct_wrapper(VkImageSubresource* pInStruct);
    vkimagesubresource_struct_wrapper(const VkImageSubresource* pInStruct);

    virtual ~vkimagesubresource_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkImageAspectFlags get_aspectMask() { return m_struct.aspectMask; }
    void set_aspectMask(VkImageAspectFlags inValue) { m_struct.aspectMask = inValue; }
    uint32_t get_mipLevel() { return m_struct.mipLevel; }
    void set_mipLevel(uint32_t inValue) { m_struct.mipLevel = inValue; }
    uint32_t get_arrayLayer() { return m_struct.arrayLayer; }
    void set_arrayLayer(uint32_t inValue) { m_struct.arrayLayer = inValue; }


private:
    VkImageSubresource m_struct;
    const VkImageSubresource* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkimagesubresourcelayers_struct_wrapper
{
public:
    vkimagesubresourcelayers_struct_wrapper();
    vkimagesubresourcelayers_struct_wrapper(VkImageSubresourceLayers* pInStruct);
    vkimagesubresourcelayers_struct_wrapper(const VkImageSubresourceLayers* pInStruct);

    virtual ~vkimagesubresourcelayers_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkImageAspectFlags get_aspectMask() { return m_struct.aspectMask; }
    void set_aspectMask(VkImageAspectFlags inValue) { m_struct.aspectMask = inValue; }
    uint32_t get_mipLevel() { return m_struct.mipLevel; }
    void set_mipLevel(uint32_t inValue) { m_struct.mipLevel = inValue; }
    uint32_t get_baseArrayLayer() { return m_struct.baseArrayLayer; }
    void set_baseArrayLayer(uint32_t inValue) { m_struct.baseArrayLayer = inValue; }
    uint32_t get_layerCount() { return m_struct.layerCount; }
    void set_layerCount(uint32_t inValue) { m_struct.layerCount = inValue; }


private:
    VkImageSubresourceLayers m_struct;
    const VkImageSubresourceLayers* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkimagesubresourcerange_struct_wrapper
{
public:
    vkimagesubresourcerange_struct_wrapper();
    vkimagesubresourcerange_struct_wrapper(VkImageSubresourceRange* pInStruct);
    vkimagesubresourcerange_struct_wrapper(const VkImageSubresourceRange* pInStruct);

    virtual ~vkimagesubresourcerange_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkImageAspectFlags get_aspectMask() { return m_struct.aspectMask; }
    void set_aspectMask(VkImageAspectFlags inValue) { m_struct.aspectMask = inValue; }
    uint32_t get_baseMipLevel() { return m_struct.baseMipLevel; }
    void set_baseMipLevel(uint32_t inValue) { m_struct.baseMipLevel = inValue; }
    uint32_t get_levelCount() { return m_struct.levelCount; }
    void set_levelCount(uint32_t inValue) { m_struct.levelCount = inValue; }
    uint32_t get_baseArrayLayer() { return m_struct.baseArrayLayer; }
    void set_baseArrayLayer(uint32_t inValue) { m_struct.baseArrayLayer = inValue; }
    uint32_t get_layerCount() { return m_struct.layerCount; }
    void set_layerCount(uint32_t inValue) { m_struct.layerCount = inValue; }


private:
    VkImageSubresourceRange m_struct;
    const VkImageSubresourceRange* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkimageviewcreateinfo_struct_wrapper
{
public:
    vkimageviewcreateinfo_struct_wrapper();
    vkimageviewcreateinfo_struct_wrapper(VkImageViewCreateInfo* pInStruct);
    vkimageviewcreateinfo_struct_wrapper(const VkImageViewCreateInfo* pInStruct);

    virtual ~vkimageviewcreateinfo_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkImageViewCreateFlags get_flags() { return m_struct.flags; }
    void set_flags(VkImageViewCreateFlags inValue) { m_struct.flags = inValue; }
    VkImage get_image() { return m_struct.image; }
    void set_image(VkImage inValue) { m_struct.image = inValue; }
    VkImageViewType get_viewType() { return m_struct.viewType; }
    void set_viewType(VkImageViewType inValue) { m_struct.viewType = inValue; }
    VkFormat get_format() { return m_struct.format; }
    void set_format(VkFormat inValue) { m_struct.format = inValue; }
    VkComponentMapping get_components() { return m_struct.components; }
    void set_components(VkComponentMapping inValue) { m_struct.components = inValue; }
    VkImageSubresourceRange get_subresourceRange() { return m_struct.subresourceRange; }
    void set_subresourceRange(VkImageSubresourceRange inValue) { m_struct.subresourceRange = inValue; }


private:
    VkImageViewCreateInfo m_struct;
    const VkImageViewCreateInfo* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkimportmemorywin32handleinfonv_struct_wrapper
{
public:
    vkimportmemorywin32handleinfonv_struct_wrapper();
    vkimportmemorywin32handleinfonv_struct_wrapper(VkImportMemoryWin32HandleInfoNV* pInStruct);
    vkimportmemorywin32handleinfonv_struct_wrapper(const VkImportMemoryWin32HandleInfoNV* pInStruct);

    virtual ~vkimportmemorywin32handleinfonv_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkExternalMemoryHandleTypeFlagsNV get_handleType() { return m_struct.handleType; }
    void set_handleType(VkExternalMemoryHandleTypeFlagsNV inValue) { m_struct.handleType = inValue; }
    HANDLE get_handle() { return m_struct.handle; }
    void set_handle(HANDLE inValue) { m_struct.handle = inValue; }


private:
    VkImportMemoryWin32HandleInfoNV m_struct;
    const VkImportMemoryWin32HandleInfoNV* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkinstancecreateinfo_struct_wrapper
{
public:
    vkinstancecreateinfo_struct_wrapper();
    vkinstancecreateinfo_struct_wrapper(VkInstanceCreateInfo* pInStruct);
    vkinstancecreateinfo_struct_wrapper(const VkInstanceCreateInfo* pInStruct);

    virtual ~vkinstancecreateinfo_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkInstanceCreateFlags get_flags() { return m_struct.flags; }
    void set_flags(VkInstanceCreateFlags inValue) { m_struct.flags = inValue; }
    const VkApplicationInfo* get_pApplicationInfo() { return m_struct.pApplicationInfo; }
    uint32_t get_enabledLayerCount() { return m_struct.enabledLayerCount; }
    void set_enabledLayerCount(uint32_t inValue) { m_struct.enabledLayerCount = inValue; }
    uint32_t get_enabledExtensionCount() { return m_struct.enabledExtensionCount; }
    void set_enabledExtensionCount(uint32_t inValue) { m_struct.enabledExtensionCount = inValue; }


private:
    VkInstanceCreateInfo m_struct;
    const VkInstanceCreateInfo* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vklayerproperties_struct_wrapper
{
public:
    vklayerproperties_struct_wrapper();
    vklayerproperties_struct_wrapper(VkLayerProperties* pInStruct);
    vklayerproperties_struct_wrapper(const VkLayerProperties* pInStruct);

    virtual ~vklayerproperties_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    uint32_t get_specVersion() { return m_struct.specVersion; }
    void set_specVersion(uint32_t inValue) { m_struct.specVersion = inValue; }
    uint32_t get_implementationVersion() { return m_struct.implementationVersion; }
    void set_implementationVersion(uint32_t inValue) { m_struct.implementationVersion = inValue; }


private:
    VkLayerProperties m_struct;
    const VkLayerProperties* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkmappedmemoryrange_struct_wrapper
{
public:
    vkmappedmemoryrange_struct_wrapper();
    vkmappedmemoryrange_struct_wrapper(VkMappedMemoryRange* pInStruct);
    vkmappedmemoryrange_struct_wrapper(const VkMappedMemoryRange* pInStruct);

    virtual ~vkmappedmemoryrange_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkDeviceMemory get_memory() { return m_struct.memory; }
    void set_memory(VkDeviceMemory inValue) { m_struct.memory = inValue; }
    VkDeviceSize get_offset() { return m_struct.offset; }
    void set_offset(VkDeviceSize inValue) { m_struct.offset = inValue; }
    VkDeviceSize get_size() { return m_struct.size; }
    void set_size(VkDeviceSize inValue) { m_struct.size = inValue; }


private:
    VkMappedMemoryRange m_struct;
    const VkMappedMemoryRange* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkmemoryallocateinfo_struct_wrapper
{
public:
    vkmemoryallocateinfo_struct_wrapper();
    vkmemoryallocateinfo_struct_wrapper(VkMemoryAllocateInfo* pInStruct);
    vkmemoryallocateinfo_struct_wrapper(const VkMemoryAllocateInfo* pInStruct);

    virtual ~vkmemoryallocateinfo_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkDeviceSize get_allocationSize() { return m_struct.allocationSize; }
    void set_allocationSize(VkDeviceSize inValue) { m_struct.allocationSize = inValue; }
    uint32_t get_memoryTypeIndex() { return m_struct.memoryTypeIndex; }
    void set_memoryTypeIndex(uint32_t inValue) { m_struct.memoryTypeIndex = inValue; }


private:
    VkMemoryAllocateInfo m_struct;
    const VkMemoryAllocateInfo* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkmemorybarrier_struct_wrapper
{
public:
    vkmemorybarrier_struct_wrapper();
    vkmemorybarrier_struct_wrapper(VkMemoryBarrier* pInStruct);
    vkmemorybarrier_struct_wrapper(const VkMemoryBarrier* pInStruct);

    virtual ~vkmemorybarrier_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkAccessFlags get_srcAccessMask() { return m_struct.srcAccessMask; }
    void set_srcAccessMask(VkAccessFlags inValue) { m_struct.srcAccessMask = inValue; }
    VkAccessFlags get_dstAccessMask() { return m_struct.dstAccessMask; }
    void set_dstAccessMask(VkAccessFlags inValue) { m_struct.dstAccessMask = inValue; }


private:
    VkMemoryBarrier m_struct;
    const VkMemoryBarrier* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkmemoryheap_struct_wrapper
{
public:
    vkmemoryheap_struct_wrapper();
    vkmemoryheap_struct_wrapper(VkMemoryHeap* pInStruct);
    vkmemoryheap_struct_wrapper(const VkMemoryHeap* pInStruct);

    virtual ~vkmemoryheap_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkDeviceSize get_size() { return m_struct.size; }
    void set_size(VkDeviceSize inValue) { m_struct.size = inValue; }
    VkMemoryHeapFlags get_flags() { return m_struct.flags; }
    void set_flags(VkMemoryHeapFlags inValue) { m_struct.flags = inValue; }


private:
    VkMemoryHeap m_struct;
    const VkMemoryHeap* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkmemoryrequirements_struct_wrapper
{
public:
    vkmemoryrequirements_struct_wrapper();
    vkmemoryrequirements_struct_wrapper(VkMemoryRequirements* pInStruct);
    vkmemoryrequirements_struct_wrapper(const VkMemoryRequirements* pInStruct);

    virtual ~vkmemoryrequirements_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkDeviceSize get_size() { return m_struct.size; }
    void set_size(VkDeviceSize inValue) { m_struct.size = inValue; }
    VkDeviceSize get_alignment() { return m_struct.alignment; }
    void set_alignment(VkDeviceSize inValue) { m_struct.alignment = inValue; }
    uint32_t get_memoryTypeBits() { return m_struct.memoryTypeBits; }
    void set_memoryTypeBits(uint32_t inValue) { m_struct.memoryTypeBits = inValue; }


private:
    VkMemoryRequirements m_struct;
    const VkMemoryRequirements* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkmemorytype_struct_wrapper
{
public:
    vkmemorytype_struct_wrapper();
    vkmemorytype_struct_wrapper(VkMemoryType* pInStruct);
    vkmemorytype_struct_wrapper(const VkMemoryType* pInStruct);

    virtual ~vkmemorytype_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkMemoryPropertyFlags get_propertyFlags() { return m_struct.propertyFlags; }
    void set_propertyFlags(VkMemoryPropertyFlags inValue) { m_struct.propertyFlags = inValue; }
    uint32_t get_heapIndex() { return m_struct.heapIndex; }
    void set_heapIndex(uint32_t inValue) { m_struct.heapIndex = inValue; }


private:
    VkMemoryType m_struct;
    const VkMemoryType* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkmirsurfacecreateinfokhr_struct_wrapper
{
public:
    vkmirsurfacecreateinfokhr_struct_wrapper();
    vkmirsurfacecreateinfokhr_struct_wrapper(VkMirSurfaceCreateInfoKHR* pInStruct);
    vkmirsurfacecreateinfokhr_struct_wrapper(const VkMirSurfaceCreateInfoKHR* pInStruct);

    virtual ~vkmirsurfacecreateinfokhr_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkMirSurfaceCreateFlagsKHR get_flags() { return m_struct.flags; }
    void set_flags(VkMirSurfaceCreateFlagsKHR inValue) { m_struct.flags = inValue; }
    MirConnection* get_connection() { return m_struct.connection; }
    void set_connection(MirConnection* inValue) { m_struct.connection = inValue; }
    MirSurface* get_mirSurface() { return m_struct.mirSurface; }
    void set_mirSurface(MirSurface* inValue) { m_struct.mirSurface = inValue; }


private:
    VkMirSurfaceCreateInfoKHR m_struct;
    const VkMirSurfaceCreateInfoKHR* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkoffset2d_struct_wrapper
{
public:
    vkoffset2d_struct_wrapper();
    vkoffset2d_struct_wrapper(VkOffset2D* pInStruct);
    vkoffset2d_struct_wrapper(const VkOffset2D* pInStruct);

    virtual ~vkoffset2d_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    int32_t get_x() { return m_struct.x; }
    void set_x(int32_t inValue) { m_struct.x = inValue; }
    int32_t get_y() { return m_struct.y; }
    void set_y(int32_t inValue) { m_struct.y = inValue; }


private:
    VkOffset2D m_struct;
    const VkOffset2D* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkoffset3d_struct_wrapper
{
public:
    vkoffset3d_struct_wrapper();
    vkoffset3d_struct_wrapper(VkOffset3D* pInStruct);
    vkoffset3d_struct_wrapper(const VkOffset3D* pInStruct);

    virtual ~vkoffset3d_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    int32_t get_x() { return m_struct.x; }
    void set_x(int32_t inValue) { m_struct.x = inValue; }
    int32_t get_y() { return m_struct.y; }
    void set_y(int32_t inValue) { m_struct.y = inValue; }
    int32_t get_z() { return m_struct.z; }
    void set_z(int32_t inValue) { m_struct.z = inValue; }


private:
    VkOffset3D m_struct;
    const VkOffset3D* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkphysicaldevicefeatures_struct_wrapper
{
public:
    vkphysicaldevicefeatures_struct_wrapper();
    vkphysicaldevicefeatures_struct_wrapper(VkPhysicalDeviceFeatures* pInStruct);
    vkphysicaldevicefeatures_struct_wrapper(const VkPhysicalDeviceFeatures* pInStruct);

    virtual ~vkphysicaldevicefeatures_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkBool32 get_robustBufferAccess() { return m_struct.robustBufferAccess; }
    void set_robustBufferAccess(VkBool32 inValue) { m_struct.robustBufferAccess = inValue; }
    VkBool32 get_fullDrawIndexUint32() { return m_struct.fullDrawIndexUint32; }
    void set_fullDrawIndexUint32(VkBool32 inValue) { m_struct.fullDrawIndexUint32 = inValue; }
    VkBool32 get_imageCubeArray() { return m_struct.imageCubeArray; }
    void set_imageCubeArray(VkBool32 inValue) { m_struct.imageCubeArray = inValue; }
    VkBool32 get_independentBlend() { return m_struct.independentBlend; }
    void set_independentBlend(VkBool32 inValue) { m_struct.independentBlend = inValue; }
    VkBool32 get_geometryShader() { return m_struct.geometryShader; }
    void set_geometryShader(VkBool32 inValue) { m_struct.geometryShader = inValue; }
    VkBool32 get_tessellationShader() { return m_struct.tessellationShader; }
    void set_tessellationShader(VkBool32 inValue) { m_struct.tessellationShader = inValue; }
    VkBool32 get_sampleRateShading() { return m_struct.sampleRateShading; }
    void set_sampleRateShading(VkBool32 inValue) { m_struct.sampleRateShading = inValue; }
    VkBool32 get_dualSrcBlend() { return m_struct.dualSrcBlend; }
    void set_dualSrcBlend(VkBool32 inValue) { m_struct.dualSrcBlend = inValue; }
    VkBool32 get_logicOp() { return m_struct.logicOp; }
    void set_logicOp(VkBool32 inValue) { m_struct.logicOp = inValue; }
    VkBool32 get_multiDrawIndirect() { return m_struct.multiDrawIndirect; }
    void set_multiDrawIndirect(VkBool32 inValue) { m_struct.multiDrawIndirect = inValue; }
    VkBool32 get_drawIndirectFirstInstance() { return m_struct.drawIndirectFirstInstance; }
    void set_drawIndirectFirstInstance(VkBool32 inValue) { m_struct.drawIndirectFirstInstance = inValue; }
    VkBool32 get_depthClamp() { return m_struct.depthClamp; }
    void set_depthClamp(VkBool32 inValue) { m_struct.depthClamp = inValue; }
    VkBool32 get_depthBiasClamp() { return m_struct.depthBiasClamp; }
    void set_depthBiasClamp(VkBool32 inValue) { m_struct.depthBiasClamp = inValue; }
    VkBool32 get_fillModeNonSolid() { return m_struct.fillModeNonSolid; }
    void set_fillModeNonSolid(VkBool32 inValue) { m_struct.fillModeNonSolid = inValue; }
    VkBool32 get_depthBounds() { return m_struct.depthBounds; }
    void set_depthBounds(VkBool32 inValue) { m_struct.depthBounds = inValue; }
    VkBool32 get_wideLines() { return m_struct.wideLines; }
    void set_wideLines(VkBool32 inValue) { m_struct.wideLines = inValue; }
    VkBool32 get_largePoints() { return m_struct.largePoints; }
    void set_largePoints(VkBool32 inValue) { m_struct.largePoints = inValue; }
    VkBool32 get_alphaToOne() { return m_struct.alphaToOne; }
    void set_alphaToOne(VkBool32 inValue) { m_struct.alphaToOne = inValue; }
    VkBool32 get_multiViewport() { return m_struct.multiViewport; }
    void set_multiViewport(VkBool32 inValue) { m_struct.multiViewport = inValue; }
    VkBool32 get_samplerAnisotropy() { return m_struct.samplerAnisotropy; }
    void set_samplerAnisotropy(VkBool32 inValue) { m_struct.samplerAnisotropy = inValue; }
    VkBool32 get_textureCompressionETC2() { return m_struct.textureCompressionETC2; }
    void set_textureCompressionETC2(VkBool32 inValue) { m_struct.textureCompressionETC2 = inValue; }
    VkBool32 get_textureCompressionASTC_LDR() { return m_struct.textureCompressionASTC_LDR; }
    void set_textureCompressionASTC_LDR(VkBool32 inValue) { m_struct.textureCompressionASTC_LDR = inValue; }
    VkBool32 get_textureCompressionBC() { return m_struct.textureCompressionBC; }
    void set_textureCompressionBC(VkBool32 inValue) { m_struct.textureCompressionBC = inValue; }
    VkBool32 get_occlusionQueryPrecise() { return m_struct.occlusionQueryPrecise; }
    void set_occlusionQueryPrecise(VkBool32 inValue) { m_struct.occlusionQueryPrecise = inValue; }
    VkBool32 get_pipelineStatisticsQuery() { return m_struct.pipelineStatisticsQuery; }
    void set_pipelineStatisticsQuery(VkBool32 inValue) { m_struct.pipelineStatisticsQuery = inValue; }
    VkBool32 get_vertexPipelineStoresAndAtomics() { return m_struct.vertexPipelineStoresAndAtomics; }
    void set_vertexPipelineStoresAndAtomics(VkBool32 inValue) { m_struct.vertexPipelineStoresAndAtomics = inValue; }
    VkBool32 get_fragmentStoresAndAtomics() { return m_struct.fragmentStoresAndAtomics; }
    void set_fragmentStoresAndAtomics(VkBool32 inValue) { m_struct.fragmentStoresAndAtomics = inValue; }
    VkBool32 get_shaderTessellationAndGeometryPointSize() { return m_struct.shaderTessellationAndGeometryPointSize; }
    void set_shaderTessellationAndGeometryPointSize(VkBool32 inValue) { m_struct.shaderTessellationAndGeometryPointSize = inValue; }
    VkBool32 get_shaderImageGatherExtended() { return m_struct.shaderImageGatherExtended; }
    void set_shaderImageGatherExtended(VkBool32 inValue) { m_struct.shaderImageGatherExtended = inValue; }
    VkBool32 get_shaderStorageImageExtendedFormats() { return m_struct.shaderStorageImageExtendedFormats; }
    void set_shaderStorageImageExtendedFormats(VkBool32 inValue) { m_struct.shaderStorageImageExtendedFormats = inValue; }
    VkBool32 get_shaderStorageImageMultisample() { return m_struct.shaderStorageImageMultisample; }
    void set_shaderStorageImageMultisample(VkBool32 inValue) { m_struct.shaderStorageImageMultisample = inValue; }
    VkBool32 get_shaderStorageImageReadWithoutFormat() { return m_struct.shaderStorageImageReadWithoutFormat; }
    void set_shaderStorageImageReadWithoutFormat(VkBool32 inValue) { m_struct.shaderStorageImageReadWithoutFormat = inValue; }
    VkBool32 get_shaderStorageImageWriteWithoutFormat() { return m_struct.shaderStorageImageWriteWithoutFormat; }
    void set_shaderStorageImageWriteWithoutFormat(VkBool32 inValue) { m_struct.shaderStorageImageWriteWithoutFormat = inValue; }
    VkBool32 get_shaderUniformBufferArrayDynamicIndexing() { return m_struct.shaderUniformBufferArrayDynamicIndexing; }
    void set_shaderUniformBufferArrayDynamicIndexing(VkBool32 inValue) { m_struct.shaderUniformBufferArrayDynamicIndexing = inValue; }
    VkBool32 get_shaderSampledImageArrayDynamicIndexing() { return m_struct.shaderSampledImageArrayDynamicIndexing; }
    void set_shaderSampledImageArrayDynamicIndexing(VkBool32 inValue) { m_struct.shaderSampledImageArrayDynamicIndexing = inValue; }
    VkBool32 get_shaderStorageBufferArrayDynamicIndexing() { return m_struct.shaderStorageBufferArrayDynamicIndexing; }
    void set_shaderStorageBufferArrayDynamicIndexing(VkBool32 inValue) { m_struct.shaderStorageBufferArrayDynamicIndexing = inValue; }
    VkBool32 get_shaderStorageImageArrayDynamicIndexing() { return m_struct.shaderStorageImageArrayDynamicIndexing; }
    void set_shaderStorageImageArrayDynamicIndexing(VkBool32 inValue) { m_struct.shaderStorageImageArrayDynamicIndexing = inValue; }
    VkBool32 get_shaderClipDistance() { return m_struct.shaderClipDistance; }
    void set_shaderClipDistance(VkBool32 inValue) { m_struct.shaderClipDistance = inValue; }
    VkBool32 get_shaderCullDistance() { return m_struct.shaderCullDistance; }
    void set_shaderCullDistance(VkBool32 inValue) { m_struct.shaderCullDistance = inValue; }
    VkBool32 get_shaderFloat64() { return m_struct.shaderFloat64; }
    void set_shaderFloat64(VkBool32 inValue) { m_struct.shaderFloat64 = inValue; }
    VkBool32 get_shaderInt64() { return m_struct.shaderInt64; }
    void set_shaderInt64(VkBool32 inValue) { m_struct.shaderInt64 = inValue; }
    VkBool32 get_shaderInt16() { return m_struct.shaderInt16; }
    void set_shaderInt16(VkBool32 inValue) { m_struct.shaderInt16 = inValue; }
    VkBool32 get_shaderResourceResidency() { return m_struct.shaderResourceResidency; }
    void set_shaderResourceResidency(VkBool32 inValue) { m_struct.shaderResourceResidency = inValue; }
    VkBool32 get_shaderResourceMinLod() { return m_struct.shaderResourceMinLod; }
    void set_shaderResourceMinLod(VkBool32 inValue) { m_struct.shaderResourceMinLod = inValue; }
    VkBool32 get_sparseBinding() { return m_struct.sparseBinding; }
    void set_sparseBinding(VkBool32 inValue) { m_struct.sparseBinding = inValue; }
    VkBool32 get_sparseResidencyBuffer() { return m_struct.sparseResidencyBuffer; }
    void set_sparseResidencyBuffer(VkBool32 inValue) { m_struct.sparseResidencyBuffer = inValue; }
    VkBool32 get_sparseResidencyImage2D() { return m_struct.sparseResidencyImage2D; }
    void set_sparseResidencyImage2D(VkBool32 inValue) { m_struct.sparseResidencyImage2D = inValue; }
    VkBool32 get_sparseResidencyImage3D() { return m_struct.sparseResidencyImage3D; }
    void set_sparseResidencyImage3D(VkBool32 inValue) { m_struct.sparseResidencyImage3D = inValue; }
    VkBool32 get_sparseResidency2Samples() { return m_struct.sparseResidency2Samples; }
    void set_sparseResidency2Samples(VkBool32 inValue) { m_struct.sparseResidency2Samples = inValue; }
    VkBool32 get_sparseResidency4Samples() { return m_struct.sparseResidency4Samples; }
    void set_sparseResidency4Samples(VkBool32 inValue) { m_struct.sparseResidency4Samples = inValue; }
    VkBool32 get_sparseResidency8Samples() { return m_struct.sparseResidency8Samples; }
    void set_sparseResidency8Samples(VkBool32 inValue) { m_struct.sparseResidency8Samples = inValue; }
    VkBool32 get_sparseResidency16Samples() { return m_struct.sparseResidency16Samples; }
    void set_sparseResidency16Samples(VkBool32 inValue) { m_struct.sparseResidency16Samples = inValue; }
    VkBool32 get_sparseResidencyAliased() { return m_struct.sparseResidencyAliased; }
    void set_sparseResidencyAliased(VkBool32 inValue) { m_struct.sparseResidencyAliased = inValue; }
    VkBool32 get_variableMultisampleRate() { return m_struct.variableMultisampleRate; }
    void set_variableMultisampleRate(VkBool32 inValue) { m_struct.variableMultisampleRate = inValue; }
    VkBool32 get_inheritedQueries() { return m_struct.inheritedQueries; }
    void set_inheritedQueries(VkBool32 inValue) { m_struct.inheritedQueries = inValue; }


private:
    VkPhysicalDeviceFeatures m_struct;
    const VkPhysicalDeviceFeatures* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkphysicaldevicelimits_struct_wrapper
{
public:
    vkphysicaldevicelimits_struct_wrapper();
    vkphysicaldevicelimits_struct_wrapper(VkPhysicalDeviceLimits* pInStruct);
    vkphysicaldevicelimits_struct_wrapper(const VkPhysicalDeviceLimits* pInStruct);

    virtual ~vkphysicaldevicelimits_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    uint32_t get_maxImageDimension1D() { return m_struct.maxImageDimension1D; }
    void set_maxImageDimension1D(uint32_t inValue) { m_struct.maxImageDimension1D = inValue; }
    uint32_t get_maxImageDimension2D() { return m_struct.maxImageDimension2D; }
    void set_maxImageDimension2D(uint32_t inValue) { m_struct.maxImageDimension2D = inValue; }
    uint32_t get_maxImageDimension3D() { return m_struct.maxImageDimension3D; }
    void set_maxImageDimension3D(uint32_t inValue) { m_struct.maxImageDimension3D = inValue; }
    uint32_t get_maxImageDimensionCube() { return m_struct.maxImageDimensionCube; }
    void set_maxImageDimensionCube(uint32_t inValue) { m_struct.maxImageDimensionCube = inValue; }
    uint32_t get_maxImageArrayLayers() { return m_struct.maxImageArrayLayers; }
    void set_maxImageArrayLayers(uint32_t inValue) { m_struct.maxImageArrayLayers = inValue; }
    uint32_t get_maxTexelBufferElements() { return m_struct.maxTexelBufferElements; }
    void set_maxTexelBufferElements(uint32_t inValue) { m_struct.maxTexelBufferElements = inValue; }
    uint32_t get_maxUniformBufferRange() { return m_struct.maxUniformBufferRange; }
    void set_maxUniformBufferRange(uint32_t inValue) { m_struct.maxUniformBufferRange = inValue; }
    uint32_t get_maxStorageBufferRange() { return m_struct.maxStorageBufferRange; }
    void set_maxStorageBufferRange(uint32_t inValue) { m_struct.maxStorageBufferRange = inValue; }
    uint32_t get_maxPushConstantsSize() { return m_struct.maxPushConstantsSize; }
    void set_maxPushConstantsSize(uint32_t inValue) { m_struct.maxPushConstantsSize = inValue; }
    uint32_t get_maxMemoryAllocationCount() { return m_struct.maxMemoryAllocationCount; }
    void set_maxMemoryAllocationCount(uint32_t inValue) { m_struct.maxMemoryAllocationCount = inValue; }
    uint32_t get_maxSamplerAllocationCount() { return m_struct.maxSamplerAllocationCount; }
    void set_maxSamplerAllocationCount(uint32_t inValue) { m_struct.maxSamplerAllocationCount = inValue; }
    VkDeviceSize get_bufferImageGranularity() { return m_struct.bufferImageGranularity; }
    void set_bufferImageGranularity(VkDeviceSize inValue) { m_struct.bufferImageGranularity = inValue; }
    VkDeviceSize get_sparseAddressSpaceSize() { return m_struct.sparseAddressSpaceSize; }
    void set_sparseAddressSpaceSize(VkDeviceSize inValue) { m_struct.sparseAddressSpaceSize = inValue; }
    uint32_t get_maxBoundDescriptorSets() { return m_struct.maxBoundDescriptorSets; }
    void set_maxBoundDescriptorSets(uint32_t inValue) { m_struct.maxBoundDescriptorSets = inValue; }
    uint32_t get_maxPerStageDescriptorSamplers() { return m_struct.maxPerStageDescriptorSamplers; }
    void set_maxPerStageDescriptorSamplers(uint32_t inValue) { m_struct.maxPerStageDescriptorSamplers = inValue; }
    uint32_t get_maxPerStageDescriptorUniformBuffers() { return m_struct.maxPerStageDescriptorUniformBuffers; }
    void set_maxPerStageDescriptorUniformBuffers(uint32_t inValue) { m_struct.maxPerStageDescriptorUniformBuffers = inValue; }
    uint32_t get_maxPerStageDescriptorStorageBuffers() { return m_struct.maxPerStageDescriptorStorageBuffers; }
    void set_maxPerStageDescriptorStorageBuffers(uint32_t inValue) { m_struct.maxPerStageDescriptorStorageBuffers = inValue; }
    uint32_t get_maxPerStageDescriptorSampledImages() { return m_struct.maxPerStageDescriptorSampledImages; }
    void set_maxPerStageDescriptorSampledImages(uint32_t inValue) { m_struct.maxPerStageDescriptorSampledImages = inValue; }
    uint32_t get_maxPerStageDescriptorStorageImages() { return m_struct.maxPerStageDescriptorStorageImages; }
    void set_maxPerStageDescriptorStorageImages(uint32_t inValue) { m_struct.maxPerStageDescriptorStorageImages = inValue; }
    uint32_t get_maxPerStageDescriptorInputAttachments() { return m_struct.maxPerStageDescriptorInputAttachments; }
    void set_maxPerStageDescriptorInputAttachments(uint32_t inValue) { m_struct.maxPerStageDescriptorInputAttachments = inValue; }
    uint32_t get_maxPerStageResources() { return m_struct.maxPerStageResources; }
    void set_maxPerStageResources(uint32_t inValue) { m_struct.maxPerStageResources = inValue; }
    uint32_t get_maxDescriptorSetSamplers() { return m_struct.maxDescriptorSetSamplers; }
    void set_maxDescriptorSetSamplers(uint32_t inValue) { m_struct.maxDescriptorSetSamplers = inValue; }
    uint32_t get_maxDescriptorSetUniformBuffers() { return m_struct.maxDescriptorSetUniformBuffers; }
    void set_maxDescriptorSetUniformBuffers(uint32_t inValue) { m_struct.maxDescriptorSetUniformBuffers = inValue; }
    uint32_t get_maxDescriptorSetUniformBuffersDynamic() { return m_struct.maxDescriptorSetUniformBuffersDynamic; }
    void set_maxDescriptorSetUniformBuffersDynamic(uint32_t inValue) { m_struct.maxDescriptorSetUniformBuffersDynamic = inValue; }
    uint32_t get_maxDescriptorSetStorageBuffers() { return m_struct.maxDescriptorSetStorageBuffers; }
    void set_maxDescriptorSetStorageBuffers(uint32_t inValue) { m_struct.maxDescriptorSetStorageBuffers = inValue; }
    uint32_t get_maxDescriptorSetStorageBuffersDynamic() { return m_struct.maxDescriptorSetStorageBuffersDynamic; }
    void set_maxDescriptorSetStorageBuffersDynamic(uint32_t inValue) { m_struct.maxDescriptorSetStorageBuffersDynamic = inValue; }
    uint32_t get_maxDescriptorSetSampledImages() { return m_struct.maxDescriptorSetSampledImages; }
    void set_maxDescriptorSetSampledImages(uint32_t inValue) { m_struct.maxDescriptorSetSampledImages = inValue; }
    uint32_t get_maxDescriptorSetStorageImages() { return m_struct.maxDescriptorSetStorageImages; }
    void set_maxDescriptorSetStorageImages(uint32_t inValue) { m_struct.maxDescriptorSetStorageImages = inValue; }
    uint32_t get_maxDescriptorSetInputAttachments() { return m_struct.maxDescriptorSetInputAttachments; }
    void set_maxDescriptorSetInputAttachments(uint32_t inValue) { m_struct.maxDescriptorSetInputAttachments = inValue; }
    uint32_t get_maxVertexInputAttributes() { return m_struct.maxVertexInputAttributes; }
    void set_maxVertexInputAttributes(uint32_t inValue) { m_struct.maxVertexInputAttributes = inValue; }
    uint32_t get_maxVertexInputBindings() { return m_struct.maxVertexInputBindings; }
    void set_maxVertexInputBindings(uint32_t inValue) { m_struct.maxVertexInputBindings = inValue; }
    uint32_t get_maxVertexInputAttributeOffset() { return m_struct.maxVertexInputAttributeOffset; }
    void set_maxVertexInputAttributeOffset(uint32_t inValue) { m_struct.maxVertexInputAttributeOffset = inValue; }
    uint32_t get_maxVertexInputBindingStride() { return m_struct.maxVertexInputBindingStride; }
    void set_maxVertexInputBindingStride(uint32_t inValue) { m_struct.maxVertexInputBindingStride = inValue; }
    uint32_t get_maxVertexOutputComponents() { return m_struct.maxVertexOutputComponents; }
    void set_maxVertexOutputComponents(uint32_t inValue) { m_struct.maxVertexOutputComponents = inValue; }
    uint32_t get_maxTessellationGenerationLevel() { return m_struct.maxTessellationGenerationLevel; }
    void set_maxTessellationGenerationLevel(uint32_t inValue) { m_struct.maxTessellationGenerationLevel = inValue; }
    uint32_t get_maxTessellationPatchSize() { return m_struct.maxTessellationPatchSize; }
    void set_maxTessellationPatchSize(uint32_t inValue) { m_struct.maxTessellationPatchSize = inValue; }
    uint32_t get_maxTessellationControlPerVertexInputComponents() { return m_struct.maxTessellationControlPerVertexInputComponents; }
    void set_maxTessellationControlPerVertexInputComponents(uint32_t inValue) { m_struct.maxTessellationControlPerVertexInputComponents = inValue; }
    uint32_t get_maxTessellationControlPerVertexOutputComponents() { return m_struct.maxTessellationControlPerVertexOutputComponents; }
    void set_maxTessellationControlPerVertexOutputComponents(uint32_t inValue) { m_struct.maxTessellationControlPerVertexOutputComponents = inValue; }
    uint32_t get_maxTessellationControlPerPatchOutputComponents() { return m_struct.maxTessellationControlPerPatchOutputComponents; }
    void set_maxTessellationControlPerPatchOutputComponents(uint32_t inValue) { m_struct.maxTessellationControlPerPatchOutputComponents = inValue; }
    uint32_t get_maxTessellationControlTotalOutputComponents() { return m_struct.maxTessellationControlTotalOutputComponents; }
    void set_maxTessellationControlTotalOutputComponents(uint32_t inValue) { m_struct.maxTessellationControlTotalOutputComponents = inValue; }
    uint32_t get_maxTessellationEvaluationInputComponents() { return m_struct.maxTessellationEvaluationInputComponents; }
    void set_maxTessellationEvaluationInputComponents(uint32_t inValue) { m_struct.maxTessellationEvaluationInputComponents = inValue; }
    uint32_t get_maxTessellationEvaluationOutputComponents() { return m_struct.maxTessellationEvaluationOutputComponents; }
    void set_maxTessellationEvaluationOutputComponents(uint32_t inValue) { m_struct.maxTessellationEvaluationOutputComponents = inValue; }
    uint32_t get_maxGeometryShaderInvocations() { return m_struct.maxGeometryShaderInvocations; }
    void set_maxGeometryShaderInvocations(uint32_t inValue) { m_struct.maxGeometryShaderInvocations = inValue; }
    uint32_t get_maxGeometryInputComponents() { return m_struct.maxGeometryInputComponents; }
    void set_maxGeometryInputComponents(uint32_t inValue) { m_struct.maxGeometryInputComponents = inValue; }
    uint32_t get_maxGeometryOutputComponents() { return m_struct.maxGeometryOutputComponents; }
    void set_maxGeometryOutputComponents(uint32_t inValue) { m_struct.maxGeometryOutputComponents = inValue; }
    uint32_t get_maxGeometryOutputVertices() { return m_struct.maxGeometryOutputVertices; }
    void set_maxGeometryOutputVertices(uint32_t inValue) { m_struct.maxGeometryOutputVertices = inValue; }
    uint32_t get_maxGeometryTotalOutputComponents() { return m_struct.maxGeometryTotalOutputComponents; }
    void set_maxGeometryTotalOutputComponents(uint32_t inValue) { m_struct.maxGeometryTotalOutputComponents = inValue; }
    uint32_t get_maxFragmentInputComponents() { return m_struct.maxFragmentInputComponents; }
    void set_maxFragmentInputComponents(uint32_t inValue) { m_struct.maxFragmentInputComponents = inValue; }
    uint32_t get_maxFragmentOutputAttachments() { return m_struct.maxFragmentOutputAttachments; }
    void set_maxFragmentOutputAttachments(uint32_t inValue) { m_struct.maxFragmentOutputAttachments = inValue; }
    uint32_t get_maxFragmentDualSrcAttachments() { return m_struct.maxFragmentDualSrcAttachments; }
    void set_maxFragmentDualSrcAttachments(uint32_t inValue) { m_struct.maxFragmentDualSrcAttachments = inValue; }
    uint32_t get_maxFragmentCombinedOutputResources() { return m_struct.maxFragmentCombinedOutputResources; }
    void set_maxFragmentCombinedOutputResources(uint32_t inValue) { m_struct.maxFragmentCombinedOutputResources = inValue; }
    uint32_t get_maxComputeSharedMemorySize() { return m_struct.maxComputeSharedMemorySize; }
    void set_maxComputeSharedMemorySize(uint32_t inValue) { m_struct.maxComputeSharedMemorySize = inValue; }
    uint32_t get_maxComputeWorkGroupInvocations() { return m_struct.maxComputeWorkGroupInvocations; }
    void set_maxComputeWorkGroupInvocations(uint32_t inValue) { m_struct.maxComputeWorkGroupInvocations = inValue; }
    uint32_t get_subPixelPrecisionBits() { return m_struct.subPixelPrecisionBits; }
    void set_subPixelPrecisionBits(uint32_t inValue) { m_struct.subPixelPrecisionBits = inValue; }
    uint32_t get_subTexelPrecisionBits() { return m_struct.subTexelPrecisionBits; }
    void set_subTexelPrecisionBits(uint32_t inValue) { m_struct.subTexelPrecisionBits = inValue; }
    uint32_t get_mipmapPrecisionBits() { return m_struct.mipmapPrecisionBits; }
    void set_mipmapPrecisionBits(uint32_t inValue) { m_struct.mipmapPrecisionBits = inValue; }
    uint32_t get_maxDrawIndexedIndexValue() { return m_struct.maxDrawIndexedIndexValue; }
    void set_maxDrawIndexedIndexValue(uint32_t inValue) { m_struct.maxDrawIndexedIndexValue = inValue; }
    uint32_t get_maxDrawIndirectCount() { return m_struct.maxDrawIndirectCount; }
    void set_maxDrawIndirectCount(uint32_t inValue) { m_struct.maxDrawIndirectCount = inValue; }
    float get_maxSamplerLodBias() { return m_struct.maxSamplerLodBias; }
    void set_maxSamplerLodBias(float inValue) { m_struct.maxSamplerLodBias = inValue; }
    float get_maxSamplerAnisotropy() { return m_struct.maxSamplerAnisotropy; }
    void set_maxSamplerAnisotropy(float inValue) { m_struct.maxSamplerAnisotropy = inValue; }
    uint32_t get_maxViewports() { return m_struct.maxViewports; }
    void set_maxViewports(uint32_t inValue) { m_struct.maxViewports = inValue; }
    uint32_t get_viewportSubPixelBits() { return m_struct.viewportSubPixelBits; }
    void set_viewportSubPixelBits(uint32_t inValue) { m_struct.viewportSubPixelBits = inValue; }
    size_t get_minMemoryMapAlignment() { return m_struct.minMemoryMapAlignment; }
    void set_minMemoryMapAlignment(size_t inValue) { m_struct.minMemoryMapAlignment = inValue; }
    VkDeviceSize get_minTexelBufferOffsetAlignment() { return m_struct.minTexelBufferOffsetAlignment; }
    void set_minTexelBufferOffsetAlignment(VkDeviceSize inValue) { m_struct.minTexelBufferOffsetAlignment = inValue; }
    VkDeviceSize get_minUniformBufferOffsetAlignment() { return m_struct.minUniformBufferOffsetAlignment; }
    void set_minUniformBufferOffsetAlignment(VkDeviceSize inValue) { m_struct.minUniformBufferOffsetAlignment = inValue; }
    VkDeviceSize get_minStorageBufferOffsetAlignment() { return m_struct.minStorageBufferOffsetAlignment; }
    void set_minStorageBufferOffsetAlignment(VkDeviceSize inValue) { m_struct.minStorageBufferOffsetAlignment = inValue; }
    int32_t get_minTexelOffset() { return m_struct.minTexelOffset; }
    void set_minTexelOffset(int32_t inValue) { m_struct.minTexelOffset = inValue; }
    uint32_t get_maxTexelOffset() { return m_struct.maxTexelOffset; }
    void set_maxTexelOffset(uint32_t inValue) { m_struct.maxTexelOffset = inValue; }
    int32_t get_minTexelGatherOffset() { return m_struct.minTexelGatherOffset; }
    void set_minTexelGatherOffset(int32_t inValue) { m_struct.minTexelGatherOffset = inValue; }
    uint32_t get_maxTexelGatherOffset() { return m_struct.maxTexelGatherOffset; }
    void set_maxTexelGatherOffset(uint32_t inValue) { m_struct.maxTexelGatherOffset = inValue; }
    float get_minInterpolationOffset() { return m_struct.minInterpolationOffset; }
    void set_minInterpolationOffset(float inValue) { m_struct.minInterpolationOffset = inValue; }
    float get_maxInterpolationOffset() { return m_struct.maxInterpolationOffset; }
    void set_maxInterpolationOffset(float inValue) { m_struct.maxInterpolationOffset = inValue; }
    uint32_t get_subPixelInterpolationOffsetBits() { return m_struct.subPixelInterpolationOffsetBits; }
    void set_subPixelInterpolationOffsetBits(uint32_t inValue) { m_struct.subPixelInterpolationOffsetBits = inValue; }
    uint32_t get_maxFramebufferWidth() { return m_struct.maxFramebufferWidth; }
    void set_maxFramebufferWidth(uint32_t inValue) { m_struct.maxFramebufferWidth = inValue; }
    uint32_t get_maxFramebufferHeight() { return m_struct.maxFramebufferHeight; }
    void set_maxFramebufferHeight(uint32_t inValue) { m_struct.maxFramebufferHeight = inValue; }
    uint32_t get_maxFramebufferLayers() { return m_struct.maxFramebufferLayers; }
    void set_maxFramebufferLayers(uint32_t inValue) { m_struct.maxFramebufferLayers = inValue; }
    VkSampleCountFlags get_framebufferColorSampleCounts() { return m_struct.framebufferColorSampleCounts; }
    void set_framebufferColorSampleCounts(VkSampleCountFlags inValue) { m_struct.framebufferColorSampleCounts = inValue; }
    VkSampleCountFlags get_framebufferDepthSampleCounts() { return m_struct.framebufferDepthSampleCounts; }
    void set_framebufferDepthSampleCounts(VkSampleCountFlags inValue) { m_struct.framebufferDepthSampleCounts = inValue; }
    VkSampleCountFlags get_framebufferStencilSampleCounts() { return m_struct.framebufferStencilSampleCounts; }
    void set_framebufferStencilSampleCounts(VkSampleCountFlags inValue) { m_struct.framebufferStencilSampleCounts = inValue; }
    VkSampleCountFlags get_framebufferNoAttachmentsSampleCounts() { return m_struct.framebufferNoAttachmentsSampleCounts; }
    void set_framebufferNoAttachmentsSampleCounts(VkSampleCountFlags inValue) { m_struct.framebufferNoAttachmentsSampleCounts = inValue; }
    uint32_t get_maxColorAttachments() { return m_struct.maxColorAttachments; }
    void set_maxColorAttachments(uint32_t inValue) { m_struct.maxColorAttachments = inValue; }
    VkSampleCountFlags get_sampledImageColorSampleCounts() { return m_struct.sampledImageColorSampleCounts; }
    void set_sampledImageColorSampleCounts(VkSampleCountFlags inValue) { m_struct.sampledImageColorSampleCounts = inValue; }
    VkSampleCountFlags get_sampledImageIntegerSampleCounts() { return m_struct.sampledImageIntegerSampleCounts; }
    void set_sampledImageIntegerSampleCounts(VkSampleCountFlags inValue) { m_struct.sampledImageIntegerSampleCounts = inValue; }
    VkSampleCountFlags get_sampledImageDepthSampleCounts() { return m_struct.sampledImageDepthSampleCounts; }
    void set_sampledImageDepthSampleCounts(VkSampleCountFlags inValue) { m_struct.sampledImageDepthSampleCounts = inValue; }
    VkSampleCountFlags get_sampledImageStencilSampleCounts() { return m_struct.sampledImageStencilSampleCounts; }
    void set_sampledImageStencilSampleCounts(VkSampleCountFlags inValue) { m_struct.sampledImageStencilSampleCounts = inValue; }
    VkSampleCountFlags get_storageImageSampleCounts() { return m_struct.storageImageSampleCounts; }
    void set_storageImageSampleCounts(VkSampleCountFlags inValue) { m_struct.storageImageSampleCounts = inValue; }
    uint32_t get_maxSampleMaskWords() { return m_struct.maxSampleMaskWords; }
    void set_maxSampleMaskWords(uint32_t inValue) { m_struct.maxSampleMaskWords = inValue; }
    VkBool32 get_timestampComputeAndGraphics() { return m_struct.timestampComputeAndGraphics; }
    void set_timestampComputeAndGraphics(VkBool32 inValue) { m_struct.timestampComputeAndGraphics = inValue; }
    float get_timestampPeriod() { return m_struct.timestampPeriod; }
    void set_timestampPeriod(float inValue) { m_struct.timestampPeriod = inValue; }
    uint32_t get_maxClipDistances() { return m_struct.maxClipDistances; }
    void set_maxClipDistances(uint32_t inValue) { m_struct.maxClipDistances = inValue; }
    uint32_t get_maxCullDistances() { return m_struct.maxCullDistances; }
    void set_maxCullDistances(uint32_t inValue) { m_struct.maxCullDistances = inValue; }
    uint32_t get_maxCombinedClipAndCullDistances() { return m_struct.maxCombinedClipAndCullDistances; }
    void set_maxCombinedClipAndCullDistances(uint32_t inValue) { m_struct.maxCombinedClipAndCullDistances = inValue; }
    uint32_t get_discreteQueuePriorities() { return m_struct.discreteQueuePriorities; }
    void set_discreteQueuePriorities(uint32_t inValue) { m_struct.discreteQueuePriorities = inValue; }
    float get_pointSizeGranularity() { return m_struct.pointSizeGranularity; }
    void set_pointSizeGranularity(float inValue) { m_struct.pointSizeGranularity = inValue; }
    float get_lineWidthGranularity() { return m_struct.lineWidthGranularity; }
    void set_lineWidthGranularity(float inValue) { m_struct.lineWidthGranularity = inValue; }
    VkBool32 get_strictLines() { return m_struct.strictLines; }
    void set_strictLines(VkBool32 inValue) { m_struct.strictLines = inValue; }
    VkBool32 get_standardSampleLocations() { return m_struct.standardSampleLocations; }
    void set_standardSampleLocations(VkBool32 inValue) { m_struct.standardSampleLocations = inValue; }
    VkDeviceSize get_optimalBufferCopyOffsetAlignment() { return m_struct.optimalBufferCopyOffsetAlignment; }
    void set_optimalBufferCopyOffsetAlignment(VkDeviceSize inValue) { m_struct.optimalBufferCopyOffsetAlignment = inValue; }
    VkDeviceSize get_optimalBufferCopyRowPitchAlignment() { return m_struct.optimalBufferCopyRowPitchAlignment; }
    void set_optimalBufferCopyRowPitchAlignment(VkDeviceSize inValue) { m_struct.optimalBufferCopyRowPitchAlignment = inValue; }
    VkDeviceSize get_nonCoherentAtomSize() { return m_struct.nonCoherentAtomSize; }
    void set_nonCoherentAtomSize(VkDeviceSize inValue) { m_struct.nonCoherentAtomSize = inValue; }


private:
    VkPhysicalDeviceLimits m_struct;
    const VkPhysicalDeviceLimits* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkphysicaldevicememoryproperties_struct_wrapper
{
public:
    vkphysicaldevicememoryproperties_struct_wrapper();
    vkphysicaldevicememoryproperties_struct_wrapper(VkPhysicalDeviceMemoryProperties* pInStruct);
    vkphysicaldevicememoryproperties_struct_wrapper(const VkPhysicalDeviceMemoryProperties* pInStruct);

    virtual ~vkphysicaldevicememoryproperties_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    uint32_t get_memoryTypeCount() { return m_struct.memoryTypeCount; }
    void set_memoryTypeCount(uint32_t inValue) { m_struct.memoryTypeCount = inValue; }
    uint32_t get_memoryHeapCount() { return m_struct.memoryHeapCount; }
    void set_memoryHeapCount(uint32_t inValue) { m_struct.memoryHeapCount = inValue; }


private:
    VkPhysicalDeviceMemoryProperties m_struct;
    const VkPhysicalDeviceMemoryProperties* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkphysicaldeviceproperties_struct_wrapper
{
public:
    vkphysicaldeviceproperties_struct_wrapper();
    vkphysicaldeviceproperties_struct_wrapper(VkPhysicalDeviceProperties* pInStruct);
    vkphysicaldeviceproperties_struct_wrapper(const VkPhysicalDeviceProperties* pInStruct);

    virtual ~vkphysicaldeviceproperties_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    uint32_t get_apiVersion() { return m_struct.apiVersion; }
    void set_apiVersion(uint32_t inValue) { m_struct.apiVersion = inValue; }
    uint32_t get_driverVersion() { return m_struct.driverVersion; }
    void set_driverVersion(uint32_t inValue) { m_struct.driverVersion = inValue; }
    uint32_t get_vendorID() { return m_struct.vendorID; }
    void set_vendorID(uint32_t inValue) { m_struct.vendorID = inValue; }
    uint32_t get_deviceID() { return m_struct.deviceID; }
    void set_deviceID(uint32_t inValue) { m_struct.deviceID = inValue; }
    VkPhysicalDeviceType get_deviceType() { return m_struct.deviceType; }
    void set_deviceType(VkPhysicalDeviceType inValue) { m_struct.deviceType = inValue; }
    VkPhysicalDeviceLimits get_limits() { return m_struct.limits; }
    void set_limits(VkPhysicalDeviceLimits inValue) { m_struct.limits = inValue; }
    VkPhysicalDeviceSparseProperties get_sparseProperties() { return m_struct.sparseProperties; }
    void set_sparseProperties(VkPhysicalDeviceSparseProperties inValue) { m_struct.sparseProperties = inValue; }


private:
    VkPhysicalDeviceProperties m_struct;
    const VkPhysicalDeviceProperties* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkphysicaldevicesparseproperties_struct_wrapper
{
public:
    vkphysicaldevicesparseproperties_struct_wrapper();
    vkphysicaldevicesparseproperties_struct_wrapper(VkPhysicalDeviceSparseProperties* pInStruct);
    vkphysicaldevicesparseproperties_struct_wrapper(const VkPhysicalDeviceSparseProperties* pInStruct);

    virtual ~vkphysicaldevicesparseproperties_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkBool32 get_residencyStandard2DBlockShape() { return m_struct.residencyStandard2DBlockShape; }
    void set_residencyStandard2DBlockShape(VkBool32 inValue) { m_struct.residencyStandard2DBlockShape = inValue; }
    VkBool32 get_residencyStandard2DMultisampleBlockShape() { return m_struct.residencyStandard2DMultisampleBlockShape; }
    void set_residencyStandard2DMultisampleBlockShape(VkBool32 inValue) { m_struct.residencyStandard2DMultisampleBlockShape = inValue; }
    VkBool32 get_residencyStandard3DBlockShape() { return m_struct.residencyStandard3DBlockShape; }
    void set_residencyStandard3DBlockShape(VkBool32 inValue) { m_struct.residencyStandard3DBlockShape = inValue; }
    VkBool32 get_residencyAlignedMipSize() { return m_struct.residencyAlignedMipSize; }
    void set_residencyAlignedMipSize(VkBool32 inValue) { m_struct.residencyAlignedMipSize = inValue; }
    VkBool32 get_residencyNonResidentStrict() { return m_struct.residencyNonResidentStrict; }
    void set_residencyNonResidentStrict(VkBool32 inValue) { m_struct.residencyNonResidentStrict = inValue; }


private:
    VkPhysicalDeviceSparseProperties m_struct;
    const VkPhysicalDeviceSparseProperties* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkpipelinecachecreateinfo_struct_wrapper
{
public:
    vkpipelinecachecreateinfo_struct_wrapper();
    vkpipelinecachecreateinfo_struct_wrapper(VkPipelineCacheCreateInfo* pInStruct);
    vkpipelinecachecreateinfo_struct_wrapper(const VkPipelineCacheCreateInfo* pInStruct);

    virtual ~vkpipelinecachecreateinfo_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkPipelineCacheCreateFlags get_flags() { return m_struct.flags; }
    void set_flags(VkPipelineCacheCreateFlags inValue) { m_struct.flags = inValue; }
    size_t get_initialDataSize() { return m_struct.initialDataSize; }
    void set_initialDataSize(size_t inValue) { m_struct.initialDataSize = inValue; }
    const void* get_pInitialData() { return m_struct.pInitialData; }


private:
    VkPipelineCacheCreateInfo m_struct;
    const VkPipelineCacheCreateInfo* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkpipelinecolorblendattachmentstate_struct_wrapper
{
public:
    vkpipelinecolorblendattachmentstate_struct_wrapper();
    vkpipelinecolorblendattachmentstate_struct_wrapper(VkPipelineColorBlendAttachmentState* pInStruct);
    vkpipelinecolorblendattachmentstate_struct_wrapper(const VkPipelineColorBlendAttachmentState* pInStruct);

    virtual ~vkpipelinecolorblendattachmentstate_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkBool32 get_blendEnable() { return m_struct.blendEnable; }
    void set_blendEnable(VkBool32 inValue) { m_struct.blendEnable = inValue; }
    VkBlendFactor get_srcColorBlendFactor() { return m_struct.srcColorBlendFactor; }
    void set_srcColorBlendFactor(VkBlendFactor inValue) { m_struct.srcColorBlendFactor = inValue; }
    VkBlendFactor get_dstColorBlendFactor() { return m_struct.dstColorBlendFactor; }
    void set_dstColorBlendFactor(VkBlendFactor inValue) { m_struct.dstColorBlendFactor = inValue; }
    VkBlendOp get_colorBlendOp() { return m_struct.colorBlendOp; }
    void set_colorBlendOp(VkBlendOp inValue) { m_struct.colorBlendOp = inValue; }
    VkBlendFactor get_srcAlphaBlendFactor() { return m_struct.srcAlphaBlendFactor; }
    void set_srcAlphaBlendFactor(VkBlendFactor inValue) { m_struct.srcAlphaBlendFactor = inValue; }
    VkBlendFactor get_dstAlphaBlendFactor() { return m_struct.dstAlphaBlendFactor; }
    void set_dstAlphaBlendFactor(VkBlendFactor inValue) { m_struct.dstAlphaBlendFactor = inValue; }
    VkBlendOp get_alphaBlendOp() { return m_struct.alphaBlendOp; }
    void set_alphaBlendOp(VkBlendOp inValue) { m_struct.alphaBlendOp = inValue; }
    VkColorComponentFlags get_colorWriteMask() { return m_struct.colorWriteMask; }
    void set_colorWriteMask(VkColorComponentFlags inValue) { m_struct.colorWriteMask = inValue; }


private:
    VkPipelineColorBlendAttachmentState m_struct;
    const VkPipelineColorBlendAttachmentState* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkpipelinecolorblendstatecreateinfo_struct_wrapper
{
public:
    vkpipelinecolorblendstatecreateinfo_struct_wrapper();
    vkpipelinecolorblendstatecreateinfo_struct_wrapper(VkPipelineColorBlendStateCreateInfo* pInStruct);
    vkpipelinecolorblendstatecreateinfo_struct_wrapper(const VkPipelineColorBlendStateCreateInfo* pInStruct);

    virtual ~vkpipelinecolorblendstatecreateinfo_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkPipelineColorBlendStateCreateFlags get_flags() { return m_struct.flags; }
    void set_flags(VkPipelineColorBlendStateCreateFlags inValue) { m_struct.flags = inValue; }
    VkBool32 get_logicOpEnable() { return m_struct.logicOpEnable; }
    void set_logicOpEnable(VkBool32 inValue) { m_struct.logicOpEnable = inValue; }
    VkLogicOp get_logicOp() { return m_struct.logicOp; }
    void set_logicOp(VkLogicOp inValue) { m_struct.logicOp = inValue; }
    uint32_t get_attachmentCount() { return m_struct.attachmentCount; }
    void set_attachmentCount(uint32_t inValue) { m_struct.attachmentCount = inValue; }


private:
    VkPipelineColorBlendStateCreateInfo m_struct;
    const VkPipelineColorBlendStateCreateInfo* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkpipelinedepthstencilstatecreateinfo_struct_wrapper
{
public:
    vkpipelinedepthstencilstatecreateinfo_struct_wrapper();
    vkpipelinedepthstencilstatecreateinfo_struct_wrapper(VkPipelineDepthStencilStateCreateInfo* pInStruct);
    vkpipelinedepthstencilstatecreateinfo_struct_wrapper(const VkPipelineDepthStencilStateCreateInfo* pInStruct);

    virtual ~vkpipelinedepthstencilstatecreateinfo_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkPipelineDepthStencilStateCreateFlags get_flags() { return m_struct.flags; }
    void set_flags(VkPipelineDepthStencilStateCreateFlags inValue) { m_struct.flags = inValue; }
    VkBool32 get_depthTestEnable() { return m_struct.depthTestEnable; }
    void set_depthTestEnable(VkBool32 inValue) { m_struct.depthTestEnable = inValue; }
    VkBool32 get_depthWriteEnable() { return m_struct.depthWriteEnable; }
    void set_depthWriteEnable(VkBool32 inValue) { m_struct.depthWriteEnable = inValue; }
    VkCompareOp get_depthCompareOp() { return m_struct.depthCompareOp; }
    void set_depthCompareOp(VkCompareOp inValue) { m_struct.depthCompareOp = inValue; }
    VkBool32 get_depthBoundsTestEnable() { return m_struct.depthBoundsTestEnable; }
    void set_depthBoundsTestEnable(VkBool32 inValue) { m_struct.depthBoundsTestEnable = inValue; }
    VkBool32 get_stencilTestEnable() { return m_struct.stencilTestEnable; }
    void set_stencilTestEnable(VkBool32 inValue) { m_struct.stencilTestEnable = inValue; }
    VkStencilOpState get_front() { return m_struct.front; }
    void set_front(VkStencilOpState inValue) { m_struct.front = inValue; }
    VkStencilOpState get_back() { return m_struct.back; }
    void set_back(VkStencilOpState inValue) { m_struct.back = inValue; }
    float get_minDepthBounds() { return m_struct.minDepthBounds; }
    void set_minDepthBounds(float inValue) { m_struct.minDepthBounds = inValue; }
    float get_maxDepthBounds() { return m_struct.maxDepthBounds; }
    void set_maxDepthBounds(float inValue) { m_struct.maxDepthBounds = inValue; }


private:
    VkPipelineDepthStencilStateCreateInfo m_struct;
    const VkPipelineDepthStencilStateCreateInfo* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkpipelinedynamicstatecreateinfo_struct_wrapper
{
public:
    vkpipelinedynamicstatecreateinfo_struct_wrapper();
    vkpipelinedynamicstatecreateinfo_struct_wrapper(VkPipelineDynamicStateCreateInfo* pInStruct);
    vkpipelinedynamicstatecreateinfo_struct_wrapper(const VkPipelineDynamicStateCreateInfo* pInStruct);

    virtual ~vkpipelinedynamicstatecreateinfo_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkPipelineDynamicStateCreateFlags get_flags() { return m_struct.flags; }
    void set_flags(VkPipelineDynamicStateCreateFlags inValue) { m_struct.flags = inValue; }
    uint32_t get_dynamicStateCount() { return m_struct.dynamicStateCount; }
    void set_dynamicStateCount(uint32_t inValue) { m_struct.dynamicStateCount = inValue; }


private:
    VkPipelineDynamicStateCreateInfo m_struct;
    const VkPipelineDynamicStateCreateInfo* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkpipelineinputassemblystatecreateinfo_struct_wrapper
{
public:
    vkpipelineinputassemblystatecreateinfo_struct_wrapper();
    vkpipelineinputassemblystatecreateinfo_struct_wrapper(VkPipelineInputAssemblyStateCreateInfo* pInStruct);
    vkpipelineinputassemblystatecreateinfo_struct_wrapper(const VkPipelineInputAssemblyStateCreateInfo* pInStruct);

    virtual ~vkpipelineinputassemblystatecreateinfo_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkPipelineInputAssemblyStateCreateFlags get_flags() { return m_struct.flags; }
    void set_flags(VkPipelineInputAssemblyStateCreateFlags inValue) { m_struct.flags = inValue; }
    VkPrimitiveTopology get_topology() { return m_struct.topology; }
    void set_topology(VkPrimitiveTopology inValue) { m_struct.topology = inValue; }
    VkBool32 get_primitiveRestartEnable() { return m_struct.primitiveRestartEnable; }
    void set_primitiveRestartEnable(VkBool32 inValue) { m_struct.primitiveRestartEnable = inValue; }


private:
    VkPipelineInputAssemblyStateCreateInfo m_struct;
    const VkPipelineInputAssemblyStateCreateInfo* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkpipelinelayoutcreateinfo_struct_wrapper
{
public:
    vkpipelinelayoutcreateinfo_struct_wrapper();
    vkpipelinelayoutcreateinfo_struct_wrapper(VkPipelineLayoutCreateInfo* pInStruct);
    vkpipelinelayoutcreateinfo_struct_wrapper(const VkPipelineLayoutCreateInfo* pInStruct);

    virtual ~vkpipelinelayoutcreateinfo_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkPipelineLayoutCreateFlags get_flags() { return m_struct.flags; }
    void set_flags(VkPipelineLayoutCreateFlags inValue) { m_struct.flags = inValue; }
    uint32_t get_setLayoutCount() { return m_struct.setLayoutCount; }
    void set_setLayoutCount(uint32_t inValue) { m_struct.setLayoutCount = inValue; }
    uint32_t get_pushConstantRangeCount() { return m_struct.pushConstantRangeCount; }
    void set_pushConstantRangeCount(uint32_t inValue) { m_struct.pushConstantRangeCount = inValue; }


private:
    VkPipelineLayoutCreateInfo m_struct;
    const VkPipelineLayoutCreateInfo* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkpipelinemultisamplestatecreateinfo_struct_wrapper
{
public:
    vkpipelinemultisamplestatecreateinfo_struct_wrapper();
    vkpipelinemultisamplestatecreateinfo_struct_wrapper(VkPipelineMultisampleStateCreateInfo* pInStruct);
    vkpipelinemultisamplestatecreateinfo_struct_wrapper(const VkPipelineMultisampleStateCreateInfo* pInStruct);

    virtual ~vkpipelinemultisamplestatecreateinfo_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkPipelineMultisampleStateCreateFlags get_flags() { return m_struct.flags; }
    void set_flags(VkPipelineMultisampleStateCreateFlags inValue) { m_struct.flags = inValue; }
    VkSampleCountFlagBits get_rasterizationSamples() { return m_struct.rasterizationSamples; }
    void set_rasterizationSamples(VkSampleCountFlagBits inValue) { m_struct.rasterizationSamples = inValue; }
    VkBool32 get_sampleShadingEnable() { return m_struct.sampleShadingEnable; }
    void set_sampleShadingEnable(VkBool32 inValue) { m_struct.sampleShadingEnable = inValue; }
    float get_minSampleShading() { return m_struct.minSampleShading; }
    void set_minSampleShading(float inValue) { m_struct.minSampleShading = inValue; }
    const VkSampleMask* get_pSampleMask() { return m_struct.pSampleMask; }
    VkBool32 get_alphaToCoverageEnable() { return m_struct.alphaToCoverageEnable; }
    void set_alphaToCoverageEnable(VkBool32 inValue) { m_struct.alphaToCoverageEnable = inValue; }
    VkBool32 get_alphaToOneEnable() { return m_struct.alphaToOneEnable; }
    void set_alphaToOneEnable(VkBool32 inValue) { m_struct.alphaToOneEnable = inValue; }


private:
    VkPipelineMultisampleStateCreateInfo m_struct;
    const VkPipelineMultisampleStateCreateInfo* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkpipelinerasterizationstatecreateinfo_struct_wrapper
{
public:
    vkpipelinerasterizationstatecreateinfo_struct_wrapper();
    vkpipelinerasterizationstatecreateinfo_struct_wrapper(VkPipelineRasterizationStateCreateInfo* pInStruct);
    vkpipelinerasterizationstatecreateinfo_struct_wrapper(const VkPipelineRasterizationStateCreateInfo* pInStruct);

    virtual ~vkpipelinerasterizationstatecreateinfo_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkPipelineRasterizationStateCreateFlags get_flags() { return m_struct.flags; }
    void set_flags(VkPipelineRasterizationStateCreateFlags inValue) { m_struct.flags = inValue; }
    VkBool32 get_depthClampEnable() { return m_struct.depthClampEnable; }
    void set_depthClampEnable(VkBool32 inValue) { m_struct.depthClampEnable = inValue; }
    VkBool32 get_rasterizerDiscardEnable() { return m_struct.rasterizerDiscardEnable; }
    void set_rasterizerDiscardEnable(VkBool32 inValue) { m_struct.rasterizerDiscardEnable = inValue; }
    VkPolygonMode get_polygonMode() { return m_struct.polygonMode; }
    void set_polygonMode(VkPolygonMode inValue) { m_struct.polygonMode = inValue; }
    VkCullModeFlags get_cullMode() { return m_struct.cullMode; }
    void set_cullMode(VkCullModeFlags inValue) { m_struct.cullMode = inValue; }
    VkFrontFace get_frontFace() { return m_struct.frontFace; }
    void set_frontFace(VkFrontFace inValue) { m_struct.frontFace = inValue; }
    VkBool32 get_depthBiasEnable() { return m_struct.depthBiasEnable; }
    void set_depthBiasEnable(VkBool32 inValue) { m_struct.depthBiasEnable = inValue; }
    float get_depthBiasConstantFactor() { return m_struct.depthBiasConstantFactor; }
    void set_depthBiasConstantFactor(float inValue) { m_struct.depthBiasConstantFactor = inValue; }
    float get_depthBiasClamp() { return m_struct.depthBiasClamp; }
    void set_depthBiasClamp(float inValue) { m_struct.depthBiasClamp = inValue; }
    float get_depthBiasSlopeFactor() { return m_struct.depthBiasSlopeFactor; }
    void set_depthBiasSlopeFactor(float inValue) { m_struct.depthBiasSlopeFactor = inValue; }
    float get_lineWidth() { return m_struct.lineWidth; }
    void set_lineWidth(float inValue) { m_struct.lineWidth = inValue; }


private:
    VkPipelineRasterizationStateCreateInfo m_struct;
    const VkPipelineRasterizationStateCreateInfo* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkpipelinerasterizationstaterasterizationorderamd_struct_wrapper
{
public:
    vkpipelinerasterizationstaterasterizationorderamd_struct_wrapper();
    vkpipelinerasterizationstaterasterizationorderamd_struct_wrapper(VkPipelineRasterizationStateRasterizationOrderAMD* pInStruct);
    vkpipelinerasterizationstaterasterizationorderamd_struct_wrapper(const VkPipelineRasterizationStateRasterizationOrderAMD* pInStruct);

    virtual ~vkpipelinerasterizationstaterasterizationorderamd_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkRasterizationOrderAMD get_rasterizationOrder() { return m_struct.rasterizationOrder; }
    void set_rasterizationOrder(VkRasterizationOrderAMD inValue) { m_struct.rasterizationOrder = inValue; }


private:
    VkPipelineRasterizationStateRasterizationOrderAMD m_struct;
    const VkPipelineRasterizationStateRasterizationOrderAMD* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkpipelineshaderstagecreateinfo_struct_wrapper
{
public:
    vkpipelineshaderstagecreateinfo_struct_wrapper();
    vkpipelineshaderstagecreateinfo_struct_wrapper(VkPipelineShaderStageCreateInfo* pInStruct);
    vkpipelineshaderstagecreateinfo_struct_wrapper(const VkPipelineShaderStageCreateInfo* pInStruct);

    virtual ~vkpipelineshaderstagecreateinfo_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkPipelineShaderStageCreateFlags get_flags() { return m_struct.flags; }
    void set_flags(VkPipelineShaderStageCreateFlags inValue) { m_struct.flags = inValue; }
    VkShaderStageFlagBits get_stage() { return m_struct.stage; }
    void set_stage(VkShaderStageFlagBits inValue) { m_struct.stage = inValue; }
    VkShaderModule get_module() { return m_struct.module; }
    void set_module(VkShaderModule inValue) { m_struct.module = inValue; }
    const char* get_pName() { return m_struct.pName; }
    const VkSpecializationInfo* get_pSpecializationInfo() { return m_struct.pSpecializationInfo; }


private:
    VkPipelineShaderStageCreateInfo m_struct;
    const VkPipelineShaderStageCreateInfo* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkpipelinetessellationstatecreateinfo_struct_wrapper
{
public:
    vkpipelinetessellationstatecreateinfo_struct_wrapper();
    vkpipelinetessellationstatecreateinfo_struct_wrapper(VkPipelineTessellationStateCreateInfo* pInStruct);
    vkpipelinetessellationstatecreateinfo_struct_wrapper(const VkPipelineTessellationStateCreateInfo* pInStruct);

    virtual ~vkpipelinetessellationstatecreateinfo_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkPipelineTessellationStateCreateFlags get_flags() { return m_struct.flags; }
    void set_flags(VkPipelineTessellationStateCreateFlags inValue) { m_struct.flags = inValue; }
    uint32_t get_patchControlPoints() { return m_struct.patchControlPoints; }
    void set_patchControlPoints(uint32_t inValue) { m_struct.patchControlPoints = inValue; }


private:
    VkPipelineTessellationStateCreateInfo m_struct;
    const VkPipelineTessellationStateCreateInfo* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkpipelinevertexinputstatecreateinfo_struct_wrapper
{
public:
    vkpipelinevertexinputstatecreateinfo_struct_wrapper();
    vkpipelinevertexinputstatecreateinfo_struct_wrapper(VkPipelineVertexInputStateCreateInfo* pInStruct);
    vkpipelinevertexinputstatecreateinfo_struct_wrapper(const VkPipelineVertexInputStateCreateInfo* pInStruct);

    virtual ~vkpipelinevertexinputstatecreateinfo_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkPipelineVertexInputStateCreateFlags get_flags() { return m_struct.flags; }
    void set_flags(VkPipelineVertexInputStateCreateFlags inValue) { m_struct.flags = inValue; }
    uint32_t get_vertexBindingDescriptionCount() { return m_struct.vertexBindingDescriptionCount; }
    void set_vertexBindingDescriptionCount(uint32_t inValue) { m_struct.vertexBindingDescriptionCount = inValue; }
    uint32_t get_vertexAttributeDescriptionCount() { return m_struct.vertexAttributeDescriptionCount; }
    void set_vertexAttributeDescriptionCount(uint32_t inValue) { m_struct.vertexAttributeDescriptionCount = inValue; }


private:
    VkPipelineVertexInputStateCreateInfo m_struct;
    const VkPipelineVertexInputStateCreateInfo* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkpipelineviewportstatecreateinfo_struct_wrapper
{
public:
    vkpipelineviewportstatecreateinfo_struct_wrapper();
    vkpipelineviewportstatecreateinfo_struct_wrapper(VkPipelineViewportStateCreateInfo* pInStruct);
    vkpipelineviewportstatecreateinfo_struct_wrapper(const VkPipelineViewportStateCreateInfo* pInStruct);

    virtual ~vkpipelineviewportstatecreateinfo_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkPipelineViewportStateCreateFlags get_flags() { return m_struct.flags; }
    void set_flags(VkPipelineViewportStateCreateFlags inValue) { m_struct.flags = inValue; }
    uint32_t get_viewportCount() { return m_struct.viewportCount; }
    void set_viewportCount(uint32_t inValue) { m_struct.viewportCount = inValue; }
    uint32_t get_scissorCount() { return m_struct.scissorCount; }
    void set_scissorCount(uint32_t inValue) { m_struct.scissorCount = inValue; }


private:
    VkPipelineViewportStateCreateInfo m_struct;
    const VkPipelineViewportStateCreateInfo* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkpresentinfokhr_struct_wrapper
{
public:
    vkpresentinfokhr_struct_wrapper();
    vkpresentinfokhr_struct_wrapper(VkPresentInfoKHR* pInStruct);
    vkpresentinfokhr_struct_wrapper(const VkPresentInfoKHR* pInStruct);

    virtual ~vkpresentinfokhr_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    uint32_t get_waitSemaphoreCount() { return m_struct.waitSemaphoreCount; }
    void set_waitSemaphoreCount(uint32_t inValue) { m_struct.waitSemaphoreCount = inValue; }
    uint32_t get_swapchainCount() { return m_struct.swapchainCount; }
    void set_swapchainCount(uint32_t inValue) { m_struct.swapchainCount = inValue; }
    VkResult* get_pResults() { return m_struct.pResults; }
    void set_pResults(VkResult* inValue) { m_struct.pResults = inValue; }


private:
    VkPresentInfoKHR m_struct;
    const VkPresentInfoKHR* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkpushconstantrange_struct_wrapper
{
public:
    vkpushconstantrange_struct_wrapper();
    vkpushconstantrange_struct_wrapper(VkPushConstantRange* pInStruct);
    vkpushconstantrange_struct_wrapper(const VkPushConstantRange* pInStruct);

    virtual ~vkpushconstantrange_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkShaderStageFlags get_stageFlags() { return m_struct.stageFlags; }
    void set_stageFlags(VkShaderStageFlags inValue) { m_struct.stageFlags = inValue; }
    uint32_t get_offset() { return m_struct.offset; }
    void set_offset(uint32_t inValue) { m_struct.offset = inValue; }
    uint32_t get_size() { return m_struct.size; }
    void set_size(uint32_t inValue) { m_struct.size = inValue; }


private:
    VkPushConstantRange m_struct;
    const VkPushConstantRange* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkquerypoolcreateinfo_struct_wrapper
{
public:
    vkquerypoolcreateinfo_struct_wrapper();
    vkquerypoolcreateinfo_struct_wrapper(VkQueryPoolCreateInfo* pInStruct);
    vkquerypoolcreateinfo_struct_wrapper(const VkQueryPoolCreateInfo* pInStruct);

    virtual ~vkquerypoolcreateinfo_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkQueryPoolCreateFlags get_flags() { return m_struct.flags; }
    void set_flags(VkQueryPoolCreateFlags inValue) { m_struct.flags = inValue; }
    VkQueryType get_queryType() { return m_struct.queryType; }
    void set_queryType(VkQueryType inValue) { m_struct.queryType = inValue; }
    uint32_t get_queryCount() { return m_struct.queryCount; }
    void set_queryCount(uint32_t inValue) { m_struct.queryCount = inValue; }
    VkQueryPipelineStatisticFlags get_pipelineStatistics() { return m_struct.pipelineStatistics; }
    void set_pipelineStatistics(VkQueryPipelineStatisticFlags inValue) { m_struct.pipelineStatistics = inValue; }


private:
    VkQueryPoolCreateInfo m_struct;
    const VkQueryPoolCreateInfo* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkqueuefamilyproperties_struct_wrapper
{
public:
    vkqueuefamilyproperties_struct_wrapper();
    vkqueuefamilyproperties_struct_wrapper(VkQueueFamilyProperties* pInStruct);
    vkqueuefamilyproperties_struct_wrapper(const VkQueueFamilyProperties* pInStruct);

    virtual ~vkqueuefamilyproperties_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkQueueFlags get_queueFlags() { return m_struct.queueFlags; }
    void set_queueFlags(VkQueueFlags inValue) { m_struct.queueFlags = inValue; }
    uint32_t get_queueCount() { return m_struct.queueCount; }
    void set_queueCount(uint32_t inValue) { m_struct.queueCount = inValue; }
    uint32_t get_timestampValidBits() { return m_struct.timestampValidBits; }
    void set_timestampValidBits(uint32_t inValue) { m_struct.timestampValidBits = inValue; }
    VkExtent3D get_minImageTransferGranularity() { return m_struct.minImageTransferGranularity; }
    void set_minImageTransferGranularity(VkExtent3D inValue) { m_struct.minImageTransferGranularity = inValue; }


private:
    VkQueueFamilyProperties m_struct;
    const VkQueueFamilyProperties* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkrect2d_struct_wrapper
{
public:
    vkrect2d_struct_wrapper();
    vkrect2d_struct_wrapper(VkRect2D* pInStruct);
    vkrect2d_struct_wrapper(const VkRect2D* pInStruct);

    virtual ~vkrect2d_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkOffset2D get_offset() { return m_struct.offset; }
    void set_offset(VkOffset2D inValue) { m_struct.offset = inValue; }
    VkExtent2D get_extent() { return m_struct.extent; }
    void set_extent(VkExtent2D inValue) { m_struct.extent = inValue; }


private:
    VkRect2D m_struct;
    const VkRect2D* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkrenderpassbegininfo_struct_wrapper
{
public:
    vkrenderpassbegininfo_struct_wrapper();
    vkrenderpassbegininfo_struct_wrapper(VkRenderPassBeginInfo* pInStruct);
    vkrenderpassbegininfo_struct_wrapper(const VkRenderPassBeginInfo* pInStruct);

    virtual ~vkrenderpassbegininfo_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkRenderPass get_renderPass() { return m_struct.renderPass; }
    void set_renderPass(VkRenderPass inValue) { m_struct.renderPass = inValue; }
    VkFramebuffer get_framebuffer() { return m_struct.framebuffer; }
    void set_framebuffer(VkFramebuffer inValue) { m_struct.framebuffer = inValue; }
    VkRect2D get_renderArea() { return m_struct.renderArea; }
    void set_renderArea(VkRect2D inValue) { m_struct.renderArea = inValue; }
    uint32_t get_clearValueCount() { return m_struct.clearValueCount; }
    void set_clearValueCount(uint32_t inValue) { m_struct.clearValueCount = inValue; }


private:
    VkRenderPassBeginInfo m_struct;
    const VkRenderPassBeginInfo* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkrenderpasscreateinfo_struct_wrapper
{
public:
    vkrenderpasscreateinfo_struct_wrapper();
    vkrenderpasscreateinfo_struct_wrapper(VkRenderPassCreateInfo* pInStruct);
    vkrenderpasscreateinfo_struct_wrapper(const VkRenderPassCreateInfo* pInStruct);

    virtual ~vkrenderpasscreateinfo_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkRenderPassCreateFlags get_flags() { return m_struct.flags; }
    void set_flags(VkRenderPassCreateFlags inValue) { m_struct.flags = inValue; }
    uint32_t get_attachmentCount() { return m_struct.attachmentCount; }
    void set_attachmentCount(uint32_t inValue) { m_struct.attachmentCount = inValue; }
    uint32_t get_subpassCount() { return m_struct.subpassCount; }
    void set_subpassCount(uint32_t inValue) { m_struct.subpassCount = inValue; }
    uint32_t get_dependencyCount() { return m_struct.dependencyCount; }
    void set_dependencyCount(uint32_t inValue) { m_struct.dependencyCount = inValue; }


private:
    VkRenderPassCreateInfo m_struct;
    const VkRenderPassCreateInfo* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vksamplercreateinfo_struct_wrapper
{
public:
    vksamplercreateinfo_struct_wrapper();
    vksamplercreateinfo_struct_wrapper(VkSamplerCreateInfo* pInStruct);
    vksamplercreateinfo_struct_wrapper(const VkSamplerCreateInfo* pInStruct);

    virtual ~vksamplercreateinfo_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkSamplerCreateFlags get_flags() { return m_struct.flags; }
    void set_flags(VkSamplerCreateFlags inValue) { m_struct.flags = inValue; }
    VkFilter get_magFilter() { return m_struct.magFilter; }
    void set_magFilter(VkFilter inValue) { m_struct.magFilter = inValue; }
    VkFilter get_minFilter() { return m_struct.minFilter; }
    void set_minFilter(VkFilter inValue) { m_struct.minFilter = inValue; }
    VkSamplerMipmapMode get_mipmapMode() { return m_struct.mipmapMode; }
    void set_mipmapMode(VkSamplerMipmapMode inValue) { m_struct.mipmapMode = inValue; }
    VkSamplerAddressMode get_addressModeU() { return m_struct.addressModeU; }
    void set_addressModeU(VkSamplerAddressMode inValue) { m_struct.addressModeU = inValue; }
    VkSamplerAddressMode get_addressModeV() { return m_struct.addressModeV; }
    void set_addressModeV(VkSamplerAddressMode inValue) { m_struct.addressModeV = inValue; }
    VkSamplerAddressMode get_addressModeW() { return m_struct.addressModeW; }
    void set_addressModeW(VkSamplerAddressMode inValue) { m_struct.addressModeW = inValue; }
    float get_mipLodBias() { return m_struct.mipLodBias; }
    void set_mipLodBias(float inValue) { m_struct.mipLodBias = inValue; }
    VkBool32 get_anisotropyEnable() { return m_struct.anisotropyEnable; }
    void set_anisotropyEnable(VkBool32 inValue) { m_struct.anisotropyEnable = inValue; }
    float get_maxAnisotropy() { return m_struct.maxAnisotropy; }
    void set_maxAnisotropy(float inValue) { m_struct.maxAnisotropy = inValue; }
    VkBool32 get_compareEnable() { return m_struct.compareEnable; }
    void set_compareEnable(VkBool32 inValue) { m_struct.compareEnable = inValue; }
    VkCompareOp get_compareOp() { return m_struct.compareOp; }
    void set_compareOp(VkCompareOp inValue) { m_struct.compareOp = inValue; }
    float get_minLod() { return m_struct.minLod; }
    void set_minLod(float inValue) { m_struct.minLod = inValue; }
    float get_maxLod() { return m_struct.maxLod; }
    void set_maxLod(float inValue) { m_struct.maxLod = inValue; }
    VkBorderColor get_borderColor() { return m_struct.borderColor; }
    void set_borderColor(VkBorderColor inValue) { m_struct.borderColor = inValue; }
    VkBool32 get_unnormalizedCoordinates() { return m_struct.unnormalizedCoordinates; }
    void set_unnormalizedCoordinates(VkBool32 inValue) { m_struct.unnormalizedCoordinates = inValue; }


private:
    VkSamplerCreateInfo m_struct;
    const VkSamplerCreateInfo* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vksemaphorecreateinfo_struct_wrapper
{
public:
    vksemaphorecreateinfo_struct_wrapper();
    vksemaphorecreateinfo_struct_wrapper(VkSemaphoreCreateInfo* pInStruct);
    vksemaphorecreateinfo_struct_wrapper(const VkSemaphoreCreateInfo* pInStruct);

    virtual ~vksemaphorecreateinfo_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkSemaphoreCreateFlags get_flags() { return m_struct.flags; }
    void set_flags(VkSemaphoreCreateFlags inValue) { m_struct.flags = inValue; }


private:
    VkSemaphoreCreateInfo m_struct;
    const VkSemaphoreCreateInfo* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkshadermodulecreateinfo_struct_wrapper
{
public:
    vkshadermodulecreateinfo_struct_wrapper();
    vkshadermodulecreateinfo_struct_wrapper(VkShaderModuleCreateInfo* pInStruct);
    vkshadermodulecreateinfo_struct_wrapper(const VkShaderModuleCreateInfo* pInStruct);

    virtual ~vkshadermodulecreateinfo_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkShaderModuleCreateFlags get_flags() { return m_struct.flags; }
    void set_flags(VkShaderModuleCreateFlags inValue) { m_struct.flags = inValue; }
    size_t get_codeSize() { return m_struct.codeSize; }
    void set_codeSize(size_t inValue) { m_struct.codeSize = inValue; }
    const uint32_t* get_pCode() { return m_struct.pCode; }


private:
    VkShaderModuleCreateInfo m_struct;
    const VkShaderModuleCreateInfo* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vksparsebuffermemorybindinfo_struct_wrapper
{
public:
    vksparsebuffermemorybindinfo_struct_wrapper();
    vksparsebuffermemorybindinfo_struct_wrapper(VkSparseBufferMemoryBindInfo* pInStruct);
    vksparsebuffermemorybindinfo_struct_wrapper(const VkSparseBufferMemoryBindInfo* pInStruct);

    virtual ~vksparsebuffermemorybindinfo_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkBuffer get_buffer() { return m_struct.buffer; }
    void set_buffer(VkBuffer inValue) { m_struct.buffer = inValue; }
    uint32_t get_bindCount() { return m_struct.bindCount; }
    void set_bindCount(uint32_t inValue) { m_struct.bindCount = inValue; }


private:
    VkSparseBufferMemoryBindInfo m_struct;
    const VkSparseBufferMemoryBindInfo* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vksparseimageformatproperties_struct_wrapper
{
public:
    vksparseimageformatproperties_struct_wrapper();
    vksparseimageformatproperties_struct_wrapper(VkSparseImageFormatProperties* pInStruct);
    vksparseimageformatproperties_struct_wrapper(const VkSparseImageFormatProperties* pInStruct);

    virtual ~vksparseimageformatproperties_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkImageAspectFlags get_aspectMask() { return m_struct.aspectMask; }
    void set_aspectMask(VkImageAspectFlags inValue) { m_struct.aspectMask = inValue; }
    VkExtent3D get_imageGranularity() { return m_struct.imageGranularity; }
    void set_imageGranularity(VkExtent3D inValue) { m_struct.imageGranularity = inValue; }
    VkSparseImageFormatFlags get_flags() { return m_struct.flags; }
    void set_flags(VkSparseImageFormatFlags inValue) { m_struct.flags = inValue; }


private:
    VkSparseImageFormatProperties m_struct;
    const VkSparseImageFormatProperties* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vksparseimagememorybind_struct_wrapper
{
public:
    vksparseimagememorybind_struct_wrapper();
    vksparseimagememorybind_struct_wrapper(VkSparseImageMemoryBind* pInStruct);
    vksparseimagememorybind_struct_wrapper(const VkSparseImageMemoryBind* pInStruct);

    virtual ~vksparseimagememorybind_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkImageSubresource get_subresource() { return m_struct.subresource; }
    void set_subresource(VkImageSubresource inValue) { m_struct.subresource = inValue; }
    VkOffset3D get_offset() { return m_struct.offset; }
    void set_offset(VkOffset3D inValue) { m_struct.offset = inValue; }
    VkExtent3D get_extent() { return m_struct.extent; }
    void set_extent(VkExtent3D inValue) { m_struct.extent = inValue; }
    VkDeviceMemory get_memory() { return m_struct.memory; }
    void set_memory(VkDeviceMemory inValue) { m_struct.memory = inValue; }
    VkDeviceSize get_memoryOffset() { return m_struct.memoryOffset; }
    void set_memoryOffset(VkDeviceSize inValue) { m_struct.memoryOffset = inValue; }
    VkSparseMemoryBindFlags get_flags() { return m_struct.flags; }
    void set_flags(VkSparseMemoryBindFlags inValue) { m_struct.flags = inValue; }


private:
    VkSparseImageMemoryBind m_struct;
    const VkSparseImageMemoryBind* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vksparseimagememorybindinfo_struct_wrapper
{
public:
    vksparseimagememorybindinfo_struct_wrapper();
    vksparseimagememorybindinfo_struct_wrapper(VkSparseImageMemoryBindInfo* pInStruct);
    vksparseimagememorybindinfo_struct_wrapper(const VkSparseImageMemoryBindInfo* pInStruct);

    virtual ~vksparseimagememorybindinfo_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkImage get_image() { return m_struct.image; }
    void set_image(VkImage inValue) { m_struct.image = inValue; }
    uint32_t get_bindCount() { return m_struct.bindCount; }
    void set_bindCount(uint32_t inValue) { m_struct.bindCount = inValue; }


private:
    VkSparseImageMemoryBindInfo m_struct;
    const VkSparseImageMemoryBindInfo* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vksparseimagememoryrequirements_struct_wrapper
{
public:
    vksparseimagememoryrequirements_struct_wrapper();
    vksparseimagememoryrequirements_struct_wrapper(VkSparseImageMemoryRequirements* pInStruct);
    vksparseimagememoryrequirements_struct_wrapper(const VkSparseImageMemoryRequirements* pInStruct);

    virtual ~vksparseimagememoryrequirements_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkSparseImageFormatProperties get_formatProperties() { return m_struct.formatProperties; }
    void set_formatProperties(VkSparseImageFormatProperties inValue) { m_struct.formatProperties = inValue; }
    uint32_t get_imageMipTailFirstLod() { return m_struct.imageMipTailFirstLod; }
    void set_imageMipTailFirstLod(uint32_t inValue) { m_struct.imageMipTailFirstLod = inValue; }
    VkDeviceSize get_imageMipTailSize() { return m_struct.imageMipTailSize; }
    void set_imageMipTailSize(VkDeviceSize inValue) { m_struct.imageMipTailSize = inValue; }
    VkDeviceSize get_imageMipTailOffset() { return m_struct.imageMipTailOffset; }
    void set_imageMipTailOffset(VkDeviceSize inValue) { m_struct.imageMipTailOffset = inValue; }
    VkDeviceSize get_imageMipTailStride() { return m_struct.imageMipTailStride; }
    void set_imageMipTailStride(VkDeviceSize inValue) { m_struct.imageMipTailStride = inValue; }


private:
    VkSparseImageMemoryRequirements m_struct;
    const VkSparseImageMemoryRequirements* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vksparseimageopaquememorybindinfo_struct_wrapper
{
public:
    vksparseimageopaquememorybindinfo_struct_wrapper();
    vksparseimageopaquememorybindinfo_struct_wrapper(VkSparseImageOpaqueMemoryBindInfo* pInStruct);
    vksparseimageopaquememorybindinfo_struct_wrapper(const VkSparseImageOpaqueMemoryBindInfo* pInStruct);

    virtual ~vksparseimageopaquememorybindinfo_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkImage get_image() { return m_struct.image; }
    void set_image(VkImage inValue) { m_struct.image = inValue; }
    uint32_t get_bindCount() { return m_struct.bindCount; }
    void set_bindCount(uint32_t inValue) { m_struct.bindCount = inValue; }


private:
    VkSparseImageOpaqueMemoryBindInfo m_struct;
    const VkSparseImageOpaqueMemoryBindInfo* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vksparsememorybind_struct_wrapper
{
public:
    vksparsememorybind_struct_wrapper();
    vksparsememorybind_struct_wrapper(VkSparseMemoryBind* pInStruct);
    vksparsememorybind_struct_wrapper(const VkSparseMemoryBind* pInStruct);

    virtual ~vksparsememorybind_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkDeviceSize get_resourceOffset() { return m_struct.resourceOffset; }
    void set_resourceOffset(VkDeviceSize inValue) { m_struct.resourceOffset = inValue; }
    VkDeviceSize get_size() { return m_struct.size; }
    void set_size(VkDeviceSize inValue) { m_struct.size = inValue; }
    VkDeviceMemory get_memory() { return m_struct.memory; }
    void set_memory(VkDeviceMemory inValue) { m_struct.memory = inValue; }
    VkDeviceSize get_memoryOffset() { return m_struct.memoryOffset; }
    void set_memoryOffset(VkDeviceSize inValue) { m_struct.memoryOffset = inValue; }
    VkSparseMemoryBindFlags get_flags() { return m_struct.flags; }
    void set_flags(VkSparseMemoryBindFlags inValue) { m_struct.flags = inValue; }


private:
    VkSparseMemoryBind m_struct;
    const VkSparseMemoryBind* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkspecializationinfo_struct_wrapper
{
public:
    vkspecializationinfo_struct_wrapper();
    vkspecializationinfo_struct_wrapper(VkSpecializationInfo* pInStruct);
    vkspecializationinfo_struct_wrapper(const VkSpecializationInfo* pInStruct);

    virtual ~vkspecializationinfo_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    uint32_t get_mapEntryCount() { return m_struct.mapEntryCount; }
    void set_mapEntryCount(uint32_t inValue) { m_struct.mapEntryCount = inValue; }
    size_t get_dataSize() { return m_struct.dataSize; }
    void set_dataSize(size_t inValue) { m_struct.dataSize = inValue; }
    const void* get_pData() { return m_struct.pData; }


private:
    VkSpecializationInfo m_struct;
    const VkSpecializationInfo* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkspecializationmapentry_struct_wrapper
{
public:
    vkspecializationmapentry_struct_wrapper();
    vkspecializationmapentry_struct_wrapper(VkSpecializationMapEntry* pInStruct);
    vkspecializationmapentry_struct_wrapper(const VkSpecializationMapEntry* pInStruct);

    virtual ~vkspecializationmapentry_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    uint32_t get_constantID() { return m_struct.constantID; }
    void set_constantID(uint32_t inValue) { m_struct.constantID = inValue; }
    uint32_t get_offset() { return m_struct.offset; }
    void set_offset(uint32_t inValue) { m_struct.offset = inValue; }
    size_t get_size() { return m_struct.size; }
    void set_size(size_t inValue) { m_struct.size = inValue; }


private:
    VkSpecializationMapEntry m_struct;
    const VkSpecializationMapEntry* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkstencilopstate_struct_wrapper
{
public:
    vkstencilopstate_struct_wrapper();
    vkstencilopstate_struct_wrapper(VkStencilOpState* pInStruct);
    vkstencilopstate_struct_wrapper(const VkStencilOpState* pInStruct);

    virtual ~vkstencilopstate_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStencilOp get_failOp() { return m_struct.failOp; }
    void set_failOp(VkStencilOp inValue) { m_struct.failOp = inValue; }
    VkStencilOp get_passOp() { return m_struct.passOp; }
    void set_passOp(VkStencilOp inValue) { m_struct.passOp = inValue; }
    VkStencilOp get_depthFailOp() { return m_struct.depthFailOp; }
    void set_depthFailOp(VkStencilOp inValue) { m_struct.depthFailOp = inValue; }
    VkCompareOp get_compareOp() { return m_struct.compareOp; }
    void set_compareOp(VkCompareOp inValue) { m_struct.compareOp = inValue; }
    uint32_t get_compareMask() { return m_struct.compareMask; }
    void set_compareMask(uint32_t inValue) { m_struct.compareMask = inValue; }
    uint32_t get_writeMask() { return m_struct.writeMask; }
    void set_writeMask(uint32_t inValue) { m_struct.writeMask = inValue; }
    uint32_t get_reference() { return m_struct.reference; }
    void set_reference(uint32_t inValue) { m_struct.reference = inValue; }


private:
    VkStencilOpState m_struct;
    const VkStencilOpState* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vksubmitinfo_struct_wrapper
{
public:
    vksubmitinfo_struct_wrapper();
    vksubmitinfo_struct_wrapper(VkSubmitInfo* pInStruct);
    vksubmitinfo_struct_wrapper(const VkSubmitInfo* pInStruct);

    virtual ~vksubmitinfo_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    uint32_t get_waitSemaphoreCount() { return m_struct.waitSemaphoreCount; }
    void set_waitSemaphoreCount(uint32_t inValue) { m_struct.waitSemaphoreCount = inValue; }
    const VkPipelineStageFlags* get_pWaitDstStageMask() { return m_struct.pWaitDstStageMask; }
    uint32_t get_commandBufferCount() { return m_struct.commandBufferCount; }
    void set_commandBufferCount(uint32_t inValue) { m_struct.commandBufferCount = inValue; }
    uint32_t get_signalSemaphoreCount() { return m_struct.signalSemaphoreCount; }
    void set_signalSemaphoreCount(uint32_t inValue) { m_struct.signalSemaphoreCount = inValue; }


private:
    VkSubmitInfo m_struct;
    const VkSubmitInfo* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vksubpassdependency_struct_wrapper
{
public:
    vksubpassdependency_struct_wrapper();
    vksubpassdependency_struct_wrapper(VkSubpassDependency* pInStruct);
    vksubpassdependency_struct_wrapper(const VkSubpassDependency* pInStruct);

    virtual ~vksubpassdependency_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    uint32_t get_srcSubpass() { return m_struct.srcSubpass; }
    void set_srcSubpass(uint32_t inValue) { m_struct.srcSubpass = inValue; }
    uint32_t get_dstSubpass() { return m_struct.dstSubpass; }
    void set_dstSubpass(uint32_t inValue) { m_struct.dstSubpass = inValue; }
    VkPipelineStageFlags get_srcStageMask() { return m_struct.srcStageMask; }
    void set_srcStageMask(VkPipelineStageFlags inValue) { m_struct.srcStageMask = inValue; }
    VkPipelineStageFlags get_dstStageMask() { return m_struct.dstStageMask; }
    void set_dstStageMask(VkPipelineStageFlags inValue) { m_struct.dstStageMask = inValue; }
    VkAccessFlags get_srcAccessMask() { return m_struct.srcAccessMask; }
    void set_srcAccessMask(VkAccessFlags inValue) { m_struct.srcAccessMask = inValue; }
    VkAccessFlags get_dstAccessMask() { return m_struct.dstAccessMask; }
    void set_dstAccessMask(VkAccessFlags inValue) { m_struct.dstAccessMask = inValue; }
    VkDependencyFlags get_dependencyFlags() { return m_struct.dependencyFlags; }
    void set_dependencyFlags(VkDependencyFlags inValue) { m_struct.dependencyFlags = inValue; }


private:
    VkSubpassDependency m_struct;
    const VkSubpassDependency* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vksubpassdescription_struct_wrapper
{
public:
    vksubpassdescription_struct_wrapper();
    vksubpassdescription_struct_wrapper(VkSubpassDescription* pInStruct);
    vksubpassdescription_struct_wrapper(const VkSubpassDescription* pInStruct);

    virtual ~vksubpassdescription_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkSubpassDescriptionFlags get_flags() { return m_struct.flags; }
    void set_flags(VkSubpassDescriptionFlags inValue) { m_struct.flags = inValue; }
    VkPipelineBindPoint get_pipelineBindPoint() { return m_struct.pipelineBindPoint; }
    void set_pipelineBindPoint(VkPipelineBindPoint inValue) { m_struct.pipelineBindPoint = inValue; }
    uint32_t get_inputAttachmentCount() { return m_struct.inputAttachmentCount; }
    void set_inputAttachmentCount(uint32_t inValue) { m_struct.inputAttachmentCount = inValue; }
    uint32_t get_colorAttachmentCount() { return m_struct.colorAttachmentCount; }
    void set_colorAttachmentCount(uint32_t inValue) { m_struct.colorAttachmentCount = inValue; }
    const VkAttachmentReference* get_pDepthStencilAttachment() { return m_struct.pDepthStencilAttachment; }
    uint32_t get_preserveAttachmentCount() { return m_struct.preserveAttachmentCount; }
    void set_preserveAttachmentCount(uint32_t inValue) { m_struct.preserveAttachmentCount = inValue; }


private:
    VkSubpassDescription m_struct;
    const VkSubpassDescription* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vksubresourcelayout_struct_wrapper
{
public:
    vksubresourcelayout_struct_wrapper();
    vksubresourcelayout_struct_wrapper(VkSubresourceLayout* pInStruct);
    vksubresourcelayout_struct_wrapper(const VkSubresourceLayout* pInStruct);

    virtual ~vksubresourcelayout_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkDeviceSize get_offset() { return m_struct.offset; }
    void set_offset(VkDeviceSize inValue) { m_struct.offset = inValue; }
    VkDeviceSize get_size() { return m_struct.size; }
    void set_size(VkDeviceSize inValue) { m_struct.size = inValue; }
    VkDeviceSize get_rowPitch() { return m_struct.rowPitch; }
    void set_rowPitch(VkDeviceSize inValue) { m_struct.rowPitch = inValue; }
    VkDeviceSize get_arrayPitch() { return m_struct.arrayPitch; }
    void set_arrayPitch(VkDeviceSize inValue) { m_struct.arrayPitch = inValue; }
    VkDeviceSize get_depthPitch() { return m_struct.depthPitch; }
    void set_depthPitch(VkDeviceSize inValue) { m_struct.depthPitch = inValue; }


private:
    VkSubresourceLayout m_struct;
    const VkSubresourceLayout* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vksurfacecapabilitieskhr_struct_wrapper
{
public:
    vksurfacecapabilitieskhr_struct_wrapper();
    vksurfacecapabilitieskhr_struct_wrapper(VkSurfaceCapabilitiesKHR* pInStruct);
    vksurfacecapabilitieskhr_struct_wrapper(const VkSurfaceCapabilitiesKHR* pInStruct);

    virtual ~vksurfacecapabilitieskhr_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    uint32_t get_minImageCount() { return m_struct.minImageCount; }
    void set_minImageCount(uint32_t inValue) { m_struct.minImageCount = inValue; }
    uint32_t get_maxImageCount() { return m_struct.maxImageCount; }
    void set_maxImageCount(uint32_t inValue) { m_struct.maxImageCount = inValue; }
    VkExtent2D get_currentExtent() { return m_struct.currentExtent; }
    void set_currentExtent(VkExtent2D inValue) { m_struct.currentExtent = inValue; }
    VkExtent2D get_minImageExtent() { return m_struct.minImageExtent; }
    void set_minImageExtent(VkExtent2D inValue) { m_struct.minImageExtent = inValue; }
    VkExtent2D get_maxImageExtent() { return m_struct.maxImageExtent; }
    void set_maxImageExtent(VkExtent2D inValue) { m_struct.maxImageExtent = inValue; }
    uint32_t get_maxImageArrayLayers() { return m_struct.maxImageArrayLayers; }
    void set_maxImageArrayLayers(uint32_t inValue) { m_struct.maxImageArrayLayers = inValue; }
    VkSurfaceTransformFlagsKHR get_supportedTransforms() { return m_struct.supportedTransforms; }
    void set_supportedTransforms(VkSurfaceTransformFlagsKHR inValue) { m_struct.supportedTransforms = inValue; }
    VkSurfaceTransformFlagBitsKHR get_currentTransform() { return m_struct.currentTransform; }
    void set_currentTransform(VkSurfaceTransformFlagBitsKHR inValue) { m_struct.currentTransform = inValue; }
    VkCompositeAlphaFlagsKHR get_supportedCompositeAlpha() { return m_struct.supportedCompositeAlpha; }
    void set_supportedCompositeAlpha(VkCompositeAlphaFlagsKHR inValue) { m_struct.supportedCompositeAlpha = inValue; }
    VkImageUsageFlags get_supportedUsageFlags() { return m_struct.supportedUsageFlags; }
    void set_supportedUsageFlags(VkImageUsageFlags inValue) { m_struct.supportedUsageFlags = inValue; }


private:
    VkSurfaceCapabilitiesKHR m_struct;
    const VkSurfaceCapabilitiesKHR* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vksurfaceformatkhr_struct_wrapper
{
public:
    vksurfaceformatkhr_struct_wrapper();
    vksurfaceformatkhr_struct_wrapper(VkSurfaceFormatKHR* pInStruct);
    vksurfaceformatkhr_struct_wrapper(const VkSurfaceFormatKHR* pInStruct);

    virtual ~vksurfaceformatkhr_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkFormat get_format() { return m_struct.format; }
    void set_format(VkFormat inValue) { m_struct.format = inValue; }
    VkColorSpaceKHR get_colorSpace() { return m_struct.colorSpace; }
    void set_colorSpace(VkColorSpaceKHR inValue) { m_struct.colorSpace = inValue; }


private:
    VkSurfaceFormatKHR m_struct;
    const VkSurfaceFormatKHR* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkswapchaincreateinfokhr_struct_wrapper
{
public:
    vkswapchaincreateinfokhr_struct_wrapper();
    vkswapchaincreateinfokhr_struct_wrapper(VkSwapchainCreateInfoKHR* pInStruct);
    vkswapchaincreateinfokhr_struct_wrapper(const VkSwapchainCreateInfoKHR* pInStruct);

    virtual ~vkswapchaincreateinfokhr_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkSwapchainCreateFlagsKHR get_flags() { return m_struct.flags; }
    void set_flags(VkSwapchainCreateFlagsKHR inValue) { m_struct.flags = inValue; }
    VkSurfaceKHR get_surface() { return m_struct.surface; }
    void set_surface(VkSurfaceKHR inValue) { m_struct.surface = inValue; }
    uint32_t get_minImageCount() { return m_struct.minImageCount; }
    void set_minImageCount(uint32_t inValue) { m_struct.minImageCount = inValue; }
    VkFormat get_imageFormat() { return m_struct.imageFormat; }
    void set_imageFormat(VkFormat inValue) { m_struct.imageFormat = inValue; }
    VkColorSpaceKHR get_imageColorSpace() { return m_struct.imageColorSpace; }
    void set_imageColorSpace(VkColorSpaceKHR inValue) { m_struct.imageColorSpace = inValue; }
    VkExtent2D get_imageExtent() { return m_struct.imageExtent; }
    void set_imageExtent(VkExtent2D inValue) { m_struct.imageExtent = inValue; }
    uint32_t get_imageArrayLayers() { return m_struct.imageArrayLayers; }
    void set_imageArrayLayers(uint32_t inValue) { m_struct.imageArrayLayers = inValue; }
    VkImageUsageFlags get_imageUsage() { return m_struct.imageUsage; }
    void set_imageUsage(VkImageUsageFlags inValue) { m_struct.imageUsage = inValue; }
    VkSharingMode get_imageSharingMode() { return m_struct.imageSharingMode; }
    void set_imageSharingMode(VkSharingMode inValue) { m_struct.imageSharingMode = inValue; }
    uint32_t get_queueFamilyIndexCount() { return m_struct.queueFamilyIndexCount; }
    void set_queueFamilyIndexCount(uint32_t inValue) { m_struct.queueFamilyIndexCount = inValue; }
    VkSurfaceTransformFlagBitsKHR get_preTransform() { return m_struct.preTransform; }
    void set_preTransform(VkSurfaceTransformFlagBitsKHR inValue) { m_struct.preTransform = inValue; }
    VkCompositeAlphaFlagBitsKHR get_compositeAlpha() { return m_struct.compositeAlpha; }
    void set_compositeAlpha(VkCompositeAlphaFlagBitsKHR inValue) { m_struct.compositeAlpha = inValue; }
    VkPresentModeKHR get_presentMode() { return m_struct.presentMode; }
    void set_presentMode(VkPresentModeKHR inValue) { m_struct.presentMode = inValue; }
    VkBool32 get_clipped() { return m_struct.clipped; }
    void set_clipped(VkBool32 inValue) { m_struct.clipped = inValue; }
    VkSwapchainKHR get_oldSwapchain() { return m_struct.oldSwapchain; }
    void set_oldSwapchain(VkSwapchainKHR inValue) { m_struct.oldSwapchain = inValue; }


private:
    VkSwapchainCreateInfoKHR m_struct;
    const VkSwapchainCreateInfoKHR* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkvalidationflagsext_struct_wrapper
{
public:
    vkvalidationflagsext_struct_wrapper();
    vkvalidationflagsext_struct_wrapper(VkValidationFlagsEXT* pInStruct);
    vkvalidationflagsext_struct_wrapper(const VkValidationFlagsEXT* pInStruct);

    virtual ~vkvalidationflagsext_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    uint32_t get_disabledValidationCheckCount() { return m_struct.disabledValidationCheckCount; }
    void set_disabledValidationCheckCount(uint32_t inValue) { m_struct.disabledValidationCheckCount = inValue; }
    VkValidationCheckEXT* get_pDisabledValidationChecks() { return m_struct.pDisabledValidationChecks; }
    void set_pDisabledValidationChecks(VkValidationCheckEXT* inValue) { m_struct.pDisabledValidationChecks = inValue; }


private:
    VkValidationFlagsEXT m_struct;
    const VkValidationFlagsEXT* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkvertexinputattributedescription_struct_wrapper
{
public:
    vkvertexinputattributedescription_struct_wrapper();
    vkvertexinputattributedescription_struct_wrapper(VkVertexInputAttributeDescription* pInStruct);
    vkvertexinputattributedescription_struct_wrapper(const VkVertexInputAttributeDescription* pInStruct);

    virtual ~vkvertexinputattributedescription_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    uint32_t get_location() { return m_struct.location; }
    void set_location(uint32_t inValue) { m_struct.location = inValue; }
    uint32_t get_binding() { return m_struct.binding; }
    void set_binding(uint32_t inValue) { m_struct.binding = inValue; }
    VkFormat get_format() { return m_struct.format; }
    void set_format(VkFormat inValue) { m_struct.format = inValue; }
    uint32_t get_offset() { return m_struct.offset; }
    void set_offset(uint32_t inValue) { m_struct.offset = inValue; }


private:
    VkVertexInputAttributeDescription m_struct;
    const VkVertexInputAttributeDescription* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkvertexinputbindingdescription_struct_wrapper
{
public:
    vkvertexinputbindingdescription_struct_wrapper();
    vkvertexinputbindingdescription_struct_wrapper(VkVertexInputBindingDescription* pInStruct);
    vkvertexinputbindingdescription_struct_wrapper(const VkVertexInputBindingDescription* pInStruct);

    virtual ~vkvertexinputbindingdescription_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    uint32_t get_binding() { return m_struct.binding; }
    void set_binding(uint32_t inValue) { m_struct.binding = inValue; }
    uint32_t get_stride() { return m_struct.stride; }
    void set_stride(uint32_t inValue) { m_struct.stride = inValue; }
    VkVertexInputRate get_inputRate() { return m_struct.inputRate; }
    void set_inputRate(VkVertexInputRate inValue) { m_struct.inputRate = inValue; }


private:
    VkVertexInputBindingDescription m_struct;
    const VkVertexInputBindingDescription* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkviewport_struct_wrapper
{
public:
    vkviewport_struct_wrapper();
    vkviewport_struct_wrapper(VkViewport* pInStruct);
    vkviewport_struct_wrapper(const VkViewport* pInStruct);

    virtual ~vkviewport_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    float get_x() { return m_struct.x; }
    void set_x(float inValue) { m_struct.x = inValue; }
    float get_y() { return m_struct.y; }
    void set_y(float inValue) { m_struct.y = inValue; }
    float get_width() { return m_struct.width; }
    void set_width(float inValue) { m_struct.width = inValue; }
    float get_height() { return m_struct.height; }
    void set_height(float inValue) { m_struct.height = inValue; }
    float get_minDepth() { return m_struct.minDepth; }
    void set_minDepth(float inValue) { m_struct.minDepth = inValue; }
    float get_maxDepth() { return m_struct.maxDepth; }
    void set_maxDepth(float inValue) { m_struct.maxDepth = inValue; }


private:
    VkViewport m_struct;
    const VkViewport* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkwaylandsurfacecreateinfokhr_struct_wrapper
{
public:
    vkwaylandsurfacecreateinfokhr_struct_wrapper();
    vkwaylandsurfacecreateinfokhr_struct_wrapper(VkWaylandSurfaceCreateInfoKHR* pInStruct);
    vkwaylandsurfacecreateinfokhr_struct_wrapper(const VkWaylandSurfaceCreateInfoKHR* pInStruct);

    virtual ~vkwaylandsurfacecreateinfokhr_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkWaylandSurfaceCreateFlagsKHR get_flags() { return m_struct.flags; }
    void set_flags(VkWaylandSurfaceCreateFlagsKHR inValue) { m_struct.flags = inValue; }
    struct wl_display* get_display() { return m_struct.display; }
    void set_display(struct wl_display* inValue) { m_struct.display = inValue; }
    struct wl_surface* get_surface() { return m_struct.surface; }
    void set_surface(struct wl_surface* inValue) { m_struct.surface = inValue; }


private:
    VkWaylandSurfaceCreateInfoKHR m_struct;
    const VkWaylandSurfaceCreateInfoKHR* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkwin32keyedmutexacquirereleaseinfonv_struct_wrapper
{
public:
    vkwin32keyedmutexacquirereleaseinfonv_struct_wrapper();
    vkwin32keyedmutexacquirereleaseinfonv_struct_wrapper(VkWin32KeyedMutexAcquireReleaseInfoNV* pInStruct);
    vkwin32keyedmutexacquirereleaseinfonv_struct_wrapper(const VkWin32KeyedMutexAcquireReleaseInfoNV* pInStruct);

    virtual ~vkwin32keyedmutexacquirereleaseinfonv_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    uint32_t get_acquireCount() { return m_struct.acquireCount; }
    void set_acquireCount(uint32_t inValue) { m_struct.acquireCount = inValue; }
    uint32_t get_releaseCount() { return m_struct.releaseCount; }
    void set_releaseCount(uint32_t inValue) { m_struct.releaseCount = inValue; }


private:
    VkWin32KeyedMutexAcquireReleaseInfoNV m_struct;
    const VkWin32KeyedMutexAcquireReleaseInfoNV* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkwin32surfacecreateinfokhr_struct_wrapper
{
public:
    vkwin32surfacecreateinfokhr_struct_wrapper();
    vkwin32surfacecreateinfokhr_struct_wrapper(VkWin32SurfaceCreateInfoKHR* pInStruct);
    vkwin32surfacecreateinfokhr_struct_wrapper(const VkWin32SurfaceCreateInfoKHR* pInStruct);

    virtual ~vkwin32surfacecreateinfokhr_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkWin32SurfaceCreateFlagsKHR get_flags() { return m_struct.flags; }
    void set_flags(VkWin32SurfaceCreateFlagsKHR inValue) { m_struct.flags = inValue; }
    HINSTANCE get_hinstance() { return m_struct.hinstance; }
    void set_hinstance(HINSTANCE inValue) { m_struct.hinstance = inValue; }
    HWND get_hwnd() { return m_struct.hwnd; }
    void set_hwnd(HWND inValue) { m_struct.hwnd = inValue; }


private:
    VkWin32SurfaceCreateInfoKHR m_struct;
    const VkWin32SurfaceCreateInfoKHR* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkwritedescriptorset_struct_wrapper
{
public:
    vkwritedescriptorset_struct_wrapper();
    vkwritedescriptorset_struct_wrapper(VkWriteDescriptorSet* pInStruct);
    vkwritedescriptorset_struct_wrapper(const VkWriteDescriptorSet* pInStruct);

    virtual ~vkwritedescriptorset_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkDescriptorSet get_dstSet() { return m_struct.dstSet; }
    void set_dstSet(VkDescriptorSet inValue) { m_struct.dstSet = inValue; }
    uint32_t get_dstBinding() { return m_struct.dstBinding; }
    void set_dstBinding(uint32_t inValue) { m_struct.dstBinding = inValue; }
    uint32_t get_dstArrayElement() { return m_struct.dstArrayElement; }
    void set_dstArrayElement(uint32_t inValue) { m_struct.dstArrayElement = inValue; }
    uint32_t get_descriptorCount() { return m_struct.descriptorCount; }
    void set_descriptorCount(uint32_t inValue) { m_struct.descriptorCount = inValue; }
    VkDescriptorType get_descriptorType() { return m_struct.descriptorType; }
    void set_descriptorType(VkDescriptorType inValue) { m_struct.descriptorType = inValue; }


private:
    VkWriteDescriptorSet m_struct;
    const VkWriteDescriptorSet* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkxcbsurfacecreateinfokhr_struct_wrapper
{
public:
    vkxcbsurfacecreateinfokhr_struct_wrapper();
    vkxcbsurfacecreateinfokhr_struct_wrapper(VkXcbSurfaceCreateInfoKHR* pInStruct);
    vkxcbsurfacecreateinfokhr_struct_wrapper(const VkXcbSurfaceCreateInfoKHR* pInStruct);

    virtual ~vkxcbsurfacecreateinfokhr_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkXcbSurfaceCreateFlagsKHR get_flags() { return m_struct.flags; }
    void set_flags(VkXcbSurfaceCreateFlagsKHR inValue) { m_struct.flags = inValue; }
    xcb_connection_t* get_connection() { return m_struct.connection; }
    void set_connection(xcb_connection_t* inValue) { m_struct.connection = inValue; }
    xcb_window_t get_window() { return m_struct.window; }
    void set_window(xcb_window_t inValue) { m_struct.window = inValue; }


private:
    VkXcbSurfaceCreateInfoKHR m_struct;
    const VkXcbSurfaceCreateInfoKHR* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class vkxlibsurfacecreateinfokhr_struct_wrapper
{
public:
    vkxlibsurfacecreateinfokhr_struct_wrapper();
    vkxlibsurfacecreateinfokhr_struct_wrapper(VkXlibSurfaceCreateInfoKHR* pInStruct);
    vkxlibsurfacecreateinfokhr_struct_wrapper(const VkXlibSurfaceCreateInfoKHR* pInStruct);

    virtual ~vkxlibsurfacecreateinfokhr_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    VkStructureType get_sType() { return m_struct.sType; }
    void set_sType(VkStructureType inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    VkXlibSurfaceCreateFlagsKHR get_flags() { return m_struct.flags; }
    void set_flags(VkXlibSurfaceCreateFlagsKHR inValue) { m_struct.flags = inValue; }
    Display* get_dpy() { return m_struct.dpy; }
    void set_dpy(Display* inValue) { m_struct.dpy = inValue; }
    Window get_window() { return m_struct.window; }
    void set_window(Window inValue) { m_struct.window = inValue; }


private:
    VkXlibSurfaceCreateInfoKHR m_struct;
    const VkXlibSurfaceCreateInfoKHR* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};

//any footer info for class
