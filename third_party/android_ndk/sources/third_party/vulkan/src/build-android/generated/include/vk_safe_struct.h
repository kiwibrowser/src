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
#pragma once
#include "vulkan/vulkan.h"
struct safe_VkApplicationInfo {
    VkStructureType sType;
    const void* pNext;
    const char* pApplicationName;
    uint32_t applicationVersion;
    const char* pEngineName;
    uint32_t engineVersion;
    uint32_t apiVersion;
    safe_VkApplicationInfo(const VkApplicationInfo* pInStruct);
    safe_VkApplicationInfo(const safe_VkApplicationInfo& src);
    safe_VkApplicationInfo();
    ~safe_VkApplicationInfo();
    void initialize(const VkApplicationInfo* pInStruct);
    void initialize(const safe_VkApplicationInfo* src);
    VkApplicationInfo *ptr() { return reinterpret_cast<VkApplicationInfo *>(this); }
    VkApplicationInfo const *ptr() const { return reinterpret_cast<VkApplicationInfo const *>(this); }
};

struct safe_VkInstanceCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkInstanceCreateFlags flags;
    safe_VkApplicationInfo* pApplicationInfo;
    uint32_t enabledLayerCount;
    const char* const* ppEnabledLayerNames;
    uint32_t enabledExtensionCount;
    const char* const* ppEnabledExtensionNames;
    safe_VkInstanceCreateInfo(const VkInstanceCreateInfo* pInStruct);
    safe_VkInstanceCreateInfo(const safe_VkInstanceCreateInfo& src);
    safe_VkInstanceCreateInfo();
    ~safe_VkInstanceCreateInfo();
    void initialize(const VkInstanceCreateInfo* pInStruct);
    void initialize(const safe_VkInstanceCreateInfo* src);
    VkInstanceCreateInfo *ptr() { return reinterpret_cast<VkInstanceCreateInfo *>(this); }
    VkInstanceCreateInfo const *ptr() const { return reinterpret_cast<VkInstanceCreateInfo const *>(this); }
};

struct safe_VkAllocationCallbacks {
    void* pUserData;
    PFN_vkAllocationFunction pfnAllocation;
    PFN_vkReallocationFunction pfnReallocation;
    PFN_vkFreeFunction pfnFree;
    PFN_vkInternalAllocationNotification pfnInternalAllocation;
    PFN_vkInternalFreeNotification pfnInternalFree;
    safe_VkAllocationCallbacks(const VkAllocationCallbacks* pInStruct);
    safe_VkAllocationCallbacks(const safe_VkAllocationCallbacks& src);
    safe_VkAllocationCallbacks();
    ~safe_VkAllocationCallbacks();
    void initialize(const VkAllocationCallbacks* pInStruct);
    void initialize(const safe_VkAllocationCallbacks* src);
    VkAllocationCallbacks *ptr() { return reinterpret_cast<VkAllocationCallbacks *>(this); }
    VkAllocationCallbacks const *ptr() const { return reinterpret_cast<VkAllocationCallbacks const *>(this); }
};

struct safe_VkDeviceQueueCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkDeviceQueueCreateFlags flags;
    uint32_t queueFamilyIndex;
    uint32_t queueCount;
    const float* pQueuePriorities;
    safe_VkDeviceQueueCreateInfo(const VkDeviceQueueCreateInfo* pInStruct);
    safe_VkDeviceQueueCreateInfo(const safe_VkDeviceQueueCreateInfo& src);
    safe_VkDeviceQueueCreateInfo();
    ~safe_VkDeviceQueueCreateInfo();
    void initialize(const VkDeviceQueueCreateInfo* pInStruct);
    void initialize(const safe_VkDeviceQueueCreateInfo* src);
    VkDeviceQueueCreateInfo *ptr() { return reinterpret_cast<VkDeviceQueueCreateInfo *>(this); }
    VkDeviceQueueCreateInfo const *ptr() const { return reinterpret_cast<VkDeviceQueueCreateInfo const *>(this); }
};

struct safe_VkDeviceCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkDeviceCreateFlags flags;
    uint32_t queueCreateInfoCount;
    safe_VkDeviceQueueCreateInfo* pQueueCreateInfos;
    uint32_t enabledLayerCount;
    const char* const* ppEnabledLayerNames;
    uint32_t enabledExtensionCount;
    const char* const* ppEnabledExtensionNames;
    const VkPhysicalDeviceFeatures* pEnabledFeatures;
    safe_VkDeviceCreateInfo(const VkDeviceCreateInfo* pInStruct);
    safe_VkDeviceCreateInfo(const safe_VkDeviceCreateInfo& src);
    safe_VkDeviceCreateInfo();
    ~safe_VkDeviceCreateInfo();
    void initialize(const VkDeviceCreateInfo* pInStruct);
    void initialize(const safe_VkDeviceCreateInfo* src);
    VkDeviceCreateInfo *ptr() { return reinterpret_cast<VkDeviceCreateInfo *>(this); }
    VkDeviceCreateInfo const *ptr() const { return reinterpret_cast<VkDeviceCreateInfo const *>(this); }
};

struct safe_VkSubmitInfo {
    VkStructureType sType;
    const void* pNext;
    uint32_t waitSemaphoreCount;
    VkSemaphore* pWaitSemaphores;
    const VkPipelineStageFlags* pWaitDstStageMask;
    uint32_t commandBufferCount;
    const VkCommandBuffer* pCommandBuffers;
    uint32_t signalSemaphoreCount;
    VkSemaphore* pSignalSemaphores;
    safe_VkSubmitInfo(const VkSubmitInfo* pInStruct);
    safe_VkSubmitInfo(const safe_VkSubmitInfo& src);
    safe_VkSubmitInfo();
    ~safe_VkSubmitInfo();
    void initialize(const VkSubmitInfo* pInStruct);
    void initialize(const safe_VkSubmitInfo* src);
    VkSubmitInfo *ptr() { return reinterpret_cast<VkSubmitInfo *>(this); }
    VkSubmitInfo const *ptr() const { return reinterpret_cast<VkSubmitInfo const *>(this); }
};

struct safe_VkMemoryAllocateInfo {
    VkStructureType sType;
    const void* pNext;
    VkDeviceSize allocationSize;
    uint32_t memoryTypeIndex;
    safe_VkMemoryAllocateInfo(const VkMemoryAllocateInfo* pInStruct);
    safe_VkMemoryAllocateInfo(const safe_VkMemoryAllocateInfo& src);
    safe_VkMemoryAllocateInfo();
    ~safe_VkMemoryAllocateInfo();
    void initialize(const VkMemoryAllocateInfo* pInStruct);
    void initialize(const safe_VkMemoryAllocateInfo* src);
    VkMemoryAllocateInfo *ptr() { return reinterpret_cast<VkMemoryAllocateInfo *>(this); }
    VkMemoryAllocateInfo const *ptr() const { return reinterpret_cast<VkMemoryAllocateInfo const *>(this); }
};

struct safe_VkMappedMemoryRange {
    VkStructureType sType;
    const void* pNext;
    VkDeviceMemory memory;
    VkDeviceSize offset;
    VkDeviceSize size;
    safe_VkMappedMemoryRange(const VkMappedMemoryRange* pInStruct);
    safe_VkMappedMemoryRange(const safe_VkMappedMemoryRange& src);
    safe_VkMappedMemoryRange();
    ~safe_VkMappedMemoryRange();
    void initialize(const VkMappedMemoryRange* pInStruct);
    void initialize(const safe_VkMappedMemoryRange* src);
    VkMappedMemoryRange *ptr() { return reinterpret_cast<VkMappedMemoryRange *>(this); }
    VkMappedMemoryRange const *ptr() const { return reinterpret_cast<VkMappedMemoryRange const *>(this); }
};

struct safe_VkSparseBufferMemoryBindInfo {
    VkBuffer buffer;
    uint32_t bindCount;
    VkSparseMemoryBind* pBinds;
    safe_VkSparseBufferMemoryBindInfo(const VkSparseBufferMemoryBindInfo* pInStruct);
    safe_VkSparseBufferMemoryBindInfo(const safe_VkSparseBufferMemoryBindInfo& src);
    safe_VkSparseBufferMemoryBindInfo();
    ~safe_VkSparseBufferMemoryBindInfo();
    void initialize(const VkSparseBufferMemoryBindInfo* pInStruct);
    void initialize(const safe_VkSparseBufferMemoryBindInfo* src);
    VkSparseBufferMemoryBindInfo *ptr() { return reinterpret_cast<VkSparseBufferMemoryBindInfo *>(this); }
    VkSparseBufferMemoryBindInfo const *ptr() const { return reinterpret_cast<VkSparseBufferMemoryBindInfo const *>(this); }
};

struct safe_VkSparseImageOpaqueMemoryBindInfo {
    VkImage image;
    uint32_t bindCount;
    VkSparseMemoryBind* pBinds;
    safe_VkSparseImageOpaqueMemoryBindInfo(const VkSparseImageOpaqueMemoryBindInfo* pInStruct);
    safe_VkSparseImageOpaqueMemoryBindInfo(const safe_VkSparseImageOpaqueMemoryBindInfo& src);
    safe_VkSparseImageOpaqueMemoryBindInfo();
    ~safe_VkSparseImageOpaqueMemoryBindInfo();
    void initialize(const VkSparseImageOpaqueMemoryBindInfo* pInStruct);
    void initialize(const safe_VkSparseImageOpaqueMemoryBindInfo* src);
    VkSparseImageOpaqueMemoryBindInfo *ptr() { return reinterpret_cast<VkSparseImageOpaqueMemoryBindInfo *>(this); }
    VkSparseImageOpaqueMemoryBindInfo const *ptr() const { return reinterpret_cast<VkSparseImageOpaqueMemoryBindInfo const *>(this); }
};

struct safe_VkSparseImageMemoryBindInfo {
    VkImage image;
    uint32_t bindCount;
    VkSparseImageMemoryBind* pBinds;
    safe_VkSparseImageMemoryBindInfo(const VkSparseImageMemoryBindInfo* pInStruct);
    safe_VkSparseImageMemoryBindInfo(const safe_VkSparseImageMemoryBindInfo& src);
    safe_VkSparseImageMemoryBindInfo();
    ~safe_VkSparseImageMemoryBindInfo();
    void initialize(const VkSparseImageMemoryBindInfo* pInStruct);
    void initialize(const safe_VkSparseImageMemoryBindInfo* src);
    VkSparseImageMemoryBindInfo *ptr() { return reinterpret_cast<VkSparseImageMemoryBindInfo *>(this); }
    VkSparseImageMemoryBindInfo const *ptr() const { return reinterpret_cast<VkSparseImageMemoryBindInfo const *>(this); }
};

