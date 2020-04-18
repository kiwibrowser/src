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
#include <stdio.h>
#include <vk_struct_wrappers.h>
#include <vk_enum_string_helper.h>

// vkallocationcallbacks_struct_wrapper class definition
vkallocationcallbacks_struct_wrapper::vkallocationcallbacks_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkallocationcallbacks_struct_wrapper::vkallocationcallbacks_struct_wrapper(VkAllocationCallbacks* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkallocationcallbacks_struct_wrapper::vkallocationcallbacks_struct_wrapper(const VkAllocationCallbacks* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkallocationcallbacks_struct_wrapper::~vkallocationcallbacks_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkallocationcallbacks_struct_wrapper::display_single_txt()
{
    printf(" %*sVkAllocationCallbacks = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkallocationcallbacks_struct_wrapper::display_struct_members()
{
    printf("%*s    %spUserData = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pUserData));
    printf("%*s    %spfnAllocation = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.pfnAllocation));
    printf("%*s    %spfnReallocation = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.pfnReallocation));
    printf("%*s    %spfnFree = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.pfnFree));
    printf("%*s    %spfnInternalAllocation = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.pfnInternalAllocation));
    printf("%*s    %spfnInternalFree = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.pfnInternalFree));
}

// Output all struct elements, each on their own line
void vkallocationcallbacks_struct_wrapper::display_txt()
{
    printf("%*sVkAllocationCallbacks struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkallocationcallbacks_struct_wrapper::display_full_txt()
{
    printf("%*sVkAllocationCallbacks struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// vkandroidsurfacecreateinfokhr_struct_wrapper class definition
vkandroidsurfacecreateinfokhr_struct_wrapper::vkandroidsurfacecreateinfokhr_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkandroidsurfacecreateinfokhr_struct_wrapper::vkandroidsurfacecreateinfokhr_struct_wrapper(VkAndroidSurfaceCreateInfoKHR* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkandroidsurfacecreateinfokhr_struct_wrapper::vkandroidsurfacecreateinfokhr_struct_wrapper(const VkAndroidSurfaceCreateInfoKHR* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkandroidsurfacecreateinfokhr_struct_wrapper::~vkandroidsurfacecreateinfokhr_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkandroidsurfacecreateinfokhr_struct_wrapper::display_single_txt()
{
    printf(" %*sVkAndroidSurfaceCreateInfoKHR = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkandroidsurfacecreateinfokhr_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
    printf("%*s    %swindow = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.window));
}

// Output all struct elements, each on their own line
void vkandroidsurfacecreateinfokhr_struct_wrapper::display_txt()
{
    printf("%*sVkAndroidSurfaceCreateInfoKHR struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkandroidsurfacecreateinfokhr_struct_wrapper::display_full_txt()
{
    printf("%*sVkAndroidSurfaceCreateInfoKHR struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkapplicationinfo_struct_wrapper class definition
vkapplicationinfo_struct_wrapper::vkapplicationinfo_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkapplicationinfo_struct_wrapper::vkapplicationinfo_struct_wrapper(VkApplicationInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkapplicationinfo_struct_wrapper::vkapplicationinfo_struct_wrapper(const VkApplicationInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkapplicationinfo_struct_wrapper::~vkapplicationinfo_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkapplicationinfo_struct_wrapper::display_single_txt()
{
    printf(" %*sVkApplicationInfo = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkapplicationinfo_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %spApplicationName = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pApplicationName));
    printf("%*s    %sapplicationVersion = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.applicationVersion));
    printf("%*s    %spEngineName = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pEngineName));
    printf("%*s    %sengineVersion = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.engineVersion));
    printf("%*s    %sapiVersion = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.apiVersion));
}

// Output all struct elements, each on their own line
void vkapplicationinfo_struct_wrapper::display_txt()
{
    printf("%*sVkApplicationInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkapplicationinfo_struct_wrapper::display_full_txt()
{
    printf("%*sVkApplicationInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkattachmentdescription_struct_wrapper class definition
vkattachmentdescription_struct_wrapper::vkattachmentdescription_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkattachmentdescription_struct_wrapper::vkattachmentdescription_struct_wrapper(VkAttachmentDescription* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkattachmentdescription_struct_wrapper::vkattachmentdescription_struct_wrapper(const VkAttachmentDescription* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkattachmentdescription_struct_wrapper::~vkattachmentdescription_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkattachmentdescription_struct_wrapper::display_single_txt()
{
    printf(" %*sVkAttachmentDescription = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkattachmentdescription_struct_wrapper::display_struct_members()
{
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
    printf("%*s    %sformat = %s\n", m_indent, "", &m_dummy_prefix, string_VkFormat(m_struct.format));
    printf("%*s    %ssamples = %s\n", m_indent, "", &m_dummy_prefix, string_VkSampleCountFlagBits(m_struct.samples));
    printf("%*s    %sloadOp = %s\n", m_indent, "", &m_dummy_prefix, string_VkAttachmentLoadOp(m_struct.loadOp));
    printf("%*s    %sstoreOp = %s\n", m_indent, "", &m_dummy_prefix, string_VkAttachmentStoreOp(m_struct.storeOp));
    printf("%*s    %sstencilLoadOp = %s\n", m_indent, "", &m_dummy_prefix, string_VkAttachmentLoadOp(m_struct.stencilLoadOp));
    printf("%*s    %sstencilStoreOp = %s\n", m_indent, "", &m_dummy_prefix, string_VkAttachmentStoreOp(m_struct.stencilStoreOp));
    printf("%*s    %sinitialLayout = %s\n", m_indent, "", &m_dummy_prefix, string_VkImageLayout(m_struct.initialLayout));
    printf("%*s    %sfinalLayout = %s\n", m_indent, "", &m_dummy_prefix, string_VkImageLayout(m_struct.finalLayout));
}

// Output all struct elements, each on their own line
void vkattachmentdescription_struct_wrapper::display_txt()
{
    printf("%*sVkAttachmentDescription struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkattachmentdescription_struct_wrapper::display_full_txt()
{
    printf("%*sVkAttachmentDescription struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// vkattachmentreference_struct_wrapper class definition
vkattachmentreference_struct_wrapper::vkattachmentreference_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkattachmentreference_struct_wrapper::vkattachmentreference_struct_wrapper(VkAttachmentReference* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkattachmentreference_struct_wrapper::vkattachmentreference_struct_wrapper(const VkAttachmentReference* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkattachmentreference_struct_wrapper::~vkattachmentreference_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkattachmentreference_struct_wrapper::display_single_txt()
{
    printf(" %*sVkAttachmentReference = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkattachmentreference_struct_wrapper::display_struct_members()
{
    printf("%*s    %sattachment = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.attachment));
    printf("%*s    %slayout = %s\n", m_indent, "", &m_dummy_prefix, string_VkImageLayout(m_struct.layout));
}

// Output all struct elements, each on their own line
void vkattachmentreference_struct_wrapper::display_txt()
{
    printf("%*sVkAttachmentReference struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkattachmentreference_struct_wrapper::display_full_txt()
{
    printf("%*sVkAttachmentReference struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// vkbindsparseinfo_struct_wrapper class definition
vkbindsparseinfo_struct_wrapper::vkbindsparseinfo_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkbindsparseinfo_struct_wrapper::vkbindsparseinfo_struct_wrapper(VkBindSparseInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkbindsparseinfo_struct_wrapper::vkbindsparseinfo_struct_wrapper(const VkBindSparseInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkbindsparseinfo_struct_wrapper::~vkbindsparseinfo_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkbindsparseinfo_struct_wrapper::display_single_txt()
{
    printf(" %*sVkBindSparseInfo = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkbindsparseinfo_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %swaitSemaphoreCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.waitSemaphoreCount));
    uint32_t i;
    for (i = 0; i<waitSemaphoreCount; i++) {
        printf("%*s    %spWaitSemaphores[%u] = 0x%p\n", m_indent, "", &m_dummy_prefix, i, (m_struct.pWaitSemaphores)[i]);
    }
    printf("%*s    %sbufferBindCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.bufferBindCount));
    for (i = 0; i<bufferBindCount; i++) {
        printf("%*s    %spBufferBinds[%u] = 0x%p\n", m_indent, "", &m_dummy_prefix, i, (void*)(m_struct.pBufferBinds)[i]);
    }
    printf("%*s    %simageOpaqueBindCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.imageOpaqueBindCount));
    for (i = 0; i<imageOpaqueBindCount; i++) {
        printf("%*s    %spImageOpaqueBinds[%u] = 0x%p\n", m_indent, "", &m_dummy_prefix, i, (void*)(m_struct.pImageOpaqueBinds)[i]);
    }
    printf("%*s    %simageBindCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.imageBindCount));
    for (i = 0; i<imageBindCount; i++) {
        printf("%*s    %spImageBinds[%u] = 0x%p\n", m_indent, "", &m_dummy_prefix, i, (void*)(m_struct.pImageBinds)[i]);
    }
    printf("%*s    %ssignalSemaphoreCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.signalSemaphoreCount));
    for (i = 0; i<signalSemaphoreCount; i++) {
        printf("%*s    %spSignalSemaphores[%u] = 0x%p\n", m_indent, "", &m_dummy_prefix, i, (m_struct.pSignalSemaphores)[i]);
    }
}

// Output all struct elements, each on their own line
void vkbindsparseinfo_struct_wrapper::display_txt()
{
    printf("%*sVkBindSparseInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkbindsparseinfo_struct_wrapper::display_full_txt()
{
    printf("%*sVkBindSparseInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    uint32_t i;
    for (i = 0; i<imageBindCount; i++) {
            vksparseimagememorybindinfo_struct_wrapper class0(&(m_struct.pImageBinds[i]));
            class0.set_indent(m_indent + 4);
            class0.display_full_txt();
    }
    for (i = 0; i<imageOpaqueBindCount; i++) {
            vksparseimageopaquememorybindinfo_struct_wrapper class1(&(m_struct.pImageOpaqueBinds[i]));
            class1.set_indent(m_indent + 4);
            class1.display_full_txt();
    }
    for (i = 0; i<bufferBindCount; i++) {
            vksparsebuffermemorybindinfo_struct_wrapper class2(&(m_struct.pBufferBinds[i]));
            class2.set_indent(m_indent + 4);
            class2.display_full_txt();
    }
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkbuffercopy_struct_wrapper class definition
vkbuffercopy_struct_wrapper::vkbuffercopy_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkbuffercopy_struct_wrapper::vkbuffercopy_struct_wrapper(VkBufferCopy* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkbuffercopy_struct_wrapper::vkbuffercopy_struct_wrapper(const VkBufferCopy* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkbuffercopy_struct_wrapper::~vkbuffercopy_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkbuffercopy_struct_wrapper::display_single_txt()
{
    printf(" %*sVkBufferCopy = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkbuffercopy_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssrcOffset = " PRINTF_SIZE_T_SPECIFIER "\n", m_indent, "", &m_dummy_prefix, (m_struct.srcOffset));
    printf("%*s    %sdstOffset = " PRINTF_SIZE_T_SPECIFIER "\n", m_indent, "", &m_dummy_prefix, (m_struct.dstOffset));
    printf("%*s    %ssize = " PRINTF_SIZE_T_SPECIFIER "\n", m_indent, "", &m_dummy_prefix, (m_struct.size));
}

// Output all struct elements, each on their own line
void vkbuffercopy_struct_wrapper::display_txt()
{
    printf("%*sVkBufferCopy struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkbuffercopy_struct_wrapper::display_full_txt()
{
    printf("%*sVkBufferCopy struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// vkbuffercreateinfo_struct_wrapper class definition
vkbuffercreateinfo_struct_wrapper::vkbuffercreateinfo_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkbuffercreateinfo_struct_wrapper::vkbuffercreateinfo_struct_wrapper(VkBufferCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkbuffercreateinfo_struct_wrapper::vkbuffercreateinfo_struct_wrapper(const VkBufferCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkbuffercreateinfo_struct_wrapper::~vkbuffercreateinfo_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkbuffercreateinfo_struct_wrapper::display_single_txt()
{
    printf(" %*sVkBufferCreateInfo = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkbuffercreateinfo_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
    printf("%*s    %ssize = " PRINTF_SIZE_T_SPECIFIER "\n", m_indent, "", &m_dummy_prefix, (m_struct.size));
    printf("%*s    %susage = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.usage));
    printf("%*s    %ssharingMode = %s\n", m_indent, "", &m_dummy_prefix, string_VkSharingMode(m_struct.sharingMode));
    printf("%*s    %squeueFamilyIndexCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.queueFamilyIndexCount));
    uint32_t i;
    for (i = 0; i<queueFamilyIndexCount; i++) {
        printf("%*s    %spQueueFamilyIndices[%u] = %u\n", m_indent, "", &m_dummy_prefix, i, (m_struct.pQueueFamilyIndices)[i]);
    }
}

// Output all struct elements, each on their own line
void vkbuffercreateinfo_struct_wrapper::display_txt()
{
    printf("%*sVkBufferCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkbuffercreateinfo_struct_wrapper::display_full_txt()
{
    printf("%*sVkBufferCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkbufferimagecopy_struct_wrapper class definition
vkbufferimagecopy_struct_wrapper::vkbufferimagecopy_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkbufferimagecopy_struct_wrapper::vkbufferimagecopy_struct_wrapper(VkBufferImageCopy* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkbufferimagecopy_struct_wrapper::vkbufferimagecopy_struct_wrapper(const VkBufferImageCopy* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkbufferimagecopy_struct_wrapper::~vkbufferimagecopy_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkbufferimagecopy_struct_wrapper::display_single_txt()
{
    printf(" %*sVkBufferImageCopy = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkbufferimagecopy_struct_wrapper::display_struct_members()
{
    printf("%*s    %sbufferOffset = " PRINTF_SIZE_T_SPECIFIER "\n", m_indent, "", &m_dummy_prefix, (m_struct.bufferOffset));
    printf("%*s    %sbufferRowLength = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.bufferRowLength));
    printf("%*s    %sbufferImageHeight = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.bufferImageHeight));
    printf("%*s    %simageSubresource = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.imageSubresource));
    printf("%*s    %simageOffset = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.imageOffset));
    printf("%*s    %simageExtent = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.imageExtent));
}

// Output all struct elements, each on their own line
void vkbufferimagecopy_struct_wrapper::display_txt()
{
    printf("%*sVkBufferImageCopy struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkbufferimagecopy_struct_wrapper::display_full_txt()
{
    printf("%*sVkBufferImageCopy struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (&m_struct.imageExtent) {
        vkextent3d_struct_wrapper class0(&m_struct.imageExtent);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
    if (&m_struct.imageOffset) {
        vkoffset3d_struct_wrapper class1(&m_struct.imageOffset);
        class1.set_indent(m_indent + 4);
        class1.display_full_txt();
    }
    if (&m_struct.imageSubresource) {
        vkimagesubresourcelayers_struct_wrapper class2(&m_struct.imageSubresource);
        class2.set_indent(m_indent + 4);
        class2.display_full_txt();
    }
}


// vkbuffermemorybarrier_struct_wrapper class definition
vkbuffermemorybarrier_struct_wrapper::vkbuffermemorybarrier_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkbuffermemorybarrier_struct_wrapper::vkbuffermemorybarrier_struct_wrapper(VkBufferMemoryBarrier* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkbuffermemorybarrier_struct_wrapper::vkbuffermemorybarrier_struct_wrapper(const VkBufferMemoryBarrier* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkbuffermemorybarrier_struct_wrapper::~vkbuffermemorybarrier_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkbuffermemorybarrier_struct_wrapper::display_single_txt()
{
    printf(" %*sVkBufferMemoryBarrier = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkbuffermemorybarrier_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %ssrcAccessMask = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.srcAccessMask));
    printf("%*s    %sdstAccessMask = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.dstAccessMask));
    printf("%*s    %ssrcQueueFamilyIndex = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.srcQueueFamilyIndex));
    printf("%*s    %sdstQueueFamilyIndex = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.dstQueueFamilyIndex));
    printf("%*s    %sbuffer = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.buffer));
    printf("%*s    %soffset = " PRINTF_SIZE_T_SPECIFIER "\n", m_indent, "", &m_dummy_prefix, (m_struct.offset));
    printf("%*s    %ssize = " PRINTF_SIZE_T_SPECIFIER "\n", m_indent, "", &m_dummy_prefix, (m_struct.size));
}

// Output all struct elements, each on their own line
void vkbuffermemorybarrier_struct_wrapper::display_txt()
{
    printf("%*sVkBufferMemoryBarrier struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkbuffermemorybarrier_struct_wrapper::display_full_txt()
{
    printf("%*sVkBufferMemoryBarrier struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkbufferviewcreateinfo_struct_wrapper class definition
vkbufferviewcreateinfo_struct_wrapper::vkbufferviewcreateinfo_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkbufferviewcreateinfo_struct_wrapper::vkbufferviewcreateinfo_struct_wrapper(VkBufferViewCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkbufferviewcreateinfo_struct_wrapper::vkbufferviewcreateinfo_struct_wrapper(const VkBufferViewCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkbufferviewcreateinfo_struct_wrapper::~vkbufferviewcreateinfo_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkbufferviewcreateinfo_struct_wrapper::display_single_txt()
{
    printf(" %*sVkBufferViewCreateInfo = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkbufferviewcreateinfo_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
    printf("%*s    %sbuffer = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.buffer));
    printf("%*s    %sformat = %s\n", m_indent, "", &m_dummy_prefix, string_VkFormat(m_struct.format));
    printf("%*s    %soffset = " PRINTF_SIZE_T_SPECIFIER "\n", m_indent, "", &m_dummy_prefix, (m_struct.offset));
    printf("%*s    %srange = " PRINTF_SIZE_T_SPECIFIER "\n", m_indent, "", &m_dummy_prefix, (m_struct.range));
}

// Output all struct elements, each on their own line
void vkbufferviewcreateinfo_struct_wrapper::display_txt()
{
    printf("%*sVkBufferViewCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkbufferviewcreateinfo_struct_wrapper::display_full_txt()
{
    printf("%*sVkBufferViewCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkclearattachment_struct_wrapper class definition
vkclearattachment_struct_wrapper::vkclearattachment_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkclearattachment_struct_wrapper::vkclearattachment_struct_wrapper(VkClearAttachment* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkclearattachment_struct_wrapper::vkclearattachment_struct_wrapper(const VkClearAttachment* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkclearattachment_struct_wrapper::~vkclearattachment_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkclearattachment_struct_wrapper::display_single_txt()
{
    printf(" %*sVkClearAttachment = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkclearattachment_struct_wrapper::display_struct_members()
{
    printf("%*s    %saspectMask = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.aspectMask));
    printf("%*s    %scolorAttachment = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.colorAttachment));
    printf("%*s    %sclearValue = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.clearValue));
}

// Output all struct elements, each on their own line
void vkclearattachment_struct_wrapper::display_txt()
{
    printf("%*sVkClearAttachment struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkclearattachment_struct_wrapper::display_full_txt()
{
    printf("%*sVkClearAttachment struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (&m_struct.clearValue) {
        vkclearvalue_struct_wrapper class0(&m_struct.clearValue);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
}


// vkclearcolorvalue_struct_wrapper class definition
vkclearcolorvalue_struct_wrapper::vkclearcolorvalue_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkclearcolorvalue_struct_wrapper::vkclearcolorvalue_struct_wrapper(VkClearColorValue* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkclearcolorvalue_struct_wrapper::vkclearcolorvalue_struct_wrapper(const VkClearColorValue* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkclearcolorvalue_struct_wrapper::~vkclearcolorvalue_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkclearcolorvalue_struct_wrapper::display_single_txt()
{
    printf(" %*sVkClearColorValue = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkclearcolorvalue_struct_wrapper::display_struct_members()
{
    uint32_t i;
    for (i = 0; i<4; i++) {
        printf("%*s    %sfloat32[%u] = %f\n", m_indent, "", &m_dummy_prefix, i, (m_struct.float32)[i]);
    }
    for (i = 0; i<4; i++) {
        printf("%*s    %sint32[%u] = %i\n", m_indent, "", &m_dummy_prefix, i, (m_struct.int32)[i]);
    }
    for (i = 0; i<4; i++) {
        printf("%*s    %suint32[%u] = %u\n", m_indent, "", &m_dummy_prefix, i, (m_struct.uint32)[i]);
    }
}

// Output all struct elements, each on their own line
void vkclearcolorvalue_struct_wrapper::display_txt()
{
    printf("%*sVkClearColorValue struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkclearcolorvalue_struct_wrapper::display_full_txt()
{
    printf("%*sVkClearColorValue struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// vkcleardepthstencilvalue_struct_wrapper class definition
vkcleardepthstencilvalue_struct_wrapper::vkcleardepthstencilvalue_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkcleardepthstencilvalue_struct_wrapper::vkcleardepthstencilvalue_struct_wrapper(VkClearDepthStencilValue* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkcleardepthstencilvalue_struct_wrapper::vkcleardepthstencilvalue_struct_wrapper(const VkClearDepthStencilValue* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkcleardepthstencilvalue_struct_wrapper::~vkcleardepthstencilvalue_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkcleardepthstencilvalue_struct_wrapper::display_single_txt()
{
    printf(" %*sVkClearDepthStencilValue = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkcleardepthstencilvalue_struct_wrapper::display_struct_members()
{
    printf("%*s    %sdepth = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.depth));
    printf("%*s    %sstencil = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.stencil));
}

// Output all struct elements, each on their own line
void vkcleardepthstencilvalue_struct_wrapper::display_txt()
{
    printf("%*sVkClearDepthStencilValue struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkcleardepthstencilvalue_struct_wrapper::display_full_txt()
{
    printf("%*sVkClearDepthStencilValue struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// vkclearrect_struct_wrapper class definition
vkclearrect_struct_wrapper::vkclearrect_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkclearrect_struct_wrapper::vkclearrect_struct_wrapper(VkClearRect* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkclearrect_struct_wrapper::vkclearrect_struct_wrapper(const VkClearRect* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkclearrect_struct_wrapper::~vkclearrect_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkclearrect_struct_wrapper::display_single_txt()
{
    printf(" %*sVkClearRect = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkclearrect_struct_wrapper::display_struct_members()
{
    printf("%*s    %srect = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.rect));
    printf("%*s    %sbaseArrayLayer = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.baseArrayLayer));
    printf("%*s    %slayerCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.layerCount));
}

// Output all struct elements, each on their own line
void vkclearrect_struct_wrapper::display_txt()
{
    printf("%*sVkClearRect struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkclearrect_struct_wrapper::display_full_txt()
{
    printf("%*sVkClearRect struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (&m_struct.rect) {
        vkrect2d_struct_wrapper class0(&m_struct.rect);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
}


// vkclearvalue_struct_wrapper class definition
vkclearvalue_struct_wrapper::vkclearvalue_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkclearvalue_struct_wrapper::vkclearvalue_struct_wrapper(VkClearValue* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkclearvalue_struct_wrapper::vkclearvalue_struct_wrapper(const VkClearValue* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkclearvalue_struct_wrapper::~vkclearvalue_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkclearvalue_struct_wrapper::display_single_txt()
{
    printf(" %*sVkClearValue = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkclearvalue_struct_wrapper::display_struct_members()
{
    printf("%*s    %scolor = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.color));
    printf("%*s    %sdepthStencil = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.depthStencil));
}

// Output all struct elements, each on their own line
void vkclearvalue_struct_wrapper::display_txt()
{
    printf("%*sVkClearValue struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkclearvalue_struct_wrapper::display_full_txt()
{
    printf("%*sVkClearValue struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (&m_struct.depthStencil) {
        vkcleardepthstencilvalue_struct_wrapper class0(&m_struct.depthStencil);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
    if (&m_struct.color) {
        vkclearcolorvalue_struct_wrapper class1(&m_struct.color);
        class1.set_indent(m_indent + 4);
        class1.display_full_txt();
    }
}


// vkcommandbufferallocateinfo_struct_wrapper class definition
vkcommandbufferallocateinfo_struct_wrapper::vkcommandbufferallocateinfo_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkcommandbufferallocateinfo_struct_wrapper::vkcommandbufferallocateinfo_struct_wrapper(VkCommandBufferAllocateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkcommandbufferallocateinfo_struct_wrapper::vkcommandbufferallocateinfo_struct_wrapper(const VkCommandBufferAllocateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkcommandbufferallocateinfo_struct_wrapper::~vkcommandbufferallocateinfo_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkcommandbufferallocateinfo_struct_wrapper::display_single_txt()
{
    printf(" %*sVkCommandBufferAllocateInfo = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkcommandbufferallocateinfo_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %scommandPool = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.commandPool));
    printf("%*s    %slevel = %s\n", m_indent, "", &m_dummy_prefix, string_VkCommandBufferLevel(m_struct.level));
    printf("%*s    %scommandBufferCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.commandBufferCount));
}

// Output all struct elements, each on their own line
void vkcommandbufferallocateinfo_struct_wrapper::display_txt()
{
    printf("%*sVkCommandBufferAllocateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkcommandbufferallocateinfo_struct_wrapper::display_full_txt()
{
    printf("%*sVkCommandBufferAllocateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkcommandbufferbegininfo_struct_wrapper class definition
vkcommandbufferbegininfo_struct_wrapper::vkcommandbufferbegininfo_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkcommandbufferbegininfo_struct_wrapper::vkcommandbufferbegininfo_struct_wrapper(VkCommandBufferBeginInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkcommandbufferbegininfo_struct_wrapper::vkcommandbufferbegininfo_struct_wrapper(const VkCommandBufferBeginInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkcommandbufferbegininfo_struct_wrapper::~vkcommandbufferbegininfo_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkcommandbufferbegininfo_struct_wrapper::display_single_txt()
{
    printf(" %*sVkCommandBufferBeginInfo = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkcommandbufferbegininfo_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
    printf("%*s    %spInheritanceInfo = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.pInheritanceInfo));
}

// Output all struct elements, each on their own line
void vkcommandbufferbegininfo_struct_wrapper::display_txt()
{
    printf("%*sVkCommandBufferBeginInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkcommandbufferbegininfo_struct_wrapper::display_full_txt()
{
    printf("%*sVkCommandBufferBeginInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pInheritanceInfo) {
        vkcommandbufferinheritanceinfo_struct_wrapper class0(m_struct.pInheritanceInfo);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkcommandbufferinheritanceinfo_struct_wrapper class definition
vkcommandbufferinheritanceinfo_struct_wrapper::vkcommandbufferinheritanceinfo_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkcommandbufferinheritanceinfo_struct_wrapper::vkcommandbufferinheritanceinfo_struct_wrapper(VkCommandBufferInheritanceInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkcommandbufferinheritanceinfo_struct_wrapper::vkcommandbufferinheritanceinfo_struct_wrapper(const VkCommandBufferInheritanceInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkcommandbufferinheritanceinfo_struct_wrapper::~vkcommandbufferinheritanceinfo_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkcommandbufferinheritanceinfo_struct_wrapper::display_single_txt()
{
    printf(" %*sVkCommandBufferInheritanceInfo = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkcommandbufferinheritanceinfo_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %srenderPass = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.renderPass));
    printf("%*s    %ssubpass = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.subpass));
    printf("%*s    %sframebuffer = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.framebuffer));
    printf("%*s    %socclusionQueryEnable = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.occlusionQueryEnable) ? "TRUE" : "FALSE");
    printf("%*s    %squeryFlags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.queryFlags));
    printf("%*s    %spipelineStatistics = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.pipelineStatistics));
}

// Output all struct elements, each on their own line
void vkcommandbufferinheritanceinfo_struct_wrapper::display_txt()
{
    printf("%*sVkCommandBufferInheritanceInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkcommandbufferinheritanceinfo_struct_wrapper::display_full_txt()
{
    printf("%*sVkCommandBufferInheritanceInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkcommandpoolcreateinfo_struct_wrapper class definition
vkcommandpoolcreateinfo_struct_wrapper::vkcommandpoolcreateinfo_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkcommandpoolcreateinfo_struct_wrapper::vkcommandpoolcreateinfo_struct_wrapper(VkCommandPoolCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkcommandpoolcreateinfo_struct_wrapper::vkcommandpoolcreateinfo_struct_wrapper(const VkCommandPoolCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkcommandpoolcreateinfo_struct_wrapper::~vkcommandpoolcreateinfo_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkcommandpoolcreateinfo_struct_wrapper::display_single_txt()
{
    printf(" %*sVkCommandPoolCreateInfo = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkcommandpoolcreateinfo_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
    printf("%*s    %squeueFamilyIndex = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.queueFamilyIndex));
}

// Output all struct elements, each on their own line
void vkcommandpoolcreateinfo_struct_wrapper::display_txt()
{
    printf("%*sVkCommandPoolCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkcommandpoolcreateinfo_struct_wrapper::display_full_txt()
{
    printf("%*sVkCommandPoolCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkcomponentmapping_struct_wrapper class definition
vkcomponentmapping_struct_wrapper::vkcomponentmapping_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkcomponentmapping_struct_wrapper::vkcomponentmapping_struct_wrapper(VkComponentMapping* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkcomponentmapping_struct_wrapper::vkcomponentmapping_struct_wrapper(const VkComponentMapping* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkcomponentmapping_struct_wrapper::~vkcomponentmapping_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkcomponentmapping_struct_wrapper::display_single_txt()
{
    printf(" %*sVkComponentMapping = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkcomponentmapping_struct_wrapper::display_struct_members()
{
    printf("%*s    %sr = %s\n", m_indent, "", &m_dummy_prefix, string_VkComponentSwizzle(m_struct.r));
    printf("%*s    %sg = %s\n", m_indent, "", &m_dummy_prefix, string_VkComponentSwizzle(m_struct.g));
    printf("%*s    %sb = %s\n", m_indent, "", &m_dummy_prefix, string_VkComponentSwizzle(m_struct.b));
    printf("%*s    %sa = %s\n", m_indent, "", &m_dummy_prefix, string_VkComponentSwizzle(m_struct.a));
}

// Output all struct elements, each on their own line
void vkcomponentmapping_struct_wrapper::display_txt()
{
    printf("%*sVkComponentMapping struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkcomponentmapping_struct_wrapper::display_full_txt()
{
    printf("%*sVkComponentMapping struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// vkcomputepipelinecreateinfo_struct_wrapper class definition
vkcomputepipelinecreateinfo_struct_wrapper::vkcomputepipelinecreateinfo_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkcomputepipelinecreateinfo_struct_wrapper::vkcomputepipelinecreateinfo_struct_wrapper(VkComputePipelineCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkcomputepipelinecreateinfo_struct_wrapper::vkcomputepipelinecreateinfo_struct_wrapper(const VkComputePipelineCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkcomputepipelinecreateinfo_struct_wrapper::~vkcomputepipelinecreateinfo_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkcomputepipelinecreateinfo_struct_wrapper::display_single_txt()
{
    printf(" %*sVkComputePipelineCreateInfo = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkcomputepipelinecreateinfo_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
    printf("%*s    %sstage = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.stage));
    printf("%*s    %slayout = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.layout));
    printf("%*s    %sbasePipelineHandle = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.basePipelineHandle));
    printf("%*s    %sbasePipelineIndex = %i\n", m_indent, "", &m_dummy_prefix, (m_struct.basePipelineIndex));
}

// Output all struct elements, each on their own line
void vkcomputepipelinecreateinfo_struct_wrapper::display_txt()
{
    printf("%*sVkComputePipelineCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkcomputepipelinecreateinfo_struct_wrapper::display_full_txt()
{
    printf("%*sVkComputePipelineCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (&m_struct.stage) {
        vkpipelineshaderstagecreateinfo_struct_wrapper class0(&m_struct.stage);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkcopydescriptorset_struct_wrapper class definition
vkcopydescriptorset_struct_wrapper::vkcopydescriptorset_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkcopydescriptorset_struct_wrapper::vkcopydescriptorset_struct_wrapper(VkCopyDescriptorSet* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkcopydescriptorset_struct_wrapper::vkcopydescriptorset_struct_wrapper(const VkCopyDescriptorSet* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkcopydescriptorset_struct_wrapper::~vkcopydescriptorset_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkcopydescriptorset_struct_wrapper::display_single_txt()
{
    printf(" %*sVkCopyDescriptorSet = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkcopydescriptorset_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %ssrcSet = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.srcSet));
    printf("%*s    %ssrcBinding = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.srcBinding));
    printf("%*s    %ssrcArrayElement = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.srcArrayElement));
    printf("%*s    %sdstSet = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.dstSet));
    printf("%*s    %sdstBinding = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.dstBinding));
    printf("%*s    %sdstArrayElement = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.dstArrayElement));
    printf("%*s    %sdescriptorCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.descriptorCount));
}

// Output all struct elements, each on their own line
void vkcopydescriptorset_struct_wrapper::display_txt()
{
    printf("%*sVkCopyDescriptorSet struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkcopydescriptorset_struct_wrapper::display_full_txt()
{
    printf("%*sVkCopyDescriptorSet struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkdebugmarkermarkerinfoext_struct_wrapper class definition
vkdebugmarkermarkerinfoext_struct_wrapper::vkdebugmarkermarkerinfoext_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkdebugmarkermarkerinfoext_struct_wrapper::vkdebugmarkermarkerinfoext_struct_wrapper(VkDebugMarkerMarkerInfoEXT* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdebugmarkermarkerinfoext_struct_wrapper::vkdebugmarkermarkerinfoext_struct_wrapper(const VkDebugMarkerMarkerInfoEXT* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdebugmarkermarkerinfoext_struct_wrapper::~vkdebugmarkermarkerinfoext_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkdebugmarkermarkerinfoext_struct_wrapper::display_single_txt()
{
    printf(" %*sVkDebugMarkerMarkerInfoEXT = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkdebugmarkermarkerinfoext_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %spMarkerName = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pMarkerName));
    uint32_t i;
    for (i = 0; i<4; i++) {
        printf("%*s    %scolor[%u] = %f\n", m_indent, "", &m_dummy_prefix, i, (m_struct.color)[i]);
    }
}

// Output all struct elements, each on their own line
void vkdebugmarkermarkerinfoext_struct_wrapper::display_txt()
{
    printf("%*sVkDebugMarkerMarkerInfoEXT struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkdebugmarkermarkerinfoext_struct_wrapper::display_full_txt()
{
    printf("%*sVkDebugMarkerMarkerInfoEXT struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkdebugmarkerobjectnameinfoext_struct_wrapper class definition
vkdebugmarkerobjectnameinfoext_struct_wrapper::vkdebugmarkerobjectnameinfoext_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkdebugmarkerobjectnameinfoext_struct_wrapper::vkdebugmarkerobjectnameinfoext_struct_wrapper(VkDebugMarkerObjectNameInfoEXT* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdebugmarkerobjectnameinfoext_struct_wrapper::vkdebugmarkerobjectnameinfoext_struct_wrapper(const VkDebugMarkerObjectNameInfoEXT* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdebugmarkerobjectnameinfoext_struct_wrapper::~vkdebugmarkerobjectnameinfoext_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkdebugmarkerobjectnameinfoext_struct_wrapper::display_single_txt()
{
    printf(" %*sVkDebugMarkerObjectNameInfoEXT = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkdebugmarkerobjectnameinfoext_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sobjectType = %s\n", m_indent, "", &m_dummy_prefix, string_VkDebugReportObjectTypeEXT(m_struct.objectType));
    printf("%*s    %sobject = %" PRId64 "\n", m_indent, "", &m_dummy_prefix, (m_struct.object));
    printf("%*s    %spObjectName = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pObjectName));
}

// Output all struct elements, each on their own line
void vkdebugmarkerobjectnameinfoext_struct_wrapper::display_txt()
{
    printf("%*sVkDebugMarkerObjectNameInfoEXT struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkdebugmarkerobjectnameinfoext_struct_wrapper::display_full_txt()
{
    printf("%*sVkDebugMarkerObjectNameInfoEXT struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkdebugmarkerobjecttaginfoext_struct_wrapper class definition
vkdebugmarkerobjecttaginfoext_struct_wrapper::vkdebugmarkerobjecttaginfoext_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkdebugmarkerobjecttaginfoext_struct_wrapper::vkdebugmarkerobjecttaginfoext_struct_wrapper(VkDebugMarkerObjectTagInfoEXT* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdebugmarkerobjecttaginfoext_struct_wrapper::vkdebugmarkerobjecttaginfoext_struct_wrapper(const VkDebugMarkerObjectTagInfoEXT* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdebugmarkerobjecttaginfoext_struct_wrapper::~vkdebugmarkerobjecttaginfoext_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkdebugmarkerobjecttaginfoext_struct_wrapper::display_single_txt()
{
    printf(" %*sVkDebugMarkerObjectTagInfoEXT = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkdebugmarkerobjecttaginfoext_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sobjectType = %s\n", m_indent, "", &m_dummy_prefix, string_VkDebugReportObjectTypeEXT(m_struct.objectType));
    printf("%*s    %sobject = %" PRId64 "\n", m_indent, "", &m_dummy_prefix, (m_struct.object));
    printf("%*s    %stagName = %" PRId64 "\n", m_indent, "", &m_dummy_prefix, (m_struct.tagName));
    printf("%*s    %stagSize = " PRINTF_SIZE_T_SPECIFIER "\n", m_indent, "", &m_dummy_prefix, (m_struct.tagSize));
    printf("%*s    %spTag = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pTag));
}

// Output all struct elements, each on their own line
void vkdebugmarkerobjecttaginfoext_struct_wrapper::display_txt()
{
    printf("%*sVkDebugMarkerObjectTagInfoEXT struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkdebugmarkerobjecttaginfoext_struct_wrapper::display_full_txt()
{
    printf("%*sVkDebugMarkerObjectTagInfoEXT struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkdebugreportcallbackcreateinfoext_struct_wrapper class definition
vkdebugreportcallbackcreateinfoext_struct_wrapper::vkdebugreportcallbackcreateinfoext_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkdebugreportcallbackcreateinfoext_struct_wrapper::vkdebugreportcallbackcreateinfoext_struct_wrapper(VkDebugReportCallbackCreateInfoEXT* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdebugreportcallbackcreateinfoext_struct_wrapper::vkdebugreportcallbackcreateinfoext_struct_wrapper(const VkDebugReportCallbackCreateInfoEXT* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdebugreportcallbackcreateinfoext_struct_wrapper::~vkdebugreportcallbackcreateinfoext_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkdebugreportcallbackcreateinfoext_struct_wrapper::display_single_txt()
{
    printf(" %*sVkDebugReportCallbackCreateInfoEXT = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkdebugreportcallbackcreateinfoext_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
    printf("%*s    %spfnCallback = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.pfnCallback));
    printf("%*s    %spUserData = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pUserData));
}

// Output all struct elements, each on their own line
void vkdebugreportcallbackcreateinfoext_struct_wrapper::display_txt()
{
    printf("%*sVkDebugReportCallbackCreateInfoEXT struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkdebugreportcallbackcreateinfoext_struct_wrapper::display_full_txt()
{
    printf("%*sVkDebugReportCallbackCreateInfoEXT struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkdedicatedallocationbuffercreateinfonv_struct_wrapper class definition
vkdedicatedallocationbuffercreateinfonv_struct_wrapper::vkdedicatedallocationbuffercreateinfonv_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkdedicatedallocationbuffercreateinfonv_struct_wrapper::vkdedicatedallocationbuffercreateinfonv_struct_wrapper(VkDedicatedAllocationBufferCreateInfoNV* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdedicatedallocationbuffercreateinfonv_struct_wrapper::vkdedicatedallocationbuffercreateinfonv_struct_wrapper(const VkDedicatedAllocationBufferCreateInfoNV* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdedicatedallocationbuffercreateinfonv_struct_wrapper::~vkdedicatedallocationbuffercreateinfonv_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkdedicatedallocationbuffercreateinfonv_struct_wrapper::display_single_txt()
{
    printf(" %*sVkDedicatedAllocationBufferCreateInfoNV = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkdedicatedallocationbuffercreateinfonv_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sdedicatedAllocation = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.dedicatedAllocation) ? "TRUE" : "FALSE");
}

// Output all struct elements, each on their own line
void vkdedicatedallocationbuffercreateinfonv_struct_wrapper::display_txt()
{
    printf("%*sVkDedicatedAllocationBufferCreateInfoNV struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkdedicatedallocationbuffercreateinfonv_struct_wrapper::display_full_txt()
{
    printf("%*sVkDedicatedAllocationBufferCreateInfoNV struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkdedicatedallocationimagecreateinfonv_struct_wrapper class definition
vkdedicatedallocationimagecreateinfonv_struct_wrapper::vkdedicatedallocationimagecreateinfonv_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkdedicatedallocationimagecreateinfonv_struct_wrapper::vkdedicatedallocationimagecreateinfonv_struct_wrapper(VkDedicatedAllocationImageCreateInfoNV* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdedicatedallocationimagecreateinfonv_struct_wrapper::vkdedicatedallocationimagecreateinfonv_struct_wrapper(const VkDedicatedAllocationImageCreateInfoNV* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdedicatedallocationimagecreateinfonv_struct_wrapper::~vkdedicatedallocationimagecreateinfonv_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkdedicatedallocationimagecreateinfonv_struct_wrapper::display_single_txt()
{
    printf(" %*sVkDedicatedAllocationImageCreateInfoNV = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkdedicatedallocationimagecreateinfonv_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sdedicatedAllocation = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.dedicatedAllocation) ? "TRUE" : "FALSE");
}

// Output all struct elements, each on their own line
void vkdedicatedallocationimagecreateinfonv_struct_wrapper::display_txt()
{
    printf("%*sVkDedicatedAllocationImageCreateInfoNV struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkdedicatedallocationimagecreateinfonv_struct_wrapper::display_full_txt()
{
    printf("%*sVkDedicatedAllocationImageCreateInfoNV struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkdedicatedallocationmemoryallocateinfonv_struct_wrapper class definition
vkdedicatedallocationmemoryallocateinfonv_struct_wrapper::vkdedicatedallocationmemoryallocateinfonv_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkdedicatedallocationmemoryallocateinfonv_struct_wrapper::vkdedicatedallocationmemoryallocateinfonv_struct_wrapper(VkDedicatedAllocationMemoryAllocateInfoNV* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdedicatedallocationmemoryallocateinfonv_struct_wrapper::vkdedicatedallocationmemoryallocateinfonv_struct_wrapper(const VkDedicatedAllocationMemoryAllocateInfoNV* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdedicatedallocationmemoryallocateinfonv_struct_wrapper::~vkdedicatedallocationmemoryallocateinfonv_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkdedicatedallocationmemoryallocateinfonv_struct_wrapper::display_single_txt()
{
    printf(" %*sVkDedicatedAllocationMemoryAllocateInfoNV = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkdedicatedallocationmemoryallocateinfonv_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %simage = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.image));
    printf("%*s    %sbuffer = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.buffer));
}

// Output all struct elements, each on their own line
void vkdedicatedallocationmemoryallocateinfonv_struct_wrapper::display_txt()
{
    printf("%*sVkDedicatedAllocationMemoryAllocateInfoNV struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkdedicatedallocationmemoryallocateinfonv_struct_wrapper::display_full_txt()
{
    printf("%*sVkDedicatedAllocationMemoryAllocateInfoNV struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkdescriptorbufferinfo_struct_wrapper class definition
vkdescriptorbufferinfo_struct_wrapper::vkdescriptorbufferinfo_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkdescriptorbufferinfo_struct_wrapper::vkdescriptorbufferinfo_struct_wrapper(VkDescriptorBufferInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdescriptorbufferinfo_struct_wrapper::vkdescriptorbufferinfo_struct_wrapper(const VkDescriptorBufferInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdescriptorbufferinfo_struct_wrapper::~vkdescriptorbufferinfo_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkdescriptorbufferinfo_struct_wrapper::display_single_txt()
{
    printf(" %*sVkDescriptorBufferInfo = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkdescriptorbufferinfo_struct_wrapper::display_struct_members()
{
    printf("%*s    %sbuffer = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.buffer));
    printf("%*s    %soffset = " PRINTF_SIZE_T_SPECIFIER "\n", m_indent, "", &m_dummy_prefix, (m_struct.offset));
    printf("%*s    %srange = " PRINTF_SIZE_T_SPECIFIER "\n", m_indent, "", &m_dummy_prefix, (m_struct.range));
}

// Output all struct elements, each on their own line
void vkdescriptorbufferinfo_struct_wrapper::display_txt()
{
    printf("%*sVkDescriptorBufferInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkdescriptorbufferinfo_struct_wrapper::display_full_txt()
{
    printf("%*sVkDescriptorBufferInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// vkdescriptorimageinfo_struct_wrapper class definition
vkdescriptorimageinfo_struct_wrapper::vkdescriptorimageinfo_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkdescriptorimageinfo_struct_wrapper::vkdescriptorimageinfo_struct_wrapper(VkDescriptorImageInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdescriptorimageinfo_struct_wrapper::vkdescriptorimageinfo_struct_wrapper(const VkDescriptorImageInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdescriptorimageinfo_struct_wrapper::~vkdescriptorimageinfo_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkdescriptorimageinfo_struct_wrapper::display_single_txt()
{
    printf(" %*sVkDescriptorImageInfo = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkdescriptorimageinfo_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssampler = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.sampler));
    printf("%*s    %simageView = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.imageView));
    printf("%*s    %simageLayout = %s\n", m_indent, "", &m_dummy_prefix, string_VkImageLayout(m_struct.imageLayout));
}

// Output all struct elements, each on their own line
void vkdescriptorimageinfo_struct_wrapper::display_txt()
{
    printf("%*sVkDescriptorImageInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkdescriptorimageinfo_struct_wrapper::display_full_txt()
{
    printf("%*sVkDescriptorImageInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// vkdescriptorpoolcreateinfo_struct_wrapper class definition
vkdescriptorpoolcreateinfo_struct_wrapper::vkdescriptorpoolcreateinfo_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkdescriptorpoolcreateinfo_struct_wrapper::vkdescriptorpoolcreateinfo_struct_wrapper(VkDescriptorPoolCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdescriptorpoolcreateinfo_struct_wrapper::vkdescriptorpoolcreateinfo_struct_wrapper(const VkDescriptorPoolCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdescriptorpoolcreateinfo_struct_wrapper::~vkdescriptorpoolcreateinfo_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkdescriptorpoolcreateinfo_struct_wrapper::display_single_txt()
{
    printf(" %*sVkDescriptorPoolCreateInfo = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkdescriptorpoolcreateinfo_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
    printf("%*s    %smaxSets = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxSets));
    printf("%*s    %spoolSizeCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.poolSizeCount));
    uint32_t i;
    for (i = 0; i<poolSizeCount; i++) {
        printf("%*s    %spPoolSizes[%u] = 0x%p\n", m_indent, "", &m_dummy_prefix, i, (void*)(m_struct.pPoolSizes)[i]);
    }
}

// Output all struct elements, each on their own line
void vkdescriptorpoolcreateinfo_struct_wrapper::display_txt()
{
    printf("%*sVkDescriptorPoolCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkdescriptorpoolcreateinfo_struct_wrapper::display_full_txt()
{
    printf("%*sVkDescriptorPoolCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    uint32_t i;
    for (i = 0; i<poolSizeCount; i++) {
            vkdescriptorpoolsize_struct_wrapper class0(&(m_struct.pPoolSizes[i]));
            class0.set_indent(m_indent + 4);
            class0.display_full_txt();
    }
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkdescriptorpoolsize_struct_wrapper class definition
vkdescriptorpoolsize_struct_wrapper::vkdescriptorpoolsize_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkdescriptorpoolsize_struct_wrapper::vkdescriptorpoolsize_struct_wrapper(VkDescriptorPoolSize* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdescriptorpoolsize_struct_wrapper::vkdescriptorpoolsize_struct_wrapper(const VkDescriptorPoolSize* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdescriptorpoolsize_struct_wrapper::~vkdescriptorpoolsize_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkdescriptorpoolsize_struct_wrapper::display_single_txt()
{
    printf(" %*sVkDescriptorPoolSize = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkdescriptorpoolsize_struct_wrapper::display_struct_members()
{
    printf("%*s    %stype = %s\n", m_indent, "", &m_dummy_prefix, string_VkDescriptorType(m_struct.type));
    printf("%*s    %sdescriptorCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.descriptorCount));
}

// Output all struct elements, each on their own line
void vkdescriptorpoolsize_struct_wrapper::display_txt()
{
    printf("%*sVkDescriptorPoolSize struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkdescriptorpoolsize_struct_wrapper::display_full_txt()
{
    printf("%*sVkDescriptorPoolSize struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// vkdescriptorsetallocateinfo_struct_wrapper class definition
vkdescriptorsetallocateinfo_struct_wrapper::vkdescriptorsetallocateinfo_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkdescriptorsetallocateinfo_struct_wrapper::vkdescriptorsetallocateinfo_struct_wrapper(VkDescriptorSetAllocateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdescriptorsetallocateinfo_struct_wrapper::vkdescriptorsetallocateinfo_struct_wrapper(const VkDescriptorSetAllocateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdescriptorsetallocateinfo_struct_wrapper::~vkdescriptorsetallocateinfo_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkdescriptorsetallocateinfo_struct_wrapper::display_single_txt()
{
    printf(" %*sVkDescriptorSetAllocateInfo = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkdescriptorsetallocateinfo_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sdescriptorPool = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.descriptorPool));
    printf("%*s    %sdescriptorSetCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.descriptorSetCount));
    uint32_t i;
    for (i = 0; i<descriptorSetCount; i++) {
        printf("%*s    %spSetLayouts[%u] = 0x%p\n", m_indent, "", &m_dummy_prefix, i, (m_struct.pSetLayouts)[i]);
    }
}

// Output all struct elements, each on their own line
void vkdescriptorsetallocateinfo_struct_wrapper::display_txt()
{
    printf("%*sVkDescriptorSetAllocateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkdescriptorsetallocateinfo_struct_wrapper::display_full_txt()
{
    printf("%*sVkDescriptorSetAllocateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkdescriptorsetlayoutbinding_struct_wrapper class definition
vkdescriptorsetlayoutbinding_struct_wrapper::vkdescriptorsetlayoutbinding_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkdescriptorsetlayoutbinding_struct_wrapper::vkdescriptorsetlayoutbinding_struct_wrapper(VkDescriptorSetLayoutBinding* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdescriptorsetlayoutbinding_struct_wrapper::vkdescriptorsetlayoutbinding_struct_wrapper(const VkDescriptorSetLayoutBinding* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdescriptorsetlayoutbinding_struct_wrapper::~vkdescriptorsetlayoutbinding_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkdescriptorsetlayoutbinding_struct_wrapper::display_single_txt()
{
    printf(" %*sVkDescriptorSetLayoutBinding = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkdescriptorsetlayoutbinding_struct_wrapper::display_struct_members()
{
    printf("%*s    %sbinding = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.binding));
    printf("%*s    %sdescriptorType = %s\n", m_indent, "", &m_dummy_prefix, string_VkDescriptorType(m_struct.descriptorType));
    printf("%*s    %sdescriptorCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.descriptorCount));
    printf("%*s    %sstageFlags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.stageFlags));
    uint32_t i;
    for (i = 0; i<descriptorCount; i++) {
        printf("%*s    %spImmutableSamplers[%u] = 0x%p\n", m_indent, "", &m_dummy_prefix, i, (m_struct.pImmutableSamplers)[i]);
    }
}

// Output all struct elements, each on their own line
void vkdescriptorsetlayoutbinding_struct_wrapper::display_txt()
{
    printf("%*sVkDescriptorSetLayoutBinding struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkdescriptorsetlayoutbinding_struct_wrapper::display_full_txt()
{
    printf("%*sVkDescriptorSetLayoutBinding struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// vkdescriptorsetlayoutcreateinfo_struct_wrapper class definition
vkdescriptorsetlayoutcreateinfo_struct_wrapper::vkdescriptorsetlayoutcreateinfo_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkdescriptorsetlayoutcreateinfo_struct_wrapper::vkdescriptorsetlayoutcreateinfo_struct_wrapper(VkDescriptorSetLayoutCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdescriptorsetlayoutcreateinfo_struct_wrapper::vkdescriptorsetlayoutcreateinfo_struct_wrapper(const VkDescriptorSetLayoutCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdescriptorsetlayoutcreateinfo_struct_wrapper::~vkdescriptorsetlayoutcreateinfo_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkdescriptorsetlayoutcreateinfo_struct_wrapper::display_single_txt()
{
    printf(" %*sVkDescriptorSetLayoutCreateInfo = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkdescriptorsetlayoutcreateinfo_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
    printf("%*s    %sbindingCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.bindingCount));
    uint32_t i;
    for (i = 0; i<bindingCount; i++) {
        printf("%*s    %spBindings[%u] = 0x%p\n", m_indent, "", &m_dummy_prefix, i, (void*)(m_struct.pBindings)[i]);
    }
}

// Output all struct elements, each on their own line
void vkdescriptorsetlayoutcreateinfo_struct_wrapper::display_txt()
{
    printf("%*sVkDescriptorSetLayoutCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkdescriptorsetlayoutcreateinfo_struct_wrapper::display_full_txt()
{
    printf("%*sVkDescriptorSetLayoutCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    uint32_t i;
    for (i = 0; i<bindingCount; i++) {
            vkdescriptorsetlayoutbinding_struct_wrapper class0(&(m_struct.pBindings[i]));
            class0.set_indent(m_indent + 4);
            class0.display_full_txt();
    }
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkdevicecreateinfo_struct_wrapper class definition
vkdevicecreateinfo_struct_wrapper::vkdevicecreateinfo_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkdevicecreateinfo_struct_wrapper::vkdevicecreateinfo_struct_wrapper(VkDeviceCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdevicecreateinfo_struct_wrapper::vkdevicecreateinfo_struct_wrapper(const VkDeviceCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdevicecreateinfo_struct_wrapper::~vkdevicecreateinfo_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkdevicecreateinfo_struct_wrapper::display_single_txt()
{
    printf(" %*sVkDeviceCreateInfo = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkdevicecreateinfo_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
    printf("%*s    %squeueCreateInfoCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.queueCreateInfoCount));
    uint32_t i;
    for (i = 0; i<queueCreateInfoCount; i++) {
        printf("%*s    %spQueueCreateInfos[%u] = 0x%p\n", m_indent, "", &m_dummy_prefix, i, (void*)(m_struct.pQueueCreateInfos)[i]);
    }
    printf("%*s    %senabledLayerCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.enabledLayerCount));
    for (i = 0; i<enabledLayerCount; i++) {
        printf("%*s    %sppEnabledLayerNames = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.ppEnabledLayerNames)[0]);
    }
    printf("%*s    %senabledExtensionCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.enabledExtensionCount));
    for (i = 0; i<enabledExtensionCount; i++) {
        printf("%*s    %sppEnabledExtensionNames = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.ppEnabledExtensionNames)[0]);
    }
    printf("%*s    %spEnabledFeatures = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.pEnabledFeatures));
}

// Output all struct elements, each on their own line
void vkdevicecreateinfo_struct_wrapper::display_txt()
{
    printf("%*sVkDeviceCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkdevicecreateinfo_struct_wrapper::display_full_txt()
{
    printf("%*sVkDeviceCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pEnabledFeatures) {
        vkphysicaldevicefeatures_struct_wrapper class0(m_struct.pEnabledFeatures);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
    uint32_t i;
    for (i = 0; i<queueCreateInfoCount; i++) {
            vkdevicequeuecreateinfo_struct_wrapper class1(&(m_struct.pQueueCreateInfos[i]));
            class1.set_indent(m_indent + 4);
            class1.display_full_txt();
    }
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkdevicequeuecreateinfo_struct_wrapper class definition
vkdevicequeuecreateinfo_struct_wrapper::vkdevicequeuecreateinfo_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkdevicequeuecreateinfo_struct_wrapper::vkdevicequeuecreateinfo_struct_wrapper(VkDeviceQueueCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdevicequeuecreateinfo_struct_wrapper::vkdevicequeuecreateinfo_struct_wrapper(const VkDeviceQueueCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdevicequeuecreateinfo_struct_wrapper::~vkdevicequeuecreateinfo_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkdevicequeuecreateinfo_struct_wrapper::display_single_txt()
{
    printf(" %*sVkDeviceQueueCreateInfo = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkdevicequeuecreateinfo_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
    printf("%*s    %squeueFamilyIndex = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.queueFamilyIndex));
    printf("%*s    %squeueCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.queueCount));
    uint32_t i;
    for (i = 0; i<queueCount; i++) {
        printf("%*s    %spQueuePriorities[%u] = %f\n", m_indent, "", &m_dummy_prefix, i, (m_struct.pQueuePriorities)[i]);
    }
}

// Output all struct elements, each on their own line
void vkdevicequeuecreateinfo_struct_wrapper::display_txt()
{
    printf("%*sVkDeviceQueueCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkdevicequeuecreateinfo_struct_wrapper::display_full_txt()
{
    printf("%*sVkDeviceQueueCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkdispatchindirectcommand_struct_wrapper class definition
vkdispatchindirectcommand_struct_wrapper::vkdispatchindirectcommand_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkdispatchindirectcommand_struct_wrapper::vkdispatchindirectcommand_struct_wrapper(VkDispatchIndirectCommand* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdispatchindirectcommand_struct_wrapper::vkdispatchindirectcommand_struct_wrapper(const VkDispatchIndirectCommand* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdispatchindirectcommand_struct_wrapper::~vkdispatchindirectcommand_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkdispatchindirectcommand_struct_wrapper::display_single_txt()
{
    printf(" %*sVkDispatchIndirectCommand = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkdispatchindirectcommand_struct_wrapper::display_struct_members()
{
    printf("%*s    %sx = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.x));
    printf("%*s    %sy = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.y));
    printf("%*s    %sz = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.z));
}

// Output all struct elements, each on their own line
void vkdispatchindirectcommand_struct_wrapper::display_txt()
{
    printf("%*sVkDispatchIndirectCommand struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkdispatchindirectcommand_struct_wrapper::display_full_txt()
{
    printf("%*sVkDispatchIndirectCommand struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// vkdisplaymodecreateinfokhr_struct_wrapper class definition
vkdisplaymodecreateinfokhr_struct_wrapper::vkdisplaymodecreateinfokhr_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkdisplaymodecreateinfokhr_struct_wrapper::vkdisplaymodecreateinfokhr_struct_wrapper(VkDisplayModeCreateInfoKHR* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdisplaymodecreateinfokhr_struct_wrapper::vkdisplaymodecreateinfokhr_struct_wrapper(const VkDisplayModeCreateInfoKHR* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdisplaymodecreateinfokhr_struct_wrapper::~vkdisplaymodecreateinfokhr_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkdisplaymodecreateinfokhr_struct_wrapper::display_single_txt()
{
    printf(" %*sVkDisplayModeCreateInfoKHR = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkdisplaymodecreateinfokhr_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
    printf("%*s    %sparameters = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.parameters));
}

// Output all struct elements, each on their own line
void vkdisplaymodecreateinfokhr_struct_wrapper::display_txt()
{
    printf("%*sVkDisplayModeCreateInfoKHR struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkdisplaymodecreateinfokhr_struct_wrapper::display_full_txt()
{
    printf("%*sVkDisplayModeCreateInfoKHR struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (&m_struct.parameters) {
        vkdisplaymodeparameterskhr_struct_wrapper class0(&m_struct.parameters);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkdisplaymodeparameterskhr_struct_wrapper class definition
vkdisplaymodeparameterskhr_struct_wrapper::vkdisplaymodeparameterskhr_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkdisplaymodeparameterskhr_struct_wrapper::vkdisplaymodeparameterskhr_struct_wrapper(VkDisplayModeParametersKHR* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdisplaymodeparameterskhr_struct_wrapper::vkdisplaymodeparameterskhr_struct_wrapper(const VkDisplayModeParametersKHR* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdisplaymodeparameterskhr_struct_wrapper::~vkdisplaymodeparameterskhr_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkdisplaymodeparameterskhr_struct_wrapper::display_single_txt()
{
    printf(" %*sVkDisplayModeParametersKHR = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkdisplaymodeparameterskhr_struct_wrapper::display_struct_members()
{
    printf("%*s    %svisibleRegion = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.visibleRegion));
    printf("%*s    %srefreshRate = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.refreshRate));
}

// Output all struct elements, each on their own line
void vkdisplaymodeparameterskhr_struct_wrapper::display_txt()
{
    printf("%*sVkDisplayModeParametersKHR struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkdisplaymodeparameterskhr_struct_wrapper::display_full_txt()
{
    printf("%*sVkDisplayModeParametersKHR struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (&m_struct.visibleRegion) {
        vkextent2d_struct_wrapper class0(&m_struct.visibleRegion);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
}


// vkdisplaymodepropertieskhr_struct_wrapper class definition
vkdisplaymodepropertieskhr_struct_wrapper::vkdisplaymodepropertieskhr_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkdisplaymodepropertieskhr_struct_wrapper::vkdisplaymodepropertieskhr_struct_wrapper(VkDisplayModePropertiesKHR* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdisplaymodepropertieskhr_struct_wrapper::vkdisplaymodepropertieskhr_struct_wrapper(const VkDisplayModePropertiesKHR* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdisplaymodepropertieskhr_struct_wrapper::~vkdisplaymodepropertieskhr_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkdisplaymodepropertieskhr_struct_wrapper::display_single_txt()
{
    printf(" %*sVkDisplayModePropertiesKHR = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkdisplaymodepropertieskhr_struct_wrapper::display_struct_members()
{
    printf("%*s    %sdisplayMode = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.displayMode));
    printf("%*s    %sparameters = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.parameters));
}

// Output all struct elements, each on their own line
void vkdisplaymodepropertieskhr_struct_wrapper::display_txt()
{
    printf("%*sVkDisplayModePropertiesKHR struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkdisplaymodepropertieskhr_struct_wrapper::display_full_txt()
{
    printf("%*sVkDisplayModePropertiesKHR struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (&m_struct.parameters) {
        vkdisplaymodeparameterskhr_struct_wrapper class0(&m_struct.parameters);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
}


// vkdisplayplanecapabilitieskhr_struct_wrapper class definition
vkdisplayplanecapabilitieskhr_struct_wrapper::vkdisplayplanecapabilitieskhr_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkdisplayplanecapabilitieskhr_struct_wrapper::vkdisplayplanecapabilitieskhr_struct_wrapper(VkDisplayPlaneCapabilitiesKHR* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdisplayplanecapabilitieskhr_struct_wrapper::vkdisplayplanecapabilitieskhr_struct_wrapper(const VkDisplayPlaneCapabilitiesKHR* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdisplayplanecapabilitieskhr_struct_wrapper::~vkdisplayplanecapabilitieskhr_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkdisplayplanecapabilitieskhr_struct_wrapper::display_single_txt()
{
    printf(" %*sVkDisplayPlaneCapabilitiesKHR = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkdisplayplanecapabilitieskhr_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssupportedAlpha = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.supportedAlpha));
    printf("%*s    %sminSrcPosition = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.minSrcPosition));
    printf("%*s    %smaxSrcPosition = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.maxSrcPosition));
    printf("%*s    %sminSrcExtent = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.minSrcExtent));
    printf("%*s    %smaxSrcExtent = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.maxSrcExtent));
    printf("%*s    %sminDstPosition = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.minDstPosition));
    printf("%*s    %smaxDstPosition = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.maxDstPosition));
    printf("%*s    %sminDstExtent = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.minDstExtent));
    printf("%*s    %smaxDstExtent = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.maxDstExtent));
}

// Output all struct elements, each on their own line
void vkdisplayplanecapabilitieskhr_struct_wrapper::display_txt()
{
    printf("%*sVkDisplayPlaneCapabilitiesKHR struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkdisplayplanecapabilitieskhr_struct_wrapper::display_full_txt()
{
    printf("%*sVkDisplayPlaneCapabilitiesKHR struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (&m_struct.maxDstExtent) {
        vkextent2d_struct_wrapper class0(&m_struct.maxDstExtent);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
    if (&m_struct.minDstExtent) {
        vkextent2d_struct_wrapper class1(&m_struct.minDstExtent);
        class1.set_indent(m_indent + 4);
        class1.display_full_txt();
    }
    if (&m_struct.maxDstPosition) {
        vkoffset2d_struct_wrapper class2(&m_struct.maxDstPosition);
        class2.set_indent(m_indent + 4);
        class2.display_full_txt();
    }
    if (&m_struct.minDstPosition) {
        vkoffset2d_struct_wrapper class3(&m_struct.minDstPosition);
        class3.set_indent(m_indent + 4);
        class3.display_full_txt();
    }
    if (&m_struct.maxSrcExtent) {
        vkextent2d_struct_wrapper class4(&m_struct.maxSrcExtent);
        class4.set_indent(m_indent + 4);
        class4.display_full_txt();
    }
    if (&m_struct.minSrcExtent) {
        vkextent2d_struct_wrapper class5(&m_struct.minSrcExtent);
        class5.set_indent(m_indent + 4);
        class5.display_full_txt();
    }
    if (&m_struct.maxSrcPosition) {
        vkoffset2d_struct_wrapper class6(&m_struct.maxSrcPosition);
        class6.set_indent(m_indent + 4);
        class6.display_full_txt();
    }
    if (&m_struct.minSrcPosition) {
        vkoffset2d_struct_wrapper class7(&m_struct.minSrcPosition);
        class7.set_indent(m_indent + 4);
        class7.display_full_txt();
    }
}


// vkdisplayplanepropertieskhr_struct_wrapper class definition
vkdisplayplanepropertieskhr_struct_wrapper::vkdisplayplanepropertieskhr_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkdisplayplanepropertieskhr_struct_wrapper::vkdisplayplanepropertieskhr_struct_wrapper(VkDisplayPlanePropertiesKHR* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdisplayplanepropertieskhr_struct_wrapper::vkdisplayplanepropertieskhr_struct_wrapper(const VkDisplayPlanePropertiesKHR* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdisplayplanepropertieskhr_struct_wrapper::~vkdisplayplanepropertieskhr_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkdisplayplanepropertieskhr_struct_wrapper::display_single_txt()
{
    printf(" %*sVkDisplayPlanePropertiesKHR = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkdisplayplanepropertieskhr_struct_wrapper::display_struct_members()
{
    printf("%*s    %scurrentDisplay = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.currentDisplay));
    printf("%*s    %scurrentStackIndex = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.currentStackIndex));
}

// Output all struct elements, each on their own line
void vkdisplayplanepropertieskhr_struct_wrapper::display_txt()
{
    printf("%*sVkDisplayPlanePropertiesKHR struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkdisplayplanepropertieskhr_struct_wrapper::display_full_txt()
{
    printf("%*sVkDisplayPlanePropertiesKHR struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// vkdisplaypresentinfokhr_struct_wrapper class definition
vkdisplaypresentinfokhr_struct_wrapper::vkdisplaypresentinfokhr_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkdisplaypresentinfokhr_struct_wrapper::vkdisplaypresentinfokhr_struct_wrapper(VkDisplayPresentInfoKHR* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdisplaypresentinfokhr_struct_wrapper::vkdisplaypresentinfokhr_struct_wrapper(const VkDisplayPresentInfoKHR* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdisplaypresentinfokhr_struct_wrapper::~vkdisplaypresentinfokhr_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkdisplaypresentinfokhr_struct_wrapper::display_single_txt()
{
    printf(" %*sVkDisplayPresentInfoKHR = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkdisplaypresentinfokhr_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %ssrcRect = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.srcRect));
    printf("%*s    %sdstRect = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.dstRect));
    printf("%*s    %spersistent = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.persistent) ? "TRUE" : "FALSE");
}

// Output all struct elements, each on their own line
void vkdisplaypresentinfokhr_struct_wrapper::display_txt()
{
    printf("%*sVkDisplayPresentInfoKHR struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkdisplaypresentinfokhr_struct_wrapper::display_full_txt()
{
    printf("%*sVkDisplayPresentInfoKHR struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (&m_struct.dstRect) {
        vkrect2d_struct_wrapper class0(&m_struct.dstRect);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
    if (&m_struct.srcRect) {
        vkrect2d_struct_wrapper class1(&m_struct.srcRect);
        class1.set_indent(m_indent + 4);
        class1.display_full_txt();
    }
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkdisplaypropertieskhr_struct_wrapper class definition
vkdisplaypropertieskhr_struct_wrapper::vkdisplaypropertieskhr_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkdisplaypropertieskhr_struct_wrapper::vkdisplaypropertieskhr_struct_wrapper(VkDisplayPropertiesKHR* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdisplaypropertieskhr_struct_wrapper::vkdisplaypropertieskhr_struct_wrapper(const VkDisplayPropertiesKHR* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdisplaypropertieskhr_struct_wrapper::~vkdisplaypropertieskhr_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkdisplaypropertieskhr_struct_wrapper::display_single_txt()
{
    printf(" %*sVkDisplayPropertiesKHR = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkdisplaypropertieskhr_struct_wrapper::display_struct_members()
{
    printf("%*s    %sdisplay = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.display));
    printf("%*s    %sdisplayName = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.displayName));
    printf("%*s    %sphysicalDimensions = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.physicalDimensions));
    printf("%*s    %sphysicalResolution = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.physicalResolution));
    printf("%*s    %ssupportedTransforms = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.supportedTransforms));
    printf("%*s    %splaneReorderPossible = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.planeReorderPossible) ? "TRUE" : "FALSE");
    printf("%*s    %spersistentContent = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.persistentContent) ? "TRUE" : "FALSE");
}

// Output all struct elements, each on their own line
void vkdisplaypropertieskhr_struct_wrapper::display_txt()
{
    printf("%*sVkDisplayPropertiesKHR struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkdisplaypropertieskhr_struct_wrapper::display_full_txt()
{
    printf("%*sVkDisplayPropertiesKHR struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (&m_struct.physicalResolution) {
        vkextent2d_struct_wrapper class0(&m_struct.physicalResolution);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
    if (&m_struct.physicalDimensions) {
        vkextent2d_struct_wrapper class1(&m_struct.physicalDimensions);
        class1.set_indent(m_indent + 4);
        class1.display_full_txt();
    }
}


// vkdisplaysurfacecreateinfokhr_struct_wrapper class definition
vkdisplaysurfacecreateinfokhr_struct_wrapper::vkdisplaysurfacecreateinfokhr_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkdisplaysurfacecreateinfokhr_struct_wrapper::vkdisplaysurfacecreateinfokhr_struct_wrapper(VkDisplaySurfaceCreateInfoKHR* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdisplaysurfacecreateinfokhr_struct_wrapper::vkdisplaysurfacecreateinfokhr_struct_wrapper(const VkDisplaySurfaceCreateInfoKHR* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdisplaysurfacecreateinfokhr_struct_wrapper::~vkdisplaysurfacecreateinfokhr_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkdisplaysurfacecreateinfokhr_struct_wrapper::display_single_txt()
{
    printf(" %*sVkDisplaySurfaceCreateInfoKHR = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkdisplaysurfacecreateinfokhr_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
    printf("%*s    %sdisplayMode = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.displayMode));
    printf("%*s    %splaneIndex = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.planeIndex));
    printf("%*s    %splaneStackIndex = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.planeStackIndex));
    printf("%*s    %stransform = %s\n", m_indent, "", &m_dummy_prefix, string_VkSurfaceTransformFlagBitsKHR(m_struct.transform));
    printf("%*s    %sglobalAlpha = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.globalAlpha));
    printf("%*s    %salphaMode = %s\n", m_indent, "", &m_dummy_prefix, string_VkDisplayPlaneAlphaFlagBitsKHR(m_struct.alphaMode));
    printf("%*s    %simageExtent = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.imageExtent));
}

// Output all struct elements, each on their own line
void vkdisplaysurfacecreateinfokhr_struct_wrapper::display_txt()
{
    printf("%*sVkDisplaySurfaceCreateInfoKHR struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkdisplaysurfacecreateinfokhr_struct_wrapper::display_full_txt()
{
    printf("%*sVkDisplaySurfaceCreateInfoKHR struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (&m_struct.imageExtent) {
        vkextent2d_struct_wrapper class0(&m_struct.imageExtent);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkdrawindexedindirectcommand_struct_wrapper class definition
vkdrawindexedindirectcommand_struct_wrapper::vkdrawindexedindirectcommand_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkdrawindexedindirectcommand_struct_wrapper::vkdrawindexedindirectcommand_struct_wrapper(VkDrawIndexedIndirectCommand* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdrawindexedindirectcommand_struct_wrapper::vkdrawindexedindirectcommand_struct_wrapper(const VkDrawIndexedIndirectCommand* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdrawindexedindirectcommand_struct_wrapper::~vkdrawindexedindirectcommand_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkdrawindexedindirectcommand_struct_wrapper::display_single_txt()
{
    printf(" %*sVkDrawIndexedIndirectCommand = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkdrawindexedindirectcommand_struct_wrapper::display_struct_members()
{
    printf("%*s    %sindexCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.indexCount));
    printf("%*s    %sinstanceCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.instanceCount));
    printf("%*s    %sfirstIndex = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.firstIndex));
    printf("%*s    %svertexOffset = %i\n", m_indent, "", &m_dummy_prefix, (m_struct.vertexOffset));
    printf("%*s    %sfirstInstance = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.firstInstance));
}

// Output all struct elements, each on their own line
void vkdrawindexedindirectcommand_struct_wrapper::display_txt()
{
    printf("%*sVkDrawIndexedIndirectCommand struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkdrawindexedindirectcommand_struct_wrapper::display_full_txt()
{
    printf("%*sVkDrawIndexedIndirectCommand struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// vkdrawindirectcommand_struct_wrapper class definition
vkdrawindirectcommand_struct_wrapper::vkdrawindirectcommand_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkdrawindirectcommand_struct_wrapper::vkdrawindirectcommand_struct_wrapper(VkDrawIndirectCommand* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdrawindirectcommand_struct_wrapper::vkdrawindirectcommand_struct_wrapper(const VkDrawIndirectCommand* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkdrawindirectcommand_struct_wrapper::~vkdrawindirectcommand_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkdrawindirectcommand_struct_wrapper::display_single_txt()
{
    printf(" %*sVkDrawIndirectCommand = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkdrawindirectcommand_struct_wrapper::display_struct_members()
{
    printf("%*s    %svertexCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.vertexCount));
    printf("%*s    %sinstanceCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.instanceCount));
    printf("%*s    %sfirstVertex = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.firstVertex));
    printf("%*s    %sfirstInstance = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.firstInstance));
}

// Output all struct elements, each on their own line
void vkdrawindirectcommand_struct_wrapper::display_txt()
{
    printf("%*sVkDrawIndirectCommand struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkdrawindirectcommand_struct_wrapper::display_full_txt()
{
    printf("%*sVkDrawIndirectCommand struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// vkeventcreateinfo_struct_wrapper class definition
vkeventcreateinfo_struct_wrapper::vkeventcreateinfo_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkeventcreateinfo_struct_wrapper::vkeventcreateinfo_struct_wrapper(VkEventCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkeventcreateinfo_struct_wrapper::vkeventcreateinfo_struct_wrapper(const VkEventCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkeventcreateinfo_struct_wrapper::~vkeventcreateinfo_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkeventcreateinfo_struct_wrapper::display_single_txt()
{
    printf(" %*sVkEventCreateInfo = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkeventcreateinfo_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
}

// Output all struct elements, each on their own line
void vkeventcreateinfo_struct_wrapper::display_txt()
{
    printf("%*sVkEventCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkeventcreateinfo_struct_wrapper::display_full_txt()
{
    printf("%*sVkEventCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkexportmemoryallocateinfonv_struct_wrapper class definition
vkexportmemoryallocateinfonv_struct_wrapper::vkexportmemoryallocateinfonv_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkexportmemoryallocateinfonv_struct_wrapper::vkexportmemoryallocateinfonv_struct_wrapper(VkExportMemoryAllocateInfoNV* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkexportmemoryallocateinfonv_struct_wrapper::vkexportmemoryallocateinfonv_struct_wrapper(const VkExportMemoryAllocateInfoNV* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkexportmemoryallocateinfonv_struct_wrapper::~vkexportmemoryallocateinfonv_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkexportmemoryallocateinfonv_struct_wrapper::display_single_txt()
{
    printf(" %*sVkExportMemoryAllocateInfoNV = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkexportmemoryallocateinfonv_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %shandleTypes = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.handleTypes));
}

// Output all struct elements, each on their own line
void vkexportmemoryallocateinfonv_struct_wrapper::display_txt()
{
    printf("%*sVkExportMemoryAllocateInfoNV struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkexportmemoryallocateinfonv_struct_wrapper::display_full_txt()
{
    printf("%*sVkExportMemoryAllocateInfoNV struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkexportmemorywin32handleinfonv_struct_wrapper class definition
vkexportmemorywin32handleinfonv_struct_wrapper::vkexportmemorywin32handleinfonv_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkexportmemorywin32handleinfonv_struct_wrapper::vkexportmemorywin32handleinfonv_struct_wrapper(VkExportMemoryWin32HandleInfoNV* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkexportmemorywin32handleinfonv_struct_wrapper::vkexportmemorywin32handleinfonv_struct_wrapper(const VkExportMemoryWin32HandleInfoNV* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkexportmemorywin32handleinfonv_struct_wrapper::~vkexportmemorywin32handleinfonv_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkexportmemorywin32handleinfonv_struct_wrapper::display_single_txt()
{
    printf(" %*sVkExportMemoryWin32HandleInfoNV = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkexportmemorywin32handleinfonv_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %spAttributes = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pAttributes));
    printf("%*s    %sdwAccess = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.dwAccess));
}

// Output all struct elements, each on their own line
void vkexportmemorywin32handleinfonv_struct_wrapper::display_txt()
{
    printf("%*sVkExportMemoryWin32HandleInfoNV struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkexportmemorywin32handleinfonv_struct_wrapper::display_full_txt()
{
    printf("%*sVkExportMemoryWin32HandleInfoNV struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkextensionproperties_struct_wrapper class definition
vkextensionproperties_struct_wrapper::vkextensionproperties_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkextensionproperties_struct_wrapper::vkextensionproperties_struct_wrapper(VkExtensionProperties* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkextensionproperties_struct_wrapper::vkextensionproperties_struct_wrapper(const VkExtensionProperties* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkextensionproperties_struct_wrapper::~vkextensionproperties_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkextensionproperties_struct_wrapper::display_single_txt()
{
    printf(" %*sVkExtensionProperties = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkextensionproperties_struct_wrapper::display_struct_members()
{
    uint32_t i;
    for (i = 0; i<VK_MAX_EXTENSION_NAME_SIZE; i++) {
        printf("%*s    %sextensionName = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.extensionName));
    }
    printf("%*s    %sspecVersion = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.specVersion));
}

// Output all struct elements, each on their own line
void vkextensionproperties_struct_wrapper::display_txt()
{
    printf("%*sVkExtensionProperties struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkextensionproperties_struct_wrapper::display_full_txt()
{
    printf("%*sVkExtensionProperties struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// vkextent2d_struct_wrapper class definition
vkextent2d_struct_wrapper::vkextent2d_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkextent2d_struct_wrapper::vkextent2d_struct_wrapper(VkExtent2D* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkextent2d_struct_wrapper::vkextent2d_struct_wrapper(const VkExtent2D* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkextent2d_struct_wrapper::~vkextent2d_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkextent2d_struct_wrapper::display_single_txt()
{
    printf(" %*sVkExtent2D = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkextent2d_struct_wrapper::display_struct_members()
{
    printf("%*s    %swidth = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.width));
    printf("%*s    %sheight = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.height));
}

// Output all struct elements, each on their own line
void vkextent2d_struct_wrapper::display_txt()
{
    printf("%*sVkExtent2D struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkextent2d_struct_wrapper::display_full_txt()
{
    printf("%*sVkExtent2D struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// vkextent3d_struct_wrapper class definition
vkextent3d_struct_wrapper::vkextent3d_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkextent3d_struct_wrapper::vkextent3d_struct_wrapper(VkExtent3D* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkextent3d_struct_wrapper::vkextent3d_struct_wrapper(const VkExtent3D* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkextent3d_struct_wrapper::~vkextent3d_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkextent3d_struct_wrapper::display_single_txt()
{
    printf(" %*sVkExtent3D = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkextent3d_struct_wrapper::display_struct_members()
{
    printf("%*s    %swidth = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.width));
    printf("%*s    %sheight = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.height));
    printf("%*s    %sdepth = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.depth));
}

// Output all struct elements, each on their own line
void vkextent3d_struct_wrapper::display_txt()
{
    printf("%*sVkExtent3D struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkextent3d_struct_wrapper::display_full_txt()
{
    printf("%*sVkExtent3D struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// vkexternalimageformatpropertiesnv_struct_wrapper class definition
vkexternalimageformatpropertiesnv_struct_wrapper::vkexternalimageformatpropertiesnv_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkexternalimageformatpropertiesnv_struct_wrapper::vkexternalimageformatpropertiesnv_struct_wrapper(VkExternalImageFormatPropertiesNV* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkexternalimageformatpropertiesnv_struct_wrapper::vkexternalimageformatpropertiesnv_struct_wrapper(const VkExternalImageFormatPropertiesNV* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkexternalimageformatpropertiesnv_struct_wrapper::~vkexternalimageformatpropertiesnv_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkexternalimageformatpropertiesnv_struct_wrapper::display_single_txt()
{
    printf(" %*sVkExternalImageFormatPropertiesNV = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkexternalimageformatpropertiesnv_struct_wrapper::display_struct_members()
{
    printf("%*s    %simageFormatProperties = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.imageFormatProperties));
    printf("%*s    %sexternalMemoryFeatures = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.externalMemoryFeatures));
    printf("%*s    %sexportFromImportedHandleTypes = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.exportFromImportedHandleTypes));
    printf("%*s    %scompatibleHandleTypes = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.compatibleHandleTypes));
}

// Output all struct elements, each on their own line
void vkexternalimageformatpropertiesnv_struct_wrapper::display_txt()
{
    printf("%*sVkExternalImageFormatPropertiesNV struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkexternalimageformatpropertiesnv_struct_wrapper::display_full_txt()
{
    printf("%*sVkExternalImageFormatPropertiesNV struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (&m_struct.imageFormatProperties) {
        vkimageformatproperties_struct_wrapper class0(&m_struct.imageFormatProperties);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
}


// vkexternalmemoryimagecreateinfonv_struct_wrapper class definition
vkexternalmemoryimagecreateinfonv_struct_wrapper::vkexternalmemoryimagecreateinfonv_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkexternalmemoryimagecreateinfonv_struct_wrapper::vkexternalmemoryimagecreateinfonv_struct_wrapper(VkExternalMemoryImageCreateInfoNV* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkexternalmemoryimagecreateinfonv_struct_wrapper::vkexternalmemoryimagecreateinfonv_struct_wrapper(const VkExternalMemoryImageCreateInfoNV* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkexternalmemoryimagecreateinfonv_struct_wrapper::~vkexternalmemoryimagecreateinfonv_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkexternalmemoryimagecreateinfonv_struct_wrapper::display_single_txt()
{
    printf(" %*sVkExternalMemoryImageCreateInfoNV = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkexternalmemoryimagecreateinfonv_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %shandleTypes = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.handleTypes));
}

// Output all struct elements, each on their own line
void vkexternalmemoryimagecreateinfonv_struct_wrapper::display_txt()
{
    printf("%*sVkExternalMemoryImageCreateInfoNV struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkexternalmemoryimagecreateinfonv_struct_wrapper::display_full_txt()
{
    printf("%*sVkExternalMemoryImageCreateInfoNV struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkfencecreateinfo_struct_wrapper class definition
vkfencecreateinfo_struct_wrapper::vkfencecreateinfo_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkfencecreateinfo_struct_wrapper::vkfencecreateinfo_struct_wrapper(VkFenceCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkfencecreateinfo_struct_wrapper::vkfencecreateinfo_struct_wrapper(const VkFenceCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkfencecreateinfo_struct_wrapper::~vkfencecreateinfo_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkfencecreateinfo_struct_wrapper::display_single_txt()
{
    printf(" %*sVkFenceCreateInfo = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkfencecreateinfo_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
}

// Output all struct elements, each on their own line
void vkfencecreateinfo_struct_wrapper::display_txt()
{
    printf("%*sVkFenceCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkfencecreateinfo_struct_wrapper::display_full_txt()
{
    printf("%*sVkFenceCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkformatproperties_struct_wrapper class definition
vkformatproperties_struct_wrapper::vkformatproperties_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkformatproperties_struct_wrapper::vkformatproperties_struct_wrapper(VkFormatProperties* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkformatproperties_struct_wrapper::vkformatproperties_struct_wrapper(const VkFormatProperties* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkformatproperties_struct_wrapper::~vkformatproperties_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkformatproperties_struct_wrapper::display_single_txt()
{
    printf(" %*sVkFormatProperties = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkformatproperties_struct_wrapper::display_struct_members()
{
    printf("%*s    %slinearTilingFeatures = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.linearTilingFeatures));
    printf("%*s    %soptimalTilingFeatures = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.optimalTilingFeatures));
    printf("%*s    %sbufferFeatures = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.bufferFeatures));
}

// Output all struct elements, each on their own line
void vkformatproperties_struct_wrapper::display_txt()
{
    printf("%*sVkFormatProperties struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkformatproperties_struct_wrapper::display_full_txt()
{
    printf("%*sVkFormatProperties struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// vkframebuffercreateinfo_struct_wrapper class definition
vkframebuffercreateinfo_struct_wrapper::vkframebuffercreateinfo_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkframebuffercreateinfo_struct_wrapper::vkframebuffercreateinfo_struct_wrapper(VkFramebufferCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkframebuffercreateinfo_struct_wrapper::vkframebuffercreateinfo_struct_wrapper(const VkFramebufferCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkframebuffercreateinfo_struct_wrapper::~vkframebuffercreateinfo_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkframebuffercreateinfo_struct_wrapper::display_single_txt()
{
    printf(" %*sVkFramebufferCreateInfo = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkframebuffercreateinfo_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
    printf("%*s    %srenderPass = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.renderPass));
    printf("%*s    %sattachmentCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.attachmentCount));
    uint32_t i;
    for (i = 0; i<attachmentCount; i++) {
        printf("%*s    %spAttachments[%u] = 0x%p\n", m_indent, "", &m_dummy_prefix, i, (m_struct.pAttachments)[i]);
    }
    printf("%*s    %swidth = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.width));
    printf("%*s    %sheight = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.height));
    printf("%*s    %slayers = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.layers));
}

// Output all struct elements, each on their own line
void vkframebuffercreateinfo_struct_wrapper::display_txt()
{
    printf("%*sVkFramebufferCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkframebuffercreateinfo_struct_wrapper::display_full_txt()
{
    printf("%*sVkFramebufferCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkgraphicspipelinecreateinfo_struct_wrapper class definition
vkgraphicspipelinecreateinfo_struct_wrapper::vkgraphicspipelinecreateinfo_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkgraphicspipelinecreateinfo_struct_wrapper::vkgraphicspipelinecreateinfo_struct_wrapper(VkGraphicsPipelineCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkgraphicspipelinecreateinfo_struct_wrapper::vkgraphicspipelinecreateinfo_struct_wrapper(const VkGraphicsPipelineCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkgraphicspipelinecreateinfo_struct_wrapper::~vkgraphicspipelinecreateinfo_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkgraphicspipelinecreateinfo_struct_wrapper::display_single_txt()
{
    printf(" %*sVkGraphicsPipelineCreateInfo = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkgraphicspipelinecreateinfo_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
    printf("%*s    %sstageCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.stageCount));
    uint32_t i;
    for (i = 0; i<stageCount; i++) {
        printf("%*s    %spStages[%u] = 0x%p\n", m_indent, "", &m_dummy_prefix, i, (void*)(m_struct.pStages)[i]);
    }
    printf("%*s    %spVertexInputState = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.pVertexInputState));
    printf("%*s    %spInputAssemblyState = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.pInputAssemblyState));
    printf("%*s    %spTessellationState = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.pTessellationState));
    printf("%*s    %spViewportState = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.pViewportState));
    printf("%*s    %spRasterizationState = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.pRasterizationState));
    printf("%*s    %spMultisampleState = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.pMultisampleState));
    printf("%*s    %spDepthStencilState = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.pDepthStencilState));
    printf("%*s    %spColorBlendState = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.pColorBlendState));
    printf("%*s    %spDynamicState = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.pDynamicState));
    printf("%*s    %slayout = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.layout));
    printf("%*s    %srenderPass = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.renderPass));
    printf("%*s    %ssubpass = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.subpass));
    printf("%*s    %sbasePipelineHandle = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.basePipelineHandle));
    printf("%*s    %sbasePipelineIndex = %i\n", m_indent, "", &m_dummy_prefix, (m_struct.basePipelineIndex));
}

// Output all struct elements, each on their own line
void vkgraphicspipelinecreateinfo_struct_wrapper::display_txt()
{
    printf("%*sVkGraphicsPipelineCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkgraphicspipelinecreateinfo_struct_wrapper::display_full_txt()
{
    printf("%*sVkGraphicsPipelineCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pDynamicState) {
        vkpipelinedynamicstatecreateinfo_struct_wrapper class0(m_struct.pDynamicState);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
    if (m_struct.pColorBlendState) {
        vkpipelinecolorblendstatecreateinfo_struct_wrapper class1(m_struct.pColorBlendState);
        class1.set_indent(m_indent + 4);
        class1.display_full_txt();
    }
    if (m_struct.pDepthStencilState) {
        vkpipelinedepthstencilstatecreateinfo_struct_wrapper class2(m_struct.pDepthStencilState);
        class2.set_indent(m_indent + 4);
        class2.display_full_txt();
    }
    if (m_struct.pMultisampleState) {
        vkpipelinemultisamplestatecreateinfo_struct_wrapper class3(m_struct.pMultisampleState);
        class3.set_indent(m_indent + 4);
        class3.display_full_txt();
    }
    if (m_struct.pRasterizationState) {
        vkpipelinerasterizationstatecreateinfo_struct_wrapper class4(m_struct.pRasterizationState);
        class4.set_indent(m_indent + 4);
        class4.display_full_txt();
    }
    if (m_struct.pViewportState) {
        vkpipelineviewportstatecreateinfo_struct_wrapper class5(m_struct.pViewportState);
        class5.set_indent(m_indent + 4);
        class5.display_full_txt();
    }
    if (m_struct.pTessellationState) {
        vkpipelinetessellationstatecreateinfo_struct_wrapper class6(m_struct.pTessellationState);
        class6.set_indent(m_indent + 4);
        class6.display_full_txt();
    }
    if (m_struct.pInputAssemblyState) {
        vkpipelineinputassemblystatecreateinfo_struct_wrapper class7(m_struct.pInputAssemblyState);
        class7.set_indent(m_indent + 4);
        class7.display_full_txt();
    }
    if (m_struct.pVertexInputState) {
        vkpipelinevertexinputstatecreateinfo_struct_wrapper class8(m_struct.pVertexInputState);
        class8.set_indent(m_indent + 4);
        class8.display_full_txt();
    }
    uint32_t i;
    for (i = 0; i<stageCount; i++) {
            vkpipelineshaderstagecreateinfo_struct_wrapper class9(&(m_struct.pStages[i]));
            class9.set_indent(m_indent + 4);
            class9.display_full_txt();
    }
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkimageblit_struct_wrapper class definition
vkimageblit_struct_wrapper::vkimageblit_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkimageblit_struct_wrapper::vkimageblit_struct_wrapper(VkImageBlit* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkimageblit_struct_wrapper::vkimageblit_struct_wrapper(const VkImageBlit* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkimageblit_struct_wrapper::~vkimageblit_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkimageblit_struct_wrapper::display_single_txt()
{
    printf(" %*sVkImageBlit = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkimageblit_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssrcSubresource = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.srcSubresource));
    uint32_t i;
    for (i = 0; i<2; i++) {
        printf("%*s    %ssrcOffsets[%u] = 0x%p\n", m_indent, "", &m_dummy_prefix, i, (void*)&(m_struct.srcOffsets)[i]);
    }
    printf("%*s    %sdstSubresource = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.dstSubresource));
    for (i = 0; i<2; i++) {
        printf("%*s    %sdstOffsets[%u] = 0x%p\n", m_indent, "", &m_dummy_prefix, i, (void*)&(m_struct.dstOffsets)[i]);
    }
}

// Output all struct elements, each on their own line
void vkimageblit_struct_wrapper::display_txt()
{
    printf("%*sVkImageBlit struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkimageblit_struct_wrapper::display_full_txt()
{
    printf("%*sVkImageBlit struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    uint32_t i;
    for (i = 0; i<2; i++) {
            vkoffset3d_struct_wrapper class0(&(m_struct.dstOffsets[i]));
            class0.set_indent(m_indent + 4);
            class0.display_full_txt();
    }
    if (&m_struct.dstSubresource) {
        vkimagesubresourcelayers_struct_wrapper class1(&m_struct.dstSubresource);
        class1.set_indent(m_indent + 4);
        class1.display_full_txt();
    }
    for (i = 0; i<2; i++) {
            vkoffset3d_struct_wrapper class2(&(m_struct.srcOffsets[i]));
            class2.set_indent(m_indent + 4);
            class2.display_full_txt();
    }
    if (&m_struct.srcSubresource) {
        vkimagesubresourcelayers_struct_wrapper class3(&m_struct.srcSubresource);
        class3.set_indent(m_indent + 4);
        class3.display_full_txt();
    }
}


// vkimagecopy_struct_wrapper class definition
vkimagecopy_struct_wrapper::vkimagecopy_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkimagecopy_struct_wrapper::vkimagecopy_struct_wrapper(VkImageCopy* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkimagecopy_struct_wrapper::vkimagecopy_struct_wrapper(const VkImageCopy* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkimagecopy_struct_wrapper::~vkimagecopy_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkimagecopy_struct_wrapper::display_single_txt()
{
    printf(" %*sVkImageCopy = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkimagecopy_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssrcSubresource = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.srcSubresource));
    printf("%*s    %ssrcOffset = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.srcOffset));
    printf("%*s    %sdstSubresource = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.dstSubresource));
    printf("%*s    %sdstOffset = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.dstOffset));
    printf("%*s    %sextent = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.extent));
}

// Output all struct elements, each on their own line
void vkimagecopy_struct_wrapper::display_txt()
{
    printf("%*sVkImageCopy struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkimagecopy_struct_wrapper::display_full_txt()
{
    printf("%*sVkImageCopy struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (&m_struct.extent) {
        vkextent3d_struct_wrapper class0(&m_struct.extent);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
    if (&m_struct.dstOffset) {
        vkoffset3d_struct_wrapper class1(&m_struct.dstOffset);
        class1.set_indent(m_indent + 4);
        class1.display_full_txt();
    }
    if (&m_struct.dstSubresource) {
        vkimagesubresourcelayers_struct_wrapper class2(&m_struct.dstSubresource);
        class2.set_indent(m_indent + 4);
        class2.display_full_txt();
    }
    if (&m_struct.srcOffset) {
        vkoffset3d_struct_wrapper class3(&m_struct.srcOffset);
        class3.set_indent(m_indent + 4);
        class3.display_full_txt();
    }
    if (&m_struct.srcSubresource) {
        vkimagesubresourcelayers_struct_wrapper class4(&m_struct.srcSubresource);
        class4.set_indent(m_indent + 4);
        class4.display_full_txt();
    }
}


// vkimagecreateinfo_struct_wrapper class definition
vkimagecreateinfo_struct_wrapper::vkimagecreateinfo_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkimagecreateinfo_struct_wrapper::vkimagecreateinfo_struct_wrapper(VkImageCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkimagecreateinfo_struct_wrapper::vkimagecreateinfo_struct_wrapper(const VkImageCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkimagecreateinfo_struct_wrapper::~vkimagecreateinfo_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkimagecreateinfo_struct_wrapper::display_single_txt()
{
    printf(" %*sVkImageCreateInfo = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkimagecreateinfo_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
    printf("%*s    %simageType = %s\n", m_indent, "", &m_dummy_prefix, string_VkImageType(m_struct.imageType));
    printf("%*s    %sformat = %s\n", m_indent, "", &m_dummy_prefix, string_VkFormat(m_struct.format));
    printf("%*s    %sextent = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.extent));
    printf("%*s    %smipLevels = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.mipLevels));
    printf("%*s    %sarrayLayers = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.arrayLayers));
    printf("%*s    %ssamples = %s\n", m_indent, "", &m_dummy_prefix, string_VkSampleCountFlagBits(m_struct.samples));
    printf("%*s    %stiling = %s\n", m_indent, "", &m_dummy_prefix, string_VkImageTiling(m_struct.tiling));
    printf("%*s    %susage = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.usage));
    printf("%*s    %ssharingMode = %s\n", m_indent, "", &m_dummy_prefix, string_VkSharingMode(m_struct.sharingMode));
    printf("%*s    %squeueFamilyIndexCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.queueFamilyIndexCount));
    uint32_t i;
    for (i = 0; i<queueFamilyIndexCount; i++) {
        printf("%*s    %spQueueFamilyIndices[%u] = %u\n", m_indent, "", &m_dummy_prefix, i, (m_struct.pQueueFamilyIndices)[i]);
    }
    printf("%*s    %sinitialLayout = %s\n", m_indent, "", &m_dummy_prefix, string_VkImageLayout(m_struct.initialLayout));
}

// Output all struct elements, each on their own line
void vkimagecreateinfo_struct_wrapper::display_txt()
{
    printf("%*sVkImageCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkimagecreateinfo_struct_wrapper::display_full_txt()
{
    printf("%*sVkImageCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (&m_struct.extent) {
        vkextent3d_struct_wrapper class0(&m_struct.extent);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkimageformatproperties_struct_wrapper class definition
vkimageformatproperties_struct_wrapper::vkimageformatproperties_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkimageformatproperties_struct_wrapper::vkimageformatproperties_struct_wrapper(VkImageFormatProperties* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkimageformatproperties_struct_wrapper::vkimageformatproperties_struct_wrapper(const VkImageFormatProperties* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkimageformatproperties_struct_wrapper::~vkimageformatproperties_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkimageformatproperties_struct_wrapper::display_single_txt()
{
    printf(" %*sVkImageFormatProperties = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkimageformatproperties_struct_wrapper::display_struct_members()
{
    printf("%*s    %smaxExtent = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.maxExtent));
    printf("%*s    %smaxMipLevels = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxMipLevels));
    printf("%*s    %smaxArrayLayers = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxArrayLayers));
    printf("%*s    %ssampleCounts = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.sampleCounts));
    printf("%*s    %smaxResourceSize = " PRINTF_SIZE_T_SPECIFIER "\n", m_indent, "", &m_dummy_prefix, (m_struct.maxResourceSize));
}

// Output all struct elements, each on their own line
void vkimageformatproperties_struct_wrapper::display_txt()
{
    printf("%*sVkImageFormatProperties struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkimageformatproperties_struct_wrapper::display_full_txt()
{
    printf("%*sVkImageFormatProperties struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (&m_struct.maxExtent) {
        vkextent3d_struct_wrapper class0(&m_struct.maxExtent);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
}


// vkimagememorybarrier_struct_wrapper class definition
vkimagememorybarrier_struct_wrapper::vkimagememorybarrier_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkimagememorybarrier_struct_wrapper::vkimagememorybarrier_struct_wrapper(VkImageMemoryBarrier* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkimagememorybarrier_struct_wrapper::vkimagememorybarrier_struct_wrapper(const VkImageMemoryBarrier* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkimagememorybarrier_struct_wrapper::~vkimagememorybarrier_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkimagememorybarrier_struct_wrapper::display_single_txt()
{
    printf(" %*sVkImageMemoryBarrier = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkimagememorybarrier_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %ssrcAccessMask = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.srcAccessMask));
    printf("%*s    %sdstAccessMask = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.dstAccessMask));
    printf("%*s    %soldLayout = %s\n", m_indent, "", &m_dummy_prefix, string_VkImageLayout(m_struct.oldLayout));
    printf("%*s    %snewLayout = %s\n", m_indent, "", &m_dummy_prefix, string_VkImageLayout(m_struct.newLayout));
    printf("%*s    %ssrcQueueFamilyIndex = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.srcQueueFamilyIndex));
    printf("%*s    %sdstQueueFamilyIndex = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.dstQueueFamilyIndex));
    printf("%*s    %simage = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.image));
    printf("%*s    %ssubresourceRange = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.subresourceRange));
}

// Output all struct elements, each on their own line
void vkimagememorybarrier_struct_wrapper::display_txt()
{
    printf("%*sVkImageMemoryBarrier struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkimagememorybarrier_struct_wrapper::display_full_txt()
{
    printf("%*sVkImageMemoryBarrier struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (&m_struct.subresourceRange) {
        vkimagesubresourcerange_struct_wrapper class0(&m_struct.subresourceRange);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkimageresolve_struct_wrapper class definition
vkimageresolve_struct_wrapper::vkimageresolve_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkimageresolve_struct_wrapper::vkimageresolve_struct_wrapper(VkImageResolve* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkimageresolve_struct_wrapper::vkimageresolve_struct_wrapper(const VkImageResolve* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkimageresolve_struct_wrapper::~vkimageresolve_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkimageresolve_struct_wrapper::display_single_txt()
{
    printf(" %*sVkImageResolve = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkimageresolve_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssrcSubresource = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.srcSubresource));
    printf("%*s    %ssrcOffset = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.srcOffset));
    printf("%*s    %sdstSubresource = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.dstSubresource));
    printf("%*s    %sdstOffset = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.dstOffset));
    printf("%*s    %sextent = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.extent));
}

// Output all struct elements, each on their own line
void vkimageresolve_struct_wrapper::display_txt()
{
    printf("%*sVkImageResolve struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkimageresolve_struct_wrapper::display_full_txt()
{
    printf("%*sVkImageResolve struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (&m_struct.extent) {
        vkextent3d_struct_wrapper class0(&m_struct.extent);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
    if (&m_struct.dstOffset) {
        vkoffset3d_struct_wrapper class1(&m_struct.dstOffset);
        class1.set_indent(m_indent + 4);
        class1.display_full_txt();
    }
    if (&m_struct.dstSubresource) {
        vkimagesubresourcelayers_struct_wrapper class2(&m_struct.dstSubresource);
        class2.set_indent(m_indent + 4);
        class2.display_full_txt();
    }
    if (&m_struct.srcOffset) {
        vkoffset3d_struct_wrapper class3(&m_struct.srcOffset);
        class3.set_indent(m_indent + 4);
        class3.display_full_txt();
    }
    if (&m_struct.srcSubresource) {
        vkimagesubresourcelayers_struct_wrapper class4(&m_struct.srcSubresource);
        class4.set_indent(m_indent + 4);
        class4.display_full_txt();
    }
}


// vkimagesubresource_struct_wrapper class definition
vkimagesubresource_struct_wrapper::vkimagesubresource_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkimagesubresource_struct_wrapper::vkimagesubresource_struct_wrapper(VkImageSubresource* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkimagesubresource_struct_wrapper::vkimagesubresource_struct_wrapper(const VkImageSubresource* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkimagesubresource_struct_wrapper::~vkimagesubresource_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkimagesubresource_struct_wrapper::display_single_txt()
{
    printf(" %*sVkImageSubresource = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkimagesubresource_struct_wrapper::display_struct_members()
{
    printf("%*s    %saspectMask = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.aspectMask));
    printf("%*s    %smipLevel = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.mipLevel));
    printf("%*s    %sarrayLayer = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.arrayLayer));
}

// Output all struct elements, each on their own line
void vkimagesubresource_struct_wrapper::display_txt()
{
    printf("%*sVkImageSubresource struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkimagesubresource_struct_wrapper::display_full_txt()
{
    printf("%*sVkImageSubresource struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// vkimagesubresourcelayers_struct_wrapper class definition
vkimagesubresourcelayers_struct_wrapper::vkimagesubresourcelayers_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkimagesubresourcelayers_struct_wrapper::vkimagesubresourcelayers_struct_wrapper(VkImageSubresourceLayers* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkimagesubresourcelayers_struct_wrapper::vkimagesubresourcelayers_struct_wrapper(const VkImageSubresourceLayers* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkimagesubresourcelayers_struct_wrapper::~vkimagesubresourcelayers_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkimagesubresourcelayers_struct_wrapper::display_single_txt()
{
    printf(" %*sVkImageSubresourceLayers = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkimagesubresourcelayers_struct_wrapper::display_struct_members()
{
    printf("%*s    %saspectMask = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.aspectMask));
    printf("%*s    %smipLevel = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.mipLevel));
    printf("%*s    %sbaseArrayLayer = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.baseArrayLayer));
    printf("%*s    %slayerCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.layerCount));
}

// Output all struct elements, each on their own line
void vkimagesubresourcelayers_struct_wrapper::display_txt()
{
    printf("%*sVkImageSubresourceLayers struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkimagesubresourcelayers_struct_wrapper::display_full_txt()
{
    printf("%*sVkImageSubresourceLayers struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// vkimagesubresourcerange_struct_wrapper class definition
vkimagesubresourcerange_struct_wrapper::vkimagesubresourcerange_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkimagesubresourcerange_struct_wrapper::vkimagesubresourcerange_struct_wrapper(VkImageSubresourceRange* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkimagesubresourcerange_struct_wrapper::vkimagesubresourcerange_struct_wrapper(const VkImageSubresourceRange* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkimagesubresourcerange_struct_wrapper::~vkimagesubresourcerange_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkimagesubresourcerange_struct_wrapper::display_single_txt()
{
    printf(" %*sVkImageSubresourceRange = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkimagesubresourcerange_struct_wrapper::display_struct_members()
{
    printf("%*s    %saspectMask = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.aspectMask));
    printf("%*s    %sbaseMipLevel = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.baseMipLevel));
    printf("%*s    %slevelCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.levelCount));
    printf("%*s    %sbaseArrayLayer = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.baseArrayLayer));
    printf("%*s    %slayerCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.layerCount));
}

// Output all struct elements, each on their own line
void vkimagesubresourcerange_struct_wrapper::display_txt()
{
    printf("%*sVkImageSubresourceRange struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkimagesubresourcerange_struct_wrapper::display_full_txt()
{
    printf("%*sVkImageSubresourceRange struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// vkimageviewcreateinfo_struct_wrapper class definition
vkimageviewcreateinfo_struct_wrapper::vkimageviewcreateinfo_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkimageviewcreateinfo_struct_wrapper::vkimageviewcreateinfo_struct_wrapper(VkImageViewCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkimageviewcreateinfo_struct_wrapper::vkimageviewcreateinfo_struct_wrapper(const VkImageViewCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkimageviewcreateinfo_struct_wrapper::~vkimageviewcreateinfo_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkimageviewcreateinfo_struct_wrapper::display_single_txt()
{
    printf(" %*sVkImageViewCreateInfo = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkimageviewcreateinfo_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
    printf("%*s    %simage = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.image));
    printf("%*s    %sviewType = %s\n", m_indent, "", &m_dummy_prefix, string_VkImageViewType(m_struct.viewType));
    printf("%*s    %sformat = %s\n", m_indent, "", &m_dummy_prefix, string_VkFormat(m_struct.format));
    printf("%*s    %scomponents = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.components));
    printf("%*s    %ssubresourceRange = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.subresourceRange));
}

// Output all struct elements, each on their own line
void vkimageviewcreateinfo_struct_wrapper::display_txt()
{
    printf("%*sVkImageViewCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkimageviewcreateinfo_struct_wrapper::display_full_txt()
{
    printf("%*sVkImageViewCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (&m_struct.subresourceRange) {
        vkimagesubresourcerange_struct_wrapper class0(&m_struct.subresourceRange);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
    if (&m_struct.components) {
        vkcomponentmapping_struct_wrapper class1(&m_struct.components);
        class1.set_indent(m_indent + 4);
        class1.display_full_txt();
    }
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkimportmemorywin32handleinfonv_struct_wrapper class definition
vkimportmemorywin32handleinfonv_struct_wrapper::vkimportmemorywin32handleinfonv_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkimportmemorywin32handleinfonv_struct_wrapper::vkimportmemorywin32handleinfonv_struct_wrapper(VkImportMemoryWin32HandleInfoNV* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkimportmemorywin32handleinfonv_struct_wrapper::vkimportmemorywin32handleinfonv_struct_wrapper(const VkImportMemoryWin32HandleInfoNV* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkimportmemorywin32handleinfonv_struct_wrapper::~vkimportmemorywin32handleinfonv_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkimportmemorywin32handleinfonv_struct_wrapper::display_single_txt()
{
    printf(" %*sVkImportMemoryWin32HandleInfoNV = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkimportmemorywin32handleinfonv_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %shandleType = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.handleType));
    printf("%*s    %shandle = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.handle));
}

// Output all struct elements, each on their own line
void vkimportmemorywin32handleinfonv_struct_wrapper::display_txt()
{
    printf("%*sVkImportMemoryWin32HandleInfoNV struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkimportmemorywin32handleinfonv_struct_wrapper::display_full_txt()
{
    printf("%*sVkImportMemoryWin32HandleInfoNV struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkinstancecreateinfo_struct_wrapper class definition
vkinstancecreateinfo_struct_wrapper::vkinstancecreateinfo_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkinstancecreateinfo_struct_wrapper::vkinstancecreateinfo_struct_wrapper(VkInstanceCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkinstancecreateinfo_struct_wrapper::vkinstancecreateinfo_struct_wrapper(const VkInstanceCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkinstancecreateinfo_struct_wrapper::~vkinstancecreateinfo_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkinstancecreateinfo_struct_wrapper::display_single_txt()
{
    printf(" %*sVkInstanceCreateInfo = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkinstancecreateinfo_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
    printf("%*s    %spApplicationInfo = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.pApplicationInfo));
    printf("%*s    %senabledLayerCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.enabledLayerCount));
    uint32_t i;
    for (i = 0; i<enabledLayerCount; i++) {
        printf("%*s    %sppEnabledLayerNames = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.ppEnabledLayerNames)[0]);
    }
    printf("%*s    %senabledExtensionCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.enabledExtensionCount));
    for (i = 0; i<enabledExtensionCount; i++) {
        printf("%*s    %sppEnabledExtensionNames = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.ppEnabledExtensionNames)[0]);
    }
}

// Output all struct elements, each on their own line
void vkinstancecreateinfo_struct_wrapper::display_txt()
{
    printf("%*sVkInstanceCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkinstancecreateinfo_struct_wrapper::display_full_txt()
{
    printf("%*sVkInstanceCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pApplicationInfo) {
        vkapplicationinfo_struct_wrapper class0(m_struct.pApplicationInfo);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vklayerproperties_struct_wrapper class definition
vklayerproperties_struct_wrapper::vklayerproperties_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vklayerproperties_struct_wrapper::vklayerproperties_struct_wrapper(VkLayerProperties* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vklayerproperties_struct_wrapper::vklayerproperties_struct_wrapper(const VkLayerProperties* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vklayerproperties_struct_wrapper::~vklayerproperties_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vklayerproperties_struct_wrapper::display_single_txt()
{
    printf(" %*sVkLayerProperties = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vklayerproperties_struct_wrapper::display_struct_members()
{
    uint32_t i;
    for (i = 0; i<VK_MAX_EXTENSION_NAME_SIZE; i++) {
        printf("%*s    %slayerName = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.layerName));
    }
    printf("%*s    %sspecVersion = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.specVersion));
    printf("%*s    %simplementationVersion = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.implementationVersion));
    for (i = 0; i<VK_MAX_DESCRIPTION_SIZE; i++) {
        printf("%*s    %sdescription = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.description));
    }
}

// Output all struct elements, each on their own line
void vklayerproperties_struct_wrapper::display_txt()
{
    printf("%*sVkLayerProperties struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vklayerproperties_struct_wrapper::display_full_txt()
{
    printf("%*sVkLayerProperties struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// vkmappedmemoryrange_struct_wrapper class definition
vkmappedmemoryrange_struct_wrapper::vkmappedmemoryrange_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkmappedmemoryrange_struct_wrapper::vkmappedmemoryrange_struct_wrapper(VkMappedMemoryRange* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkmappedmemoryrange_struct_wrapper::vkmappedmemoryrange_struct_wrapper(const VkMappedMemoryRange* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkmappedmemoryrange_struct_wrapper::~vkmappedmemoryrange_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkmappedmemoryrange_struct_wrapper::display_single_txt()
{
    printf(" %*sVkMappedMemoryRange = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkmappedmemoryrange_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %smemory = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.memory));
    printf("%*s    %soffset = " PRINTF_SIZE_T_SPECIFIER "\n", m_indent, "", &m_dummy_prefix, (m_struct.offset));
    printf("%*s    %ssize = " PRINTF_SIZE_T_SPECIFIER "\n", m_indent, "", &m_dummy_prefix, (m_struct.size));
}

// Output all struct elements, each on their own line
void vkmappedmemoryrange_struct_wrapper::display_txt()
{
    printf("%*sVkMappedMemoryRange struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkmappedmemoryrange_struct_wrapper::display_full_txt()
{
    printf("%*sVkMappedMemoryRange struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkmemoryallocateinfo_struct_wrapper class definition
vkmemoryallocateinfo_struct_wrapper::vkmemoryallocateinfo_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkmemoryallocateinfo_struct_wrapper::vkmemoryallocateinfo_struct_wrapper(VkMemoryAllocateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkmemoryallocateinfo_struct_wrapper::vkmemoryallocateinfo_struct_wrapper(const VkMemoryAllocateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkmemoryallocateinfo_struct_wrapper::~vkmemoryallocateinfo_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkmemoryallocateinfo_struct_wrapper::display_single_txt()
{
    printf(" %*sVkMemoryAllocateInfo = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkmemoryallocateinfo_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sallocationSize = " PRINTF_SIZE_T_SPECIFIER "\n", m_indent, "", &m_dummy_prefix, (m_struct.allocationSize));
    printf("%*s    %smemoryTypeIndex = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.memoryTypeIndex));
}

// Output all struct elements, each on their own line
void vkmemoryallocateinfo_struct_wrapper::display_txt()
{
    printf("%*sVkMemoryAllocateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkmemoryallocateinfo_struct_wrapper::display_full_txt()
{
    printf("%*sVkMemoryAllocateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkmemorybarrier_struct_wrapper class definition
vkmemorybarrier_struct_wrapper::vkmemorybarrier_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkmemorybarrier_struct_wrapper::vkmemorybarrier_struct_wrapper(VkMemoryBarrier* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkmemorybarrier_struct_wrapper::vkmemorybarrier_struct_wrapper(const VkMemoryBarrier* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkmemorybarrier_struct_wrapper::~vkmemorybarrier_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkmemorybarrier_struct_wrapper::display_single_txt()
{
    printf(" %*sVkMemoryBarrier = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkmemorybarrier_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %ssrcAccessMask = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.srcAccessMask));
    printf("%*s    %sdstAccessMask = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.dstAccessMask));
}

// Output all struct elements, each on their own line
void vkmemorybarrier_struct_wrapper::display_txt()
{
    printf("%*sVkMemoryBarrier struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkmemorybarrier_struct_wrapper::display_full_txt()
{
    printf("%*sVkMemoryBarrier struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkmemoryheap_struct_wrapper class definition
vkmemoryheap_struct_wrapper::vkmemoryheap_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkmemoryheap_struct_wrapper::vkmemoryheap_struct_wrapper(VkMemoryHeap* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkmemoryheap_struct_wrapper::vkmemoryheap_struct_wrapper(const VkMemoryHeap* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkmemoryheap_struct_wrapper::~vkmemoryheap_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkmemoryheap_struct_wrapper::display_single_txt()
{
    printf(" %*sVkMemoryHeap = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkmemoryheap_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssize = " PRINTF_SIZE_T_SPECIFIER "\n", m_indent, "", &m_dummy_prefix, (m_struct.size));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
}

// Output all struct elements, each on their own line
void vkmemoryheap_struct_wrapper::display_txt()
{
    printf("%*sVkMemoryHeap struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkmemoryheap_struct_wrapper::display_full_txt()
{
    printf("%*sVkMemoryHeap struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// vkmemoryrequirements_struct_wrapper class definition
vkmemoryrequirements_struct_wrapper::vkmemoryrequirements_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkmemoryrequirements_struct_wrapper::vkmemoryrequirements_struct_wrapper(VkMemoryRequirements* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkmemoryrequirements_struct_wrapper::vkmemoryrequirements_struct_wrapper(const VkMemoryRequirements* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkmemoryrequirements_struct_wrapper::~vkmemoryrequirements_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkmemoryrequirements_struct_wrapper::display_single_txt()
{
    printf(" %*sVkMemoryRequirements = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkmemoryrequirements_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssize = " PRINTF_SIZE_T_SPECIFIER "\n", m_indent, "", &m_dummy_prefix, (m_struct.size));
    printf("%*s    %salignment = " PRINTF_SIZE_T_SPECIFIER "\n", m_indent, "", &m_dummy_prefix, (m_struct.alignment));
    printf("%*s    %smemoryTypeBits = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.memoryTypeBits));
}

// Output all struct elements, each on their own line
void vkmemoryrequirements_struct_wrapper::display_txt()
{
    printf("%*sVkMemoryRequirements struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkmemoryrequirements_struct_wrapper::display_full_txt()
{
    printf("%*sVkMemoryRequirements struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// vkmemorytype_struct_wrapper class definition
vkmemorytype_struct_wrapper::vkmemorytype_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkmemorytype_struct_wrapper::vkmemorytype_struct_wrapper(VkMemoryType* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkmemorytype_struct_wrapper::vkmemorytype_struct_wrapper(const VkMemoryType* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkmemorytype_struct_wrapper::~vkmemorytype_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkmemorytype_struct_wrapper::display_single_txt()
{
    printf(" %*sVkMemoryType = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkmemorytype_struct_wrapper::display_struct_members()
{
    printf("%*s    %spropertyFlags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.propertyFlags));
    printf("%*s    %sheapIndex = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.heapIndex));
}

// Output all struct elements, each on their own line
void vkmemorytype_struct_wrapper::display_txt()
{
    printf("%*sVkMemoryType struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkmemorytype_struct_wrapper::display_full_txt()
{
    printf("%*sVkMemoryType struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// vkmirsurfacecreateinfokhr_struct_wrapper class definition
vkmirsurfacecreateinfokhr_struct_wrapper::vkmirsurfacecreateinfokhr_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkmirsurfacecreateinfokhr_struct_wrapper::vkmirsurfacecreateinfokhr_struct_wrapper(VkMirSurfaceCreateInfoKHR* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkmirsurfacecreateinfokhr_struct_wrapper::vkmirsurfacecreateinfokhr_struct_wrapper(const VkMirSurfaceCreateInfoKHR* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkmirsurfacecreateinfokhr_struct_wrapper::~vkmirsurfacecreateinfokhr_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkmirsurfacecreateinfokhr_struct_wrapper::display_single_txt()
{
    printf(" %*sVkMirSurfaceCreateInfoKHR = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkmirsurfacecreateinfokhr_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
    printf("%*s    %sconnection = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.connection));
    printf("%*s    %smirSurface = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.mirSurface));
}

// Output all struct elements, each on their own line
void vkmirsurfacecreateinfokhr_struct_wrapper::display_txt()
{
    printf("%*sVkMirSurfaceCreateInfoKHR struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkmirsurfacecreateinfokhr_struct_wrapper::display_full_txt()
{
    printf("%*sVkMirSurfaceCreateInfoKHR struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkoffset2d_struct_wrapper class definition
vkoffset2d_struct_wrapper::vkoffset2d_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkoffset2d_struct_wrapper::vkoffset2d_struct_wrapper(VkOffset2D* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkoffset2d_struct_wrapper::vkoffset2d_struct_wrapper(const VkOffset2D* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkoffset2d_struct_wrapper::~vkoffset2d_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkoffset2d_struct_wrapper::display_single_txt()
{
    printf(" %*sVkOffset2D = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkoffset2d_struct_wrapper::display_struct_members()
{
    printf("%*s    %sx = %i\n", m_indent, "", &m_dummy_prefix, (m_struct.x));
    printf("%*s    %sy = %i\n", m_indent, "", &m_dummy_prefix, (m_struct.y));
}

// Output all struct elements, each on their own line
void vkoffset2d_struct_wrapper::display_txt()
{
    printf("%*sVkOffset2D struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkoffset2d_struct_wrapper::display_full_txt()
{
    printf("%*sVkOffset2D struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// vkoffset3d_struct_wrapper class definition
vkoffset3d_struct_wrapper::vkoffset3d_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkoffset3d_struct_wrapper::vkoffset3d_struct_wrapper(VkOffset3D* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkoffset3d_struct_wrapper::vkoffset3d_struct_wrapper(const VkOffset3D* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkoffset3d_struct_wrapper::~vkoffset3d_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkoffset3d_struct_wrapper::display_single_txt()
{
    printf(" %*sVkOffset3D = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkoffset3d_struct_wrapper::display_struct_members()
{
    printf("%*s    %sx = %i\n", m_indent, "", &m_dummy_prefix, (m_struct.x));
    printf("%*s    %sy = %i\n", m_indent, "", &m_dummy_prefix, (m_struct.y));
    printf("%*s    %sz = %i\n", m_indent, "", &m_dummy_prefix, (m_struct.z));
}

// Output all struct elements, each on their own line
void vkoffset3d_struct_wrapper::display_txt()
{
    printf("%*sVkOffset3D struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkoffset3d_struct_wrapper::display_full_txt()
{
    printf("%*sVkOffset3D struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// vkphysicaldevicefeatures_struct_wrapper class definition
vkphysicaldevicefeatures_struct_wrapper::vkphysicaldevicefeatures_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkphysicaldevicefeatures_struct_wrapper::vkphysicaldevicefeatures_struct_wrapper(VkPhysicalDeviceFeatures* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkphysicaldevicefeatures_struct_wrapper::vkphysicaldevicefeatures_struct_wrapper(const VkPhysicalDeviceFeatures* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkphysicaldevicefeatures_struct_wrapper::~vkphysicaldevicefeatures_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkphysicaldevicefeatures_struct_wrapper::display_single_txt()
{
    printf(" %*sVkPhysicalDeviceFeatures = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkphysicaldevicefeatures_struct_wrapper::display_struct_members()
{
    printf("%*s    %srobustBufferAccess = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.robustBufferAccess) ? "TRUE" : "FALSE");
    printf("%*s    %sfullDrawIndexUint32 = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.fullDrawIndexUint32) ? "TRUE" : "FALSE");
    printf("%*s    %simageCubeArray = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.imageCubeArray) ? "TRUE" : "FALSE");
    printf("%*s    %sindependentBlend = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.independentBlend) ? "TRUE" : "FALSE");
    printf("%*s    %sgeometryShader = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.geometryShader) ? "TRUE" : "FALSE");
    printf("%*s    %stessellationShader = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.tessellationShader) ? "TRUE" : "FALSE");
    printf("%*s    %ssampleRateShading = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.sampleRateShading) ? "TRUE" : "FALSE");
    printf("%*s    %sdualSrcBlend = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.dualSrcBlend) ? "TRUE" : "FALSE");
    printf("%*s    %slogicOp = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.logicOp) ? "TRUE" : "FALSE");
    printf("%*s    %smultiDrawIndirect = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.multiDrawIndirect) ? "TRUE" : "FALSE");
    printf("%*s    %sdrawIndirectFirstInstance = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.drawIndirectFirstInstance) ? "TRUE" : "FALSE");
    printf("%*s    %sdepthClamp = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.depthClamp) ? "TRUE" : "FALSE");
    printf("%*s    %sdepthBiasClamp = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.depthBiasClamp) ? "TRUE" : "FALSE");
    printf("%*s    %sfillModeNonSolid = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.fillModeNonSolid) ? "TRUE" : "FALSE");
    printf("%*s    %sdepthBounds = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.depthBounds) ? "TRUE" : "FALSE");
    printf("%*s    %swideLines = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.wideLines) ? "TRUE" : "FALSE");
    printf("%*s    %slargePoints = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.largePoints) ? "TRUE" : "FALSE");
    printf("%*s    %salphaToOne = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.alphaToOne) ? "TRUE" : "FALSE");
    printf("%*s    %smultiViewport = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.multiViewport) ? "TRUE" : "FALSE");
    printf("%*s    %ssamplerAnisotropy = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.samplerAnisotropy) ? "TRUE" : "FALSE");
    printf("%*s    %stextureCompressionETC2 = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.textureCompressionETC2) ? "TRUE" : "FALSE");
    printf("%*s    %stextureCompressionASTC_LDR = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.textureCompressionASTC_LDR) ? "TRUE" : "FALSE");
    printf("%*s    %stextureCompressionBC = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.textureCompressionBC) ? "TRUE" : "FALSE");
    printf("%*s    %socclusionQueryPrecise = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.occlusionQueryPrecise) ? "TRUE" : "FALSE");
    printf("%*s    %spipelineStatisticsQuery = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.pipelineStatisticsQuery) ? "TRUE" : "FALSE");
    printf("%*s    %svertexPipelineStoresAndAtomics = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.vertexPipelineStoresAndAtomics) ? "TRUE" : "FALSE");
    printf("%*s    %sfragmentStoresAndAtomics = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.fragmentStoresAndAtomics) ? "TRUE" : "FALSE");
    printf("%*s    %sshaderTessellationAndGeometryPointSize = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.shaderTessellationAndGeometryPointSize) ? "TRUE" : "FALSE");
    printf("%*s    %sshaderImageGatherExtended = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.shaderImageGatherExtended) ? "TRUE" : "FALSE");
    printf("%*s    %sshaderStorageImageExtendedFormats = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.shaderStorageImageExtendedFormats) ? "TRUE" : "FALSE");
    printf("%*s    %sshaderStorageImageMultisample = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.shaderStorageImageMultisample) ? "TRUE" : "FALSE");
    printf("%*s    %sshaderStorageImageReadWithoutFormat = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.shaderStorageImageReadWithoutFormat) ? "TRUE" : "FALSE");
    printf("%*s    %sshaderStorageImageWriteWithoutFormat = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.shaderStorageImageWriteWithoutFormat) ? "TRUE" : "FALSE");
    printf("%*s    %sshaderUniformBufferArrayDynamicIndexing = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.shaderUniformBufferArrayDynamicIndexing) ? "TRUE" : "FALSE");
    printf("%*s    %sshaderSampledImageArrayDynamicIndexing = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.shaderSampledImageArrayDynamicIndexing) ? "TRUE" : "FALSE");
    printf("%*s    %sshaderStorageBufferArrayDynamicIndexing = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.shaderStorageBufferArrayDynamicIndexing) ? "TRUE" : "FALSE");
    printf("%*s    %sshaderStorageImageArrayDynamicIndexing = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.shaderStorageImageArrayDynamicIndexing) ? "TRUE" : "FALSE");
    printf("%*s    %sshaderClipDistance = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.shaderClipDistance) ? "TRUE" : "FALSE");
    printf("%*s    %sshaderCullDistance = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.shaderCullDistance) ? "TRUE" : "FALSE");
    printf("%*s    %sshaderFloat64 = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.shaderFloat64) ? "TRUE" : "FALSE");
    printf("%*s    %sshaderInt64 = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.shaderInt64) ? "TRUE" : "FALSE");
    printf("%*s    %sshaderInt16 = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.shaderInt16) ? "TRUE" : "FALSE");
    printf("%*s    %sshaderResourceResidency = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.shaderResourceResidency) ? "TRUE" : "FALSE");
    printf("%*s    %sshaderResourceMinLod = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.shaderResourceMinLod) ? "TRUE" : "FALSE");
    printf("%*s    %ssparseBinding = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.sparseBinding) ? "TRUE" : "FALSE");
    printf("%*s    %ssparseResidencyBuffer = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.sparseResidencyBuffer) ? "TRUE" : "FALSE");
    printf("%*s    %ssparseResidencyImage2D = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.sparseResidencyImage2D) ? "TRUE" : "FALSE");
    printf("%*s    %ssparseResidencyImage3D = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.sparseResidencyImage3D) ? "TRUE" : "FALSE");
    printf("%*s    %ssparseResidency2Samples = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.sparseResidency2Samples) ? "TRUE" : "FALSE");
    printf("%*s    %ssparseResidency4Samples = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.sparseResidency4Samples) ? "TRUE" : "FALSE");
    printf("%*s    %ssparseResidency8Samples = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.sparseResidency8Samples) ? "TRUE" : "FALSE");
    printf("%*s    %ssparseResidency16Samples = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.sparseResidency16Samples) ? "TRUE" : "FALSE");
    printf("%*s    %ssparseResidencyAliased = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.sparseResidencyAliased) ? "TRUE" : "FALSE");
    printf("%*s    %svariableMultisampleRate = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.variableMultisampleRate) ? "TRUE" : "FALSE");
    printf("%*s    %sinheritedQueries = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.inheritedQueries) ? "TRUE" : "FALSE");
}

// Output all struct elements, each on their own line
void vkphysicaldevicefeatures_struct_wrapper::display_txt()
{
    printf("%*sVkPhysicalDeviceFeatures struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkphysicaldevicefeatures_struct_wrapper::display_full_txt()
{
    printf("%*sVkPhysicalDeviceFeatures struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// vkphysicaldevicelimits_struct_wrapper class definition
vkphysicaldevicelimits_struct_wrapper::vkphysicaldevicelimits_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkphysicaldevicelimits_struct_wrapper::vkphysicaldevicelimits_struct_wrapper(VkPhysicalDeviceLimits* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkphysicaldevicelimits_struct_wrapper::vkphysicaldevicelimits_struct_wrapper(const VkPhysicalDeviceLimits* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkphysicaldevicelimits_struct_wrapper::~vkphysicaldevicelimits_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkphysicaldevicelimits_struct_wrapper::display_single_txt()
{
    printf(" %*sVkPhysicalDeviceLimits = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkphysicaldevicelimits_struct_wrapper::display_struct_members()
{
    printf("%*s    %smaxImageDimension1D = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxImageDimension1D));
    printf("%*s    %smaxImageDimension2D = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxImageDimension2D));
    printf("%*s    %smaxImageDimension3D = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxImageDimension3D));
    printf("%*s    %smaxImageDimensionCube = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxImageDimensionCube));
    printf("%*s    %smaxImageArrayLayers = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxImageArrayLayers));
    printf("%*s    %smaxTexelBufferElements = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxTexelBufferElements));
    printf("%*s    %smaxUniformBufferRange = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxUniformBufferRange));
    printf("%*s    %smaxStorageBufferRange = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxStorageBufferRange));
    printf("%*s    %smaxPushConstantsSize = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxPushConstantsSize));
    printf("%*s    %smaxMemoryAllocationCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxMemoryAllocationCount));
    printf("%*s    %smaxSamplerAllocationCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxSamplerAllocationCount));
    printf("%*s    %sbufferImageGranularity = " PRINTF_SIZE_T_SPECIFIER "\n", m_indent, "", &m_dummy_prefix, (m_struct.bufferImageGranularity));
    printf("%*s    %ssparseAddressSpaceSize = " PRINTF_SIZE_T_SPECIFIER "\n", m_indent, "", &m_dummy_prefix, (m_struct.sparseAddressSpaceSize));
    printf("%*s    %smaxBoundDescriptorSets = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxBoundDescriptorSets));
    printf("%*s    %smaxPerStageDescriptorSamplers = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxPerStageDescriptorSamplers));
    printf("%*s    %smaxPerStageDescriptorUniformBuffers = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxPerStageDescriptorUniformBuffers));
    printf("%*s    %smaxPerStageDescriptorStorageBuffers = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxPerStageDescriptorStorageBuffers));
    printf("%*s    %smaxPerStageDescriptorSampledImages = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxPerStageDescriptorSampledImages));
    printf("%*s    %smaxPerStageDescriptorStorageImages = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxPerStageDescriptorStorageImages));
    printf("%*s    %smaxPerStageDescriptorInputAttachments = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxPerStageDescriptorInputAttachments));
    printf("%*s    %smaxPerStageResources = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxPerStageResources));
    printf("%*s    %smaxDescriptorSetSamplers = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxDescriptorSetSamplers));
    printf("%*s    %smaxDescriptorSetUniformBuffers = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxDescriptorSetUniformBuffers));
    printf("%*s    %smaxDescriptorSetUniformBuffersDynamic = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxDescriptorSetUniformBuffersDynamic));
    printf("%*s    %smaxDescriptorSetStorageBuffers = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxDescriptorSetStorageBuffers));
    printf("%*s    %smaxDescriptorSetStorageBuffersDynamic = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxDescriptorSetStorageBuffersDynamic));
    printf("%*s    %smaxDescriptorSetSampledImages = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxDescriptorSetSampledImages));
    printf("%*s    %smaxDescriptorSetStorageImages = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxDescriptorSetStorageImages));
    printf("%*s    %smaxDescriptorSetInputAttachments = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxDescriptorSetInputAttachments));
    printf("%*s    %smaxVertexInputAttributes = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxVertexInputAttributes));
    printf("%*s    %smaxVertexInputBindings = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxVertexInputBindings));
    printf("%*s    %smaxVertexInputAttributeOffset = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxVertexInputAttributeOffset));
    printf("%*s    %smaxVertexInputBindingStride = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxVertexInputBindingStride));
    printf("%*s    %smaxVertexOutputComponents = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxVertexOutputComponents));
    printf("%*s    %smaxTessellationGenerationLevel = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxTessellationGenerationLevel));
    printf("%*s    %smaxTessellationPatchSize = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxTessellationPatchSize));
    printf("%*s    %smaxTessellationControlPerVertexInputComponents = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxTessellationControlPerVertexInputComponents));
    printf("%*s    %smaxTessellationControlPerVertexOutputComponents = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxTessellationControlPerVertexOutputComponents));
    printf("%*s    %smaxTessellationControlPerPatchOutputComponents = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxTessellationControlPerPatchOutputComponents));
    printf("%*s    %smaxTessellationControlTotalOutputComponents = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxTessellationControlTotalOutputComponents));
    printf("%*s    %smaxTessellationEvaluationInputComponents = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxTessellationEvaluationInputComponents));
    printf("%*s    %smaxTessellationEvaluationOutputComponents = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxTessellationEvaluationOutputComponents));
    printf("%*s    %smaxGeometryShaderInvocations = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxGeometryShaderInvocations));
    printf("%*s    %smaxGeometryInputComponents = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxGeometryInputComponents));
    printf("%*s    %smaxGeometryOutputComponents = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxGeometryOutputComponents));
    printf("%*s    %smaxGeometryOutputVertices = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxGeometryOutputVertices));
    printf("%*s    %smaxGeometryTotalOutputComponents = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxGeometryTotalOutputComponents));
    printf("%*s    %smaxFragmentInputComponents = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxFragmentInputComponents));
    printf("%*s    %smaxFragmentOutputAttachments = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxFragmentOutputAttachments));
    printf("%*s    %smaxFragmentDualSrcAttachments = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxFragmentDualSrcAttachments));
    printf("%*s    %smaxFragmentCombinedOutputResources = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxFragmentCombinedOutputResources));
    printf("%*s    %smaxComputeSharedMemorySize = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxComputeSharedMemorySize));
    uint32_t i;
    for (i = 0; i<3; i++) {
        printf("%*s    %smaxComputeWorkGroupCount[%u] = %u\n", m_indent, "", &m_dummy_prefix, i, (m_struct.maxComputeWorkGroupCount)[i]);
    }
    printf("%*s    %smaxComputeWorkGroupInvocations = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxComputeWorkGroupInvocations));
    for (i = 0; i<3; i++) {
        printf("%*s    %smaxComputeWorkGroupSize[%u] = %u\n", m_indent, "", &m_dummy_prefix, i, (m_struct.maxComputeWorkGroupSize)[i]);
    }
    printf("%*s    %ssubPixelPrecisionBits = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.subPixelPrecisionBits));
    printf("%*s    %ssubTexelPrecisionBits = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.subTexelPrecisionBits));
    printf("%*s    %smipmapPrecisionBits = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.mipmapPrecisionBits));
    printf("%*s    %smaxDrawIndexedIndexValue = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxDrawIndexedIndexValue));
    printf("%*s    %smaxDrawIndirectCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxDrawIndirectCount));
    printf("%*s    %smaxSamplerLodBias = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.maxSamplerLodBias));
    printf("%*s    %smaxSamplerAnisotropy = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.maxSamplerAnisotropy));
    printf("%*s    %smaxViewports = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxViewports));
    for (i = 0; i<2; i++) {
        printf("%*s    %smaxViewportDimensions[%u] = %u\n", m_indent, "", &m_dummy_prefix, i, (m_struct.maxViewportDimensions)[i]);
    }
    for (i = 0; i<2; i++) {
        printf("%*s    %sviewportBoundsRange[%u] = %f\n", m_indent, "", &m_dummy_prefix, i, (m_struct.viewportBoundsRange)[i]);
    }
    printf("%*s    %sviewportSubPixelBits = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.viewportSubPixelBits));
    printf("%*s    %sminMemoryMapAlignment = " PRINTF_SIZE_T_SPECIFIER "\n", m_indent, "", &m_dummy_prefix, (m_struct.minMemoryMapAlignment));
    printf("%*s    %sminTexelBufferOffsetAlignment = " PRINTF_SIZE_T_SPECIFIER "\n", m_indent, "", &m_dummy_prefix, (m_struct.minTexelBufferOffsetAlignment));
    printf("%*s    %sminUniformBufferOffsetAlignment = " PRINTF_SIZE_T_SPECIFIER "\n", m_indent, "", &m_dummy_prefix, (m_struct.minUniformBufferOffsetAlignment));
    printf("%*s    %sminStorageBufferOffsetAlignment = " PRINTF_SIZE_T_SPECIFIER "\n", m_indent, "", &m_dummy_prefix, (m_struct.minStorageBufferOffsetAlignment));
    printf("%*s    %sminTexelOffset = %i\n", m_indent, "", &m_dummy_prefix, (m_struct.minTexelOffset));
    printf("%*s    %smaxTexelOffset = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxTexelOffset));
    printf("%*s    %sminTexelGatherOffset = %i\n", m_indent, "", &m_dummy_prefix, (m_struct.minTexelGatherOffset));
    printf("%*s    %smaxTexelGatherOffset = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxTexelGatherOffset));
    printf("%*s    %sminInterpolationOffset = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.minInterpolationOffset));
    printf("%*s    %smaxInterpolationOffset = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.maxInterpolationOffset));
    printf("%*s    %ssubPixelInterpolationOffsetBits = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.subPixelInterpolationOffsetBits));
    printf("%*s    %smaxFramebufferWidth = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxFramebufferWidth));
    printf("%*s    %smaxFramebufferHeight = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxFramebufferHeight));
    printf("%*s    %smaxFramebufferLayers = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxFramebufferLayers));
    printf("%*s    %sframebufferColorSampleCounts = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.framebufferColorSampleCounts));
    printf("%*s    %sframebufferDepthSampleCounts = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.framebufferDepthSampleCounts));
    printf("%*s    %sframebufferStencilSampleCounts = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.framebufferStencilSampleCounts));
    printf("%*s    %sframebufferNoAttachmentsSampleCounts = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.framebufferNoAttachmentsSampleCounts));
    printf("%*s    %smaxColorAttachments = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxColorAttachments));
    printf("%*s    %ssampledImageColorSampleCounts = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.sampledImageColorSampleCounts));
    printf("%*s    %ssampledImageIntegerSampleCounts = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.sampledImageIntegerSampleCounts));
    printf("%*s    %ssampledImageDepthSampleCounts = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.sampledImageDepthSampleCounts));
    printf("%*s    %ssampledImageStencilSampleCounts = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.sampledImageStencilSampleCounts));
    printf("%*s    %sstorageImageSampleCounts = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.storageImageSampleCounts));
    printf("%*s    %smaxSampleMaskWords = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxSampleMaskWords));
    printf("%*s    %stimestampComputeAndGraphics = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.timestampComputeAndGraphics) ? "TRUE" : "FALSE");
    printf("%*s    %stimestampPeriod = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.timestampPeriod));
    printf("%*s    %smaxClipDistances = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxClipDistances));
    printf("%*s    %smaxCullDistances = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxCullDistances));
    printf("%*s    %smaxCombinedClipAndCullDistances = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxCombinedClipAndCullDistances));
    printf("%*s    %sdiscreteQueuePriorities = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.discreteQueuePriorities));
    for (i = 0; i<2; i++) {
        printf("%*s    %spointSizeRange[%u] = %f\n", m_indent, "", &m_dummy_prefix, i, (m_struct.pointSizeRange)[i]);
    }
    for (i = 0; i<2; i++) {
        printf("%*s    %slineWidthRange[%u] = %f\n", m_indent, "", &m_dummy_prefix, i, (m_struct.lineWidthRange)[i]);
    }
    printf("%*s    %spointSizeGranularity = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.pointSizeGranularity));
    printf("%*s    %slineWidthGranularity = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.lineWidthGranularity));
    printf("%*s    %sstrictLines = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.strictLines) ? "TRUE" : "FALSE");
    printf("%*s    %sstandardSampleLocations = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.standardSampleLocations) ? "TRUE" : "FALSE");
    printf("%*s    %soptimalBufferCopyOffsetAlignment = " PRINTF_SIZE_T_SPECIFIER "\n", m_indent, "", &m_dummy_prefix, (m_struct.optimalBufferCopyOffsetAlignment));
    printf("%*s    %soptimalBufferCopyRowPitchAlignment = " PRINTF_SIZE_T_SPECIFIER "\n", m_indent, "", &m_dummy_prefix, (m_struct.optimalBufferCopyRowPitchAlignment));
    printf("%*s    %snonCoherentAtomSize = " PRINTF_SIZE_T_SPECIFIER "\n", m_indent, "", &m_dummy_prefix, (m_struct.nonCoherentAtomSize));
}

// Output all struct elements, each on their own line
void vkphysicaldevicelimits_struct_wrapper::display_txt()
{
    printf("%*sVkPhysicalDeviceLimits struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkphysicaldevicelimits_struct_wrapper::display_full_txt()
{
    printf("%*sVkPhysicalDeviceLimits struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// vkphysicaldevicememoryproperties_struct_wrapper class definition
vkphysicaldevicememoryproperties_struct_wrapper::vkphysicaldevicememoryproperties_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkphysicaldevicememoryproperties_struct_wrapper::vkphysicaldevicememoryproperties_struct_wrapper(VkPhysicalDeviceMemoryProperties* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkphysicaldevicememoryproperties_struct_wrapper::vkphysicaldevicememoryproperties_struct_wrapper(const VkPhysicalDeviceMemoryProperties* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkphysicaldevicememoryproperties_struct_wrapper::~vkphysicaldevicememoryproperties_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkphysicaldevicememoryproperties_struct_wrapper::display_single_txt()
{
    printf(" %*sVkPhysicalDeviceMemoryProperties = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkphysicaldevicememoryproperties_struct_wrapper::display_struct_members()
{
    printf("%*s    %smemoryTypeCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.memoryTypeCount));
    uint32_t i;
    for (i = 0; i<VK_MAX_MEMORY_TYPES; i++) {
        printf("%*s    %smemoryTypes[%u] = 0x%p\n", m_indent, "", &m_dummy_prefix, i, (void*)&(m_struct.memoryTypes)[i]);
    }
    printf("%*s    %smemoryHeapCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.memoryHeapCount));
    for (i = 0; i<VK_MAX_MEMORY_HEAPS; i++) {
        printf("%*s    %smemoryHeaps[%u] = 0x%p\n", m_indent, "", &m_dummy_prefix, i, (void*)&(m_struct.memoryHeaps)[i]);
    }
}

// Output all struct elements, each on their own line
void vkphysicaldevicememoryproperties_struct_wrapper::display_txt()
{
    printf("%*sVkPhysicalDeviceMemoryProperties struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkphysicaldevicememoryproperties_struct_wrapper::display_full_txt()
{
    printf("%*sVkPhysicalDeviceMemoryProperties struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    uint32_t i;
    for (i = 0; i<VK_MAX_MEMORY_HEAPS; i++) {
            vkmemoryheap_struct_wrapper class0(&(m_struct.memoryHeaps[i]));
            class0.set_indent(m_indent + 4);
            class0.display_full_txt();
    }
    for (i = 0; i<VK_MAX_MEMORY_TYPES; i++) {
            vkmemorytype_struct_wrapper class1(&(m_struct.memoryTypes[i]));
            class1.set_indent(m_indent + 4);
            class1.display_full_txt();
    }
}


// vkphysicaldeviceproperties_struct_wrapper class definition
vkphysicaldeviceproperties_struct_wrapper::vkphysicaldeviceproperties_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkphysicaldeviceproperties_struct_wrapper::vkphysicaldeviceproperties_struct_wrapper(VkPhysicalDeviceProperties* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkphysicaldeviceproperties_struct_wrapper::vkphysicaldeviceproperties_struct_wrapper(const VkPhysicalDeviceProperties* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkphysicaldeviceproperties_struct_wrapper::~vkphysicaldeviceproperties_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkphysicaldeviceproperties_struct_wrapper::display_single_txt()
{
    printf(" %*sVkPhysicalDeviceProperties = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkphysicaldeviceproperties_struct_wrapper::display_struct_members()
{
    printf("%*s    %sapiVersion = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.apiVersion));
    printf("%*s    %sdriverVersion = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.driverVersion));
    printf("%*s    %svendorID = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.vendorID));
    printf("%*s    %sdeviceID = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.deviceID));
    printf("%*s    %sdeviceType = %s\n", m_indent, "", &m_dummy_prefix, string_VkPhysicalDeviceType(m_struct.deviceType));
    uint32_t i;
    for (i = 0; i<VK_MAX_PHYSICAL_DEVICE_NAME_SIZE; i++) {
        printf("%*s    %sdeviceName = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.deviceName));
    }
    for (i = 0; i<VK_UUID_SIZE; i++) {
        printf("%*s    %spipelineCacheUUID[%u] = %hu\n", m_indent, "", &m_dummy_prefix, i, (m_struct.pipelineCacheUUID)[i]);
    }
    printf("%*s    %slimits = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.limits));
    printf("%*s    %ssparseProperties = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.sparseProperties));
}

// Output all struct elements, each on their own line
void vkphysicaldeviceproperties_struct_wrapper::display_txt()
{
    printf("%*sVkPhysicalDeviceProperties struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkphysicaldeviceproperties_struct_wrapper::display_full_txt()
{
    printf("%*sVkPhysicalDeviceProperties struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (&m_struct.sparseProperties) {
        vkphysicaldevicesparseproperties_struct_wrapper class0(&m_struct.sparseProperties);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
    if (&m_struct.limits) {
        vkphysicaldevicelimits_struct_wrapper class1(&m_struct.limits);
        class1.set_indent(m_indent + 4);
        class1.display_full_txt();
    }
}


// vkphysicaldevicesparseproperties_struct_wrapper class definition
vkphysicaldevicesparseproperties_struct_wrapper::vkphysicaldevicesparseproperties_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkphysicaldevicesparseproperties_struct_wrapper::vkphysicaldevicesparseproperties_struct_wrapper(VkPhysicalDeviceSparseProperties* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkphysicaldevicesparseproperties_struct_wrapper::vkphysicaldevicesparseproperties_struct_wrapper(const VkPhysicalDeviceSparseProperties* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkphysicaldevicesparseproperties_struct_wrapper::~vkphysicaldevicesparseproperties_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkphysicaldevicesparseproperties_struct_wrapper::display_single_txt()
{
    printf(" %*sVkPhysicalDeviceSparseProperties = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkphysicaldevicesparseproperties_struct_wrapper::display_struct_members()
{
    printf("%*s    %sresidencyStandard2DBlockShape = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.residencyStandard2DBlockShape) ? "TRUE" : "FALSE");
    printf("%*s    %sresidencyStandard2DMultisampleBlockShape = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.residencyStandard2DMultisampleBlockShape) ? "TRUE" : "FALSE");
    printf("%*s    %sresidencyStandard3DBlockShape = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.residencyStandard3DBlockShape) ? "TRUE" : "FALSE");
    printf("%*s    %sresidencyAlignedMipSize = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.residencyAlignedMipSize) ? "TRUE" : "FALSE");
    printf("%*s    %sresidencyNonResidentStrict = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.residencyNonResidentStrict) ? "TRUE" : "FALSE");
}

// Output all struct elements, each on their own line
void vkphysicaldevicesparseproperties_struct_wrapper::display_txt()
{
    printf("%*sVkPhysicalDeviceSparseProperties struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkphysicaldevicesparseproperties_struct_wrapper::display_full_txt()
{
    printf("%*sVkPhysicalDeviceSparseProperties struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// vkpipelinecachecreateinfo_struct_wrapper class definition
vkpipelinecachecreateinfo_struct_wrapper::vkpipelinecachecreateinfo_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkpipelinecachecreateinfo_struct_wrapper::vkpipelinecachecreateinfo_struct_wrapper(VkPipelineCacheCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkpipelinecachecreateinfo_struct_wrapper::vkpipelinecachecreateinfo_struct_wrapper(const VkPipelineCacheCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkpipelinecachecreateinfo_struct_wrapper::~vkpipelinecachecreateinfo_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkpipelinecachecreateinfo_struct_wrapper::display_single_txt()
{
    printf(" %*sVkPipelineCacheCreateInfo = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkpipelinecachecreateinfo_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
    printf("%*s    %sinitialDataSize = " PRINTF_SIZE_T_SPECIFIER "\n", m_indent, "", &m_dummy_prefix, (m_struct.initialDataSize));
    printf("%*s    %spInitialData = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pInitialData));
}

// Output all struct elements, each on their own line
void vkpipelinecachecreateinfo_struct_wrapper::display_txt()
{
    printf("%*sVkPipelineCacheCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkpipelinecachecreateinfo_struct_wrapper::display_full_txt()
{
    printf("%*sVkPipelineCacheCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkpipelinecolorblendattachmentstate_struct_wrapper class definition
vkpipelinecolorblendattachmentstate_struct_wrapper::vkpipelinecolorblendattachmentstate_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkpipelinecolorblendattachmentstate_struct_wrapper::vkpipelinecolorblendattachmentstate_struct_wrapper(VkPipelineColorBlendAttachmentState* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkpipelinecolorblendattachmentstate_struct_wrapper::vkpipelinecolorblendattachmentstate_struct_wrapper(const VkPipelineColorBlendAttachmentState* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkpipelinecolorblendattachmentstate_struct_wrapper::~vkpipelinecolorblendattachmentstate_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkpipelinecolorblendattachmentstate_struct_wrapper::display_single_txt()
{
    printf(" %*sVkPipelineColorBlendAttachmentState = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkpipelinecolorblendattachmentstate_struct_wrapper::display_struct_members()
{
    printf("%*s    %sblendEnable = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.blendEnable) ? "TRUE" : "FALSE");
    printf("%*s    %ssrcColorBlendFactor = %s\n", m_indent, "", &m_dummy_prefix, string_VkBlendFactor(m_struct.srcColorBlendFactor));
    printf("%*s    %sdstColorBlendFactor = %s\n", m_indent, "", &m_dummy_prefix, string_VkBlendFactor(m_struct.dstColorBlendFactor));
    printf("%*s    %scolorBlendOp = %s\n", m_indent, "", &m_dummy_prefix, string_VkBlendOp(m_struct.colorBlendOp));
    printf("%*s    %ssrcAlphaBlendFactor = %s\n", m_indent, "", &m_dummy_prefix, string_VkBlendFactor(m_struct.srcAlphaBlendFactor));
    printf("%*s    %sdstAlphaBlendFactor = %s\n", m_indent, "", &m_dummy_prefix, string_VkBlendFactor(m_struct.dstAlphaBlendFactor));
    printf("%*s    %salphaBlendOp = %s\n", m_indent, "", &m_dummy_prefix, string_VkBlendOp(m_struct.alphaBlendOp));
    printf("%*s    %scolorWriteMask = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.colorWriteMask));
}

// Output all struct elements, each on their own line
void vkpipelinecolorblendattachmentstate_struct_wrapper::display_txt()
{
    printf("%*sVkPipelineColorBlendAttachmentState struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkpipelinecolorblendattachmentstate_struct_wrapper::display_full_txt()
{
    printf("%*sVkPipelineColorBlendAttachmentState struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// vkpipelinecolorblendstatecreateinfo_struct_wrapper class definition
vkpipelinecolorblendstatecreateinfo_struct_wrapper::vkpipelinecolorblendstatecreateinfo_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkpipelinecolorblendstatecreateinfo_struct_wrapper::vkpipelinecolorblendstatecreateinfo_struct_wrapper(VkPipelineColorBlendStateCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkpipelinecolorblendstatecreateinfo_struct_wrapper::vkpipelinecolorblendstatecreateinfo_struct_wrapper(const VkPipelineColorBlendStateCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkpipelinecolorblendstatecreateinfo_struct_wrapper::~vkpipelinecolorblendstatecreateinfo_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkpipelinecolorblendstatecreateinfo_struct_wrapper::display_single_txt()
{
    printf(" %*sVkPipelineColorBlendStateCreateInfo = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkpipelinecolorblendstatecreateinfo_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
    printf("%*s    %slogicOpEnable = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.logicOpEnable) ? "TRUE" : "FALSE");
    printf("%*s    %slogicOp = %s\n", m_indent, "", &m_dummy_prefix, string_VkLogicOp(m_struct.logicOp));
    printf("%*s    %sattachmentCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.attachmentCount));
    uint32_t i;
    for (i = 0; i<attachmentCount; i++) {
        printf("%*s    %spAttachments[%u] = 0x%p\n", m_indent, "", &m_dummy_prefix, i, (void*)(m_struct.pAttachments)[i]);
    }
    for (i = 0; i<4; i++) {
        printf("%*s    %sblendConstants[%u] = %f\n", m_indent, "", &m_dummy_prefix, i, (m_struct.blendConstants)[i]);
    }
}

// Output all struct elements, each on their own line
void vkpipelinecolorblendstatecreateinfo_struct_wrapper::display_txt()
{
    printf("%*sVkPipelineColorBlendStateCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkpipelinecolorblendstatecreateinfo_struct_wrapper::display_full_txt()
{
    printf("%*sVkPipelineColorBlendStateCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    uint32_t i;
    for (i = 0; i<attachmentCount; i++) {
            vkpipelinecolorblendattachmentstate_struct_wrapper class0(&(m_struct.pAttachments[i]));
            class0.set_indent(m_indent + 4);
            class0.display_full_txt();
    }
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkpipelinedepthstencilstatecreateinfo_struct_wrapper class definition
vkpipelinedepthstencilstatecreateinfo_struct_wrapper::vkpipelinedepthstencilstatecreateinfo_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkpipelinedepthstencilstatecreateinfo_struct_wrapper::vkpipelinedepthstencilstatecreateinfo_struct_wrapper(VkPipelineDepthStencilStateCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkpipelinedepthstencilstatecreateinfo_struct_wrapper::vkpipelinedepthstencilstatecreateinfo_struct_wrapper(const VkPipelineDepthStencilStateCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkpipelinedepthstencilstatecreateinfo_struct_wrapper::~vkpipelinedepthstencilstatecreateinfo_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkpipelinedepthstencilstatecreateinfo_struct_wrapper::display_single_txt()
{
    printf(" %*sVkPipelineDepthStencilStateCreateInfo = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkpipelinedepthstencilstatecreateinfo_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
    printf("%*s    %sdepthTestEnable = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.depthTestEnable) ? "TRUE" : "FALSE");
    printf("%*s    %sdepthWriteEnable = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.depthWriteEnable) ? "TRUE" : "FALSE");
    printf("%*s    %sdepthCompareOp = %s\n", m_indent, "", &m_dummy_prefix, string_VkCompareOp(m_struct.depthCompareOp));
    printf("%*s    %sdepthBoundsTestEnable = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.depthBoundsTestEnable) ? "TRUE" : "FALSE");
    printf("%*s    %sstencilTestEnable = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.stencilTestEnable) ? "TRUE" : "FALSE");
    printf("%*s    %sfront = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.front));
    printf("%*s    %sback = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.back));
    printf("%*s    %sminDepthBounds = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.minDepthBounds));
    printf("%*s    %smaxDepthBounds = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.maxDepthBounds));
}

// Output all struct elements, each on their own line
void vkpipelinedepthstencilstatecreateinfo_struct_wrapper::display_txt()
{
    printf("%*sVkPipelineDepthStencilStateCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkpipelinedepthstencilstatecreateinfo_struct_wrapper::display_full_txt()
{
    printf("%*sVkPipelineDepthStencilStateCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (&m_struct.back) {
        vkstencilopstate_struct_wrapper class0(&m_struct.back);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
    if (&m_struct.front) {
        vkstencilopstate_struct_wrapper class1(&m_struct.front);
        class1.set_indent(m_indent + 4);
        class1.display_full_txt();
    }
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkpipelinedynamicstatecreateinfo_struct_wrapper class definition
vkpipelinedynamicstatecreateinfo_struct_wrapper::vkpipelinedynamicstatecreateinfo_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkpipelinedynamicstatecreateinfo_struct_wrapper::vkpipelinedynamicstatecreateinfo_struct_wrapper(VkPipelineDynamicStateCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkpipelinedynamicstatecreateinfo_struct_wrapper::vkpipelinedynamicstatecreateinfo_struct_wrapper(const VkPipelineDynamicStateCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkpipelinedynamicstatecreateinfo_struct_wrapper::~vkpipelinedynamicstatecreateinfo_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkpipelinedynamicstatecreateinfo_struct_wrapper::display_single_txt()
{
    printf(" %*sVkPipelineDynamicStateCreateInfo = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkpipelinedynamicstatecreateinfo_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
    printf("%*s    %sdynamicStateCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.dynamicStateCount));
    uint32_t i;
    for (i = 0; i<dynamicStateCount; i++) {
        printf("%*s    %spDynamicStates[%u] = 0x%s\n", m_indent, "", &m_dummy_prefix, i, string_VkDynamicState(*m_struct.pDynamicStates)[i]);
    }
}

// Output all struct elements, each on their own line
void vkpipelinedynamicstatecreateinfo_struct_wrapper::display_txt()
{
    printf("%*sVkPipelineDynamicStateCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkpipelinedynamicstatecreateinfo_struct_wrapper::display_full_txt()
{
    printf("%*sVkPipelineDynamicStateCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkpipelineinputassemblystatecreateinfo_struct_wrapper class definition
vkpipelineinputassemblystatecreateinfo_struct_wrapper::vkpipelineinputassemblystatecreateinfo_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkpipelineinputassemblystatecreateinfo_struct_wrapper::vkpipelineinputassemblystatecreateinfo_struct_wrapper(VkPipelineInputAssemblyStateCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkpipelineinputassemblystatecreateinfo_struct_wrapper::vkpipelineinputassemblystatecreateinfo_struct_wrapper(const VkPipelineInputAssemblyStateCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkpipelineinputassemblystatecreateinfo_struct_wrapper::~vkpipelineinputassemblystatecreateinfo_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkpipelineinputassemblystatecreateinfo_struct_wrapper::display_single_txt()
{
    printf(" %*sVkPipelineInputAssemblyStateCreateInfo = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkpipelineinputassemblystatecreateinfo_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
    printf("%*s    %stopology = %s\n", m_indent, "", &m_dummy_prefix, string_VkPrimitiveTopology(m_struct.topology));
    printf("%*s    %sprimitiveRestartEnable = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.primitiveRestartEnable) ? "TRUE" : "FALSE");
}

// Output all struct elements, each on their own line
void vkpipelineinputassemblystatecreateinfo_struct_wrapper::display_txt()
{
    printf("%*sVkPipelineInputAssemblyStateCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkpipelineinputassemblystatecreateinfo_struct_wrapper::display_full_txt()
{
    printf("%*sVkPipelineInputAssemblyStateCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkpipelinelayoutcreateinfo_struct_wrapper class definition
vkpipelinelayoutcreateinfo_struct_wrapper::vkpipelinelayoutcreateinfo_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkpipelinelayoutcreateinfo_struct_wrapper::vkpipelinelayoutcreateinfo_struct_wrapper(VkPipelineLayoutCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkpipelinelayoutcreateinfo_struct_wrapper::vkpipelinelayoutcreateinfo_struct_wrapper(const VkPipelineLayoutCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkpipelinelayoutcreateinfo_struct_wrapper::~vkpipelinelayoutcreateinfo_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkpipelinelayoutcreateinfo_struct_wrapper::display_single_txt()
{
    printf(" %*sVkPipelineLayoutCreateInfo = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkpipelinelayoutcreateinfo_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
    printf("%*s    %ssetLayoutCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.setLayoutCount));
    uint32_t i;
    for (i = 0; i<setLayoutCount; i++) {
        printf("%*s    %spSetLayouts[%u] = 0x%p\n", m_indent, "", &m_dummy_prefix, i, (m_struct.pSetLayouts)[i]);
    }
    printf("%*s    %spushConstantRangeCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.pushConstantRangeCount));
    for (i = 0; i<pushConstantRangeCount; i++) {
        printf("%*s    %spPushConstantRanges[%u] = 0x%p\n", m_indent, "", &m_dummy_prefix, i, (void*)(m_struct.pPushConstantRanges)[i]);
    }
}

// Output all struct elements, each on their own line
void vkpipelinelayoutcreateinfo_struct_wrapper::display_txt()
{
    printf("%*sVkPipelineLayoutCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkpipelinelayoutcreateinfo_struct_wrapper::display_full_txt()
{
    printf("%*sVkPipelineLayoutCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    uint32_t i;
    for (i = 0; i<pushConstantRangeCount; i++) {
            vkpushconstantrange_struct_wrapper class0(&(m_struct.pPushConstantRanges[i]));
            class0.set_indent(m_indent + 4);
            class0.display_full_txt();
    }
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkpipelinemultisamplestatecreateinfo_struct_wrapper class definition
vkpipelinemultisamplestatecreateinfo_struct_wrapper::vkpipelinemultisamplestatecreateinfo_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkpipelinemultisamplestatecreateinfo_struct_wrapper::vkpipelinemultisamplestatecreateinfo_struct_wrapper(VkPipelineMultisampleStateCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkpipelinemultisamplestatecreateinfo_struct_wrapper::vkpipelinemultisamplestatecreateinfo_struct_wrapper(const VkPipelineMultisampleStateCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkpipelinemultisamplestatecreateinfo_struct_wrapper::~vkpipelinemultisamplestatecreateinfo_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkpipelinemultisamplestatecreateinfo_struct_wrapper::display_single_txt()
{
    printf(" %*sVkPipelineMultisampleStateCreateInfo = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkpipelinemultisamplestatecreateinfo_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
    printf("%*s    %srasterizationSamples = %s\n", m_indent, "", &m_dummy_prefix, string_VkSampleCountFlagBits(m_struct.rasterizationSamples));
    printf("%*s    %ssampleShadingEnable = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.sampleShadingEnable) ? "TRUE" : "FALSE");
    printf("%*s    %sminSampleShading = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.minSampleShading));
    printf("%*s    %spSampleMask = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.pSampleMask));
    printf("%*s    %salphaToCoverageEnable = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.alphaToCoverageEnable) ? "TRUE" : "FALSE");
    printf("%*s    %salphaToOneEnable = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.alphaToOneEnable) ? "TRUE" : "FALSE");
}

// Output all struct elements, each on their own line
void vkpipelinemultisamplestatecreateinfo_struct_wrapper::display_txt()
{
    printf("%*sVkPipelineMultisampleStateCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkpipelinemultisamplestatecreateinfo_struct_wrapper::display_full_txt()
{
    printf("%*sVkPipelineMultisampleStateCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkpipelinerasterizationstatecreateinfo_struct_wrapper class definition
vkpipelinerasterizationstatecreateinfo_struct_wrapper::vkpipelinerasterizationstatecreateinfo_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkpipelinerasterizationstatecreateinfo_struct_wrapper::vkpipelinerasterizationstatecreateinfo_struct_wrapper(VkPipelineRasterizationStateCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkpipelinerasterizationstatecreateinfo_struct_wrapper::vkpipelinerasterizationstatecreateinfo_struct_wrapper(const VkPipelineRasterizationStateCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkpipelinerasterizationstatecreateinfo_struct_wrapper::~vkpipelinerasterizationstatecreateinfo_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkpipelinerasterizationstatecreateinfo_struct_wrapper::display_single_txt()
{
    printf(" %*sVkPipelineRasterizationStateCreateInfo = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkpipelinerasterizationstatecreateinfo_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
    printf("%*s    %sdepthClampEnable = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.depthClampEnable) ? "TRUE" : "FALSE");
    printf("%*s    %srasterizerDiscardEnable = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.rasterizerDiscardEnable) ? "TRUE" : "FALSE");
    printf("%*s    %spolygonMode = %s\n", m_indent, "", &m_dummy_prefix, string_VkPolygonMode(m_struct.polygonMode));
    printf("%*s    %scullMode = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.cullMode));
    printf("%*s    %sfrontFace = %s\n", m_indent, "", &m_dummy_prefix, string_VkFrontFace(m_struct.frontFace));
    printf("%*s    %sdepthBiasEnable = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.depthBiasEnable) ? "TRUE" : "FALSE");
    printf("%*s    %sdepthBiasConstantFactor = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.depthBiasConstantFactor));
    printf("%*s    %sdepthBiasClamp = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.depthBiasClamp));
    printf("%*s    %sdepthBiasSlopeFactor = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.depthBiasSlopeFactor));
    printf("%*s    %slineWidth = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.lineWidth));
}

// Output all struct elements, each on their own line
void vkpipelinerasterizationstatecreateinfo_struct_wrapper::display_txt()
{
    printf("%*sVkPipelineRasterizationStateCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkpipelinerasterizationstatecreateinfo_struct_wrapper::display_full_txt()
{
    printf("%*sVkPipelineRasterizationStateCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkpipelinerasterizationstaterasterizationorderamd_struct_wrapper class definition
vkpipelinerasterizationstaterasterizationorderamd_struct_wrapper::vkpipelinerasterizationstaterasterizationorderamd_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkpipelinerasterizationstaterasterizationorderamd_struct_wrapper::vkpipelinerasterizationstaterasterizationorderamd_struct_wrapper(VkPipelineRasterizationStateRasterizationOrderAMD* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkpipelinerasterizationstaterasterizationorderamd_struct_wrapper::vkpipelinerasterizationstaterasterizationorderamd_struct_wrapper(const VkPipelineRasterizationStateRasterizationOrderAMD* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkpipelinerasterizationstaterasterizationorderamd_struct_wrapper::~vkpipelinerasterizationstaterasterizationorderamd_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkpipelinerasterizationstaterasterizationorderamd_struct_wrapper::display_single_txt()
{
    printf(" %*sVkPipelineRasterizationStateRasterizationOrderAMD = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkpipelinerasterizationstaterasterizationorderamd_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %srasterizationOrder = %s\n", m_indent, "", &m_dummy_prefix, string_VkRasterizationOrderAMD(m_struct.rasterizationOrder));
}

// Output all struct elements, each on their own line
void vkpipelinerasterizationstaterasterizationorderamd_struct_wrapper::display_txt()
{
    printf("%*sVkPipelineRasterizationStateRasterizationOrderAMD struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkpipelinerasterizationstaterasterizationorderamd_struct_wrapper::display_full_txt()
{
    printf("%*sVkPipelineRasterizationStateRasterizationOrderAMD struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkpipelineshaderstagecreateinfo_struct_wrapper class definition
vkpipelineshaderstagecreateinfo_struct_wrapper::vkpipelineshaderstagecreateinfo_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkpipelineshaderstagecreateinfo_struct_wrapper::vkpipelineshaderstagecreateinfo_struct_wrapper(VkPipelineShaderStageCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkpipelineshaderstagecreateinfo_struct_wrapper::vkpipelineshaderstagecreateinfo_struct_wrapper(const VkPipelineShaderStageCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkpipelineshaderstagecreateinfo_struct_wrapper::~vkpipelineshaderstagecreateinfo_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkpipelineshaderstagecreateinfo_struct_wrapper::display_single_txt()
{
    printf(" %*sVkPipelineShaderStageCreateInfo = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkpipelineshaderstagecreateinfo_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
    printf("%*s    %sstage = %s\n", m_indent, "", &m_dummy_prefix, string_VkShaderStageFlagBits(m_struct.stage));
    printf("%*s    %smodule = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.module));
    printf("%*s    %spName = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pName));
    printf("%*s    %spSpecializationInfo = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.pSpecializationInfo));
}

// Output all struct elements, each on their own line
void vkpipelineshaderstagecreateinfo_struct_wrapper::display_txt()
{
    printf("%*sVkPipelineShaderStageCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkpipelineshaderstagecreateinfo_struct_wrapper::display_full_txt()
{
    printf("%*sVkPipelineShaderStageCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pSpecializationInfo) {
        vkspecializationinfo_struct_wrapper class0(m_struct.pSpecializationInfo);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkpipelinetessellationstatecreateinfo_struct_wrapper class definition
vkpipelinetessellationstatecreateinfo_struct_wrapper::vkpipelinetessellationstatecreateinfo_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkpipelinetessellationstatecreateinfo_struct_wrapper::vkpipelinetessellationstatecreateinfo_struct_wrapper(VkPipelineTessellationStateCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkpipelinetessellationstatecreateinfo_struct_wrapper::vkpipelinetessellationstatecreateinfo_struct_wrapper(const VkPipelineTessellationStateCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkpipelinetessellationstatecreateinfo_struct_wrapper::~vkpipelinetessellationstatecreateinfo_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkpipelinetessellationstatecreateinfo_struct_wrapper::display_single_txt()
{
    printf(" %*sVkPipelineTessellationStateCreateInfo = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkpipelinetessellationstatecreateinfo_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
    printf("%*s    %spatchControlPoints = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.patchControlPoints));
}

// Output all struct elements, each on their own line
void vkpipelinetessellationstatecreateinfo_struct_wrapper::display_txt()
{
    printf("%*sVkPipelineTessellationStateCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkpipelinetessellationstatecreateinfo_struct_wrapper::display_full_txt()
{
    printf("%*sVkPipelineTessellationStateCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkpipelinevertexinputstatecreateinfo_struct_wrapper class definition
vkpipelinevertexinputstatecreateinfo_struct_wrapper::vkpipelinevertexinputstatecreateinfo_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkpipelinevertexinputstatecreateinfo_struct_wrapper::vkpipelinevertexinputstatecreateinfo_struct_wrapper(VkPipelineVertexInputStateCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkpipelinevertexinputstatecreateinfo_struct_wrapper::vkpipelinevertexinputstatecreateinfo_struct_wrapper(const VkPipelineVertexInputStateCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkpipelinevertexinputstatecreateinfo_struct_wrapper::~vkpipelinevertexinputstatecreateinfo_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkpipelinevertexinputstatecreateinfo_struct_wrapper::display_single_txt()
{
    printf(" %*sVkPipelineVertexInputStateCreateInfo = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkpipelinevertexinputstatecreateinfo_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
    printf("%*s    %svertexBindingDescriptionCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.vertexBindingDescriptionCount));
    uint32_t i;
    for (i = 0; i<vertexBindingDescriptionCount; i++) {
        printf("%*s    %spVertexBindingDescriptions[%u] = 0x%p\n", m_indent, "", &m_dummy_prefix, i, (void*)(m_struct.pVertexBindingDescriptions)[i]);
    }
    printf("%*s    %svertexAttributeDescriptionCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.vertexAttributeDescriptionCount));
    for (i = 0; i<vertexAttributeDescriptionCount; i++) {
        printf("%*s    %spVertexAttributeDescriptions[%u] = 0x%p\n", m_indent, "", &m_dummy_prefix, i, (void*)(m_struct.pVertexAttributeDescriptions)[i]);
    }
}

// Output all struct elements, each on their own line
void vkpipelinevertexinputstatecreateinfo_struct_wrapper::display_txt()
{
    printf("%*sVkPipelineVertexInputStateCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkpipelinevertexinputstatecreateinfo_struct_wrapper::display_full_txt()
{
    printf("%*sVkPipelineVertexInputStateCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    uint32_t i;
    for (i = 0; i<vertexAttributeDescriptionCount; i++) {
            vkvertexinputattributedescription_struct_wrapper class0(&(m_struct.pVertexAttributeDescriptions[i]));
            class0.set_indent(m_indent + 4);
            class0.display_full_txt();
    }
    for (i = 0; i<vertexBindingDescriptionCount; i++) {
            vkvertexinputbindingdescription_struct_wrapper class1(&(m_struct.pVertexBindingDescriptions[i]));
            class1.set_indent(m_indent + 4);
            class1.display_full_txt();
    }
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkpipelineviewportstatecreateinfo_struct_wrapper class definition
vkpipelineviewportstatecreateinfo_struct_wrapper::vkpipelineviewportstatecreateinfo_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkpipelineviewportstatecreateinfo_struct_wrapper::vkpipelineviewportstatecreateinfo_struct_wrapper(VkPipelineViewportStateCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkpipelineviewportstatecreateinfo_struct_wrapper::vkpipelineviewportstatecreateinfo_struct_wrapper(const VkPipelineViewportStateCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkpipelineviewportstatecreateinfo_struct_wrapper::~vkpipelineviewportstatecreateinfo_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkpipelineviewportstatecreateinfo_struct_wrapper::display_single_txt()
{
    printf(" %*sVkPipelineViewportStateCreateInfo = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkpipelineviewportstatecreateinfo_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
    printf("%*s    %sviewportCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.viewportCount));
    uint32_t i;
    for (i = 0; i<viewportCount; i++) {
        printf("%*s    %spViewports[%u] = 0x%p\n", m_indent, "", &m_dummy_prefix, i, (void*)(m_struct.pViewports)[i]);
    }
    printf("%*s    %sscissorCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.scissorCount));
    for (i = 0; i<scissorCount; i++) {
        printf("%*s    %spScissors[%u] = 0x%p\n", m_indent, "", &m_dummy_prefix, i, (void*)(m_struct.pScissors)[i]);
    }
}

// Output all struct elements, each on their own line
void vkpipelineviewportstatecreateinfo_struct_wrapper::display_txt()
{
    printf("%*sVkPipelineViewportStateCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkpipelineviewportstatecreateinfo_struct_wrapper::display_full_txt()
{
    printf("%*sVkPipelineViewportStateCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    uint32_t i;
    for (i = 0; i<scissorCount; i++) {
            vkrect2d_struct_wrapper class0(&(m_struct.pScissors[i]));
            class0.set_indent(m_indent + 4);
            class0.display_full_txt();
    }
    for (i = 0; i<viewportCount; i++) {
            vkviewport_struct_wrapper class1(&(m_struct.pViewports[i]));
            class1.set_indent(m_indent + 4);
            class1.display_full_txt();
    }
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkpresentinfokhr_struct_wrapper class definition
vkpresentinfokhr_struct_wrapper::vkpresentinfokhr_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkpresentinfokhr_struct_wrapper::vkpresentinfokhr_struct_wrapper(VkPresentInfoKHR* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkpresentinfokhr_struct_wrapper::vkpresentinfokhr_struct_wrapper(const VkPresentInfoKHR* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkpresentinfokhr_struct_wrapper::~vkpresentinfokhr_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkpresentinfokhr_struct_wrapper::display_single_txt()
{
    printf(" %*sVkPresentInfoKHR = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkpresentinfokhr_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %swaitSemaphoreCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.waitSemaphoreCount));
    uint32_t i;
    for (i = 0; i<waitSemaphoreCount; i++) {
        printf("%*s    %spWaitSemaphores[%u] = 0x%p\n", m_indent, "", &m_dummy_prefix, i, (m_struct.pWaitSemaphores)[i]);
    }
    printf("%*s    %sswapchainCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.swapchainCount));
    for (i = 0; i<swapchainCount; i++) {
        printf("%*s    %spSwapchains[%u] = 0x%p\n", m_indent, "", &m_dummy_prefix, i, (m_struct.pSwapchains)[i]);
    }
    for (i = 0; i<swapchainCount; i++) {
        printf("%*s    %spImageIndices[%u] = %u\n", m_indent, "", &m_dummy_prefix, i, (m_struct.pImageIndices)[i]);
    }
    printf("%*s    %spResults = 0x%s\n", m_indent, "", &m_dummy_prefix, string_VkResult(*m_struct.pResults));
}

// Output all struct elements, each on their own line
void vkpresentinfokhr_struct_wrapper::display_txt()
{
    printf("%*sVkPresentInfoKHR struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkpresentinfokhr_struct_wrapper::display_full_txt()
{
    printf("%*sVkPresentInfoKHR struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkpushconstantrange_struct_wrapper class definition
vkpushconstantrange_struct_wrapper::vkpushconstantrange_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkpushconstantrange_struct_wrapper::vkpushconstantrange_struct_wrapper(VkPushConstantRange* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkpushconstantrange_struct_wrapper::vkpushconstantrange_struct_wrapper(const VkPushConstantRange* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkpushconstantrange_struct_wrapper::~vkpushconstantrange_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkpushconstantrange_struct_wrapper::display_single_txt()
{
    printf(" %*sVkPushConstantRange = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkpushconstantrange_struct_wrapper::display_struct_members()
{
    printf("%*s    %sstageFlags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.stageFlags));
    printf("%*s    %soffset = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.offset));
    printf("%*s    %ssize = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.size));
}

// Output all struct elements, each on their own line
void vkpushconstantrange_struct_wrapper::display_txt()
{
    printf("%*sVkPushConstantRange struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkpushconstantrange_struct_wrapper::display_full_txt()
{
    printf("%*sVkPushConstantRange struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// vkquerypoolcreateinfo_struct_wrapper class definition
vkquerypoolcreateinfo_struct_wrapper::vkquerypoolcreateinfo_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkquerypoolcreateinfo_struct_wrapper::vkquerypoolcreateinfo_struct_wrapper(VkQueryPoolCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkquerypoolcreateinfo_struct_wrapper::vkquerypoolcreateinfo_struct_wrapper(const VkQueryPoolCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkquerypoolcreateinfo_struct_wrapper::~vkquerypoolcreateinfo_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkquerypoolcreateinfo_struct_wrapper::display_single_txt()
{
    printf(" %*sVkQueryPoolCreateInfo = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkquerypoolcreateinfo_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
    printf("%*s    %squeryType = %s\n", m_indent, "", &m_dummy_prefix, string_VkQueryType(m_struct.queryType));
    printf("%*s    %squeryCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.queryCount));
    printf("%*s    %spipelineStatistics = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.pipelineStatistics));
}

// Output all struct elements, each on their own line
void vkquerypoolcreateinfo_struct_wrapper::display_txt()
{
    printf("%*sVkQueryPoolCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkquerypoolcreateinfo_struct_wrapper::display_full_txt()
{
    printf("%*sVkQueryPoolCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkqueuefamilyproperties_struct_wrapper class definition
vkqueuefamilyproperties_struct_wrapper::vkqueuefamilyproperties_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkqueuefamilyproperties_struct_wrapper::vkqueuefamilyproperties_struct_wrapper(VkQueueFamilyProperties* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkqueuefamilyproperties_struct_wrapper::vkqueuefamilyproperties_struct_wrapper(const VkQueueFamilyProperties* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkqueuefamilyproperties_struct_wrapper::~vkqueuefamilyproperties_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkqueuefamilyproperties_struct_wrapper::display_single_txt()
{
    printf(" %*sVkQueueFamilyProperties = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkqueuefamilyproperties_struct_wrapper::display_struct_members()
{
    printf("%*s    %squeueFlags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.queueFlags));
    printf("%*s    %squeueCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.queueCount));
    printf("%*s    %stimestampValidBits = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.timestampValidBits));
    printf("%*s    %sminImageTransferGranularity = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.minImageTransferGranularity));
}

// Output all struct elements, each on their own line
void vkqueuefamilyproperties_struct_wrapper::display_txt()
{
    printf("%*sVkQueueFamilyProperties struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkqueuefamilyproperties_struct_wrapper::display_full_txt()
{
    printf("%*sVkQueueFamilyProperties struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (&m_struct.minImageTransferGranularity) {
        vkextent3d_struct_wrapper class0(&m_struct.minImageTransferGranularity);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
}


// vkrect2d_struct_wrapper class definition
vkrect2d_struct_wrapper::vkrect2d_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkrect2d_struct_wrapper::vkrect2d_struct_wrapper(VkRect2D* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkrect2d_struct_wrapper::vkrect2d_struct_wrapper(const VkRect2D* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkrect2d_struct_wrapper::~vkrect2d_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkrect2d_struct_wrapper::display_single_txt()
{
    printf(" %*sVkRect2D = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkrect2d_struct_wrapper::display_struct_members()
{
    printf("%*s    %soffset = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.offset));
    printf("%*s    %sextent = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.extent));
}

// Output all struct elements, each on their own line
void vkrect2d_struct_wrapper::display_txt()
{
    printf("%*sVkRect2D struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkrect2d_struct_wrapper::display_full_txt()
{
    printf("%*sVkRect2D struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (&m_struct.extent) {
        vkextent2d_struct_wrapper class0(&m_struct.extent);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
    if (&m_struct.offset) {
        vkoffset2d_struct_wrapper class1(&m_struct.offset);
        class1.set_indent(m_indent + 4);
        class1.display_full_txt();
    }
}


// vkrenderpassbegininfo_struct_wrapper class definition
vkrenderpassbegininfo_struct_wrapper::vkrenderpassbegininfo_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkrenderpassbegininfo_struct_wrapper::vkrenderpassbegininfo_struct_wrapper(VkRenderPassBeginInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkrenderpassbegininfo_struct_wrapper::vkrenderpassbegininfo_struct_wrapper(const VkRenderPassBeginInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkrenderpassbegininfo_struct_wrapper::~vkrenderpassbegininfo_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkrenderpassbegininfo_struct_wrapper::display_single_txt()
{
    printf(" %*sVkRenderPassBeginInfo = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkrenderpassbegininfo_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %srenderPass = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.renderPass));
    printf("%*s    %sframebuffer = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.framebuffer));
    printf("%*s    %srenderArea = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.renderArea));
    printf("%*s    %sclearValueCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.clearValueCount));
    uint32_t i;
    for (i = 0; i<clearValueCount; i++) {
        printf("%*s    %spClearValues[%u] = 0x%p\n", m_indent, "", &m_dummy_prefix, i, (void*)(m_struct.pClearValues)[i]);
    }
}

// Output all struct elements, each on their own line
void vkrenderpassbegininfo_struct_wrapper::display_txt()
{
    printf("%*sVkRenderPassBeginInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkrenderpassbegininfo_struct_wrapper::display_full_txt()
{
    printf("%*sVkRenderPassBeginInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    uint32_t i;
    for (i = 0; i<clearValueCount; i++) {
            vkclearvalue_struct_wrapper class0(&(m_struct.pClearValues[i]));
            class0.set_indent(m_indent + 4);
            class0.display_full_txt();
    }
    if (&m_struct.renderArea) {
        vkrect2d_struct_wrapper class1(&m_struct.renderArea);
        class1.set_indent(m_indent + 4);
        class1.display_full_txt();
    }
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkrenderpasscreateinfo_struct_wrapper class definition
vkrenderpasscreateinfo_struct_wrapper::vkrenderpasscreateinfo_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkrenderpasscreateinfo_struct_wrapper::vkrenderpasscreateinfo_struct_wrapper(VkRenderPassCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkrenderpasscreateinfo_struct_wrapper::vkrenderpasscreateinfo_struct_wrapper(const VkRenderPassCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkrenderpasscreateinfo_struct_wrapper::~vkrenderpasscreateinfo_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkrenderpasscreateinfo_struct_wrapper::display_single_txt()
{
    printf(" %*sVkRenderPassCreateInfo = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkrenderpasscreateinfo_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
    printf("%*s    %sattachmentCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.attachmentCount));
    uint32_t i;
    for (i = 0; i<attachmentCount; i++) {
        printf("%*s    %spAttachments[%u] = 0x%p\n", m_indent, "", &m_dummy_prefix, i, (void*)(m_struct.pAttachments)[i]);
    }
    printf("%*s    %ssubpassCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.subpassCount));
    for (i = 0; i<subpassCount; i++) {
        printf("%*s    %spSubpasses[%u] = 0x%p\n", m_indent, "", &m_dummy_prefix, i, (void*)(m_struct.pSubpasses)[i]);
    }
    printf("%*s    %sdependencyCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.dependencyCount));
    for (i = 0; i<dependencyCount; i++) {
        printf("%*s    %spDependencies[%u] = 0x%p\n", m_indent, "", &m_dummy_prefix, i, (void*)(m_struct.pDependencies)[i]);
    }
}

// Output all struct elements, each on their own line
void vkrenderpasscreateinfo_struct_wrapper::display_txt()
{
    printf("%*sVkRenderPassCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkrenderpasscreateinfo_struct_wrapper::display_full_txt()
{
    printf("%*sVkRenderPassCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    uint32_t i;
    for (i = 0; i<dependencyCount; i++) {
            vksubpassdependency_struct_wrapper class0(&(m_struct.pDependencies[i]));
            class0.set_indent(m_indent + 4);
            class0.display_full_txt();
    }
    for (i = 0; i<subpassCount; i++) {
            vksubpassdescription_struct_wrapper class1(&(m_struct.pSubpasses[i]));
            class1.set_indent(m_indent + 4);
            class1.display_full_txt();
    }
    for (i = 0; i<attachmentCount; i++) {
            vkattachmentdescription_struct_wrapper class2(&(m_struct.pAttachments[i]));
            class2.set_indent(m_indent + 4);
            class2.display_full_txt();
    }
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vksamplercreateinfo_struct_wrapper class definition
vksamplercreateinfo_struct_wrapper::vksamplercreateinfo_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vksamplercreateinfo_struct_wrapper::vksamplercreateinfo_struct_wrapper(VkSamplerCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vksamplercreateinfo_struct_wrapper::vksamplercreateinfo_struct_wrapper(const VkSamplerCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vksamplercreateinfo_struct_wrapper::~vksamplercreateinfo_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vksamplercreateinfo_struct_wrapper::display_single_txt()
{
    printf(" %*sVkSamplerCreateInfo = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vksamplercreateinfo_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
    printf("%*s    %smagFilter = %s\n", m_indent, "", &m_dummy_prefix, string_VkFilter(m_struct.magFilter));
    printf("%*s    %sminFilter = %s\n", m_indent, "", &m_dummy_prefix, string_VkFilter(m_struct.minFilter));
    printf("%*s    %smipmapMode = %s\n", m_indent, "", &m_dummy_prefix, string_VkSamplerMipmapMode(m_struct.mipmapMode));
    printf("%*s    %saddressModeU = %s\n", m_indent, "", &m_dummy_prefix, string_VkSamplerAddressMode(m_struct.addressModeU));
    printf("%*s    %saddressModeV = %s\n", m_indent, "", &m_dummy_prefix, string_VkSamplerAddressMode(m_struct.addressModeV));
    printf("%*s    %saddressModeW = %s\n", m_indent, "", &m_dummy_prefix, string_VkSamplerAddressMode(m_struct.addressModeW));
    printf("%*s    %smipLodBias = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.mipLodBias));
    printf("%*s    %sanisotropyEnable = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.anisotropyEnable) ? "TRUE" : "FALSE");
    printf("%*s    %smaxAnisotropy = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.maxAnisotropy));
    printf("%*s    %scompareEnable = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.compareEnable) ? "TRUE" : "FALSE");
    printf("%*s    %scompareOp = %s\n", m_indent, "", &m_dummy_prefix, string_VkCompareOp(m_struct.compareOp));
    printf("%*s    %sminLod = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.minLod));
    printf("%*s    %smaxLod = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.maxLod));
    printf("%*s    %sborderColor = %s\n", m_indent, "", &m_dummy_prefix, string_VkBorderColor(m_struct.borderColor));
    printf("%*s    %sunnormalizedCoordinates = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.unnormalizedCoordinates) ? "TRUE" : "FALSE");
}

// Output all struct elements, each on their own line
void vksamplercreateinfo_struct_wrapper::display_txt()
{
    printf("%*sVkSamplerCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vksamplercreateinfo_struct_wrapper::display_full_txt()
{
    printf("%*sVkSamplerCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vksemaphorecreateinfo_struct_wrapper class definition
vksemaphorecreateinfo_struct_wrapper::vksemaphorecreateinfo_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vksemaphorecreateinfo_struct_wrapper::vksemaphorecreateinfo_struct_wrapper(VkSemaphoreCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vksemaphorecreateinfo_struct_wrapper::vksemaphorecreateinfo_struct_wrapper(const VkSemaphoreCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vksemaphorecreateinfo_struct_wrapper::~vksemaphorecreateinfo_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vksemaphorecreateinfo_struct_wrapper::display_single_txt()
{
    printf(" %*sVkSemaphoreCreateInfo = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vksemaphorecreateinfo_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
}

// Output all struct elements, each on their own line
void vksemaphorecreateinfo_struct_wrapper::display_txt()
{
    printf("%*sVkSemaphoreCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vksemaphorecreateinfo_struct_wrapper::display_full_txt()
{
    printf("%*sVkSemaphoreCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkshadermodulecreateinfo_struct_wrapper class definition
vkshadermodulecreateinfo_struct_wrapper::vkshadermodulecreateinfo_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkshadermodulecreateinfo_struct_wrapper::vkshadermodulecreateinfo_struct_wrapper(VkShaderModuleCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkshadermodulecreateinfo_struct_wrapper::vkshadermodulecreateinfo_struct_wrapper(const VkShaderModuleCreateInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkshadermodulecreateinfo_struct_wrapper::~vkshadermodulecreateinfo_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkshadermodulecreateinfo_struct_wrapper::display_single_txt()
{
    printf(" %*sVkShaderModuleCreateInfo = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkshadermodulecreateinfo_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
    printf("%*s    %scodeSize = " PRINTF_SIZE_T_SPECIFIER "\n", m_indent, "", &m_dummy_prefix, (m_struct.codeSize));
    printf("%*s    %spCode = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.pCode));
}

// Output all struct elements, each on their own line
void vkshadermodulecreateinfo_struct_wrapper::display_txt()
{
    printf("%*sVkShaderModuleCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkshadermodulecreateinfo_struct_wrapper::display_full_txt()
{
    printf("%*sVkShaderModuleCreateInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vksparsebuffermemorybindinfo_struct_wrapper class definition
vksparsebuffermemorybindinfo_struct_wrapper::vksparsebuffermemorybindinfo_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vksparsebuffermemorybindinfo_struct_wrapper::vksparsebuffermemorybindinfo_struct_wrapper(VkSparseBufferMemoryBindInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vksparsebuffermemorybindinfo_struct_wrapper::vksparsebuffermemorybindinfo_struct_wrapper(const VkSparseBufferMemoryBindInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vksparsebuffermemorybindinfo_struct_wrapper::~vksparsebuffermemorybindinfo_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vksparsebuffermemorybindinfo_struct_wrapper::display_single_txt()
{
    printf(" %*sVkSparseBufferMemoryBindInfo = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vksparsebuffermemorybindinfo_struct_wrapper::display_struct_members()
{
    printf("%*s    %sbuffer = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.buffer));
    printf("%*s    %sbindCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.bindCount));
    uint32_t i;
    for (i = 0; i<bindCount; i++) {
        printf("%*s    %spBinds[%u] = 0x%p\n", m_indent, "", &m_dummy_prefix, i, (void*)(m_struct.pBinds)[i]);
    }
}

// Output all struct elements, each on their own line
void vksparsebuffermemorybindinfo_struct_wrapper::display_txt()
{
    printf("%*sVkSparseBufferMemoryBindInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vksparsebuffermemorybindinfo_struct_wrapper::display_full_txt()
{
    printf("%*sVkSparseBufferMemoryBindInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    uint32_t i;
    for (i = 0; i<bindCount; i++) {
            vksparsememorybind_struct_wrapper class0(&(m_struct.pBinds[i]));
            class0.set_indent(m_indent + 4);
            class0.display_full_txt();
    }
}


// vksparseimageformatproperties_struct_wrapper class definition
vksparseimageformatproperties_struct_wrapper::vksparseimageformatproperties_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vksparseimageformatproperties_struct_wrapper::vksparseimageformatproperties_struct_wrapper(VkSparseImageFormatProperties* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vksparseimageformatproperties_struct_wrapper::vksparseimageformatproperties_struct_wrapper(const VkSparseImageFormatProperties* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vksparseimageformatproperties_struct_wrapper::~vksparseimageformatproperties_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vksparseimageformatproperties_struct_wrapper::display_single_txt()
{
    printf(" %*sVkSparseImageFormatProperties = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vksparseimageformatproperties_struct_wrapper::display_struct_members()
{
    printf("%*s    %saspectMask = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.aspectMask));
    printf("%*s    %simageGranularity = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.imageGranularity));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
}

// Output all struct elements, each on their own line
void vksparseimageformatproperties_struct_wrapper::display_txt()
{
    printf("%*sVkSparseImageFormatProperties struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vksparseimageformatproperties_struct_wrapper::display_full_txt()
{
    printf("%*sVkSparseImageFormatProperties struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (&m_struct.imageGranularity) {
        vkextent3d_struct_wrapper class0(&m_struct.imageGranularity);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
}


// vksparseimagememorybind_struct_wrapper class definition
vksparseimagememorybind_struct_wrapper::vksparseimagememorybind_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vksparseimagememorybind_struct_wrapper::vksparseimagememorybind_struct_wrapper(VkSparseImageMemoryBind* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vksparseimagememorybind_struct_wrapper::vksparseimagememorybind_struct_wrapper(const VkSparseImageMemoryBind* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vksparseimagememorybind_struct_wrapper::~vksparseimagememorybind_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vksparseimagememorybind_struct_wrapper::display_single_txt()
{
    printf(" %*sVkSparseImageMemoryBind = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vksparseimagememorybind_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssubresource = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.subresource));
    printf("%*s    %soffset = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.offset));
    printf("%*s    %sextent = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.extent));
    printf("%*s    %smemory = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.memory));
    printf("%*s    %smemoryOffset = " PRINTF_SIZE_T_SPECIFIER "\n", m_indent, "", &m_dummy_prefix, (m_struct.memoryOffset));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
}

// Output all struct elements, each on their own line
void vksparseimagememorybind_struct_wrapper::display_txt()
{
    printf("%*sVkSparseImageMemoryBind struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vksparseimagememorybind_struct_wrapper::display_full_txt()
{
    printf("%*sVkSparseImageMemoryBind struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (&m_struct.extent) {
        vkextent3d_struct_wrapper class0(&m_struct.extent);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
    if (&m_struct.offset) {
        vkoffset3d_struct_wrapper class1(&m_struct.offset);
        class1.set_indent(m_indent + 4);
        class1.display_full_txt();
    }
    if (&m_struct.subresource) {
        vkimagesubresource_struct_wrapper class2(&m_struct.subresource);
        class2.set_indent(m_indent + 4);
        class2.display_full_txt();
    }
}


// vksparseimagememorybindinfo_struct_wrapper class definition
vksparseimagememorybindinfo_struct_wrapper::vksparseimagememorybindinfo_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vksparseimagememorybindinfo_struct_wrapper::vksparseimagememorybindinfo_struct_wrapper(VkSparseImageMemoryBindInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vksparseimagememorybindinfo_struct_wrapper::vksparseimagememorybindinfo_struct_wrapper(const VkSparseImageMemoryBindInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vksparseimagememorybindinfo_struct_wrapper::~vksparseimagememorybindinfo_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vksparseimagememorybindinfo_struct_wrapper::display_single_txt()
{
    printf(" %*sVkSparseImageMemoryBindInfo = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vksparseimagememorybindinfo_struct_wrapper::display_struct_members()
{
    printf("%*s    %simage = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.image));
    printf("%*s    %sbindCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.bindCount));
    uint32_t i;
    for (i = 0; i<bindCount; i++) {
        printf("%*s    %spBinds[%u] = 0x%p\n", m_indent, "", &m_dummy_prefix, i, (void*)(m_struct.pBinds)[i]);
    }
}

// Output all struct elements, each on their own line
void vksparseimagememorybindinfo_struct_wrapper::display_txt()
{
    printf("%*sVkSparseImageMemoryBindInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vksparseimagememorybindinfo_struct_wrapper::display_full_txt()
{
    printf("%*sVkSparseImageMemoryBindInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    uint32_t i;
    for (i = 0; i<bindCount; i++) {
            vksparseimagememorybind_struct_wrapper class0(&(m_struct.pBinds[i]));
            class0.set_indent(m_indent + 4);
            class0.display_full_txt();
    }
}


// vksparseimagememoryrequirements_struct_wrapper class definition
vksparseimagememoryrequirements_struct_wrapper::vksparseimagememoryrequirements_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vksparseimagememoryrequirements_struct_wrapper::vksparseimagememoryrequirements_struct_wrapper(VkSparseImageMemoryRequirements* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vksparseimagememoryrequirements_struct_wrapper::vksparseimagememoryrequirements_struct_wrapper(const VkSparseImageMemoryRequirements* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vksparseimagememoryrequirements_struct_wrapper::~vksparseimagememoryrequirements_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vksparseimagememoryrequirements_struct_wrapper::display_single_txt()
{
    printf(" %*sVkSparseImageMemoryRequirements = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vksparseimagememoryrequirements_struct_wrapper::display_struct_members()
{
    printf("%*s    %sformatProperties = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.formatProperties));
    printf("%*s    %simageMipTailFirstLod = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.imageMipTailFirstLod));
    printf("%*s    %simageMipTailSize = " PRINTF_SIZE_T_SPECIFIER "\n", m_indent, "", &m_dummy_prefix, (m_struct.imageMipTailSize));
    printf("%*s    %simageMipTailOffset = " PRINTF_SIZE_T_SPECIFIER "\n", m_indent, "", &m_dummy_prefix, (m_struct.imageMipTailOffset));
    printf("%*s    %simageMipTailStride = " PRINTF_SIZE_T_SPECIFIER "\n", m_indent, "", &m_dummy_prefix, (m_struct.imageMipTailStride));
}

// Output all struct elements, each on their own line
void vksparseimagememoryrequirements_struct_wrapper::display_txt()
{
    printf("%*sVkSparseImageMemoryRequirements struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vksparseimagememoryrequirements_struct_wrapper::display_full_txt()
{
    printf("%*sVkSparseImageMemoryRequirements struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (&m_struct.formatProperties) {
        vksparseimageformatproperties_struct_wrapper class0(&m_struct.formatProperties);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
}


// vksparseimageopaquememorybindinfo_struct_wrapper class definition
vksparseimageopaquememorybindinfo_struct_wrapper::vksparseimageopaquememorybindinfo_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vksparseimageopaquememorybindinfo_struct_wrapper::vksparseimageopaquememorybindinfo_struct_wrapper(VkSparseImageOpaqueMemoryBindInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vksparseimageopaquememorybindinfo_struct_wrapper::vksparseimageopaquememorybindinfo_struct_wrapper(const VkSparseImageOpaqueMemoryBindInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vksparseimageopaquememorybindinfo_struct_wrapper::~vksparseimageopaquememorybindinfo_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vksparseimageopaquememorybindinfo_struct_wrapper::display_single_txt()
{
    printf(" %*sVkSparseImageOpaqueMemoryBindInfo = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vksparseimageopaquememorybindinfo_struct_wrapper::display_struct_members()
{
    printf("%*s    %simage = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.image));
    printf("%*s    %sbindCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.bindCount));
    uint32_t i;
    for (i = 0; i<bindCount; i++) {
        printf("%*s    %spBinds[%u] = 0x%p\n", m_indent, "", &m_dummy_prefix, i, (void*)(m_struct.pBinds)[i]);
    }
}

// Output all struct elements, each on their own line
void vksparseimageopaquememorybindinfo_struct_wrapper::display_txt()
{
    printf("%*sVkSparseImageOpaqueMemoryBindInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vksparseimageopaquememorybindinfo_struct_wrapper::display_full_txt()
{
    printf("%*sVkSparseImageOpaqueMemoryBindInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    uint32_t i;
    for (i = 0; i<bindCount; i++) {
            vksparsememorybind_struct_wrapper class0(&(m_struct.pBinds[i]));
            class0.set_indent(m_indent + 4);
            class0.display_full_txt();
    }
}


// vksparsememorybind_struct_wrapper class definition
vksparsememorybind_struct_wrapper::vksparsememorybind_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vksparsememorybind_struct_wrapper::vksparsememorybind_struct_wrapper(VkSparseMemoryBind* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vksparsememorybind_struct_wrapper::vksparsememorybind_struct_wrapper(const VkSparseMemoryBind* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vksparsememorybind_struct_wrapper::~vksparsememorybind_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vksparsememorybind_struct_wrapper::display_single_txt()
{
    printf(" %*sVkSparseMemoryBind = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vksparsememorybind_struct_wrapper::display_struct_members()
{
    printf("%*s    %sresourceOffset = " PRINTF_SIZE_T_SPECIFIER "\n", m_indent, "", &m_dummy_prefix, (m_struct.resourceOffset));
    printf("%*s    %ssize = " PRINTF_SIZE_T_SPECIFIER "\n", m_indent, "", &m_dummy_prefix, (m_struct.size));
    printf("%*s    %smemory = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.memory));
    printf("%*s    %smemoryOffset = " PRINTF_SIZE_T_SPECIFIER "\n", m_indent, "", &m_dummy_prefix, (m_struct.memoryOffset));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
}

// Output all struct elements, each on their own line
void vksparsememorybind_struct_wrapper::display_txt()
{
    printf("%*sVkSparseMemoryBind struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vksparsememorybind_struct_wrapper::display_full_txt()
{
    printf("%*sVkSparseMemoryBind struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// vkspecializationinfo_struct_wrapper class definition
vkspecializationinfo_struct_wrapper::vkspecializationinfo_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkspecializationinfo_struct_wrapper::vkspecializationinfo_struct_wrapper(VkSpecializationInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkspecializationinfo_struct_wrapper::vkspecializationinfo_struct_wrapper(const VkSpecializationInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkspecializationinfo_struct_wrapper::~vkspecializationinfo_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkspecializationinfo_struct_wrapper::display_single_txt()
{
    printf(" %*sVkSpecializationInfo = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkspecializationinfo_struct_wrapper::display_struct_members()
{
    printf("%*s    %smapEntryCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.mapEntryCount));
    uint32_t i;
    for (i = 0; i<mapEntryCount; i++) {
        printf("%*s    %spMapEntries[%u] = 0x%p\n", m_indent, "", &m_dummy_prefix, i, (void*)(m_struct.pMapEntries)[i]);
    }
    printf("%*s    %sdataSize = " PRINTF_SIZE_T_SPECIFIER "\n", m_indent, "", &m_dummy_prefix, (m_struct.dataSize));
    printf("%*s    %spData = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pData));
}

// Output all struct elements, each on their own line
void vkspecializationinfo_struct_wrapper::display_txt()
{
    printf("%*sVkSpecializationInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkspecializationinfo_struct_wrapper::display_full_txt()
{
    printf("%*sVkSpecializationInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    uint32_t i;
    for (i = 0; i<mapEntryCount; i++) {
            vkspecializationmapentry_struct_wrapper class0(&(m_struct.pMapEntries[i]));
            class0.set_indent(m_indent + 4);
            class0.display_full_txt();
    }
}


// vkspecializationmapentry_struct_wrapper class definition
vkspecializationmapentry_struct_wrapper::vkspecializationmapentry_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkspecializationmapentry_struct_wrapper::vkspecializationmapentry_struct_wrapper(VkSpecializationMapEntry* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkspecializationmapentry_struct_wrapper::vkspecializationmapentry_struct_wrapper(const VkSpecializationMapEntry* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkspecializationmapentry_struct_wrapper::~vkspecializationmapentry_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkspecializationmapentry_struct_wrapper::display_single_txt()
{
    printf(" %*sVkSpecializationMapEntry = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkspecializationmapentry_struct_wrapper::display_struct_members()
{
    printf("%*s    %sconstantID = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.constantID));
    printf("%*s    %soffset = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.offset));
    printf("%*s    %ssize = " PRINTF_SIZE_T_SPECIFIER "\n", m_indent, "", &m_dummy_prefix, (m_struct.size));
}

// Output all struct elements, each on their own line
void vkspecializationmapentry_struct_wrapper::display_txt()
{
    printf("%*sVkSpecializationMapEntry struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkspecializationmapentry_struct_wrapper::display_full_txt()
{
    printf("%*sVkSpecializationMapEntry struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// vkstencilopstate_struct_wrapper class definition
vkstencilopstate_struct_wrapper::vkstencilopstate_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkstencilopstate_struct_wrapper::vkstencilopstate_struct_wrapper(VkStencilOpState* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkstencilopstate_struct_wrapper::vkstencilopstate_struct_wrapper(const VkStencilOpState* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkstencilopstate_struct_wrapper::~vkstencilopstate_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkstencilopstate_struct_wrapper::display_single_txt()
{
    printf(" %*sVkStencilOpState = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkstencilopstate_struct_wrapper::display_struct_members()
{
    printf("%*s    %sfailOp = %s\n", m_indent, "", &m_dummy_prefix, string_VkStencilOp(m_struct.failOp));
    printf("%*s    %spassOp = %s\n", m_indent, "", &m_dummy_prefix, string_VkStencilOp(m_struct.passOp));
    printf("%*s    %sdepthFailOp = %s\n", m_indent, "", &m_dummy_prefix, string_VkStencilOp(m_struct.depthFailOp));
    printf("%*s    %scompareOp = %s\n", m_indent, "", &m_dummy_prefix, string_VkCompareOp(m_struct.compareOp));
    printf("%*s    %scompareMask = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.compareMask));
    printf("%*s    %swriteMask = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.writeMask));
    printf("%*s    %sreference = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.reference));
}

// Output all struct elements, each on their own line
void vkstencilopstate_struct_wrapper::display_txt()
{
    printf("%*sVkStencilOpState struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkstencilopstate_struct_wrapper::display_full_txt()
{
    printf("%*sVkStencilOpState struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// vksubmitinfo_struct_wrapper class definition
vksubmitinfo_struct_wrapper::vksubmitinfo_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vksubmitinfo_struct_wrapper::vksubmitinfo_struct_wrapper(VkSubmitInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vksubmitinfo_struct_wrapper::vksubmitinfo_struct_wrapper(const VkSubmitInfo* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vksubmitinfo_struct_wrapper::~vksubmitinfo_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vksubmitinfo_struct_wrapper::display_single_txt()
{
    printf(" %*sVkSubmitInfo = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vksubmitinfo_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %swaitSemaphoreCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.waitSemaphoreCount));
    uint32_t i;
    for (i = 0; i<waitSemaphoreCount; i++) {
        printf("%*s    %spWaitSemaphores[%u] = 0x%p\n", m_indent, "", &m_dummy_prefix, i, (m_struct.pWaitSemaphores)[i]);
    }
    printf("%*s    %spWaitDstStageMask = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.pWaitDstStageMask));
    printf("%*s    %scommandBufferCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.commandBufferCount));
    for (i = 0; i<commandBufferCount; i++) {
        printf("%*s    %spCommandBuffers[%u] = 0x%p\n", m_indent, "", &m_dummy_prefix, i, (m_struct.pCommandBuffers)[i]);
    }
    printf("%*s    %ssignalSemaphoreCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.signalSemaphoreCount));
    for (i = 0; i<signalSemaphoreCount; i++) {
        printf("%*s    %spSignalSemaphores[%u] = 0x%p\n", m_indent, "", &m_dummy_prefix, i, (m_struct.pSignalSemaphores)[i]);
    }
}

// Output all struct elements, each on their own line
void vksubmitinfo_struct_wrapper::display_txt()
{
    printf("%*sVkSubmitInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vksubmitinfo_struct_wrapper::display_full_txt()
{
    printf("%*sVkSubmitInfo struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vksubpassdependency_struct_wrapper class definition
vksubpassdependency_struct_wrapper::vksubpassdependency_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vksubpassdependency_struct_wrapper::vksubpassdependency_struct_wrapper(VkSubpassDependency* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vksubpassdependency_struct_wrapper::vksubpassdependency_struct_wrapper(const VkSubpassDependency* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vksubpassdependency_struct_wrapper::~vksubpassdependency_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vksubpassdependency_struct_wrapper::display_single_txt()
{
    printf(" %*sVkSubpassDependency = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vksubpassdependency_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssrcSubpass = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.srcSubpass));
    printf("%*s    %sdstSubpass = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.dstSubpass));
    printf("%*s    %ssrcStageMask = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.srcStageMask));
    printf("%*s    %sdstStageMask = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.dstStageMask));
    printf("%*s    %ssrcAccessMask = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.srcAccessMask));
    printf("%*s    %sdstAccessMask = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.dstAccessMask));
    printf("%*s    %sdependencyFlags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.dependencyFlags));
}

// Output all struct elements, each on their own line
void vksubpassdependency_struct_wrapper::display_txt()
{
    printf("%*sVkSubpassDependency struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vksubpassdependency_struct_wrapper::display_full_txt()
{
    printf("%*sVkSubpassDependency struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// vksubpassdescription_struct_wrapper class definition
vksubpassdescription_struct_wrapper::vksubpassdescription_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vksubpassdescription_struct_wrapper::vksubpassdescription_struct_wrapper(VkSubpassDescription* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vksubpassdescription_struct_wrapper::vksubpassdescription_struct_wrapper(const VkSubpassDescription* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vksubpassdescription_struct_wrapper::~vksubpassdescription_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vksubpassdescription_struct_wrapper::display_single_txt()
{
    printf(" %*sVkSubpassDescription = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vksubpassdescription_struct_wrapper::display_struct_members()
{
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
    printf("%*s    %spipelineBindPoint = %s\n", m_indent, "", &m_dummy_prefix, string_VkPipelineBindPoint(m_struct.pipelineBindPoint));
    printf("%*s    %sinputAttachmentCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.inputAttachmentCount));
    uint32_t i;
    for (i = 0; i<inputAttachmentCount; i++) {
        printf("%*s    %spInputAttachments[%u] = 0x%p\n", m_indent, "", &m_dummy_prefix, i, (void*)(m_struct.pInputAttachments)[i]);
    }
    printf("%*s    %scolorAttachmentCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.colorAttachmentCount));
    for (i = 0; i<colorAttachmentCount; i++) {
        printf("%*s    %spColorAttachments[%u] = 0x%p\n", m_indent, "", &m_dummy_prefix, i, (void*)(m_struct.pColorAttachments)[i]);
    }
    for (i = 0; i<colorAttachmentCount; i++) {
        printf("%*s    %spResolveAttachments[%u] = 0x%p\n", m_indent, "", &m_dummy_prefix, i, (void*)(m_struct.pResolveAttachments)[i]);
    }
    printf("%*s    %spDepthStencilAttachment = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.pDepthStencilAttachment));
    printf("%*s    %spreserveAttachmentCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.preserveAttachmentCount));
    for (i = 0; i<preserveAttachmentCount; i++) {
        printf("%*s    %spPreserveAttachments[%u] = %u\n", m_indent, "", &m_dummy_prefix, i, (m_struct.pPreserveAttachments)[i]);
    }
}

// Output all struct elements, each on their own line
void vksubpassdescription_struct_wrapper::display_txt()
{
    printf("%*sVkSubpassDescription struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vksubpassdescription_struct_wrapper::display_full_txt()
{
    printf("%*sVkSubpassDescription struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pDepthStencilAttachment) {
        vkattachmentreference_struct_wrapper class0(m_struct.pDepthStencilAttachment);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
    uint32_t i;
    for (i = 0; i<colorAttachmentCount; i++) {
            vkattachmentreference_struct_wrapper class1(&(m_struct.pResolveAttachments[i]));
            class1.set_indent(m_indent + 4);
            class1.display_full_txt();
    }
    for (i = 0; i<colorAttachmentCount; i++) {
            vkattachmentreference_struct_wrapper class2(&(m_struct.pColorAttachments[i]));
            class2.set_indent(m_indent + 4);
            class2.display_full_txt();
    }
    for (i = 0; i<inputAttachmentCount; i++) {
            vkattachmentreference_struct_wrapper class3(&(m_struct.pInputAttachments[i]));
            class3.set_indent(m_indent + 4);
            class3.display_full_txt();
    }
}


// vksubresourcelayout_struct_wrapper class definition
vksubresourcelayout_struct_wrapper::vksubresourcelayout_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vksubresourcelayout_struct_wrapper::vksubresourcelayout_struct_wrapper(VkSubresourceLayout* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vksubresourcelayout_struct_wrapper::vksubresourcelayout_struct_wrapper(const VkSubresourceLayout* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vksubresourcelayout_struct_wrapper::~vksubresourcelayout_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vksubresourcelayout_struct_wrapper::display_single_txt()
{
    printf(" %*sVkSubresourceLayout = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vksubresourcelayout_struct_wrapper::display_struct_members()
{
    printf("%*s    %soffset = " PRINTF_SIZE_T_SPECIFIER "\n", m_indent, "", &m_dummy_prefix, (m_struct.offset));
    printf("%*s    %ssize = " PRINTF_SIZE_T_SPECIFIER "\n", m_indent, "", &m_dummy_prefix, (m_struct.size));
    printf("%*s    %srowPitch = " PRINTF_SIZE_T_SPECIFIER "\n", m_indent, "", &m_dummy_prefix, (m_struct.rowPitch));
    printf("%*s    %sarrayPitch = " PRINTF_SIZE_T_SPECIFIER "\n", m_indent, "", &m_dummy_prefix, (m_struct.arrayPitch));
    printf("%*s    %sdepthPitch = " PRINTF_SIZE_T_SPECIFIER "\n", m_indent, "", &m_dummy_prefix, (m_struct.depthPitch));
}

// Output all struct elements, each on their own line
void vksubresourcelayout_struct_wrapper::display_txt()
{
    printf("%*sVkSubresourceLayout struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vksubresourcelayout_struct_wrapper::display_full_txt()
{
    printf("%*sVkSubresourceLayout struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// vksurfacecapabilitieskhr_struct_wrapper class definition
vksurfacecapabilitieskhr_struct_wrapper::vksurfacecapabilitieskhr_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vksurfacecapabilitieskhr_struct_wrapper::vksurfacecapabilitieskhr_struct_wrapper(VkSurfaceCapabilitiesKHR* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vksurfacecapabilitieskhr_struct_wrapper::vksurfacecapabilitieskhr_struct_wrapper(const VkSurfaceCapabilitiesKHR* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vksurfacecapabilitieskhr_struct_wrapper::~vksurfacecapabilitieskhr_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vksurfacecapabilitieskhr_struct_wrapper::display_single_txt()
{
    printf(" %*sVkSurfaceCapabilitiesKHR = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vksurfacecapabilitieskhr_struct_wrapper::display_struct_members()
{
    printf("%*s    %sminImageCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.minImageCount));
    printf("%*s    %smaxImageCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxImageCount));
    printf("%*s    %scurrentExtent = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.currentExtent));
    printf("%*s    %sminImageExtent = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.minImageExtent));
    printf("%*s    %smaxImageExtent = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.maxImageExtent));
    printf("%*s    %smaxImageArrayLayers = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxImageArrayLayers));
    printf("%*s    %ssupportedTransforms = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.supportedTransforms));
    printf("%*s    %scurrentTransform = %s\n", m_indent, "", &m_dummy_prefix, string_VkSurfaceTransformFlagBitsKHR(m_struct.currentTransform));
    printf("%*s    %ssupportedCompositeAlpha = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.supportedCompositeAlpha));
    printf("%*s    %ssupportedUsageFlags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.supportedUsageFlags));
}

// Output all struct elements, each on their own line
void vksurfacecapabilitieskhr_struct_wrapper::display_txt()
{
    printf("%*sVkSurfaceCapabilitiesKHR struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vksurfacecapabilitieskhr_struct_wrapper::display_full_txt()
{
    printf("%*sVkSurfaceCapabilitiesKHR struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (&m_struct.maxImageExtent) {
        vkextent2d_struct_wrapper class0(&m_struct.maxImageExtent);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
    if (&m_struct.minImageExtent) {
        vkextent2d_struct_wrapper class1(&m_struct.minImageExtent);
        class1.set_indent(m_indent + 4);
        class1.display_full_txt();
    }
    if (&m_struct.currentExtent) {
        vkextent2d_struct_wrapper class2(&m_struct.currentExtent);
        class2.set_indent(m_indent + 4);
        class2.display_full_txt();
    }
}


// vksurfaceformatkhr_struct_wrapper class definition
vksurfaceformatkhr_struct_wrapper::vksurfaceformatkhr_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vksurfaceformatkhr_struct_wrapper::vksurfaceformatkhr_struct_wrapper(VkSurfaceFormatKHR* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vksurfaceformatkhr_struct_wrapper::vksurfaceformatkhr_struct_wrapper(const VkSurfaceFormatKHR* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vksurfaceformatkhr_struct_wrapper::~vksurfaceformatkhr_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vksurfaceformatkhr_struct_wrapper::display_single_txt()
{
    printf(" %*sVkSurfaceFormatKHR = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vksurfaceformatkhr_struct_wrapper::display_struct_members()
{
    printf("%*s    %sformat = %s\n", m_indent, "", &m_dummy_prefix, string_VkFormat(m_struct.format));
    printf("%*s    %scolorSpace = %s\n", m_indent, "", &m_dummy_prefix, string_VkColorSpaceKHR(m_struct.colorSpace));
}

// Output all struct elements, each on their own line
void vksurfaceformatkhr_struct_wrapper::display_txt()
{
    printf("%*sVkSurfaceFormatKHR struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vksurfaceformatkhr_struct_wrapper::display_full_txt()
{
    printf("%*sVkSurfaceFormatKHR struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// vkswapchaincreateinfokhr_struct_wrapper class definition
vkswapchaincreateinfokhr_struct_wrapper::vkswapchaincreateinfokhr_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkswapchaincreateinfokhr_struct_wrapper::vkswapchaincreateinfokhr_struct_wrapper(VkSwapchainCreateInfoKHR* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkswapchaincreateinfokhr_struct_wrapper::vkswapchaincreateinfokhr_struct_wrapper(const VkSwapchainCreateInfoKHR* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkswapchaincreateinfokhr_struct_wrapper::~vkswapchaincreateinfokhr_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkswapchaincreateinfokhr_struct_wrapper::display_single_txt()
{
    printf(" %*sVkSwapchainCreateInfoKHR = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkswapchaincreateinfokhr_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
    printf("%*s    %ssurface = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.surface));
    printf("%*s    %sminImageCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.minImageCount));
    printf("%*s    %simageFormat = %s\n", m_indent, "", &m_dummy_prefix, string_VkFormat(m_struct.imageFormat));
    printf("%*s    %simageColorSpace = %s\n", m_indent, "", &m_dummy_prefix, string_VkColorSpaceKHR(m_struct.imageColorSpace));
    printf("%*s    %simageExtent = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.imageExtent));
    printf("%*s    %simageArrayLayers = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.imageArrayLayers));
    printf("%*s    %simageUsage = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.imageUsage));
    printf("%*s    %simageSharingMode = %s\n", m_indent, "", &m_dummy_prefix, string_VkSharingMode(m_struct.imageSharingMode));
    printf("%*s    %squeueFamilyIndexCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.queueFamilyIndexCount));
    uint32_t i;
    for (i = 0; i<queueFamilyIndexCount; i++) {
        printf("%*s    %spQueueFamilyIndices[%u] = %u\n", m_indent, "", &m_dummy_prefix, i, (m_struct.pQueueFamilyIndices)[i]);
    }
    printf("%*s    %spreTransform = %s\n", m_indent, "", &m_dummy_prefix, string_VkSurfaceTransformFlagBitsKHR(m_struct.preTransform));
    printf("%*s    %scompositeAlpha = %s\n", m_indent, "", &m_dummy_prefix, string_VkCompositeAlphaFlagBitsKHR(m_struct.compositeAlpha));
    printf("%*s    %spresentMode = %s\n", m_indent, "", &m_dummy_prefix, string_VkPresentModeKHR(m_struct.presentMode));
    printf("%*s    %sclipped = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.clipped) ? "TRUE" : "FALSE");
    printf("%*s    %soldSwapchain = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.oldSwapchain));
}

// Output all struct elements, each on their own line
void vkswapchaincreateinfokhr_struct_wrapper::display_txt()
{
    printf("%*sVkSwapchainCreateInfoKHR struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkswapchaincreateinfokhr_struct_wrapper::display_full_txt()
{
    printf("%*sVkSwapchainCreateInfoKHR struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (&m_struct.imageExtent) {
        vkextent2d_struct_wrapper class0(&m_struct.imageExtent);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkvalidationflagsext_struct_wrapper class definition
vkvalidationflagsext_struct_wrapper::vkvalidationflagsext_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkvalidationflagsext_struct_wrapper::vkvalidationflagsext_struct_wrapper(VkValidationFlagsEXT* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkvalidationflagsext_struct_wrapper::vkvalidationflagsext_struct_wrapper(const VkValidationFlagsEXT* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkvalidationflagsext_struct_wrapper::~vkvalidationflagsext_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkvalidationflagsext_struct_wrapper::display_single_txt()
{
    printf(" %*sVkValidationFlagsEXT = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkvalidationflagsext_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sdisabledValidationCheckCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.disabledValidationCheckCount));
    printf("%*s    %spDisabledValidationChecks = 0x%s\n", m_indent, "", &m_dummy_prefix, string_VkValidationCheckEXT(*m_struct.pDisabledValidationChecks));
}

// Output all struct elements, each on their own line
void vkvalidationflagsext_struct_wrapper::display_txt()
{
    printf("%*sVkValidationFlagsEXT struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkvalidationflagsext_struct_wrapper::display_full_txt()
{
    printf("%*sVkValidationFlagsEXT struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkvertexinputattributedescription_struct_wrapper class definition
vkvertexinputattributedescription_struct_wrapper::vkvertexinputattributedescription_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkvertexinputattributedescription_struct_wrapper::vkvertexinputattributedescription_struct_wrapper(VkVertexInputAttributeDescription* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkvertexinputattributedescription_struct_wrapper::vkvertexinputattributedescription_struct_wrapper(const VkVertexInputAttributeDescription* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkvertexinputattributedescription_struct_wrapper::~vkvertexinputattributedescription_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkvertexinputattributedescription_struct_wrapper::display_single_txt()
{
    printf(" %*sVkVertexInputAttributeDescription = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkvertexinputattributedescription_struct_wrapper::display_struct_members()
{
    printf("%*s    %slocation = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.location));
    printf("%*s    %sbinding = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.binding));
    printf("%*s    %sformat = %s\n", m_indent, "", &m_dummy_prefix, string_VkFormat(m_struct.format));
    printf("%*s    %soffset = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.offset));
}

// Output all struct elements, each on their own line
void vkvertexinputattributedescription_struct_wrapper::display_txt()
{
    printf("%*sVkVertexInputAttributeDescription struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkvertexinputattributedescription_struct_wrapper::display_full_txt()
{
    printf("%*sVkVertexInputAttributeDescription struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// vkvertexinputbindingdescription_struct_wrapper class definition
vkvertexinputbindingdescription_struct_wrapper::vkvertexinputbindingdescription_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkvertexinputbindingdescription_struct_wrapper::vkvertexinputbindingdescription_struct_wrapper(VkVertexInputBindingDescription* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkvertexinputbindingdescription_struct_wrapper::vkvertexinputbindingdescription_struct_wrapper(const VkVertexInputBindingDescription* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkvertexinputbindingdescription_struct_wrapper::~vkvertexinputbindingdescription_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkvertexinputbindingdescription_struct_wrapper::display_single_txt()
{
    printf(" %*sVkVertexInputBindingDescription = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkvertexinputbindingdescription_struct_wrapper::display_struct_members()
{
    printf("%*s    %sbinding = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.binding));
    printf("%*s    %sstride = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.stride));
    printf("%*s    %sinputRate = %s\n", m_indent, "", &m_dummy_prefix, string_VkVertexInputRate(m_struct.inputRate));
}

// Output all struct elements, each on their own line
void vkvertexinputbindingdescription_struct_wrapper::display_txt()
{
    printf("%*sVkVertexInputBindingDescription struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkvertexinputbindingdescription_struct_wrapper::display_full_txt()
{
    printf("%*sVkVertexInputBindingDescription struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// vkviewport_struct_wrapper class definition
vkviewport_struct_wrapper::vkviewport_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkviewport_struct_wrapper::vkviewport_struct_wrapper(VkViewport* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkviewport_struct_wrapper::vkviewport_struct_wrapper(const VkViewport* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkviewport_struct_wrapper::~vkviewport_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkviewport_struct_wrapper::display_single_txt()
{
    printf(" %*sVkViewport = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkviewport_struct_wrapper::display_struct_members()
{
    printf("%*s    %sx = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.x));
    printf("%*s    %sy = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.y));
    printf("%*s    %swidth = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.width));
    printf("%*s    %sheight = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.height));
    printf("%*s    %sminDepth = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.minDepth));
    printf("%*s    %smaxDepth = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.maxDepth));
}

// Output all struct elements, each on their own line
void vkviewport_struct_wrapper::display_txt()
{
    printf("%*sVkViewport struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkviewport_struct_wrapper::display_full_txt()
{
    printf("%*sVkViewport struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// vkwaylandsurfacecreateinfokhr_struct_wrapper class definition
vkwaylandsurfacecreateinfokhr_struct_wrapper::vkwaylandsurfacecreateinfokhr_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkwaylandsurfacecreateinfokhr_struct_wrapper::vkwaylandsurfacecreateinfokhr_struct_wrapper(VkWaylandSurfaceCreateInfoKHR* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkwaylandsurfacecreateinfokhr_struct_wrapper::vkwaylandsurfacecreateinfokhr_struct_wrapper(const VkWaylandSurfaceCreateInfoKHR* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkwaylandsurfacecreateinfokhr_struct_wrapper::~vkwaylandsurfacecreateinfokhr_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkwaylandsurfacecreateinfokhr_struct_wrapper::display_single_txt()
{
    printf(" %*sVkWaylandSurfaceCreateInfoKHR = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkwaylandsurfacecreateinfokhr_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
    printf("%*s    %sdisplay = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.display));
    printf("%*s    %ssurface = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.surface));
}

// Output all struct elements, each on their own line
void vkwaylandsurfacecreateinfokhr_struct_wrapper::display_txt()
{
    printf("%*sVkWaylandSurfaceCreateInfoKHR struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkwaylandsurfacecreateinfokhr_struct_wrapper::display_full_txt()
{
    printf("%*sVkWaylandSurfaceCreateInfoKHR struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkwin32keyedmutexacquirereleaseinfonv_struct_wrapper class definition
vkwin32keyedmutexacquirereleaseinfonv_struct_wrapper::vkwin32keyedmutexacquirereleaseinfonv_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkwin32keyedmutexacquirereleaseinfonv_struct_wrapper::vkwin32keyedmutexacquirereleaseinfonv_struct_wrapper(VkWin32KeyedMutexAcquireReleaseInfoNV* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkwin32keyedmutexacquirereleaseinfonv_struct_wrapper::vkwin32keyedmutexacquirereleaseinfonv_struct_wrapper(const VkWin32KeyedMutexAcquireReleaseInfoNV* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkwin32keyedmutexacquirereleaseinfonv_struct_wrapper::~vkwin32keyedmutexacquirereleaseinfonv_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkwin32keyedmutexacquirereleaseinfonv_struct_wrapper::display_single_txt()
{
    printf(" %*sVkWin32KeyedMutexAcquireReleaseInfoNV = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkwin32keyedmutexacquirereleaseinfonv_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sacquireCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.acquireCount));
    uint32_t i;
    for (i = 0; i<acquireCount; i++) {
        printf("%*s    %spAcquireSyncs[%u] = 0x%p\n", m_indent, "", &m_dummy_prefix, i, (m_struct.pAcquireSyncs)[i]);
    }
    for (i = 0; i<acquireCount; i++) {
        printf("%*s    %spAcquireKeys[%u] = %" PRId64 "\n", m_indent, "", &m_dummy_prefix, i, (m_struct.pAcquireKeys)[i]);
    }
    for (i = 0; i<acquireCount; i++) {
        printf("%*s    %spAcquireTimeoutMilliseconds[%u] = %u\n", m_indent, "", &m_dummy_prefix, i, (m_struct.pAcquireTimeoutMilliseconds)[i]);
    }
    printf("%*s    %sreleaseCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.releaseCount));
    for (i = 0; i<releaseCount; i++) {
        printf("%*s    %spReleaseSyncs[%u] = 0x%p\n", m_indent, "", &m_dummy_prefix, i, (m_struct.pReleaseSyncs)[i]);
    }
    for (i = 0; i<releaseCount; i++) {
        printf("%*s    %spReleaseKeys[%u] = %" PRId64 "\n", m_indent, "", &m_dummy_prefix, i, (m_struct.pReleaseKeys)[i]);
    }
}

// Output all struct elements, each on their own line
void vkwin32keyedmutexacquirereleaseinfonv_struct_wrapper::display_txt()
{
    printf("%*sVkWin32KeyedMutexAcquireReleaseInfoNV struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkwin32keyedmutexacquirereleaseinfonv_struct_wrapper::display_full_txt()
{
    printf("%*sVkWin32KeyedMutexAcquireReleaseInfoNV struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkwin32surfacecreateinfokhr_struct_wrapper class definition
vkwin32surfacecreateinfokhr_struct_wrapper::vkwin32surfacecreateinfokhr_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkwin32surfacecreateinfokhr_struct_wrapper::vkwin32surfacecreateinfokhr_struct_wrapper(VkWin32SurfaceCreateInfoKHR* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkwin32surfacecreateinfokhr_struct_wrapper::vkwin32surfacecreateinfokhr_struct_wrapper(const VkWin32SurfaceCreateInfoKHR* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkwin32surfacecreateinfokhr_struct_wrapper::~vkwin32surfacecreateinfokhr_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkwin32surfacecreateinfokhr_struct_wrapper::display_single_txt()
{
    printf(" %*sVkWin32SurfaceCreateInfoKHR = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkwin32surfacecreateinfokhr_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
    printf("%*s    %shinstance = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.hinstance));
    printf("%*s    %shwnd = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.hwnd));
}

// Output all struct elements, each on their own line
void vkwin32surfacecreateinfokhr_struct_wrapper::display_txt()
{
    printf("%*sVkWin32SurfaceCreateInfoKHR struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkwin32surfacecreateinfokhr_struct_wrapper::display_full_txt()
{
    printf("%*sVkWin32SurfaceCreateInfoKHR struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkwritedescriptorset_struct_wrapper class definition
vkwritedescriptorset_struct_wrapper::vkwritedescriptorset_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkwritedescriptorset_struct_wrapper::vkwritedescriptorset_struct_wrapper(VkWriteDescriptorSet* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkwritedescriptorset_struct_wrapper::vkwritedescriptorset_struct_wrapper(const VkWriteDescriptorSet* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkwritedescriptorset_struct_wrapper::~vkwritedescriptorset_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkwritedescriptorset_struct_wrapper::display_single_txt()
{
    printf(" %*sVkWriteDescriptorSet = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkwritedescriptorset_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sdstSet = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.dstSet));
    printf("%*s    %sdstBinding = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.dstBinding));
    printf("%*s    %sdstArrayElement = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.dstArrayElement));
    printf("%*s    %sdescriptorCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.descriptorCount));
    printf("%*s    %sdescriptorType = %s\n", m_indent, "", &m_dummy_prefix, string_VkDescriptorType(m_struct.descriptorType));
    uint32_t i;
    for (i = 0; i<descriptorCount; i++) {
        printf("%*s    %spImageInfo[%u] = 0x%p\n", m_indent, "", &m_dummy_prefix, i, (void*)(m_struct.pImageInfo)[i]);
    }
    for (i = 0; i<descriptorCount; i++) {
        printf("%*s    %spBufferInfo[%u] = 0x%p\n", m_indent, "", &m_dummy_prefix, i, (void*)(m_struct.pBufferInfo)[i]);
    }
    for (i = 0; i<descriptorCount; i++) {
        printf("%*s    %spTexelBufferView[%u] = 0x%p\n", m_indent, "", &m_dummy_prefix, i, (m_struct.pTexelBufferView)[i]);
    }
}

// Output all struct elements, each on their own line
void vkwritedescriptorset_struct_wrapper::display_txt()
{
    printf("%*sVkWriteDescriptorSet struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkwritedescriptorset_struct_wrapper::display_full_txt()
{
    printf("%*sVkWriteDescriptorSet struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    uint32_t i;
    for (i = 0; i<descriptorCount; i++) {
            vkdescriptorbufferinfo_struct_wrapper class0(&(m_struct.pBufferInfo[i]));
            class0.set_indent(m_indent + 4);
            class0.display_full_txt();
    }
    for (i = 0; i<descriptorCount; i++) {
            vkdescriptorimageinfo_struct_wrapper class1(&(m_struct.pImageInfo[i]));
            class1.set_indent(m_indent + 4);
            class1.display_full_txt();
    }
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkxcbsurfacecreateinfokhr_struct_wrapper class definition
vkxcbsurfacecreateinfokhr_struct_wrapper::vkxcbsurfacecreateinfokhr_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkxcbsurfacecreateinfokhr_struct_wrapper::vkxcbsurfacecreateinfokhr_struct_wrapper(VkXcbSurfaceCreateInfoKHR* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkxcbsurfacecreateinfokhr_struct_wrapper::vkxcbsurfacecreateinfokhr_struct_wrapper(const VkXcbSurfaceCreateInfoKHR* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkxcbsurfacecreateinfokhr_struct_wrapper::~vkxcbsurfacecreateinfokhr_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkxcbsurfacecreateinfokhr_struct_wrapper::display_single_txt()
{
    printf(" %*sVkXcbSurfaceCreateInfoKHR = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkxcbsurfacecreateinfokhr_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
    printf("%*s    %sconnection = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.connection));
    printf("%*s    %swindow = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.window));
}

// Output all struct elements, each on their own line
void vkxcbsurfacecreateinfokhr_struct_wrapper::display_txt()
{
    printf("%*sVkXcbSurfaceCreateInfoKHR struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkxcbsurfacecreateinfokhr_struct_wrapper::display_full_txt()
{
    printf("%*sVkXcbSurfaceCreateInfoKHR struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// vkxlibsurfacecreateinfokhr_struct_wrapper class definition
vkxlibsurfacecreateinfokhr_struct_wrapper::vkxlibsurfacecreateinfokhr_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
vkxlibsurfacecreateinfokhr_struct_wrapper::vkxlibsurfacecreateinfokhr_struct_wrapper(VkXlibSurfaceCreateInfoKHR* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkxlibsurfacecreateinfokhr_struct_wrapper::vkxlibsurfacecreateinfokhr_struct_wrapper(const VkXlibSurfaceCreateInfoKHR* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
vkxlibsurfacecreateinfokhr_struct_wrapper::~vkxlibsurfacecreateinfokhr_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void vkxlibsurfacecreateinfokhr_struct_wrapper::display_single_txt()
{
    printf(" %*sVkXlibSurfaceCreateInfoKHR = 0x%p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void vkxlibsurfacecreateinfokhr_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_VkStructureType(m_struct.sType));
    printf("%*s    %spNext = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
    printf("%*s    %sdpy = 0x%p\n", m_indent, "", &m_dummy_prefix, (m_struct.dpy));
    printf("%*s    %swindow = 0x%p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.window));
}

// Output all struct elements, each on their own line
void vkxlibsurfacecreateinfokhr_struct_wrapper::display_txt()
{
    printf("%*sVkXlibSurfaceCreateInfoKHR struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void vkxlibsurfacecreateinfokhr_struct_wrapper::display_full_txt()
{
    printf("%*sVkXlibSurfaceCreateInfoKHR struct contents at 0x%p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}

//any footer info for class