struct safe_VkBindSparseInfo {
    VkStructureType sType;
    const void* pNext;
    uint32_t waitSemaphoreCount;
    VkSemaphore* pWaitSemaphores;
    uint32_t bufferBindCount;
    safe_VkSparseBufferMemoryBindInfo* pBufferBinds;
    uint32_t imageOpaqueBindCount;
    safe_VkSparseImageOpaqueMemoryBindInfo* pImageOpaqueBinds;
    uint32_t imageBindCount;
    safe_VkSparseImageMemoryBindInfo* pImageBinds;
    uint32_t signalSemaphoreCount;
    VkSemaphore* pSignalSemaphores;
    safe_VkBindSparseInfo(const VkBindSparseInfo* pInStruct);
    safe_VkBindSparseInfo(const safe_VkBindSparseInfo& src);
    safe_VkBindSparseInfo();
    ~safe_VkBindSparseInfo();
    void initialize(const VkBindSparseInfo* pInStruct);
    void initialize(const safe_VkBindSparseInfo* src);
    VkBindSparseInfo *ptr() { return reinterpret_cast<VkBindSparseInfo *>(this); }
    VkBindSparseInfo const *ptr() const { return reinterpret_cast<VkBindSparseInfo const *>(this); }
};

struct safe_VkFenceCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkFenceCreateFlags flags;
    safe_VkFenceCreateInfo(const VkFenceCreateInfo* pInStruct);
    safe_VkFenceCreateInfo(const safe_VkFenceCreateInfo& src);
    safe_VkFenceCreateInfo();
    ~safe_VkFenceCreateInfo();
    void initialize(const VkFenceCreateInfo* pInStruct);
    void initialize(const safe_VkFenceCreateInfo* src);
    VkFenceCreateInfo *ptr() { return reinterpret_cast<VkFenceCreateInfo *>(this); }
    VkFenceCreateInfo const *ptr() const { return reinterpret_cast<VkFenceCreateInfo const *>(this); }
};

struct safe_VkSemaphoreCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkSemaphoreCreateFlags flags;
    safe_VkSemaphoreCreateInfo(const VkSemaphoreCreateInfo* pInStruct);
    safe_VkSemaphoreCreateInfo(const safe_VkSemaphoreCreateInfo& src);
    safe_VkSemaphoreCreateInfo();
    ~safe_VkSemaphoreCreateInfo();
    void initialize(const VkSemaphoreCreateInfo* pInStruct);
    void initialize(const safe_VkSemaphoreCreateInfo* src);
    VkSemaphoreCreateInfo *ptr() { return reinterpret_cast<VkSemaphoreCreateInfo *>(this); }
    VkSemaphoreCreateInfo const *ptr() const { return reinterpret_cast<VkSemaphoreCreateInfo const *>(this); }
};

struct safe_VkEventCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkEventCreateFlags flags;
    safe_VkEventCreateInfo(const VkEventCreateInfo* pInStruct);
    safe_VkEventCreateInfo(const safe_VkEventCreateInfo& src);
    safe_VkEventCreateInfo();
    ~safe_VkEventCreateInfo();
    void initialize(const VkEventCreateInfo* pInStruct);
    void initialize(const safe_VkEventCreateInfo* src);
    VkEventCreateInfo *ptr() { return reinterpret_cast<VkEventCreateInfo *>(this); }
    VkEventCreateInfo const *ptr() const { return reinterpret_cast<VkEventCreateInfo const *>(this); }
};

struct safe_VkQueryPoolCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkQueryPoolCreateFlags flags;
    VkQueryType queryType;
    uint32_t queryCount;
    VkQueryPipelineStatisticFlags pipelineStatistics;
    safe_VkQueryPoolCreateInfo(const VkQueryPoolCreateInfo* pInStruct);
    safe_VkQueryPoolCreateInfo(const safe_VkQueryPoolCreateInfo& src);
    safe_VkQueryPoolCreateInfo();
    ~safe_VkQueryPoolCreateInfo();
    void initialize(const VkQueryPoolCreateInfo* pInStruct);
    void initialize(const safe_VkQueryPoolCreateInfo* src);
    VkQueryPoolCreateInfo *ptr() { return reinterpret_cast<VkQueryPoolCreateInfo *>(this); }
    VkQueryPoolCreateInfo const *ptr() const { return reinterpret_cast<VkQueryPoolCreateInfo const *>(this); }
};

struct safe_VkBufferCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkBufferCreateFlags flags;
    VkDeviceSize size;
    VkBufferUsageFlags usage;
    VkSharingMode sharingMode;
    uint32_t queueFamilyIndexCount;
    const uint32_t* pQueueFamilyIndices;
    safe_VkBufferCreateInfo(const VkBufferCreateInfo* pInStruct);
    safe_VkBufferCreateInfo(const safe_VkBufferCreateInfo& src);
    safe_VkBufferCreateInfo();
    ~safe_VkBufferCreateInfo();
    void initialize(const VkBufferCreateInfo* pInStruct);
    void initialize(const safe_VkBufferCreateInfo* src);
    VkBufferCreateInfo *ptr() { return reinterpret_cast<VkBufferCreateInfo *>(this); }
    VkBufferCreateInfo const *ptr() const { return reinterpret_cast<VkBufferCreateInfo const *>(this); }
};

struct safe_VkBufferViewCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkBufferViewCreateFlags flags;
    VkBuffer buffer;
    VkFormat format;
    VkDeviceSize offset;
    VkDeviceSize range;
    safe_VkBufferViewCreateInfo(const VkBufferViewCreateInfo* pInStruct);
    safe_VkBufferViewCreateInfo(const safe_VkBufferViewCreateInfo& src);
    safe_VkBufferViewCreateInfo();
    ~safe_VkBufferViewCreateInfo();
    void initialize(const VkBufferViewCreateInfo* pInStruct);
    void initialize(const safe_VkBufferViewCreateInfo* src);
    VkBufferViewCreateInfo *ptr() { return reinterpret_cast<VkBufferViewCreateInfo *>(this); }
    VkBufferViewCreateInfo const *ptr() const { return reinterpret_cast<VkBufferViewCreateInfo const *>(this); }
};

struct safe_VkImageCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkImageCreateFlags flags;
    VkImageType imageType;
    VkFormat format;
    VkExtent3D extent;
    uint32_t mipLevels;
    uint32_t arrayLayers;
    VkSampleCountFlagBits samples;
    VkImageTiling tiling;
    VkImageUsageFlags usage;
    VkSharingMode sharingMode;
    uint32_t queueFamilyIndexCount;
    const uint32_t* pQueueFamilyIndices;
    VkImageLayout initialLayout;
    safe_VkImageCreateInfo(const VkImageCreateInfo* pInStruct);
    safe_VkImageCreateInfo(const safe_VkImageCreateInfo& src);
    safe_VkImageCreateInfo();
    ~safe_VkImageCreateInfo();
    void initialize(const VkImageCreateInfo* pInStruct);
    void initialize(const safe_VkImageCreateInfo* src);
    VkImageCreateInfo *ptr() { return reinterpret_cast<VkImageCreateInfo *>(this); }
    VkImageCreateInfo const *ptr() const { return reinterpret_cast<VkImageCreateInfo const *>(this); }
};

struct safe_VkImageViewCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkImageViewCreateFlags flags;
    VkImage image;
    VkImageViewType viewType;
    VkFormat format;
    VkComponentMapping components;
    VkImageSubresourceRange subresourceRange;
    safe_VkImageViewCreateInfo(const VkImageViewCreateInfo* pInStruct);
    safe_VkImageViewCreateInfo(const safe_VkImageViewCreateInfo& src);
    safe_VkImageViewCreateInfo();
    ~safe_VkImageViewCreateInfo();
    void initialize(const VkImageViewCreateInfo* pInStruct);
    void initialize(const safe_VkImageViewCreateInfo* src);
    VkImageViewCreateInfo *ptr() { return reinterpret_cast<VkImageViewCreateInfo *>(this); }
    VkImageViewCreateInfo const *ptr() const { return reinterpret_cast<VkImageViewCreateInfo const *>(this); }
};

struct safe_VkShaderModuleCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkShaderModuleCreateFlags flags;
    size_t codeSize;
    const uint32_t* pCode;
    safe_VkShaderModuleCreateInfo(const VkShaderModuleCreateInfo* pInStruct);
    safe_VkShaderModuleCreateInfo(const safe_VkShaderModuleCreateInfo& src);
    safe_VkShaderModuleCreateInfo();
    ~safe_VkShaderModuleCreateInfo();
    void initialize(const VkShaderModuleCreateInfo* pInStruct);
    void initialize(const safe_VkShaderModuleCreateInfo* src);
    VkShaderModuleCreateInfo *ptr() { return reinterpret_cast<VkShaderModuleCreateInfo *>(this); }
    VkShaderModuleCreateInfo const *ptr() const { return reinterpret_cast<VkShaderModuleCreateInfo const *>(this); }
};

struct safe_VkPipelineCacheCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkPipelineCacheCreateFlags flags;
    size_t initialDataSize;
    const void* pInitialData;
    safe_VkPipelineCacheCreateInfo(const VkPipelineCacheCreateInfo* pInStruct);
    safe_VkPipelineCacheCreateInfo(const safe_VkPipelineCacheCreateInfo& src);
    safe_VkPipelineCacheCreateInfo();
    ~safe_VkPipelineCacheCreateInfo();
    void initialize(const VkPipelineCacheCreateInfo* pInStruct);
    void initialize(const safe_VkPipelineCacheCreateInfo* src);
    VkPipelineCacheCreateInfo *ptr() { return reinterpret_cast<VkPipelineCacheCreateInfo *>(this); }
    VkPipelineCacheCreateInfo const *ptr() const { return reinterpret_cast<VkPipelineCacheCreateInfo const *>(this); }
};

struct safe_VkSpecializationInfo {
    uint32_t mapEntryCount;
    const VkSpecializationMapEntry* pMapEntries;
    size_t dataSize;
    const void* pData;
    safe_VkSpecializationInfo(const VkSpecializationInfo* pInStruct);
    safe_VkSpecializationInfo(const safe_VkSpecializationInfo& src);
    safe_VkSpecializationInfo();
    ~safe_VkSpecializationInfo();
    void initialize(const VkSpecializationInfo* pInStruct);
    void initialize(const safe_VkSpecializationInfo* src);
    VkSpecializationInfo *ptr() { return reinterpret_cast<VkSpecializationInfo *>(this); }
    VkSpecializationInfo const *ptr() const { return reinterpret_cast<VkSpecializationInfo const *>(this); }
};

struct safe_VkPipelineShaderStageCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkPipelineShaderStageCreateFlags flags;
    VkShaderStageFlagBits stage;
    VkShaderModule module;
    const char* pName;
    safe_VkSpecializationInfo* pSpecializationInfo;
    safe_VkPipelineShaderStageCreateInfo(const VkPipelineShaderStageCreateInfo* pInStruct);
    safe_VkPipelineShaderStageCreateInfo(const safe_VkPipelineShaderStageCreateInfo& src);
    safe_VkPipelineShaderStageCreateInfo();
    ~safe_VkPipelineShaderStageCreateInfo();
    void initialize(const VkPipelineShaderStageCreateInfo* pInStruct);
    void initialize(const safe_VkPipelineShaderStageCreateInfo* src);
    VkPipelineShaderStageCreateInfo *ptr() { return reinterpret_cast<VkPipelineShaderStageCreateInfo *>(this); }
    VkPipelineShaderStageCreateInfo const *ptr() const { return reinterpret_cast<VkPipelineShaderStageCreateInfo const *>(this); }
};

struct safe_VkPipelineVertexInputStateCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkPipelineVertexInputStateCreateFlags flags;
    uint32_t vertexBindingDescriptionCount;
    const VkVertexInputBindingDescription* pVertexBindingDescriptions;
    uint32_t vertexAttributeDescriptionCount;
    const VkVertexInputAttributeDescription* pVertexAttributeDescriptions;
    safe_VkPipelineVertexInputStateCreateInfo(const VkPipelineVertexInputStateCreateInfo* pInStruct);
    safe_VkPipelineVertexInputStateCreateInfo(const safe_VkPipelineVertexInputStateCreateInfo& src);
    safe_VkPipelineVertexInputStateCreateInfo();
    ~safe_VkPipelineVertexInputStateCreateInfo();
    void initialize(const VkPipelineVertexInputStateCreateInfo* pInStruct);
    void initialize(const safe_VkPipelineVertexInputStateCreateInfo* src);
    VkPipelineVertexInputStateCreateInfo *ptr() { return reinterpret_cast<VkPipelineVertexInputStateCreateInfo *>(this); }
    VkPipelineVertexInputStateCreateInfo const *ptr() const { return reinterpret_cast<VkPipelineVertexInputStateCreateInfo const *>(this); }
};

struct safe_VkPipelineInputAssemblyStateCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkPipelineInputAssemblyStateCreateFlags flags;
    VkPrimitiveTopology topology;
    VkBool32 primitiveRestartEnable;
    safe_VkPipelineInputAssemblyStateCreateInfo(const VkPipelineInputAssemblyStateCreateInfo* pInStruct);
    safe_VkPipelineInputAssemblyStateCreateInfo(const safe_VkPipelineInputAssemblyStateCreateInfo& src);
    safe_VkPipelineInputAssemblyStateCreateInfo();
    ~safe_VkPipelineInputAssemblyStateCreateInfo();
    void initialize(const VkPipelineInputAssemblyStateCreateInfo* pInStruct);
    void initialize(const safe_VkPipelineInputAssemblyStateCreateInfo* src);
    VkPipelineInputAssemblyStateCreateInfo *ptr() { return reinterpret_cast<VkPipelineInputAssemblyStateCreateInfo *>(this); }
    VkPipelineInputAssemblyStateCreateInfo const *ptr() const { return reinterpret_cast<VkPipelineInputAssemblyStateCreateInfo const *>(this); }
};

struct safe_VkPipelineTessellationStateCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkPipelineTessellationStateCreateFlags flags;
    uint32_t patchControlPoints;
    safe_VkPipelineTessellationStateCreateInfo(const VkPipelineTessellationStateCreateInfo* pInStruct);
    safe_VkPipelineTessellationStateCreateInfo(const safe_VkPipelineTessellationStateCreateInfo& src);
    safe_VkPipelineTessellationStateCreateInfo();
    ~safe_VkPipelineTessellationStateCreateInfo();
    void initialize(const VkPipelineTessellationStateCreateInfo* pInStruct);
    void initialize(const safe_VkPipelineTessellationStateCreateInfo* src);
    VkPipelineTessellationStateCreateInfo *ptr() { return reinterpret_cast<VkPipelineTessellationStateCreateInfo *>(this); }
    VkPipelineTessellationStateCreateInfo const *ptr() const { return reinterpret_cast<VkPipelineTessellationStateCreateInfo const *>(this); }
};

struct safe_VkPipelineViewportStateCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkPipelineViewportStateCreateFlags flags;
    uint32_t viewportCount;
    const VkViewport* pViewports;
    uint32_t scissorCount;
    const VkRect2D* pScissors;
    safe_VkPipelineViewportStateCreateInfo(const VkPipelineViewportStateCreateInfo* pInStruct);
    safe_VkPipelineViewportStateCreateInfo(const safe_VkPipelineViewportStateCreateInfo& src);
    safe_VkPipelineViewportStateCreateInfo();
    ~safe_VkPipelineViewportStateCreateInfo();
    void initialize(const VkPipelineViewportStateCreateInfo* pInStruct);
    void initialize(const safe_VkPipelineViewportStateCreateInfo* src);
    VkPipelineViewportStateCreateInfo *ptr() { return reinterpret_cast<VkPipelineViewportStateCreateInfo *>(this); }
    VkPipelineViewportStateCreateInfo const *ptr() const { return reinterpret_cast<VkPipelineViewportStateCreateInfo const *>(this); }
};

struct safe_VkPipelineRasterizationStateCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkPipelineRasterizationStateCreateFlags flags;
    VkBool32 depthClampEnable;
    VkBool32 rasterizerDiscardEnable;
    VkPolygonMode polygonMode;
    VkCullModeFlags cullMode;
    VkFrontFace frontFace;
    VkBool32 depthBiasEnable;
    float depthBiasConstantFactor;
    float depthBiasClamp;
    float depthBiasSlopeFactor;
    float lineWidth;
    safe_VkPipelineRasterizationStateCreateInfo(const VkPipelineRasterizationStateCreateInfo* pInStruct);
    safe_VkPipelineRasterizationStateCreateInfo(const safe_VkPipelineRasterizationStateCreateInfo& src);
    safe_VkPipelineRasterizationStateCreateInfo();
    ~safe_VkPipelineRasterizationStateCreateInfo();
    void initialize(const VkPipelineRasterizationStateCreateInfo* pInStruct);
    void initialize(const safe_VkPipelineRasterizationStateCreateInfo* src);
    VkPipelineRasterizationStateCreateInfo *ptr() { return reinterpret_cast<VkPipelineRasterizationStateCreateInfo *>(this); }
    VkPipelineRasterizationStateCreateInfo const *ptr() const { return reinterpret_cast<VkPipelineRasterizationStateCreateInfo const *>(this); }
};

struct safe_VkPipelineMultisampleStateCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkPipelineMultisampleStateCreateFlags flags;
    VkSampleCountFlagBits rasterizationSamples;
    VkBool32 sampleShadingEnable;
    float minSampleShading;
    const VkSampleMask* pSampleMask;
    VkBool32 alphaToCoverageEnable;
    VkBool32 alphaToOneEnable;
    safe_VkPipelineMultisampleStateCreateInfo(const VkPipelineMultisampleStateCreateInfo* pInStruct);
    safe_VkPipelineMultisampleStateCreateInfo(const safe_VkPipelineMultisampleStateCreateInfo& src);
    safe_VkPipelineMultisampleStateCreateInfo();
    ~safe_VkPipelineMultisampleStateCreateInfo();
    void initialize(const VkPipelineMultisampleStateCreateInfo* pInStruct);
    void initialize(const safe_VkPipelineMultisampleStateCreateInfo* src);
    VkPipelineMultisampleStateCreateInfo *ptr() { return reinterpret_cast<VkPipelineMultisampleStateCreateInfo *>(this); }
    VkPipelineMultisampleStateCreateInfo const *ptr() const { return reinterpret_cast<VkPipelineMultisampleStateCreateInfo const *>(this); }
};

struct safe_VkPipelineDepthStencilStateCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkPipelineDepthStencilStateCreateFlags flags;
    VkBool32 depthTestEnable;
    VkBool32 depthWriteEnable;
    VkCompareOp depthCompareOp;
    VkBool32 depthBoundsTestEnable;
    VkBool32 stencilTestEnable;
    VkStencilOpState front;
    VkStencilOpState back;
    float minDepthBounds;
    float maxDepthBounds;
    safe_VkPipelineDepthStencilStateCreateInfo(const VkPipelineDepthStencilStateCreateInfo* pInStruct);
    safe_VkPipelineDepthStencilStateCreateInfo(const safe_VkPipelineDepthStencilStateCreateInfo& src);
    safe_VkPipelineDepthStencilStateCreateInfo();
    ~safe_VkPipelineDepthStencilStateCreateInfo();
    void initialize(const VkPipelineDepthStencilStateCreateInfo* pInStruct);
    void initialize(const safe_VkPipelineDepthStencilStateCreateInfo* src);
    VkPipelineDepthStencilStateCreateInfo *ptr() { return reinterpret_cast<VkPipelineDepthStencilStateCreateInfo *>(this); }
    VkPipelineDepthStencilStateCreateInfo const *ptr() const { return reinterpret_cast<VkPipelineDepthStencilStateCreateInfo const *>(this); }
};

struct safe_VkPipelineColorBlendStateCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkPipelineColorBlendStateCreateFlags flags;
    VkBool32 logicOpEnable;
    VkLogicOp logicOp;
    uint32_t attachmentCount;
    const VkPipelineColorBlendAttachmentState* pAttachments;
    float blendConstants[4];
    safe_VkPipelineColorBlendStateCreateInfo(const VkPipelineColorBlendStateCreateInfo* pInStruct);
    safe_VkPipelineColorBlendStateCreateInfo(const safe_VkPipelineColorBlendStateCreateInfo& src);
    safe_VkPipelineColorBlendStateCreateInfo();
    ~safe_VkPipelineColorBlendStateCreateInfo();
    void initialize(const VkPipelineColorBlendStateCreateInfo* pInStruct);
    void initialize(const safe_VkPipelineColorBlendStateCreateInfo* src);
    VkPipelineColorBlendStateCreateInfo *ptr() { return reinterpret_cast<VkPipelineColorBlendStateCreateInfo *>(this); }
    VkPipelineColorBlendStateCreateInfo const *ptr() const { return reinterpret_cast<VkPipelineColorBlendStateCreateInfo const *>(this); }
};

struct safe_VkPipelineDynamicStateCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkPipelineDynamicStateCreateFlags flags;
    uint32_t dynamicStateCount;
    const VkDynamicState* pDynamicStates;
    safe_VkPipelineDynamicStateCreateInfo(const VkPipelineDynamicStateCreateInfo* pInStruct);
    safe_VkPipelineDynamicStateCreateInfo(const safe_VkPipelineDynamicStateCreateInfo& src);
    safe_VkPipelineDynamicStateCreateInfo();
    ~safe_VkPipelineDynamicStateCreateInfo();
    void initialize(const VkPipelineDynamicStateCreateInfo* pInStruct);
    void initialize(const safe_VkPipelineDynamicStateCreateInfo* src);
    VkPipelineDynamicStateCreateInfo *ptr() { return reinterpret_cast<VkPipelineDynamicStateCreateInfo *>(this); }
    VkPipelineDynamicStateCreateInfo const *ptr() const { return reinterpret_cast<VkPipelineDynamicStateCreateInfo const *>(this); }
};

struct safe_VkGraphicsPipelineCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkPipelineCreateFlags flags;
    uint32_t stageCount;
    safe_VkPipelineShaderStageCreateInfo* pStages;
    safe_VkPipelineVertexInputStateCreateInfo* pVertexInputState;
    safe_VkPipelineInputAssemblyStateCreateInfo* pInputAssemblyState;
    safe_VkPipelineTessellationStateCreateInfo* pTessellationState;
    safe_VkPipelineViewportStateCreateInfo* pViewportState;
    safe_VkPipelineRasterizationStateCreateInfo* pRasterizationState;
    safe_VkPipelineMultisampleStateCreateInfo* pMultisampleState;
    safe_VkPipelineDepthStencilStateCreateInfo* pDepthStencilState;
    safe_VkPipelineColorBlendStateCreateInfo* pColorBlendState;
    safe_VkPipelineDynamicStateCreateInfo* pDynamicState;
    VkPipelineLayout layout;
    VkRenderPass renderPass;
    uint32_t subpass;
    VkPipeline basePipelineHandle;
    int32_t basePipelineIndex;
    safe_VkGraphicsPipelineCreateInfo(const VkGraphicsPipelineCreateInfo* pInStruct);
    safe_VkGraphicsPipelineCreateInfo(const safe_VkGraphicsPipelineCreateInfo& src);
    safe_VkGraphicsPipelineCreateInfo();
    ~safe_VkGraphicsPipelineCreateInfo();
    void initialize(const VkGraphicsPipelineCreateInfo* pInStruct);
    void initialize(const safe_VkGraphicsPipelineCreateInfo* src);
    VkGraphicsPipelineCreateInfo *ptr() { return reinterpret_cast<VkGraphicsPipelineCreateInfo *>(this); }
    VkGraphicsPipelineCreateInfo const *ptr() const { return reinterpret_cast<VkGraphicsPipelineCreateInfo const *>(this); }
};

struct safe_VkComputePipelineCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkPipelineCreateFlags flags;
    safe_VkPipelineShaderStageCreateInfo stage;
    VkPipelineLayout layout;
    VkPipeline basePipelineHandle;
    int32_t basePipelineIndex;
    safe_VkComputePipelineCreateInfo(const VkComputePipelineCreateInfo* pInStruct);
    safe_VkComputePipelineCreateInfo(const safe_VkComputePipelineCreateInfo& src);
    safe_VkComputePipelineCreateInfo();
    ~safe_VkComputePipelineCreateInfo();
    void initialize(const VkComputePipelineCreateInfo* pInStruct);
    void initialize(const safe_VkComputePipelineCreateInfo* src);
    VkComputePipelineCreateInfo *ptr() { return reinterpret_cast<VkComputePipelineCreateInfo *>(this); }
    VkComputePipelineCreateInfo const *ptr() const { return reinterpret_cast<VkComputePipelineCreateInfo const *>(this); }
};

struct safe_VkPipelineLayoutCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkPipelineLayoutCreateFlags flags;
    uint32_t setLayoutCount;
    VkDescriptorSetLayout* pSetLayouts;
    uint32_t pushConstantRangeCount;
    const VkPushConstantRange* pPushConstantRanges;
    safe_VkPipelineLayoutCreateInfo(const VkPipelineLayoutCreateInfo* pInStruct);
    safe_VkPipelineLayoutCreateInfo(const safe_VkPipelineLayoutCreateInfo& src);
    safe_VkPipelineLayoutCreateInfo();
    ~safe_VkPipelineLayoutCreateInfo();
    void initialize(const VkPipelineLayoutCreateInfo* pInStruct);
    void initialize(const safe_VkPipelineLayoutCreateInfo* src);
    VkPipelineLayoutCreateInfo *ptr() { return reinterpret_cast<VkPipelineLayoutCreateInfo *>(this); }
    VkPipelineLayoutCreateInfo const *ptr() const { return reinterpret_cast<VkPipelineLayoutCreateInfo const *>(this); }
};

struct safe_VkSamplerCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkSamplerCreateFlags flags;
    VkFilter magFilter;
    VkFilter minFilter;
    VkSamplerMipmapMode mipmapMode;
    VkSamplerAddressMode addressModeU;
    VkSamplerAddressMode addressModeV;
    VkSamplerAddressMode addressModeW;
    float mipLodBias;
    VkBool32 anisotropyEnable;
    float maxAnisotropy;
    VkBool32 compareEnable;
    VkCompareOp compareOp;
    float minLod;
    float maxLod;
    VkBorderColor borderColor;
    VkBool32 unnormalizedCoordinates;
    safe_VkSamplerCreateInfo(const VkSamplerCreateInfo* pInStruct);
    safe_VkSamplerCreateInfo(const safe_VkSamplerCreateInfo& src);
    safe_VkSamplerCreateInfo();
    ~safe_VkSamplerCreateInfo();
    void initialize(const VkSamplerCreateInfo* pInStruct);
    void initialize(const safe_VkSamplerCreateInfo* src);
    VkSamplerCreateInfo *ptr() { return reinterpret_cast<VkSamplerCreateInfo *>(this); }
    VkSamplerCreateInfo const *ptr() const { return reinterpret_cast<VkSamplerCreateInfo const *>(this); }
};

struct safe_VkDescriptorSetLayoutBinding {
    uint32_t binding;
    VkDescriptorType descriptorType;
    uint32_t descriptorCount;
    VkShaderStageFlags stageFlags;
    VkSampler* pImmutableSamplers;
    safe_VkDescriptorSetLayoutBinding(const VkDescriptorSetLayoutBinding* pInStruct);
    safe_VkDescriptorSetLayoutBinding(const safe_VkDescriptorSetLayoutBinding& src);
    safe_VkDescriptorSetLayoutBinding();
    ~safe_VkDescriptorSetLayoutBinding();
    void initialize(const VkDescriptorSetLayoutBinding* pInStruct);
    void initialize(const safe_VkDescriptorSetLayoutBinding* src);
    VkDescriptorSetLayoutBinding *ptr() { return reinterpret_cast<VkDescriptorSetLayoutBinding *>(this); }
    VkDescriptorSetLayoutBinding const *ptr() const { return reinterpret_cast<VkDescriptorSetLayoutBinding const *>(this); }
};

struct safe_VkDescriptorSetLayoutCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkDescriptorSetLayoutCreateFlags flags;
    uint32_t bindingCount;
    safe_VkDescriptorSetLayoutBinding* pBindings;
    safe_VkDescriptorSetLayoutCreateInfo(const VkDescriptorSetLayoutCreateInfo* pInStruct);
    safe_VkDescriptorSetLayoutCreateInfo(const safe_VkDescriptorSetLayoutCreateInfo& src);
    safe_VkDescriptorSetLayoutCreateInfo();
    ~safe_VkDescriptorSetLayoutCreateInfo();
    void initialize(const VkDescriptorSetLayoutCreateInfo* pInStruct);
    void initialize(const safe_VkDescriptorSetLayoutCreateInfo* src);
    VkDescriptorSetLayoutCreateInfo *ptr() { return reinterpret_cast<VkDescriptorSetLayoutCreateInfo *>(this); }
    VkDescriptorSetLayoutCreateInfo const *ptr() const { return reinterpret_cast<VkDescriptorSetLayoutCreateInfo const *>(this); }
};

struct safe_VkDescriptorPoolCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkDescriptorPoolCreateFlags flags;
    uint32_t maxSets;
    uint32_t poolSizeCount;
    const VkDescriptorPoolSize* pPoolSizes;
    safe_VkDescriptorPoolCreateInfo(const VkDescriptorPoolCreateInfo* pInStruct);
    safe_VkDescriptorPoolCreateInfo(const safe_VkDescriptorPoolCreateInfo& src);
    safe_VkDescriptorPoolCreateInfo();
    ~safe_VkDescriptorPoolCreateInfo();
    void initialize(const VkDescriptorPoolCreateInfo* pInStruct);
    void initialize(const safe_VkDescriptorPoolCreateInfo* src);
    VkDescriptorPoolCreateInfo *ptr() { return reinterpret_cast<VkDescriptorPoolCreateInfo *>(this); }
    VkDescriptorPoolCreateInfo const *ptr() const { return reinterpret_cast<VkDescriptorPoolCreateInfo const *>(this); }
};

struct safe_VkDescriptorSetAllocateInfo {
    VkStructureType sType;
    const void* pNext;
    VkDescriptorPool descriptorPool;
    uint32_t descriptorSetCount;
    VkDescriptorSetLayout* pSetLayouts;
    safe_VkDescriptorSetAllocateInfo(const VkDescriptorSetAllocateInfo* pInStruct);
    safe_VkDescriptorSetAllocateInfo(const safe_VkDescriptorSetAllocateInfo& src);
    safe_VkDescriptorSetAllocateInfo();
    ~safe_VkDescriptorSetAllocateInfo();
    void initialize(const VkDescriptorSetAllocateInfo* pInStruct);
    void initialize(const safe_VkDescriptorSetAllocateInfo* src);
    VkDescriptorSetAllocateInfo *ptr() { return reinterpret_cast<VkDescriptorSetAllocateInfo *>(this); }
    VkDescriptorSetAllocateInfo const *ptr() const { return reinterpret_cast<VkDescriptorSetAllocateInfo const *>(this); }
};

struct safe_VkWriteDescriptorSet {
    VkStructureType sType;
    const void* pNext;
    VkDescriptorSet dstSet;
    uint32_t dstBinding;
    uint32_t dstArrayElement;
    uint32_t descriptorCount;
    VkDescriptorType descriptorType;
    VkDescriptorImageInfo* pImageInfo;
    VkDescriptorBufferInfo* pBufferInfo;
    VkBufferView* pTexelBufferView;
    safe_VkWriteDescriptorSet(const VkWriteDescriptorSet* pInStruct);
    safe_VkWriteDescriptorSet(const safe_VkWriteDescriptorSet& src);
    safe_VkWriteDescriptorSet();
    ~safe_VkWriteDescriptorSet();
    void initialize(const VkWriteDescriptorSet* pInStruct);
    void initialize(const safe_VkWriteDescriptorSet* src);
    VkWriteDescriptorSet *ptr() { return reinterpret_cast<VkWriteDescriptorSet *>(this); }
    VkWriteDescriptorSet const *ptr() const { return reinterpret_cast<VkWriteDescriptorSet const *>(this); }
};

struct safe_VkCopyDescriptorSet {
    VkStructureType sType;
    const void* pNext;
    VkDescriptorSet srcSet;
    uint32_t srcBinding;
    uint32_t srcArrayElement;
    VkDescriptorSet dstSet;
    uint32_t dstBinding;
    uint32_t dstArrayElement;
    uint32_t descriptorCount;
    safe_VkCopyDescriptorSet(const VkCopyDescriptorSet* pInStruct);
    safe_VkCopyDescriptorSet(const safe_VkCopyDescriptorSet& src);
    safe_VkCopyDescriptorSet();
    ~safe_VkCopyDescriptorSet();
    void initialize(const VkCopyDescriptorSet* pInStruct);
    void initialize(const safe_VkCopyDescriptorSet* src);
    VkCopyDescriptorSet *ptr() { return reinterpret_cast<VkCopyDescriptorSet *>(this); }
    VkCopyDescriptorSet const *ptr() const { return reinterpret_cast<VkCopyDescriptorSet const *>(this); }
};

struct safe_VkFramebufferCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkFramebufferCreateFlags flags;
    VkRenderPass renderPass;
    uint32_t attachmentCount;
    VkImageView* pAttachments;
    uint32_t width;
    uint32_t height;
    uint32_t layers;
    safe_VkFramebufferCreateInfo(const VkFramebufferCreateInfo* pInStruct);
    safe_VkFramebufferCreateInfo(const safe_VkFramebufferCreateInfo& src);
    safe_VkFramebufferCreateInfo();
    ~safe_VkFramebufferCreateInfo();
    void initialize(const VkFramebufferCreateInfo* pInStruct);
    void initialize(const safe_VkFramebufferCreateInfo* src);
    VkFramebufferCreateInfo *ptr() { return reinterpret_cast<VkFramebufferCreateInfo *>(this); }
    VkFramebufferCreateInfo const *ptr() const { return reinterpret_cast<VkFramebufferCreateInfo const *>(this); }
};

struct safe_VkSubpassDescription {
    VkSubpassDescriptionFlags flags;
    VkPipelineBindPoint pipelineBindPoint;
    uint32_t inputAttachmentCount;
    const VkAttachmentReference* pInputAttachments;
    uint32_t colorAttachmentCount;
    const VkAttachmentReference* pColorAttachments;
    const VkAttachmentReference* pResolveAttachments;
    const VkAttachmentReference* pDepthStencilAttachment;
    uint32_t preserveAttachmentCount;
    const uint32_t* pPreserveAttachments;
    safe_VkSubpassDescription(const VkSubpassDescription* pInStruct);
    safe_VkSubpassDescription(const safe_VkSubpassDescription& src);
    safe_VkSubpassDescription();
    ~safe_VkSubpassDescription();
    void initialize(const VkSubpassDescription* pInStruct);
    void initialize(const safe_VkSubpassDescription* src);
    VkSubpassDescription *ptr() { return reinterpret_cast<VkSubpassDescription *>(this); }
    VkSubpassDescription const *ptr() const { return reinterpret_cast<VkSubpassDescription const *>(this); }
};

struct safe_VkRenderPassCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkRenderPassCreateFlags flags;
    uint32_t attachmentCount;
    const VkAttachmentDescription* pAttachments;
    uint32_t subpassCount;
    safe_VkSubpassDescription* pSubpasses;
    uint32_t dependencyCount;
    const VkSubpassDependency* pDependencies;
    safe_VkRenderPassCreateInfo(const VkRenderPassCreateInfo* pInStruct);
    safe_VkRenderPassCreateInfo(const safe_VkRenderPassCreateInfo& src);
    safe_VkRenderPassCreateInfo();
    ~safe_VkRenderPassCreateInfo();
    void initialize(const VkRenderPassCreateInfo* pInStruct);
    void initialize(const safe_VkRenderPassCreateInfo* src);
    VkRenderPassCreateInfo *ptr() { return reinterpret_cast<VkRenderPassCreateInfo *>(this); }
    VkRenderPassCreateInfo const *ptr() const { return reinterpret_cast<VkRenderPassCreateInfo const *>(this); }
};

struct safe_VkCommandPoolCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkCommandPoolCreateFlags flags;
    uint32_t queueFamilyIndex;
    safe_VkCommandPoolCreateInfo(const VkCommandPoolCreateInfo* pInStruct);
    safe_VkCommandPoolCreateInfo(const safe_VkCommandPoolCreateInfo& src);
    safe_VkCommandPoolCreateInfo();
    ~safe_VkCommandPoolCreateInfo();
    void initialize(const VkCommandPoolCreateInfo* pInStruct);
    void initialize(const safe_VkCommandPoolCreateInfo* src);
    VkCommandPoolCreateInfo *ptr() { return reinterpret_cast<VkCommandPoolCreateInfo *>(this); }
    VkCommandPoolCreateInfo const *ptr() const { return reinterpret_cast<VkCommandPoolCreateInfo const *>(this); }
};

struct safe_VkCommandBufferAllocateInfo {
    VkStructureType sType;
    const void* pNext;
    VkCommandPool commandPool;
    VkCommandBufferLevel level;
    uint32_t commandBufferCount;
    safe_VkCommandBufferAllocateInfo(const VkCommandBufferAllocateInfo* pInStruct);
    safe_VkCommandBufferAllocateInfo(const safe_VkCommandBufferAllocateInfo& src);
    safe_VkCommandBufferAllocateInfo();
    ~safe_VkCommandBufferAllocateInfo();
    void initialize(const VkCommandBufferAllocateInfo* pInStruct);
    void initialize(const safe_VkCommandBufferAllocateInfo* src);
    VkCommandBufferAllocateInfo *ptr() { return reinterpret_cast<VkCommandBufferAllocateInfo *>(this); }
    VkCommandBufferAllocateInfo const *ptr() const { return reinterpret_cast<VkCommandBufferAllocateInfo const *>(this); }
};

struct safe_VkCommandBufferInheritanceInfo {
    VkStructureType sType;
    const void* pNext;
    VkRenderPass renderPass;
    uint32_t subpass;
    VkFramebuffer framebuffer;
    VkBool32 occlusionQueryEnable;
    VkQueryControlFlags queryFlags;
    VkQueryPipelineStatisticFlags pipelineStatistics;
    safe_VkCommandBufferInheritanceInfo(const VkCommandBufferInheritanceInfo* pInStruct);
    safe_VkCommandBufferInheritanceInfo(const safe_VkCommandBufferInheritanceInfo& src);
    safe_VkCommandBufferInheritanceInfo();
    ~safe_VkCommandBufferInheritanceInfo();
    void initialize(const VkCommandBufferInheritanceInfo* pInStruct);
    void initialize(const safe_VkCommandBufferInheritanceInfo* src);
    VkCommandBufferInheritanceInfo *ptr() { return reinterpret_cast<VkCommandBufferInheritanceInfo *>(this); }
    VkCommandBufferInheritanceInfo const *ptr() const { return reinterpret_cast<VkCommandBufferInheritanceInfo const *>(this); }
};

struct safe_VkCommandBufferBeginInfo {
    VkStructureType sType;
    const void* pNext;
    VkCommandBufferUsageFlags flags;
    safe_VkCommandBufferInheritanceInfo* pInheritanceInfo;
    safe_VkCommandBufferBeginInfo(const VkCommandBufferBeginInfo* pInStruct);
    safe_VkCommandBufferBeginInfo(const safe_VkCommandBufferBeginInfo& src);
    safe_VkCommandBufferBeginInfo();
    ~safe_VkCommandBufferBeginInfo();
    void initialize(const VkCommandBufferBeginInfo* pInStruct);
    void initialize(const safe_VkCommandBufferBeginInfo* src);
    VkCommandBufferBeginInfo *ptr() { return reinterpret_cast<VkCommandBufferBeginInfo *>(this); }
    VkCommandBufferBeginInfo const *ptr() const { return reinterpret_cast<VkCommandBufferBeginInfo const *>(this); }
};

struct safe_VkMemoryBarrier {
    VkStructureType sType;
    const void* pNext;
    VkAccessFlags srcAccessMask;
    VkAccessFlags dstAccessMask;
    safe_VkMemoryBarrier(const VkMemoryBarrier* pInStruct);
    safe_VkMemoryBarrier(const safe_VkMemoryBarrier& src);
    safe_VkMemoryBarrier();
    ~safe_VkMemoryBarrier();
    void initialize(const VkMemoryBarrier* pInStruct);
    void initialize(const safe_VkMemoryBarrier* src);
    VkMemoryBarrier *ptr() { return reinterpret_cast<VkMemoryBarrier *>(this); }
    VkMemoryBarrier const *ptr() const { return reinterpret_cast<VkMemoryBarrier const *>(this); }
};

struct safe_VkBufferMemoryBarrier {
    VkStructureType sType;
    const void* pNext;
    VkAccessFlags srcAccessMask;
    VkAccessFlags dstAccessMask;
    uint32_t srcQueueFamilyIndex;
    uint32_t dstQueueFamilyIndex;
    VkBuffer buffer;
    VkDeviceSize offset;
    VkDeviceSize size;
    safe_VkBufferMemoryBarrier(const VkBufferMemoryBarrier* pInStruct);
    safe_VkBufferMemoryBarrier(const safe_VkBufferMemoryBarrier& src);
    safe_VkBufferMemoryBarrier();
    ~safe_VkBufferMemoryBarrier();
    void initialize(const VkBufferMemoryBarrier* pInStruct);
    void initialize(const safe_VkBufferMemoryBarrier* src);
    VkBufferMemoryBarrier *ptr() { return reinterpret_cast<VkBufferMemoryBarrier *>(this); }
    VkBufferMemoryBarrier const *ptr() const { return reinterpret_cast<VkBufferMemoryBarrier const *>(this); }
};

struct safe_VkImageMemoryBarrier {
    VkStructureType sType;
    const void* pNext;
    VkAccessFlags srcAccessMask;
    VkAccessFlags dstAccessMask;
    VkImageLayout oldLayout;
    VkImageLayout newLayout;
    uint32_t srcQueueFamilyIndex;
    uint32_t dstQueueFamilyIndex;
    VkImage image;
    VkImageSubresourceRange subresourceRange;
    safe_VkImageMemoryBarrier(const VkImageMemoryBarrier* pInStruct);
    safe_VkImageMemoryBarrier(const safe_VkImageMemoryBarrier& src);
    safe_VkImageMemoryBarrier();
    ~safe_VkImageMemoryBarrier();
    void initialize(const VkImageMemoryBarrier* pInStruct);
    void initialize(const safe_VkImageMemoryBarrier* src);
    VkImageMemoryBarrier *ptr() { return reinterpret_cast<VkImageMemoryBarrier *>(this); }
    VkImageMemoryBarrier const *ptr() const { return reinterpret_cast<VkImageMemoryBarrier const *>(this); }
};

struct safe_VkRenderPassBeginInfo {
    VkStructureType sType;
    const void* pNext;
    VkRenderPass renderPass;
    VkFramebuffer framebuffer;
    VkRect2D renderArea;
    uint32_t clearValueCount;
    const VkClearValue* pClearValues;
    safe_VkRenderPassBeginInfo(const VkRenderPassBeginInfo* pInStruct);
    safe_VkRenderPassBeginInfo(const safe_VkRenderPassBeginInfo& src);
    safe_VkRenderPassBeginInfo();
    ~safe_VkRenderPassBeginInfo();
    void initialize(const VkRenderPassBeginInfo* pInStruct);
    void initialize(const safe_VkRenderPassBeginInfo* src);
    VkRenderPassBeginInfo *ptr() { return reinterpret_cast<VkRenderPassBeginInfo *>(this); }
    VkRenderPassBeginInfo const *ptr() const { return reinterpret_cast<VkRenderPassBeginInfo const *>(this); }
};

struct safe_VkSwapchainCreateInfoKHR {
    VkStructureType sType;
    const void* pNext;
    VkSwapchainCreateFlagsKHR flags;
    VkSurfaceKHR surface;
    uint32_t minImageCount;
    VkFormat imageFormat;
    VkColorSpaceKHR imageColorSpace;
    VkExtent2D imageExtent;
    uint32_t imageArrayLayers;
    VkImageUsageFlags imageUsage;
    VkSharingMode imageSharingMode;
    uint32_t queueFamilyIndexCount;
    const uint32_t* pQueueFamilyIndices;
    VkSurfaceTransformFlagBitsKHR preTransform;
    VkCompositeAlphaFlagBitsKHR compositeAlpha;
    VkPresentModeKHR presentMode;
    VkBool32 clipped;
    VkSwapchainKHR oldSwapchain;
    safe_VkSwapchainCreateInfoKHR(const VkSwapchainCreateInfoKHR* pInStruct);
    safe_VkSwapchainCreateInfoKHR(const safe_VkSwapchainCreateInfoKHR& src);
    safe_VkSwapchainCreateInfoKHR();
    ~safe_VkSwapchainCreateInfoKHR();
    void initialize(const VkSwapchainCreateInfoKHR* pInStruct);
    void initialize(const safe_VkSwapchainCreateInfoKHR* src);
    VkSwapchainCreateInfoKHR *ptr() { return reinterpret_cast<VkSwapchainCreateInfoKHR *>(this); }
    VkSwapchainCreateInfoKHR const *ptr() const { return reinterpret_cast<VkSwapchainCreateInfoKHR const *>(this); }
};

struct safe_VkPresentInfoKHR {
    VkStructureType sType;
    const void* pNext;
    uint32_t waitSemaphoreCount;
    VkSemaphore* pWaitSemaphores;
    uint32_t swapchainCount;
    VkSwapchainKHR* pSwapchains;
    const uint32_t* pImageIndices;
    VkResult* pResults;
    safe_VkPresentInfoKHR(const VkPresentInfoKHR* pInStruct);
    safe_VkPresentInfoKHR(const safe_VkPresentInfoKHR& src);
    safe_VkPresentInfoKHR();
    ~safe_VkPresentInfoKHR();
    void initialize(const VkPresentInfoKHR* pInStruct);
    void initialize(const safe_VkPresentInfoKHR* src);
    VkPresentInfoKHR *ptr() { return reinterpret_cast<VkPresentInfoKHR *>(this); }
    VkPresentInfoKHR const *ptr() const { return reinterpret_cast<VkPresentInfoKHR const *>(this); }
};

struct safe_VkDisplayPropertiesKHR {
    VkDisplayKHR display;
    const char* displayName;
    VkExtent2D physicalDimensions;
    VkExtent2D physicalResolution;
    VkSurfaceTransformFlagsKHR supportedTransforms;
    VkBool32 planeReorderPossible;
    VkBool32 persistentContent;
    safe_VkDisplayPropertiesKHR(const VkDisplayPropertiesKHR* pInStruct);
    safe_VkDisplayPropertiesKHR(const safe_VkDisplayPropertiesKHR& src);
    safe_VkDisplayPropertiesKHR();
    ~safe_VkDisplayPropertiesKHR();
    void initialize(const VkDisplayPropertiesKHR* pInStruct);
    void initialize(const safe_VkDisplayPropertiesKHR* src);
    VkDisplayPropertiesKHR *ptr() { return reinterpret_cast<VkDisplayPropertiesKHR *>(this); }
    VkDisplayPropertiesKHR const *ptr() const { return reinterpret_cast<VkDisplayPropertiesKHR const *>(this); }
};

struct safe_VkDisplayModePropertiesKHR {
    VkDisplayModeKHR displayMode;
    VkDisplayModeParametersKHR parameters;
    safe_VkDisplayModePropertiesKHR(const VkDisplayModePropertiesKHR* pInStruct);
    safe_VkDisplayModePropertiesKHR(const safe_VkDisplayModePropertiesKHR& src);
    safe_VkDisplayModePropertiesKHR();
    ~safe_VkDisplayModePropertiesKHR();
    void initialize(const VkDisplayModePropertiesKHR* pInStruct);
    void initialize(const safe_VkDisplayModePropertiesKHR* src);
    VkDisplayModePropertiesKHR *ptr() { return reinterpret_cast<VkDisplayModePropertiesKHR *>(this); }
    VkDisplayModePropertiesKHR const *ptr() const { return reinterpret_cast<VkDisplayModePropertiesKHR const *>(this); }
};

struct safe_VkDisplayModeCreateInfoKHR {
    VkStructureType sType;
    const void* pNext;
    VkDisplayModeCreateFlagsKHR flags;
    VkDisplayModeParametersKHR parameters;
    safe_VkDisplayModeCreateInfoKHR(const VkDisplayModeCreateInfoKHR* pInStruct);
    safe_VkDisplayModeCreateInfoKHR(const safe_VkDisplayModeCreateInfoKHR& src);
    safe_VkDisplayModeCreateInfoKHR();
    ~safe_VkDisplayModeCreateInfoKHR();
    void initialize(const VkDisplayModeCreateInfoKHR* pInStruct);
    void initialize(const safe_VkDisplayModeCreateInfoKHR* src);
    VkDisplayModeCreateInfoKHR *ptr() { return reinterpret_cast<VkDisplayModeCreateInfoKHR *>(this); }
    VkDisplayModeCreateInfoKHR const *ptr() const { return reinterpret_cast<VkDisplayModeCreateInfoKHR const *>(this); }
};

struct safe_VkDisplayPlanePropertiesKHR {
    VkDisplayKHR currentDisplay;
    uint32_t currentStackIndex;
    safe_VkDisplayPlanePropertiesKHR(const VkDisplayPlanePropertiesKHR* pInStruct);
    safe_VkDisplayPlanePropertiesKHR(const safe_VkDisplayPlanePropertiesKHR& src);
    safe_VkDisplayPlanePropertiesKHR();
    ~safe_VkDisplayPlanePropertiesKHR();
    void initialize(const VkDisplayPlanePropertiesKHR* pInStruct);
    void initialize(const safe_VkDisplayPlanePropertiesKHR* src);
    VkDisplayPlanePropertiesKHR *ptr() { return reinterpret_cast<VkDisplayPlanePropertiesKHR *>(this); }
    VkDisplayPlanePropertiesKHR const *ptr() const { return reinterpret_cast<VkDisplayPlanePropertiesKHR const *>(this); }
};

struct safe_VkDisplaySurfaceCreateInfoKHR {
    VkStructureType sType;
    const void* pNext;
    VkDisplaySurfaceCreateFlagsKHR flags;
    VkDisplayModeKHR displayMode;
    uint32_t planeIndex;
    uint32_t planeStackIndex;
    VkSurfaceTransformFlagBitsKHR transform;
    float globalAlpha;
    VkDisplayPlaneAlphaFlagBitsKHR alphaMode;
    VkExtent2D imageExtent;
    safe_VkDisplaySurfaceCreateInfoKHR(const VkDisplaySurfaceCreateInfoKHR* pInStruct);
    safe_VkDisplaySurfaceCreateInfoKHR(const safe_VkDisplaySurfaceCreateInfoKHR& src);
    safe_VkDisplaySurfaceCreateInfoKHR();
    ~safe_VkDisplaySurfaceCreateInfoKHR();
    void initialize(const VkDisplaySurfaceCreateInfoKHR* pInStruct);
    void initialize(const safe_VkDisplaySurfaceCreateInfoKHR* src);
    VkDisplaySurfaceCreateInfoKHR *ptr() { return reinterpret_cast<VkDisplaySurfaceCreateInfoKHR *>(this); }
    VkDisplaySurfaceCreateInfoKHR const *ptr() const { return reinterpret_cast<VkDisplaySurfaceCreateInfoKHR const *>(this); }
};

struct safe_VkDisplayPresentInfoKHR {
    VkStructureType sType;
    const void* pNext;
    VkRect2D srcRect;
    VkRect2D dstRect;
    VkBool32 persistent;
    safe_VkDisplayPresentInfoKHR(const VkDisplayPresentInfoKHR* pInStruct);
    safe_VkDisplayPresentInfoKHR(const safe_VkDisplayPresentInfoKHR& src);
    safe_VkDisplayPresentInfoKHR();
    ~safe_VkDisplayPresentInfoKHR();
    void initialize(const VkDisplayPresentInfoKHR* pInStruct);
    void initialize(const safe_VkDisplayPresentInfoKHR* src);
    VkDisplayPresentInfoKHR *ptr() { return reinterpret_cast<VkDisplayPresentInfoKHR *>(this); }
    VkDisplayPresentInfoKHR const *ptr() const { return reinterpret_cast<VkDisplayPresentInfoKHR const *>(this); }
};
#ifdef VK_USE_PLATFORM_XLIB_KHR

struct safe_VkXlibSurfaceCreateInfoKHR {
    VkStructureType sType;
    const void* pNext;
    VkXlibSurfaceCreateFlagsKHR flags;
    Display* dpy;
    Window window;
    safe_VkXlibSurfaceCreateInfoKHR(const VkXlibSurfaceCreateInfoKHR* pInStruct);
    safe_VkXlibSurfaceCreateInfoKHR(const safe_VkXlibSurfaceCreateInfoKHR& src);
    safe_VkXlibSurfaceCreateInfoKHR();
    ~safe_VkXlibSurfaceCreateInfoKHR();
    void initialize(const VkXlibSurfaceCreateInfoKHR* pInStruct);
    void initialize(const safe_VkXlibSurfaceCreateInfoKHR* src);
    VkXlibSurfaceCreateInfoKHR *ptr() { return reinterpret_cast<VkXlibSurfaceCreateInfoKHR *>(this); }
    VkXlibSurfaceCreateInfoKHR const *ptr() const { return reinterpret_cast<VkXlibSurfaceCreateInfoKHR const *>(this); }
};
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR

struct safe_VkXcbSurfaceCreateInfoKHR {
    VkStructureType sType;
    const void* pNext;
    VkXcbSurfaceCreateFlagsKHR flags;
    xcb_connection_t* connection;
    xcb_window_t window;
    safe_VkXcbSurfaceCreateInfoKHR(const VkXcbSurfaceCreateInfoKHR* pInStruct);
    safe_VkXcbSurfaceCreateInfoKHR(const safe_VkXcbSurfaceCreateInfoKHR& src);
    safe_VkXcbSurfaceCreateInfoKHR();
    ~safe_VkXcbSurfaceCreateInfoKHR();
    void initialize(const VkXcbSurfaceCreateInfoKHR* pInStruct);
    void initialize(const safe_VkXcbSurfaceCreateInfoKHR* src);
    VkXcbSurfaceCreateInfoKHR *ptr() { return reinterpret_cast<VkXcbSurfaceCreateInfoKHR *>(this); }
    VkXcbSurfaceCreateInfoKHR const *ptr() const { return reinterpret_cast<VkXcbSurfaceCreateInfoKHR const *>(this); }
};
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR

struct safe_VkWaylandSurfaceCreateInfoKHR {
    VkStructureType sType;
    const void* pNext;
    VkWaylandSurfaceCreateFlagsKHR flags;
    struct wl_display* display;
    struct wl_surface* surface;
    safe_VkWaylandSurfaceCreateInfoKHR(const VkWaylandSurfaceCreateInfoKHR* pInStruct);
    safe_VkWaylandSurfaceCreateInfoKHR(const safe_VkWaylandSurfaceCreateInfoKHR& src);
    safe_VkWaylandSurfaceCreateInfoKHR();
    ~safe_VkWaylandSurfaceCreateInfoKHR();
    void initialize(const VkWaylandSurfaceCreateInfoKHR* pInStruct);
    void initialize(const safe_VkWaylandSurfaceCreateInfoKHR* src);
    VkWaylandSurfaceCreateInfoKHR *ptr() { return reinterpret_cast<VkWaylandSurfaceCreateInfoKHR *>(this); }
    VkWaylandSurfaceCreateInfoKHR const *ptr() const { return reinterpret_cast<VkWaylandSurfaceCreateInfoKHR const *>(this); }
};
#endif
#ifdef VK_USE_PLATFORM_MIR_KHR

struct safe_VkMirSurfaceCreateInfoKHR {
    VkStructureType sType;
    const void* pNext;
    VkMirSurfaceCreateFlagsKHR flags;
    MirConnection* connection;
    MirSurface* mirSurface;
    safe_VkMirSurfaceCreateInfoKHR(const VkMirSurfaceCreateInfoKHR* pInStruct);
    safe_VkMirSurfaceCreateInfoKHR(const safe_VkMirSurfaceCreateInfoKHR& src);
    safe_VkMirSurfaceCreateInfoKHR();
    ~safe_VkMirSurfaceCreateInfoKHR();
    void initialize(const VkMirSurfaceCreateInfoKHR* pInStruct);
    void initialize(const safe_VkMirSurfaceCreateInfoKHR* src);
    VkMirSurfaceCreateInfoKHR *ptr() { return reinterpret_cast<VkMirSurfaceCreateInfoKHR *>(this); }
    VkMirSurfaceCreateInfoKHR const *ptr() const { return reinterpret_cast<VkMirSurfaceCreateInfoKHR const *>(this); }
};
#endif
#ifdef VK_USE_PLATFORM_ANDROID_KHR

struct safe_VkAndroidSurfaceCreateInfoKHR {
    VkStructureType sType;
    const void* pNext;
    VkAndroidSurfaceCreateFlagsKHR flags;
    ANativeWindow* window;
    safe_VkAndroidSurfaceCreateInfoKHR(const VkAndroidSurfaceCreateInfoKHR* pInStruct);
    safe_VkAndroidSurfaceCreateInfoKHR(const safe_VkAndroidSurfaceCreateInfoKHR& src);
    safe_VkAndroidSurfaceCreateInfoKHR();
    ~safe_VkAndroidSurfaceCreateInfoKHR();
    void initialize(const VkAndroidSurfaceCreateInfoKHR* pInStruct);
    void initialize(const safe_VkAndroidSurfaceCreateInfoKHR* src);
    VkAndroidSurfaceCreateInfoKHR *ptr() { return reinterpret_cast<VkAndroidSurfaceCreateInfoKHR *>(this); }
    VkAndroidSurfaceCreateInfoKHR const *ptr() const { return reinterpret_cast<VkAndroidSurfaceCreateInfoKHR const *>(this); }
};
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR

struct safe_VkWin32SurfaceCreateInfoKHR {
    VkStructureType sType;
    const void* pNext;
    VkWin32SurfaceCreateFlagsKHR flags;
    HINSTANCE hinstance;
    HWND hwnd;
    safe_VkWin32SurfaceCreateInfoKHR(const VkWin32SurfaceCreateInfoKHR* pInStruct);
    safe_VkWin32SurfaceCreateInfoKHR(const safe_VkWin32SurfaceCreateInfoKHR& src);
    safe_VkWin32SurfaceCreateInfoKHR();
    ~safe_VkWin32SurfaceCreateInfoKHR();
    void initialize(const VkWin32SurfaceCreateInfoKHR* pInStruct);
    void initialize(const safe_VkWin32SurfaceCreateInfoKHR* src);
    VkWin32SurfaceCreateInfoKHR *ptr() { return reinterpret_cast<VkWin32SurfaceCreateInfoKHR *>(this); }
    VkWin32SurfaceCreateInfoKHR const *ptr() const { return reinterpret_cast<VkWin32SurfaceCreateInfoKHR const *>(this); }
};
#endif

struct safe_VkDebugReportCallbackCreateInfoEXT {
    VkStructureType sType;
    const void* pNext;
    VkDebugReportFlagsEXT flags;
    PFN_vkDebugReportCallbackEXT pfnCallback;
    void* pUserData;
    safe_VkDebugReportCallbackCreateInfoEXT(const VkDebugReportCallbackCreateInfoEXT* pInStruct);
    safe_VkDebugReportCallbackCreateInfoEXT(const safe_VkDebugReportCallbackCreateInfoEXT& src);
    safe_VkDebugReportCallbackCreateInfoEXT();
    ~safe_VkDebugReportCallbackCreateInfoEXT();
    void initialize(const VkDebugReportCallbackCreateInfoEXT* pInStruct);
    void initialize(const safe_VkDebugReportCallbackCreateInfoEXT* src);
    VkDebugReportCallbackCreateInfoEXT *ptr() { return reinterpret_cast<VkDebugReportCallbackCreateInfoEXT *>(this); }
    VkDebugReportCallbackCreateInfoEXT const *ptr() const { return reinterpret_cast<VkDebugReportCallbackCreateInfoEXT const *>(this); }
};

struct safe_VkPipelineRasterizationStateRasterizationOrderAMD {
    VkStructureType sType;
    const void* pNext;
    VkRasterizationOrderAMD rasterizationOrder;
    safe_VkPipelineRasterizationStateRasterizationOrderAMD(const VkPipelineRasterizationStateRasterizationOrderAMD* pInStruct);
    safe_VkPipelineRasterizationStateRasterizationOrderAMD(const safe_VkPipelineRasterizationStateRasterizationOrderAMD& src);
    safe_VkPipelineRasterizationStateRasterizationOrderAMD();
    ~safe_VkPipelineRasterizationStateRasterizationOrderAMD();
    void initialize(const VkPipelineRasterizationStateRasterizationOrderAMD* pInStruct);
    void initialize(const safe_VkPipelineRasterizationStateRasterizationOrderAMD* src);
    VkPipelineRasterizationStateRasterizationOrderAMD *ptr() { return reinterpret_cast<VkPipelineRasterizationStateRasterizationOrderAMD *>(this); }
    VkPipelineRasterizationStateRasterizationOrderAMD const *ptr() const { return reinterpret_cast<VkPipelineRasterizationStateRasterizationOrderAMD const *>(this); }
};

struct safe_VkDebugMarkerObjectNameInfoEXT {
    VkStructureType sType;
    const void* pNext;
    VkDebugReportObjectTypeEXT objectType;
    uint64_t object;
    const char* pObjectName;
    safe_VkDebugMarkerObjectNameInfoEXT(const VkDebugMarkerObjectNameInfoEXT* pInStruct);
    safe_VkDebugMarkerObjectNameInfoEXT(const safe_VkDebugMarkerObjectNameInfoEXT& src);
    safe_VkDebugMarkerObjectNameInfoEXT();
    ~safe_VkDebugMarkerObjectNameInfoEXT();
    void initialize(const VkDebugMarkerObjectNameInfoEXT* pInStruct);
    void initialize(const safe_VkDebugMarkerObjectNameInfoEXT* src);
    VkDebugMarkerObjectNameInfoEXT *ptr() { return reinterpret_cast<VkDebugMarkerObjectNameInfoEXT *>(this); }
    VkDebugMarkerObjectNameInfoEXT const *ptr() const { return reinterpret_cast<VkDebugMarkerObjectNameInfoEXT const *>(this); }
};

struct safe_VkDebugMarkerObjectTagInfoEXT {
    VkStructureType sType;
    const void* pNext;
    VkDebugReportObjectTypeEXT objectType;
    uint64_t object;
    uint64_t tagName;
    size_t tagSize;
    const void* pTag;
    safe_VkDebugMarkerObjectTagInfoEXT(const VkDebugMarkerObjectTagInfoEXT* pInStruct);
    safe_VkDebugMarkerObjectTagInfoEXT(const safe_VkDebugMarkerObjectTagInfoEXT& src);
    safe_VkDebugMarkerObjectTagInfoEXT();
    ~safe_VkDebugMarkerObjectTagInfoEXT();
    void initialize(const VkDebugMarkerObjectTagInfoEXT* pInStruct);
    void initialize(const safe_VkDebugMarkerObjectTagInfoEXT* src);
    VkDebugMarkerObjectTagInfoEXT *ptr() { return reinterpret_cast<VkDebugMarkerObjectTagInfoEXT *>(this); }
    VkDebugMarkerObjectTagInfoEXT const *ptr() const { return reinterpret_cast<VkDebugMarkerObjectTagInfoEXT const *>(this); }
};

struct safe_VkDebugMarkerMarkerInfoEXT {
    VkStructureType sType;
    const void* pNext;
    const char* pMarkerName;
    float color[4];
    safe_VkDebugMarkerMarkerInfoEXT(const VkDebugMarkerMarkerInfoEXT* pInStruct);
    safe_VkDebugMarkerMarkerInfoEXT(const safe_VkDebugMarkerMarkerInfoEXT& src);
    safe_VkDebugMarkerMarkerInfoEXT();
    ~safe_VkDebugMarkerMarkerInfoEXT();
    void initialize(const VkDebugMarkerMarkerInfoEXT* pInStruct);
    void initialize(const safe_VkDebugMarkerMarkerInfoEXT* src);
    VkDebugMarkerMarkerInfoEXT *ptr() { return reinterpret_cast<VkDebugMarkerMarkerInfoEXT *>(this); }
    VkDebugMarkerMarkerInfoEXT const *ptr() const { return reinterpret_cast<VkDebugMarkerMarkerInfoEXT const *>(this); }
};

struct safe_VkDedicatedAllocationImageCreateInfoNV {
    VkStructureType sType;
    const void* pNext;
    VkBool32 dedicatedAllocation;
    safe_VkDedicatedAllocationImageCreateInfoNV(const VkDedicatedAllocationImageCreateInfoNV* pInStruct);
    safe_VkDedicatedAllocationImageCreateInfoNV(const safe_VkDedicatedAllocationImageCreateInfoNV& src);
    safe_VkDedicatedAllocationImageCreateInfoNV();
    ~safe_VkDedicatedAllocationImageCreateInfoNV();
    void initialize(const VkDedicatedAllocationImageCreateInfoNV* pInStruct);
    void initialize(const safe_VkDedicatedAllocationImageCreateInfoNV* src);
    VkDedicatedAllocationImageCreateInfoNV *ptr() { return reinterpret_cast<VkDedicatedAllocationImageCreateInfoNV *>(this); }
    VkDedicatedAllocationImageCreateInfoNV const *ptr() const { return reinterpret_cast<VkDedicatedAllocationImageCreateInfoNV const *>(this); }
};

struct safe_VkDedicatedAllocationBufferCreateInfoNV {
    VkStructureType sType;
    const void* pNext;
    VkBool32 dedicatedAllocation;
    safe_VkDedicatedAllocationBufferCreateInfoNV(const VkDedicatedAllocationBufferCreateInfoNV* pInStruct);
    safe_VkDedicatedAllocationBufferCreateInfoNV(const safe_VkDedicatedAllocationBufferCreateInfoNV& src);
    safe_VkDedicatedAllocationBufferCreateInfoNV();
    ~safe_VkDedicatedAllocationBufferCreateInfoNV();
    void initialize(const VkDedicatedAllocationBufferCreateInfoNV* pInStruct);
    void initialize(const safe_VkDedicatedAllocationBufferCreateInfoNV* src);
    VkDedicatedAllocationBufferCreateInfoNV *ptr() { return reinterpret_cast<VkDedicatedAllocationBufferCreateInfoNV *>(this); }
    VkDedicatedAllocationBufferCreateInfoNV const *ptr() const { return reinterpret_cast<VkDedicatedAllocationBufferCreateInfoNV const *>(this); }
};

struct safe_VkDedicatedAllocationMemoryAllocateInfoNV {
    VkStructureType sType;
    const void* pNext;
    VkImage image;
    VkBuffer buffer;
    safe_VkDedicatedAllocationMemoryAllocateInfoNV(const VkDedicatedAllocationMemoryAllocateInfoNV* pInStruct);
    safe_VkDedicatedAllocationMemoryAllocateInfoNV(const safe_VkDedicatedAllocationMemoryAllocateInfoNV& src);
    safe_VkDedicatedAllocationMemoryAllocateInfoNV();
    ~safe_VkDedicatedAllocationMemoryAllocateInfoNV();
    void initialize(const VkDedicatedAllocationMemoryAllocateInfoNV* pInStruct);
    void initialize(const safe_VkDedicatedAllocationMemoryAllocateInfoNV* src);
    VkDedicatedAllocationMemoryAllocateInfoNV *ptr() { return reinterpret_cast<VkDedicatedAllocationMemoryAllocateInfoNV *>(this); }
    VkDedicatedAllocationMemoryAllocateInfoNV const *ptr() const { return reinterpret_cast<VkDedicatedAllocationMemoryAllocateInfoNV const *>(this); }
};

struct safe_VkExternalMemoryImageCreateInfoNV {
    VkStructureType sType;
    const void* pNext;
    VkExternalMemoryHandleTypeFlagsNV handleTypes;
    safe_VkExternalMemoryImageCreateInfoNV(const VkExternalMemoryImageCreateInfoNV* pInStruct);
    safe_VkExternalMemoryImageCreateInfoNV(const safe_VkExternalMemoryImageCreateInfoNV& src);
    safe_VkExternalMemoryImageCreateInfoNV();
    ~safe_VkExternalMemoryImageCreateInfoNV();
    void initialize(const VkExternalMemoryImageCreateInfoNV* pInStruct);
    void initialize(const safe_VkExternalMemoryImageCreateInfoNV* src);
    VkExternalMemoryImageCreateInfoNV *ptr() { return reinterpret_cast<VkExternalMemoryImageCreateInfoNV *>(this); }
    VkExternalMemoryImageCreateInfoNV const *ptr() const { return reinterpret_cast<VkExternalMemoryImageCreateInfoNV const *>(this); }
};

struct safe_VkExportMemoryAllocateInfoNV {
    VkStructureType sType;
    const void* pNext;
    VkExternalMemoryHandleTypeFlagsNV handleTypes;
    safe_VkExportMemoryAllocateInfoNV(const VkExportMemoryAllocateInfoNV* pInStruct);
    safe_VkExportMemoryAllocateInfoNV(const safe_VkExportMemoryAllocateInfoNV& src);
    safe_VkExportMemoryAllocateInfoNV();
    ~safe_VkExportMemoryAllocateInfoNV();
    void initialize(const VkExportMemoryAllocateInfoNV* pInStruct);
    void initialize(const safe_VkExportMemoryAllocateInfoNV* src);
    VkExportMemoryAllocateInfoNV *ptr() { return reinterpret_cast<VkExportMemoryAllocateInfoNV *>(this); }
    VkExportMemoryAllocateInfoNV const *ptr() const { return reinterpret_cast<VkExportMemoryAllocateInfoNV const *>(this); }
};
#ifdef VK_USE_PLATFORM_WIN32_KHR

struct safe_VkImportMemoryWin32HandleInfoNV {
    VkStructureType sType;
    const void* pNext;
    VkExternalMemoryHandleTypeFlagsNV handleType;
    HANDLE handle;
    safe_VkImportMemoryWin32HandleInfoNV(const VkImportMemoryWin32HandleInfoNV* pInStruct);
    safe_VkImportMemoryWin32HandleInfoNV(const safe_VkImportMemoryWin32HandleInfoNV& src);
    safe_VkImportMemoryWin32HandleInfoNV();
    ~safe_VkImportMemoryWin32HandleInfoNV();
    void initialize(const VkImportMemoryWin32HandleInfoNV* pInStruct);
    void initialize(const safe_VkImportMemoryWin32HandleInfoNV* src);
    VkImportMemoryWin32HandleInfoNV *ptr() { return reinterpret_cast<VkImportMemoryWin32HandleInfoNV *>(this); }
    VkImportMemoryWin32HandleInfoNV const *ptr() const { return reinterpret_cast<VkImportMemoryWin32HandleInfoNV const *>(this); }
};
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR

struct safe_VkExportMemoryWin32HandleInfoNV {
    VkStructureType sType;
    const void* pNext;
    const SECURITY_ATTRIBUTES* pAttributes;
    DWORD dwAccess;
    safe_VkExportMemoryWin32HandleInfoNV(const VkExportMemoryWin32HandleInfoNV* pInStruct);
    safe_VkExportMemoryWin32HandleInfoNV(const safe_VkExportMemoryWin32HandleInfoNV& src);
    safe_VkExportMemoryWin32HandleInfoNV();
    ~safe_VkExportMemoryWin32HandleInfoNV();
    void initialize(const VkExportMemoryWin32HandleInfoNV* pInStruct);
    void initialize(const safe_VkExportMemoryWin32HandleInfoNV* src);
    VkExportMemoryWin32HandleInfoNV *ptr() { return reinterpret_cast<VkExportMemoryWin32HandleInfoNV *>(this); }
    VkExportMemoryWin32HandleInfoNV const *ptr() const { return reinterpret_cast<VkExportMemoryWin32HandleInfoNV const *>(this); }
};
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR

struct safe_VkWin32KeyedMutexAcquireReleaseInfoNV {
    VkStructureType sType;
    const void* pNext;
    uint32_t acquireCount;
    VkDeviceMemory* pAcquireSyncs;
    const uint64_t* pAcquireKeys;
    const uint32_t* pAcquireTimeoutMilliseconds;
    uint32_t releaseCount;
    VkDeviceMemory* pReleaseSyncs;
    const uint64_t* pReleaseKeys;
    safe_VkWin32KeyedMutexAcquireReleaseInfoNV(const VkWin32KeyedMutexAcquireReleaseInfoNV* pInStruct);
    safe_VkWin32KeyedMutexAcquireReleaseInfoNV(const safe_VkWin32KeyedMutexAcquireReleaseInfoNV& src);
    safe_VkWin32KeyedMutexAcquireReleaseInfoNV();
    ~safe_VkWin32KeyedMutexAcquireReleaseInfoNV();
    void initialize(const VkWin32KeyedMutexAcquireReleaseInfoNV* pInStruct);
    void initialize(const safe_VkWin32KeyedMutexAcquireReleaseInfoNV* src);
    VkWin32KeyedMutexAcquireReleaseInfoNV *ptr() { return reinterpret_cast<VkWin32KeyedMutexAcquireReleaseInfoNV *>(this); }
    VkWin32KeyedMutexAcquireReleaseInfoNV const *ptr() const { return reinterpret_cast<VkWin32KeyedMutexAcquireReleaseInfoNV const *>(this); }
};
#endif

struct safe_VkValidationFlagsEXT {
    VkStructureType sType;
    const void* pNext;
    uint32_t disabledValidationCheckCount;
    VkValidationCheckEXT* pDisabledValidationChecks;
    safe_VkValidationFlagsEXT(const VkValidationFlagsEXT* pInStruct);
    safe_VkValidationFlagsEXT(const safe_VkValidationFlagsEXT& src);
    safe_VkValidationFlagsEXT();
    ~safe_VkValidationFlagsEXT();
    void initialize(const VkValidationFlagsEXT* pInStruct);
    void initialize(const safe_VkValidationFlagsEXT* src);
    VkValidationFlagsEXT *ptr() { return reinterpret_cast<VkValidationFlagsEXT *>(this); }
    VkValidationFlagsEXT const *ptr() const { return reinterpret_cast<VkValidationFlagsEXT const *>(this); }
};