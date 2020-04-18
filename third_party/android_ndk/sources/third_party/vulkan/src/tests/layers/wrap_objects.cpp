/*
 * Copyright (c) 2015-2016 Valve Corporation
 * Copyright (c) 2015-2016 LunarG, Inc.
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
 * Author: Jon Ashburn <jon@lunarg.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "vk_loader_platform.h"
#include "vulkan/vk_layer.h"
#include "vk_dispatch_table_helper.h"
#include "vk_layer_extension_utils.h"
#include "vk_layer_utils.h"
#include "wrap_objects.h"

namespace wrap_objects {

static const VkLayerProperties global_layer = {
    "VK_LAYER_LUNARG_wrap_objects", VK_LAYER_API_VERSION, 1, "LunarG Test Layer",
};

//TODO Add wrapping of Vkdevice, Vkqueue, VkcommandBuffer

VKAPI_ATTR VkResult VKAPI_CALL vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance)
{
    VkLayerInstanceCreateInfo *chain_info = get_chain_info(pCreateInfo, VK_LAYER_LINK_INFO);
    PFN_vkGetInstanceProcAddr fpGetInstanceProcAddr = chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    PFN_vkCreateInstance fpCreateInstance = (PFN_vkCreateInstance) fpGetInstanceProcAddr(NULL, "vkCreateInstance");
    if (fpCreateInstance == NULL) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    // Advance the link info for the next element on the chain
    chain_info->u.pLayerInfo = chain_info->u.pLayerInfo->pNext;
    VkResult result = fpCreateInstance(pCreateInfo, pAllocator, pInstance);
    if (result != VK_SUCCESS) {
        return result;
    }
    auto inst = new wrapped_inst_obj;
    if (!inst)
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    memset(inst, 0, sizeof(*inst));
    inst->obj = (*pInstance);
    *pInstance = reinterpret_cast<VkInstance> (inst);
    // store the loader callback for initializing created dispatchable objects
    chain_info = get_chain_info(pCreateInfo, VK_LOADER_DATA_CALLBACK);
    if (chain_info) {
        inst->pfn_inst_init = chain_info->u.pfnSetInstanceLoaderData;
        result = inst->pfn_inst_init(inst->obj, reinterpret_cast<void *> (inst));
        if (VK_SUCCESS != result)
            return result;
    } else {
        inst->pfn_inst_init = NULL;
        inst->loader_disp = *(reinterpret_cast<VkLayerInstanceDispatchTable **> (*pInstance));
    }
    layer_init_instance_dispatch_table(*pInstance, &inst->layer_disp, fpGetInstanceProcAddr);

    return result;
}


VKAPI_ATTR void VKAPI_CALL vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks* pAllocator)
{
    wrapped_inst_obj *inst;
    auto vk_inst = unwrap_instance(instance, &inst);
    VkLayerInstanceDispatchTable *pDisp  =  &inst->layer_disp;
    pDisp->DestroyInstance(vk_inst, pAllocator);
    if (inst->ptr_phys_devs)
        delete[] inst->ptr_phys_devs;
    delete inst;
}

VKAPI_ATTR VkResult VKAPI_CALL vkEnumeratePhysicalDevices(VkInstance instance, uint32_t* pPhysicalDeviceCount, VkPhysicalDevice* pPhysicalDevices)
{
    wrapped_inst_obj *inst;
    auto vk_inst = unwrap_instance(instance, &inst);
    VkResult result = inst->layer_disp.EnumeratePhysicalDevices(vk_inst, pPhysicalDeviceCount, pPhysicalDevices);

    if (VK_SUCCESS != result)
        return result;

    if (pPhysicalDevices != NULL) {
        assert(pPhysicalDeviceCount);
        auto phys_devs = new wrapped_phys_dev_obj[*pPhysicalDeviceCount];
        if (!phys_devs)
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        if (inst->ptr_phys_devs)
            delete[] inst->ptr_phys_devs;
        inst->ptr_phys_devs = phys_devs;
        for (uint32_t i = 0; i < *pPhysicalDeviceCount; i++) {
            if (inst->pfn_inst_init == NULL) {
                phys_devs[i].loader_disp = *(reinterpret_cast<VkLayerInstanceDispatchTable **> (pPhysicalDevices[i]));
            } else {
                result = inst->pfn_inst_init(vk_inst, reinterpret_cast<void *> (&phys_devs[i]));
                if (VK_SUCCESS != result)
                    return result;

            }
            phys_devs[i].obj = reinterpret_cast<void *> (pPhysicalDevices[i]);
            phys_devs[i].inst = inst;
            pPhysicalDevices[i] = reinterpret_cast<VkPhysicalDevice> (&phys_devs[i]);
        }
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFeatures(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures* pFeatures)
{
    wrapped_phys_dev_obj *phys_dev;
    auto vk_phys_dev = unwrap_phys_dev(physicalDevice, &phys_dev);
    phys_dev->inst->layer_disp.GetPhysicalDeviceFeatures(vk_phys_dev, pFeatures);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkFormatProperties* pFormatProperties)
{
    wrapped_phys_dev_obj *phys_dev;
    auto vk_phys_dev = unwrap_phys_dev(physicalDevice, &phys_dev);
    phys_dev->inst->layer_disp.GetPhysicalDeviceFormatProperties(vk_phys_dev, format, pFormatProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags, VkImageFormatProperties* pImageFormatProperties)
{
    wrapped_phys_dev_obj *phys_dev;
    auto vk_phys_dev = unwrap_phys_dev(physicalDevice, &phys_dev);
    VkResult result = phys_dev->inst->layer_disp.GetPhysicalDeviceImageFormatProperties(vk_phys_dev, format, type, tiling, usage, flags, pImageFormatProperties);
    return result;
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties* pProperties)
{
    wrapped_phys_dev_obj *phys_dev;
    auto vk_phys_dev = unwrap_phys_dev(physicalDevice, &phys_dev);
    phys_dev->inst->layer_disp.GetPhysicalDeviceProperties(vk_phys_dev, pProperties);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice physicalDevice, uint32_t* pQueueFamilyPropertyCount, VkQueueFamilyProperties* pQueueFamilyProperties)
{
    wrapped_phys_dev_obj *phys_dev;
    auto vk_phys_dev = unwrap_phys_dev(physicalDevice, &phys_dev);
    phys_dev->inst->layer_disp.GetPhysicalDeviceQueueFamilyProperties(vk_phys_dev, pQueueFamilyPropertyCount, pQueueFamilyProperties);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceMemoryProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties* pMemoryProperties)
{
    wrapped_phys_dev_obj *phys_dev;
    auto vk_phys_dev = unwrap_phys_dev(physicalDevice, &phys_dev);
    phys_dev->inst->layer_disp.GetPhysicalDeviceMemoryProperties(vk_phys_dev, pMemoryProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice)
{
    wrapped_phys_dev_obj *phys_dev;
    auto vk_phys_dev = unwrap_phys_dev(physicalDevice, &phys_dev);
    VkLayerDeviceCreateInfo *chain_info = get_chain_info(pCreateInfo, VK_LAYER_LINK_INFO);
    PFN_vkGetInstanceProcAddr fpGetInstanceProcAddr = chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    PFN_vkGetDeviceProcAddr fpGetDeviceProcAddr = chain_info->u.pLayerInfo->pfnNextGetDeviceProcAddr;
    PFN_vkCreateDevice fpCreateDevice = (PFN_vkCreateDevice) fpGetInstanceProcAddr(NULL, "vkCreateDevice");
    if (fpCreateDevice == NULL) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    // Advance the link info for the next element on the chain
    chain_info->u.pLayerInfo = chain_info->u.pLayerInfo->pNext;
    VkResult result = fpCreateDevice(vk_phys_dev, pCreateInfo, pAllocator, pDevice);
    if (result != VK_SUCCESS) {
        return result;
    }
    initDeviceTable(*pDevice, fpGetDeviceProcAddr);

#if 0 // TODO add once device is wrapped
    // store the loader callback for initializing created dispatchable objects
    chain_info = get_chain_info(pCreateInfo, VK_LOADER_DATA_CALLBACK);
    if (chain_info) {
        dev->pfn_dev_init = chain_info->u.pfnSetDeviceLoaderData;
    } else {
        dev->pfn_dev_init = NULL;
    }
#endif
    return result;
}


VKAPI_ATTR void VKAPI_CALL vkDestroyDevice(VkDevice device, const VkAllocationCallbacks* pAllocator)
{
    dispatch_key key = get_dispatch_key(device);
    VkLayerDispatchTable *pDisp  =  device_dispatch_table(device);
    pDisp->DestroyDevice(device, pAllocator);
    destroy_device_dispatch_table(key);
}

VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice, const char* pLayerName, uint32_t* pPropertyCount, VkExtensionProperties* pProperties)
{
    wrapped_phys_dev_obj *phys_dev;
    auto vk_phys_dev = unwrap_phys_dev(physicalDevice, &phys_dev);

    if (pLayerName && !strcmp(pLayerName, global_layer.layerName))
        return util_GetExtensionProperties(0, nullptr, pPropertyCount, pProperties);

    return phys_dev->inst->layer_disp.EnumerateDeviceExtensionProperties(vk_phys_dev, pLayerName, pPropertyCount, pProperties);
}

VKAPI_ATTR void VKAPI_CALL vkGetDeviceQueue(VkDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, VkQueue* pQueue)
{
    device_dispatch_table(device)->GetDeviceQueue(device, queueFamilyIndex, queueIndex, pQueue);
}

VKAPI_ATTR VkResult VKAPI_CALL vkQueueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo* pSubmits, VkFence fence)
{
    VkResult result = device_dispatch_table(queue)->QueueSubmit(queue, submitCount, pSubmits, fence);
    return result;
}


VKAPI_ATTR VkResult VKAPI_CALL vkQueueWaitIdle(VkQueue queue)
{
    VkResult result = device_dispatch_table(queue)->QueueWaitIdle(queue);
    return result;
}


VKAPI_ATTR VkResult VKAPI_CALL vkDeviceWaitIdle(VkDevice device)
{
    VkResult result = device_dispatch_table(device)->DeviceWaitIdle(device);
    return result;
}


VKAPI_ATTR VkResult VKAPI_CALL vkAllocateMemory(VkDevice device, const VkMemoryAllocateInfo* pAllocateInfo, const VkAllocationCallbacks* pAllocator, VkDeviceMemory* pMemory)
{
    VkResult result = device_dispatch_table(device)->AllocateMemory(device, pAllocateInfo, pAllocator, pMemory);
    return result;
}


VKAPI_ATTR void VKAPI_CALL vkFreeMemory(VkDevice device, VkDeviceMemory memory, const VkAllocationCallbacks* pAllocator)
{
    device_dispatch_table(device)->FreeMemory(device, memory, pAllocator);
}


VKAPI_ATTR VkResult VKAPI_CALL vkMapMemory(VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void** ppData)
{
    VkResult result = device_dispatch_table(device)->MapMemory(device, memory, offset, size, flags, ppData);
    return result;
}

VKAPI_ATTR void VKAPI_CALL vkUnmapMemory(VkDevice device, VkDeviceMemory memory)
{
    device_dispatch_table(device)->UnmapMemory(device, memory);
}


VKAPI_ATTR VkResult VKAPI_CALL vkFlushMappedMemoryRanges(VkDevice device, uint32_t memoryRangeCount, const VkMappedMemoryRange* pMemoryRanges)
{
    VkResult result = device_dispatch_table(device)->FlushMappedMemoryRanges(device, memoryRangeCount, pMemoryRanges);
    return result;
}



VKAPI_ATTR VkResult VKAPI_CALL vkInvalidateMappedMemoryRanges(VkDevice device, uint32_t memoryRangeCount, const VkMappedMemoryRange* pMemoryRanges)
{
    VkResult result = device_dispatch_table(device)->InvalidateMappedMemoryRanges(device, memoryRangeCount, pMemoryRanges);
    return result;
}


VKAPI_ATTR void VKAPI_CALL vkGetDeviceMemoryCommitment(VkDevice device, VkDeviceMemory memory, VkDeviceSize* pCommittedMemoryInBytes)
{
    device_dispatch_table(device)->GetDeviceMemoryCommitment(device, memory, pCommittedMemoryInBytes);
}


VKAPI_ATTR VkResult VKAPI_CALL vkBindBufferMemory(VkDevice device, VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize memoryOffset)
{
    VkResult result = device_dispatch_table(device)->BindBufferMemory(device, buffer, memory, memoryOffset);
    return result;
}


VKAPI_ATTR VkResult VKAPI_CALL vkBindImageMemory(VkDevice device, VkImage image, VkDeviceMemory memory, VkDeviceSize memoryOffset)
{
    VkResult result = device_dispatch_table(device)->BindImageMemory(device, image, memory, memoryOffset);
    return result;
}

VKAPI_ATTR void VKAPI_CALL vkGetBufferMemoryRequirements(VkDevice device, VkBuffer buffer, VkMemoryRequirements* pMemoryRequirements)
{
    device_dispatch_table(device)->GetBufferMemoryRequirements(device, buffer, pMemoryRequirements);
}


VKAPI_ATTR void VKAPI_CALL vkGetImageMemoryRequirements(VkDevice device, VkImage image, VkMemoryRequirements* pMemoryRequirements)
{
    device_dispatch_table(device)->GetImageMemoryRequirements(device, image, pMemoryRequirements);
}


VKAPI_ATTR void VKAPI_CALL vkGetImageSparseMemoryRequirements(VkDevice device, VkImage image, uint32_t* pSparseMemoryRequirementCount, VkSparseImageMemoryRequirements* pSparseMemoryRequirements)
{
    device_dispatch_table(device)->GetImageSparseMemoryRequirements(device, image, pSparseMemoryRequirementCount, pSparseMemoryRequirements);
}


VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceSparseImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkSampleCountFlagBits samples, VkImageUsageFlags usage, VkImageTiling tiling, uint32_t* pPropertyCount, VkSparseImageFormatProperties* pProperties)
{
    wrapped_phys_dev_obj *phys_dev;
    auto vk_phys_dev = unwrap_phys_dev(physicalDevice, &phys_dev);
    phys_dev->inst->layer_disp.GetPhysicalDeviceSparseImageFormatProperties(vk_phys_dev, format, type, samples, usage, tiling, pPropertyCount, pProperties);
}


VKAPI_ATTR VkResult VKAPI_CALL vkQueueBindSparse(VkQueue queue, uint32_t bindInfoCount, const VkBindSparseInfo* pBindInfo, VkFence fence)
{
    VkResult result = device_dispatch_table(queue)->QueueBindSparse(queue, bindInfoCount, pBindInfo, fence);
    return result;
}


VKAPI_ATTR VkResult VKAPI_CALL vkCreateFence(VkDevice device, const VkFenceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFence* pFence)
{
    VkResult result = device_dispatch_table(device)->CreateFence(device, pCreateInfo, pAllocator, pFence);
    return result;
}


VKAPI_ATTR void VKAPI_CALL vkDestroyFence(VkDevice device, VkFence fence, const VkAllocationCallbacks* pAllocator)
{
    device_dispatch_table(device)->DestroyFence(device, fence, pAllocator);
}


VKAPI_ATTR VkResult VKAPI_CALL vkResetFences(VkDevice device, uint32_t fenceCount, const VkFence* pFences)
{
    VkResult result = device_dispatch_table(device)->ResetFences(device, fenceCount, pFences);
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetFenceStatus(VkDevice device, VkFence fence)
{
    VkResult result = device_dispatch_table(device)->GetFenceStatus(device, fence);
    return result;
}


VKAPI_ATTR VkResult VKAPI_CALL vkWaitForFences(VkDevice device, uint32_t fenceCount, const VkFence* pFences, VkBool32 waitAll, uint64_t timeout)
{
    VkResult result = device_dispatch_table(device)->WaitForFences(device, fenceCount, pFences, waitAll, timeout);
    return result;
}


VKAPI_ATTR VkResult VKAPI_CALL vkCreateSemaphore(VkDevice device, const VkSemaphoreCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSemaphore* pSemaphore)
{
    VkResult result = device_dispatch_table(device)->CreateSemaphore(device, pCreateInfo, pAllocator, pSemaphore);
    return result;
}


VKAPI_ATTR void VKAPI_CALL vkDestroySemaphore(VkDevice device, VkSemaphore semaphore, const VkAllocationCallbacks* pAllocator)
{
    device_dispatch_table(device)->DestroySemaphore(device, semaphore, pAllocator);
}


VKAPI_ATTR VkResult VKAPI_CALL vkCreateEvent(VkDevice device, const VkEventCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkEvent* pEvent)
{
    VkResult result = device_dispatch_table(device)->CreateEvent(device, pCreateInfo, pAllocator, pEvent);
    return result;
}


VKAPI_ATTR void VKAPI_CALL vkDestroyEvent(VkDevice device, VkEvent event, const VkAllocationCallbacks* pAllocator)
{
    device_dispatch_table(device)->DestroyEvent(device, event, pAllocator);
}


VKAPI_ATTR VkResult VKAPI_CALL vkGetEventStatus(VkDevice device, VkEvent event)
{
    VkResult result = device_dispatch_table(device)->GetEventStatus(device, event);
    return result;
}


VKAPI_ATTR VkResult VKAPI_CALL vkSetEvent(VkDevice device, VkEvent event)
{
    VkResult result = device_dispatch_table(device)->SetEvent(device, event);
    return result;
}


VKAPI_ATTR VkResult VKAPI_CALL vkResetEvent(VkDevice device, VkEvent event)
{
    VkResult result = device_dispatch_table(device)->ResetEvent(device, event);
    return result;
}


VKAPI_ATTR VkResult VKAPI_CALL vkCreateQueryPool(VkDevice device, const VkQueryPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkQueryPool* pQueryPool)
{
    VkResult result = device_dispatch_table(device)->CreateQueryPool(device, pCreateInfo, pAllocator, pQueryPool);
    return result;
}

VKAPI_ATTR void VKAPI_CALL vkDestroyQueryPool(VkDevice device, VkQueryPool queryPool, const VkAllocationCallbacks* pAllocator)
{
    device_dispatch_table(device)->DestroyQueryPool(device, queryPool, pAllocator);
}


VKAPI_ATTR VkResult VKAPI_CALL vkGetQueryPoolResults(VkDevice device, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount, size_t dataSize, void* pData, VkDeviceSize stride, VkQueryResultFlags flags)
{
    VkResult result = device_dispatch_table(device)->GetQueryPoolResults(device, queryPool, firstQuery, queryCount, dataSize, pData, stride, flags);
    return result;
}


VKAPI_ATTR VkResult VKAPI_CALL vkCreateBuffer(VkDevice device, const VkBufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBuffer* pBuffer)
{
    VkResult result = device_dispatch_table(device)->CreateBuffer(device, pCreateInfo, pAllocator, pBuffer);
    return result;
}


VKAPI_ATTR void VKAPI_CALL vkDestroyBuffer(VkDevice device, VkBuffer buffer, const VkAllocationCallbacks* pAllocator)
{
    device_dispatch_table(device)->DestroyBuffer(device, buffer, pAllocator);
}


VKAPI_ATTR VkResult VKAPI_CALL vkCreateBufferView(VkDevice device, const VkBufferViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBufferView* pView)
{
    VkResult result = device_dispatch_table(device)->CreateBufferView(device, pCreateInfo, pAllocator, pView);
    return result;
}


VKAPI_ATTR void VKAPI_CALL vkDestroyBufferView(VkDevice device, VkBufferView bufferView, const VkAllocationCallbacks* pAllocator)
{
    device_dispatch_table(device)->DestroyBufferView(device, bufferView, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateImage(VkDevice device, const VkImageCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImage* pImage)
{
    VkResult result = device_dispatch_table(device)->CreateImage(device, pCreateInfo, pAllocator, pImage);
    return result;
}


VKAPI_ATTR void VKAPI_CALL vkDestroyImage(VkDevice device, VkImage image, const VkAllocationCallbacks* pAllocator)
{
    device_dispatch_table(device)->DestroyImage(device, image, pAllocator);
}


VKAPI_ATTR void VKAPI_CALL vkGetImageSubresourceLayout(VkDevice device, VkImage image, const VkImageSubresource* pSubresource, VkSubresourceLayout* pLayout)
{
    device_dispatch_table(device)->GetImageSubresourceLayout(device, image, pSubresource, pLayout);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateImageView(VkDevice device, const VkImageViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImageView* pView)
{
    VkResult result = device_dispatch_table(device)->CreateImageView(device, pCreateInfo, pAllocator, pView);
    return result;
}

VKAPI_ATTR void VKAPI_CALL vkDestroyImageView(VkDevice device, VkImageView imageView, const VkAllocationCallbacks* pAllocator)
{
    device_dispatch_table(device)->DestroyImageView(device, imageView, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateShaderModule(VkDevice device, const VkShaderModuleCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkShaderModule* pShaderModule)
{
    VkResult result = device_dispatch_table(device)->CreateShaderModule(device, pCreateInfo, pAllocator, pShaderModule);
    return result;
}


VKAPI_ATTR void VKAPI_CALL vkDestroyShaderModule(VkDevice device, VkShaderModule shaderModule, const VkAllocationCallbacks* pAllocator)
{
    device_dispatch_table(device)->DestroyShaderModule(device, shaderModule, pAllocator);
}


VKAPI_ATTR VkResult VKAPI_CALL vkCreatePipelineCache(VkDevice device, const VkPipelineCacheCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipelineCache* pPipelineCache)
{
    VkResult result = device_dispatch_table(device)->CreatePipelineCache(device, pCreateInfo, pAllocator, pPipelineCache);
    return result;
}

VKAPI_ATTR void VKAPI_CALL vkDestroyPipelineCache(VkDevice device, VkPipelineCache pipelineCache, const VkAllocationCallbacks* pAllocator)
{
    device_dispatch_table(device)->DestroyPipelineCache(device, pipelineCache, pAllocator);
}


VKAPI_ATTR VkResult VKAPI_CALL vkGetPipelineCacheData(VkDevice device, VkPipelineCache pipelineCache, size_t* pDataSize, void* pData)
{
    VkResult result = device_dispatch_table(device)->GetPipelineCacheData(device, pipelineCache, pDataSize, pData);
    return result;
}


VKAPI_ATTR VkResult VKAPI_CALL vkMergePipelineCaches(VkDevice device, VkPipelineCache dstCache, uint32_t srcCacheCount, const VkPipelineCache* pSrcCaches)
{
    VkResult result = device_dispatch_table(device)->MergePipelineCaches(device, dstCache, srcCacheCount, pSrcCaches);
    return result;
}


VKAPI_ATTR VkResult VKAPI_CALL vkCreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines)
{
    VkResult result = device_dispatch_table(device)->CreateGraphicsPipelines(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
    return result;
}


VKAPI_ATTR VkResult VKAPI_CALL vkCreateComputePipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkComputePipelineCreateInfo* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines)
{
    VkResult result = device_dispatch_table(device)->CreateComputePipelines(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
    return result;
}


VKAPI_ATTR void VKAPI_CALL vkDestroyPipeline(VkDevice device, VkPipeline pipeline, const VkAllocationCallbacks* pAllocator)
{
    device_dispatch_table(device)->DestroyPipeline(device, pipeline, pAllocator);
}


VKAPI_ATTR VkResult VKAPI_CALL vkCreatePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipelineLayout* pPipelineLayout)
{
    VkResult result = device_dispatch_table(device)->CreatePipelineLayout(device, pCreateInfo, pAllocator, pPipelineLayout);
    return result;
}


VKAPI_ATTR void VKAPI_CALL vkDestroyPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout, const VkAllocationCallbacks* pAllocator)
{
    device_dispatch_table(device)->DestroyPipelineLayout(device, pipelineLayout, pAllocator);
}


VKAPI_ATTR VkResult VKAPI_CALL vkCreateSampler(VkDevice device, const VkSamplerCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSampler* pSampler)
{
    VkResult result = device_dispatch_table(device)->CreateSampler(device, pCreateInfo, pAllocator, pSampler);
    return result;
}


VKAPI_ATTR void VKAPI_CALL vkDestroySampler(VkDevice device, VkSampler sampler, const VkAllocationCallbacks* pAllocator)
{
    device_dispatch_table(device)->DestroySampler(device, sampler, pAllocator);
}


VKAPI_ATTR VkResult VKAPI_CALL vkCreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorSetLayout* pSetLayout)
{
    VkResult result = device_dispatch_table(device)->CreateDescriptorSetLayout(device, pCreateInfo, pAllocator, pSetLayout);
    return result;
}


VKAPI_ATTR void VKAPI_CALL vkDestroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, const VkAllocationCallbacks* pAllocator)
{
    device_dispatch_table(device)->DestroyDescriptorSetLayout(device, descriptorSetLayout, pAllocator);
}


VKAPI_ATTR VkResult VKAPI_CALL vkCreateDescriptorPool(VkDevice device, const VkDescriptorPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorPool* pDescriptorPool)
{
    VkResult result = device_dispatch_table(device)->CreateDescriptorPool(device, pCreateInfo, pAllocator, pDescriptorPool);
    return result;
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, const VkAllocationCallbacks* pAllocator)
{
    device_dispatch_table(device)->DestroyDescriptorPool(device, descriptorPool, pAllocator);
}


VKAPI_ATTR VkResult VKAPI_CALL vkResetDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorPoolResetFlags flags)
{
    VkResult result = device_dispatch_table(device)->ResetDescriptorPool(device, descriptorPool, flags);
    return result;
}


VKAPI_ATTR VkResult VKAPI_CALL vkAllocateDescriptorSets(VkDevice device, const VkDescriptorSetAllocateInfo* pAllocateInfo, VkDescriptorSet* pDescriptorSets)
{
    VkResult result = device_dispatch_table(device)->AllocateDescriptorSets(device, pAllocateInfo, pDescriptorSets);
    return result;
}


VKAPI_ATTR VkResult VKAPI_CALL vkFreeDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets)
{
    VkResult result = device_dispatch_table(device)->FreeDescriptorSets(device, descriptorPool, descriptorSetCount, pDescriptorSets);
    return result;
}


VKAPI_ATTR void VKAPI_CALL vkUpdateDescriptorSets(VkDevice device, uint32_t descriptorWriteCount, const VkWriteDescriptorSet* pDescriptorWrites, uint32_t descriptorCopyCount, const VkCopyDescriptorSet* pDescriptorCopies)
{
    device_dispatch_table(device)->UpdateDescriptorSets(device, descriptorWriteCount, pDescriptorWrites, descriptorCopyCount, pDescriptorCopies);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateFramebuffer(VkDevice device, const VkFramebufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFramebuffer* pFramebuffer)
{
    VkResult result = device_dispatch_table(device)->CreateFramebuffer(device, pCreateInfo, pAllocator, pFramebuffer);
    return result;
}


VKAPI_ATTR void VKAPI_CALL vkDestroyFramebuffer(VkDevice device, VkFramebuffer framebuffer, const VkAllocationCallbacks* pAllocator)
{
    device_dispatch_table(device)->DestroyFramebuffer(device, framebuffer, pAllocator);
}


VKAPI_ATTR VkResult VKAPI_CALL vkCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkRenderPass* pRenderPass)
{
    VkResult result = device_dispatch_table(device)->CreateRenderPass(device, pCreateInfo, pAllocator, pRenderPass);
    return result;
}

VKAPI_ATTR void VKAPI_CALL vkDestroyRenderPass(VkDevice device, VkRenderPass renderPass, const VkAllocationCallbacks* pAllocator)
{
    device_dispatch_table(device)->DestroyRenderPass(device, renderPass, pAllocator);
}


VKAPI_ATTR void VKAPI_CALL vkGetRenderAreaGranularity(VkDevice device, VkRenderPass renderPass, VkExtent2D* pGranularity)
{
    device_dispatch_table(device)->GetRenderAreaGranularity(device, renderPass, pGranularity);
}


VKAPI_ATTR VkResult VKAPI_CALL vkCreateCommandPool(VkDevice device, const VkCommandPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkCommandPool* pCommandPool)
{
    VkResult result = device_dispatch_table(device)->CreateCommandPool(device, pCreateInfo, pAllocator, pCommandPool);
    return result;
}


VKAPI_ATTR void VKAPI_CALL vkDestroyCommandPool(VkDevice device, VkCommandPool commandPool, const VkAllocationCallbacks* pAllocator)
{
    device_dispatch_table(device)->DestroyCommandPool(device, commandPool, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkResetCommandPool(VkDevice device, VkCommandPool commandPool, VkCommandPoolResetFlags flags)
{
    VkResult result = device_dispatch_table(device)->ResetCommandPool(device, commandPool, flags);
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL vkAllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo* pAllocateInfo, VkCommandBuffer* pCommandBuffers)
{
    VkResult result = device_dispatch_table(device)->AllocateCommandBuffers(device, pAllocateInfo, pCommandBuffers);
    return result;
}

VKAPI_ATTR void VKAPI_CALL vkFreeCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers)
{
    device_dispatch_table(device)->FreeCommandBuffers(device, commandPool, commandBufferCount, pCommandBuffers);
}


VKAPI_ATTR VkResult VKAPI_CALL vkBeginCommandBuffer(VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo* pBeginInfo)
{
    VkResult result = device_dispatch_table(commandBuffer)->BeginCommandBuffer(commandBuffer, pBeginInfo);
    return result;
}


VKAPI_ATTR VkResult VKAPI_CALL vkEndCommandBuffer(VkCommandBuffer commandBuffer)
{
    VkResult result = device_dispatch_table(commandBuffer)->EndCommandBuffer(commandBuffer);
    return result;
}


VKAPI_ATTR VkResult VKAPI_CALL vkResetCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferResetFlags flags)
{
    VkResult result = device_dispatch_table(commandBuffer)->ResetCommandBuffer(commandBuffer, flags);
    return result;
}

VKAPI_ATTR void VKAPI_CALL vkCmdBindPipeline(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline)
{
    device_dispatch_table(commandBuffer)->CmdBindPipeline(commandBuffer, pipelineBindPoint, pipeline);
}


VKAPI_ATTR void VKAPI_CALL vkCmdSetViewport(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const VkViewport* pViewports)
{
    device_dispatch_table(commandBuffer)->CmdSetViewport(commandBuffer, firstViewport, viewportCount, pViewports);
}


VKAPI_ATTR void VKAPI_CALL vkCmdSetScissor(VkCommandBuffer commandBuffer, uint32_t firstScissor, uint32_t scissorCount, const VkRect2D* pScissors)
{
    device_dispatch_table(commandBuffer)->CmdSetScissor(commandBuffer, firstScissor, scissorCount, pScissors);
}


VKAPI_ATTR void VKAPI_CALL vkCmdSetLineWidth(VkCommandBuffer commandBuffer, float lineWidth)
{
    device_dispatch_table(commandBuffer)->CmdSetLineWidth(commandBuffer, lineWidth);
}


VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthBias(VkCommandBuffer commandBuffer, float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor)
{
    device_dispatch_table(commandBuffer)->CmdSetDepthBias(commandBuffer, depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor);
}


VKAPI_ATTR void VKAPI_CALL vkCmdSetBlendConstants(VkCommandBuffer commandBuffer, const float blendConstants[4])
{
    device_dispatch_table(commandBuffer)->CmdSetBlendConstants(commandBuffer, blendConstants);
}


VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthBounds(VkCommandBuffer commandBuffer, float minDepthBounds, float maxDepthBounds)
{
    device_dispatch_table(commandBuffer)->CmdSetDepthBounds(commandBuffer, minDepthBounds, maxDepthBounds);
}


VKAPI_ATTR void VKAPI_CALL vkCmdSetStencilCompareMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t compareMask)
{
    device_dispatch_table(commandBuffer)->CmdSetStencilCompareMask(commandBuffer, faceMask, compareMask);
}


VKAPI_ATTR void VKAPI_CALL vkCmdSetStencilWriteMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t writeMask)
{
    device_dispatch_table(commandBuffer)->CmdSetStencilWriteMask(commandBuffer, faceMask, writeMask);
}


VKAPI_ATTR void VKAPI_CALL vkCmdSetStencilReference(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t reference)
{
    device_dispatch_table(commandBuffer)->CmdSetStencilReference(commandBuffer, faceMask, reference);
}


VKAPI_ATTR void VKAPI_CALL vkCmdBindDescriptorSets(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t firstSet, uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t* pDynamicOffsets)
{
    device_dispatch_table(commandBuffer)->CmdBindDescriptorSets(commandBuffer, pipelineBindPoint, layout, firstSet, descriptorSetCount, pDescriptorSets, dynamicOffsetCount, pDynamicOffsets);
}


VKAPI_ATTR void VKAPI_CALL vkCmdBindIndexBuffer(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType)
{
    device_dispatch_table(commandBuffer)->CmdBindIndexBuffer(commandBuffer, buffer, offset, indexType);
}


VKAPI_ATTR void VKAPI_CALL vkCmdBindVertexBuffers(VkCommandBuffer commandBuffer, uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* pBuffers, const VkDeviceSize* pOffsets)
{
    device_dispatch_table(commandBuffer)->CmdBindVertexBuffers(commandBuffer, firstBinding, bindingCount, pBuffers, pOffsets);
}


VKAPI_ATTR void VKAPI_CALL vkCmdDraw(VkCommandBuffer commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
    device_dispatch_table(commandBuffer)->CmdDraw(commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}


VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndexed(VkCommandBuffer commandBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
{
    device_dispatch_table(commandBuffer)->CmdDrawIndexed(commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}


VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
{
    device_dispatch_table(commandBuffer)->CmdDrawIndirect(commandBuffer, buffer, offset, drawCount, stride);
}


VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndexedIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
{
    device_dispatch_table(commandBuffer)->CmdDrawIndexedIndirect(commandBuffer, buffer, offset, drawCount, stride);
}


VKAPI_ATTR void VKAPI_CALL vkCmdDispatch(VkCommandBuffer commandBuffer, uint32_t x, uint32_t y, uint32_t z)
{
    device_dispatch_table(commandBuffer)->CmdDispatch(commandBuffer, x, y, z);
}


VKAPI_ATTR void VKAPI_CALL vkCmdDispatchIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset)
{
    device_dispatch_table(commandBuffer)->CmdDispatchIndirect(commandBuffer, buffer, offset);
}


VKAPI_ATTR void VKAPI_CALL vkCmdCopyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferCopy* pRegions)
{
    device_dispatch_table(commandBuffer)->CmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, regionCount, pRegions);
}


VKAPI_ATTR void VKAPI_CALL vkCmdCopyImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageCopy* pRegions)
{
    device_dispatch_table(commandBuffer)->CmdCopyImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
}


VKAPI_ATTR void VKAPI_CALL vkCmdBlitImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageBlit* pRegions, VkFilter filter)
{
    device_dispatch_table(commandBuffer)->CmdBlitImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions, filter);
}


VKAPI_ATTR void VKAPI_CALL vkCmdCopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkBufferImageCopy* pRegions)
{
    device_dispatch_table(commandBuffer)->CmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, dstImageLayout, regionCount, pRegions);
}

VKAPI_ATTR void VKAPI_CALL vkCmdCopyImageToBuffer(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferImageCopy* pRegions)
{
    device_dispatch_table(commandBuffer)->CmdCopyImageToBuffer(commandBuffer, srcImage, srcImageLayout, dstBuffer, regionCount, pRegions);
}


VKAPI_ATTR void VKAPI_CALL vkCmdUpdateBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const uint32_t* pData)
{
    device_dispatch_table(commandBuffer)->CmdUpdateBuffer(commandBuffer, dstBuffer, dstOffset, dataSize, pData);
}

VKAPI_ATTR void VKAPI_CALL vkCmdFillBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size, uint32_t data)
{
    device_dispatch_table(commandBuffer)->CmdFillBuffer(commandBuffer, dstBuffer, dstOffset, size, data);
}


VKAPI_ATTR void VKAPI_CALL vkCmdClearColorImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout, const VkClearColorValue* pColor, uint32_t rangeCount, const VkImageSubresourceRange* pRanges)
{
    device_dispatch_table(commandBuffer)->CmdClearColorImage(commandBuffer, image, imageLayout, pColor, rangeCount, pRanges);
}

VKAPI_ATTR void VKAPI_CALL vkCmdClearDepthStencilImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout, const VkClearDepthStencilValue* pDepthStencil, uint32_t rangeCount, const VkImageSubresourceRange* pRanges)
{
    device_dispatch_table(commandBuffer)->CmdClearDepthStencilImage(commandBuffer, image, imageLayout, pDepthStencil, rangeCount, pRanges);
}


VKAPI_ATTR void VKAPI_CALL vkCmdClearAttachments(VkCommandBuffer commandBuffer, uint32_t attachmentCount, const VkClearAttachment* pAttachments, uint32_t rectCount, const VkClearRect* pRects)
{
    device_dispatch_table(commandBuffer)->CmdClearAttachments(commandBuffer, attachmentCount, pAttachments, rectCount, pRects);
}


VKAPI_ATTR void VKAPI_CALL vkCmdResolveImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageResolve* pRegions)
{
    device_dispatch_table(commandBuffer)->CmdResolveImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
}


VKAPI_ATTR void VKAPI_CALL vkCmdSetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask)
{
    device_dispatch_table(commandBuffer)->CmdSetEvent(commandBuffer, event, stageMask);
}


VKAPI_ATTR void VKAPI_CALL vkCmdResetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask)
{
    device_dispatch_table(commandBuffer)->CmdResetEvent(commandBuffer, event, stageMask);
}


VKAPI_ATTR void VKAPI_CALL vkCmdWaitEvents(VkCommandBuffer commandBuffer, uint32_t eventCount, const VkEvent* pEvents, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers)
{
    device_dispatch_table(commandBuffer)->CmdWaitEvents(commandBuffer, eventCount, pEvents, srcStageMask, dstStageMask, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
}

VKAPI_ATTR void VKAPI_CALL vkCmdPipelineBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers)
{
    device_dispatch_table(commandBuffer)->CmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, dependencyFlags, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
}

VKAPI_ATTR void VKAPI_CALL vkCmdBeginQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query, VkQueryControlFlags flags)
{
    device_dispatch_table(commandBuffer)->CmdBeginQuery(commandBuffer, queryPool, query, flags);
}

VKAPI_ATTR void VKAPI_CALL vkCmdEndQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query)
{
    device_dispatch_table(commandBuffer)->CmdEndQuery(commandBuffer, queryPool, query);
}

VKAPI_ATTR void VKAPI_CALL vkCmdResetQueryPool(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount)
{
    device_dispatch_table(commandBuffer)->CmdResetQueryPool(commandBuffer, queryPool, firstQuery, queryCount);
}

VKAPI_ATTR void VKAPI_CALL vkCmdWriteTimestamp(VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage, VkQueryPool queryPool, uint32_t query)
{
    device_dispatch_table(commandBuffer)->CmdWriteTimestamp(commandBuffer, pipelineStage, queryPool, query);
}

VKAPI_ATTR void VKAPI_CALL vkCmdCopyQueryPoolResults(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize stride, VkQueryResultFlags flags)
{
    device_dispatch_table(commandBuffer)->CmdCopyQueryPoolResults(commandBuffer, queryPool, firstQuery, queryCount, dstBuffer, dstOffset, stride, flags);
}

VKAPI_ATTR void VKAPI_CALL vkCmdPushConstants(VkCommandBuffer commandBuffer, VkPipelineLayout layout, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void* pValues)
{
    device_dispatch_table(commandBuffer)->CmdPushConstants(commandBuffer, layout, stageFlags, offset, size, pValues);
}

VKAPI_ATTR void VKAPI_CALL vkCmdBeginRenderPass(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo* pRenderPassBegin, VkSubpassContents contents)
{
    device_dispatch_table(commandBuffer)->CmdBeginRenderPass(commandBuffer, pRenderPassBegin, contents);
}

VKAPI_ATTR void VKAPI_CALL vkCmdNextSubpass(VkCommandBuffer commandBuffer, VkSubpassContents contents)
{
    device_dispatch_table(commandBuffer)->CmdNextSubpass(commandBuffer, contents);
}

VKAPI_ATTR void VKAPI_CALL vkCmdEndRenderPass(VkCommandBuffer commandBuffer)
{
    device_dispatch_table(commandBuffer)->CmdEndRenderPass(commandBuffer);
}

VKAPI_ATTR void VKAPI_CALL vkCmdExecuteCommands(VkCommandBuffer commandBuffer, uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers)
{
    device_dispatch_table(commandBuffer)->CmdExecuteCommands(commandBuffer, commandBufferCount, pCommandBuffers);
}

VKAPI_ATTR void VKAPI_CALL vkDestroySurfaceKHR(VkInstance instance, VkSurfaceKHR surface, const VkAllocationCallbacks* pAllocator)
{
    wrapped_inst_obj *inst;
    auto vk_inst = unwrap_instance(instance, &inst);
    inst->layer_disp.DestroySurfaceKHR(vk_inst, surface, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, VkSurfaceKHR surface, VkBool32* pSupported)
{
    wrapped_phys_dev_obj *phys_dev;
    auto vk_phys_dev = unwrap_phys_dev(physicalDevice, &phys_dev);
    VkResult result = phys_dev->inst->layer_disp.GetPhysicalDeviceSurfaceSupportKHR(vk_phys_dev, queueFamilyIndex, surface, pSupported);
    return result;
}


VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR* pSurfaceCapabilities)
{
    wrapped_phys_dev_obj *phys_dev;
    auto vk_phys_dev = unwrap_phys_dev(physicalDevice, &phys_dev);
    VkResult result = phys_dev->inst->layer_disp.GetPhysicalDeviceSurfaceCapabilitiesKHR(vk_phys_dev, surface, pSurfaceCapabilities);
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceFormatsKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t* pSurfaceFormatCount, VkSurfaceFormatKHR* pSurfaceFormats)
{
    wrapped_phys_dev_obj *phys_dev;
    auto vk_phys_dev = unwrap_phys_dev(physicalDevice, &phys_dev);
    VkResult result = phys_dev->inst->layer_disp.GetPhysicalDeviceSurfaceFormatsKHR(vk_phys_dev, surface, pSurfaceFormatCount, pSurfaceFormats);
    return result;
}


VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfacePresentModesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t* pPresentModeCount, VkPresentModeKHR* pPresentModes)
{
    wrapped_phys_dev_obj *phys_dev;
    auto vk_phys_dev = unwrap_phys_dev(physicalDevice, &phys_dev);
    VkResult result = phys_dev->inst->layer_disp.GetPhysicalDeviceSurfacePresentModesKHR(vk_phys_dev, surface, pPresentModeCount, pPresentModes);
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pSwapchain)
{
    VkResult result = device_dispatch_table(device)->CreateSwapchainKHR(device, pCreateInfo, pAllocator, pSwapchain);
    return result;
}

VKAPI_ATTR void VKAPI_CALL vkDestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain, const VkAllocationCallbacks* pAllocator)
{
    device_dispatch_table(device)->DestroySwapchainKHR(device, swapchain, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain, uint32_t* pSwapchainImageCount, VkImage* pSwapchainImages)
{
    VkResult result = device_dispatch_table(device)->GetSwapchainImagesKHR(device, swapchain, pSwapchainImageCount, pSwapchainImages);
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL vkAcquireNextImageKHR(VkDevice device, VkSwapchainKHR swapchain, uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t* pImageIndex)
{
    VkResult result = device_dispatch_table(device)->AcquireNextImageKHR(device, swapchain, timeout, semaphore, fence, pImageIndex);
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL vkQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR* pPresentInfo)
{
    VkResult result = device_dispatch_table(queue)->QueuePresentKHR(queue, pPresentInfo);
    return result;
}

#ifdef VK_USE_PLATFORM_WIN32_KHR

VKAPI_ATTR VkResult VKAPI_CALL
vkCreateWin32SurfaceKHR(VkInstance instance, const VkWin32SurfaceCreateInfoKHR *pCreateInfo,
                        const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
    VkResult result;
    wrapped_inst_obj *inst;
    auto vk_inst = unwrap_instance(instance, &inst);
    result = inst->layer_disp.CreateWin32SurfaceKHR(vk_inst, pCreateInfo, pAllocator, pSurface);
    return result;
}

VKAPI_ATTR VkBool32 VKAPI_CALL
vkGetPhysicalDeviceWin32PresentationSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex) {
    VkBool32 result;
    wrapped_phys_dev_obj *phys_dev;
    auto vk_phys_dev = unwrap_phys_dev(physicalDevice, &phys_dev);
    result = phys_dev->inst->layer_disp.GetPhysicalDeviceWin32PresentationSupportKHR(vk_phys_dev, queueFamilyIndex);
    return result;
}
#endif // VK_USE_PLATFORM_WIN32_KHR

#ifdef VK_USE_PLATFORM_XCB_KHR

VKAPI_ATTR VkResult VKAPI_CALL vkCreateXcbSurfaceKHR(VkInstance instance, const VkXcbSurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface)
{
    wrapped_inst_obj *inst;
    auto vk_inst = unwrap_instance(instance, &inst);
    VkResult result = inst->layer_disp.CreateXcbSurfaceKHR(vk_inst, pCreateInfo, pAllocator, pSurface);
    return result;
}

VKAPI_ATTR VkBool32 VKAPI_CALL vkGetPhysicalDeviceXcbPresentationSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, xcb_connection_t* connection, xcb_visualid_t visual_id)
{
    wrapped_phys_dev_obj *phys_dev;
    auto vk_phys_dev = unwrap_phys_dev(physicalDevice, &phys_dev);
    VkBool32 result = phys_dev->inst->layer_disp.GetPhysicalDeviceXcbPresentationSupportKHR(vk_phys_dev, queueFamilyIndex, connection, visual_id);
    return result;
}
#endif  // VK_USE_PLATFORM_XCB_KHR


#ifdef VK_USE_PLATFORM_XLIB_KHR

VKAPI_ATTR VkResult VKAPI_CALL vkCreateXlibSurfaceKHR(VkInstance instance, const VkXlibSurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface)
{
    wrapped_inst_obj *inst;
    auto vk_inst = unwrap_instance(instance, &inst);
    VkResult result = inst->layer_disp.CreateXlibSurfaceKHR(vk_inst, pCreateInfo, pAllocator, pSurface);
    return result;
}

VKAPI_ATTR VkBool32 VKAPI_CALL vkGetPhysicalDeviceXlibPresentationSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, Display* dpy, VisualID visualID)
{
    wrapped_phys_dev_obj *phys_dev;
    auto vk_phys_dev = unwrap_phys_dev(physicalDevice, &phys_dev);
    VkBool32 result = phys_dev->inst->layer_disp.GetPhysicalDeviceXlibPresentationSupportKHR(vk_phys_dev, queueFamilyIndex, dpy, visualID);
    return result;
}
#endif  // VK_USE_PLATFORM_XLIB_KHR

#ifdef VK_USE_PLATFORM_WAYLAND_KHR

VKAPI_ATTR VkResult VKAPI_CALL vkCreateWaylandSurfaceKHR(VkInstance instance, const VkWaylandSurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface)
{
    wrapped_inst_obj *inst;
    auto vk_inst = unwrap_instance(instance, &inst);
    VkResult result = inst->layer_disp.CreateWaylandSurfaceKHR(vk_inst, pCreateInfo, pAllocator, pSurface);
    return result;
}

VKAPI_ATTR VkBool32 VKAPI_CALL vkGetPhysicalDeviceWaylandPresentationSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, struct wl_display* display)
{
    wrapped_phys_dev_obj *phys_dev;
    auto vk_phys_dev = unwrap_phys_dev(physicalDevice, &phys_dev);
    VkBool32 result = phys_dev->inst->layer_disp.GetPhysicalDeviceWaylandPresentationSupportKHR(vk_phys_dev, queueFamilyIndex, display);
    return result;
}
#endif  // VK_USE_PLATFORM_WAYLAND_KHR


#ifdef VK_USE_PLATFORM_MIR_KHR


VKAPI_ATTR VkResult VKAPI_CALL vkCreateMirSurfaceKHR(VkInstance instance, const VkMirSurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface)
{
    wrapped_inst_obj *inst;
    auto vk_inst = unwrap_instance(instance, &inst);
    VkResult result = inst->layer_disp.CreateMirSurfaceKHR(vk_inst, pCreateInfo, pAllocator, pSurface);
    return result;
}

VKAPI_ATTR VkBool32 VKAPI_CALL vkGetPhysicalDeviceMirPresentationSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, MirConnection* connection)
{
    wrapped_phys_dev_obj *phys_dev;
    auto vk_phys_dev = unwrap_phys_dev(physicalDevice, &phys_dev);
    VkBool32 result = phys_dev->inst->layer_disp.GetPhysicalDeviceMirPresentationSupportKHR(vk_phys_dev, queueFamilyIndex, connection);
    return result;
}
#endif  // VK_USE_PLATFORM_MIR_KHR

VKAPI_ATTR VkResult VKAPI_CALL
vkCreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT *pCreateInfo,
                             const VkAllocationCallbacks *pAllocator, VkDebugReportCallbackEXT *pMsgCallback) {
    wrapped_inst_obj *inst;
    auto vk_inst = unwrap_instance(instance, &inst);

    VkResult res = inst->layer_disp.CreateDebugReportCallbackEXT(vk_inst, pCreateInfo, pAllocator, pMsgCallback);
    return res;
}

VKAPI_ATTR void VKAPI_CALL
vkDestroyDebugReportCallbackEXT(VkInstance instance,
                              VkDebugReportCallbackEXT msgCallback,
                              const VkAllocationCallbacks *pAllocator) {
    wrapped_inst_obj *inst;
    auto vk_inst = unwrap_instance(instance, &inst);
    inst->layer_disp.DestroyDebugReportCallbackEXT(vk_inst, msgCallback, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL
vkDebugReportMessageEXT(VkInstance instance, VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t object,
                      size_t location, int32_t msgCode, const char *pLayerPrefix, const char *pMsg) {
    wrapped_inst_obj *inst;
    auto vk_inst = unwrap_instance(instance, &inst);
    inst->layer_disp.DebugReportMessageEXT(vk_inst, flags, objType, object, location, msgCode, pLayerPrefix,
                                                            pMsg);
}

static inline PFN_vkVoidFunction layer_intercept_proc(const char *name)
{
    if (!name || name[0] != 'v' || name[1] != 'k')
        return NULL;

    name += 2;
    if (!strcmp(name, "CreateInstance"))
        return (PFN_vkVoidFunction) vkCreateInstance;
    if (!strcmp(name, "DestroyInstance"))
        return (PFN_vkVoidFunction) vkDestroyInstance;
    if (!strcmp(name, "EnumeratePhysicalDevices"))
        return (PFN_vkVoidFunction) vkEnumeratePhysicalDevices;
    if (!strcmp(name, "GetPhysicalDeviceFeatures"))
        return (PFN_vkVoidFunction) vkGetPhysicalDeviceFeatures;
    if (!strcmp(name, "GetPhysicalDeviceFormatProperties"))
        return (PFN_vkVoidFunction) vkGetPhysicalDeviceFormatProperties;
    if (!strcmp(name, "GetPhysicalDeviceImageFormatProperties"))
        return (PFN_vkVoidFunction) vkGetPhysicalDeviceImageFormatProperties;
    if (!strcmp(name, "GetPhysicalDeviceProperties"))
        return (PFN_vkVoidFunction) vkGetPhysicalDeviceProperties;
    if (!strcmp(name, "GetPhysicalDeviceQueueFamilyProperties"))
        return (PFN_vkVoidFunction) vkGetPhysicalDeviceQueueFamilyProperties;
    if (!strcmp(name, "GetPhysicalDeviceMemoryProperties"))
        return (PFN_vkVoidFunction) vkGetPhysicalDeviceMemoryProperties;
    if (!strcmp(name, "CreateDevice"))
        return (PFN_vkVoidFunction) vkCreateDevice;
    if (!strcmp(name, "DestroyDevice"))
        return (PFN_vkVoidFunction) vkDestroyDevice;
    if (!strcmp(name, "GetDeviceQueue"))
        return (PFN_vkVoidFunction) vkGetDeviceQueue;
    if (!strcmp(name, "QueueSubmit"))
        return (PFN_vkVoidFunction) vkQueueSubmit;
    if (!strcmp(name, "QueueWaitIdle"))
        return (PFN_vkVoidFunction) vkQueueWaitIdle;
    if (!strcmp(name, "DeviceWaitIdle"))
        return (PFN_vkVoidFunction) vkDeviceWaitIdle;
    if (!strcmp(name, "AllocateMemory"))
        return (PFN_vkVoidFunction) vkAllocateMemory;
    if (!strcmp(name, "FreeMemory"))
        return (PFN_vkVoidFunction) vkFreeMemory;
    if (!strcmp(name, "MapMemory"))
        return (PFN_vkVoidFunction) vkMapMemory;
    if (!strcmp(name, "UnmapMemory"))
        return (PFN_vkVoidFunction) vkUnmapMemory;
    if (!strcmp(name, "FlushMappedMemoryRanges"))
        return (PFN_vkVoidFunction) vkFlushMappedMemoryRanges;
    if (!strcmp(name, "InvalidateMappedMemoryRanges"))
        return (PFN_vkVoidFunction) vkInvalidateMappedMemoryRanges;
    if (!strcmp(name, "GetDeviceMemoryCommitment"))
        return (PFN_vkVoidFunction) vkGetDeviceMemoryCommitment;
    if (!strcmp(name, "BindBufferMemory"))
        return (PFN_vkVoidFunction) vkBindBufferMemory;
    if (!strcmp(name, "BindImageMemory"))
        return (PFN_vkVoidFunction) vkBindImageMemory;
    if (!strcmp(name, "GetBufferMemoryRequirements"))
        return (PFN_vkVoidFunction) vkGetBufferMemoryRequirements;
    if (!strcmp(name, "GetImageMemoryRequirements"))
        return (PFN_vkVoidFunction) vkGetImageMemoryRequirements;
    if (!strcmp(name, "GetImageSparseMemoryRequirements"))
        return (PFN_vkVoidFunction) vkGetImageSparseMemoryRequirements;
    if (!strcmp(name, "GetPhysicalDeviceSparseImageFormatProperties"))
        return (PFN_vkVoidFunction) vkGetPhysicalDeviceSparseImageFormatProperties;
    if (!strcmp(name, "QueueBindSparse"))
        return (PFN_vkVoidFunction) vkQueueBindSparse;
    if (!strcmp(name, "CreateFence"))
        return (PFN_vkVoidFunction) vkCreateFence;
    if (!strcmp(name, "DestroyFence"))
        return (PFN_vkVoidFunction) vkDestroyFence;
    if (!strcmp(name, "ResetFences"))
        return (PFN_vkVoidFunction) vkResetFences;
    if (!strcmp(name, "GetFenceStatus"))
        return (PFN_vkVoidFunction) vkGetFenceStatus;
    if (!strcmp(name, "WaitForFences"))
        return (PFN_vkVoidFunction) vkWaitForFences;
    if (!strcmp(name, "CreateSemaphore"))
        return (PFN_vkVoidFunction) vkCreateSemaphore;
    if (!strcmp(name, "DestroySemaphore"))
        return (PFN_vkVoidFunction) vkDestroySemaphore;
    if (!strcmp(name, "CreateEvent"))
        return (PFN_vkVoidFunction) vkCreateEvent;
    if (!strcmp(name, "DestroyEvent"))
        return (PFN_vkVoidFunction) vkDestroyEvent;
    if (!strcmp(name, "GetEventStatus"))
        return (PFN_vkVoidFunction) vkGetEventStatus;
    if (!strcmp(name, "SetEvent"))
        return (PFN_vkVoidFunction) vkSetEvent;
    if (!strcmp(name, "ResetEvent"))
        return (PFN_vkVoidFunction) vkResetEvent;
    if (!strcmp(name, "CreateQueryPool"))
        return (PFN_vkVoidFunction) vkCreateQueryPool;
    if (!strcmp(name, "DestroyQueryPool"))
        return (PFN_vkVoidFunction) vkDestroyQueryPool;
    if (!strcmp(name, "GetQueryPoolResults"))
        return (PFN_vkVoidFunction) vkGetQueryPoolResults;
    if (!strcmp(name, "CreateBuffer"))
        return (PFN_vkVoidFunction) vkCreateBuffer;
    if (!strcmp(name, "DestroyBuffer"))
        return (PFN_vkVoidFunction) vkDestroyBuffer;
    if (!strcmp(name, "CreateBufferView"))
        return (PFN_vkVoidFunction) vkCreateBufferView;
    if (!strcmp(name, "DestroyBufferView"))
        return (PFN_vkVoidFunction) vkDestroyBufferView;
    if (!strcmp(name, "CreateImage"))
        return (PFN_vkVoidFunction) vkCreateImage;
    if (!strcmp(name, "DestroyImage"))
        return (PFN_vkVoidFunction) vkDestroyImage;
    if (!strcmp(name, "GetImageSubresourceLayout"))
        return (PFN_vkVoidFunction) vkGetImageSubresourceLayout;
    if (!strcmp(name, "CreateImageView"))
        return (PFN_vkVoidFunction) vkCreateImageView;
    if (!strcmp(name, "DestroyImageView"))
        return (PFN_vkVoidFunction) vkDestroyImageView;
    if (!strcmp(name, "CreateShaderModule"))
        return (PFN_vkVoidFunction) vkCreateShaderModule;
    if (!strcmp(name, "DestroyShaderModule"))
        return (PFN_vkVoidFunction) vkDestroyShaderModule;
    if (!strcmp(name, "CreatePipelineCache"))
        return (PFN_vkVoidFunction) vkCreatePipelineCache;
    if (!strcmp(name, "DestroyPipelineCache"))
        return (PFN_vkVoidFunction) vkDestroyPipelineCache;
    if (!strcmp(name, "GetPipelineCacheData"))
        return (PFN_vkVoidFunction) vkGetPipelineCacheData;
    if (!strcmp(name, "MergePipelineCaches"))
        return (PFN_vkVoidFunction) vkMergePipelineCaches;
    if (!strcmp(name, "CreateGraphicsPipelines"))
        return (PFN_vkVoidFunction) vkCreateGraphicsPipelines;
    if (!strcmp(name, "CreateComputePipelines"))
        return (PFN_vkVoidFunction) vkCreateComputePipelines;
    if (!strcmp(name, "DestroyPipeline"))
        return (PFN_vkVoidFunction) vkDestroyPipeline;
    if (!strcmp(name, "CreatePipelineLayout"))
        return (PFN_vkVoidFunction) vkCreatePipelineLayout;
    if (!strcmp(name, "DestroyPipelineLayout"))
        return (PFN_vkVoidFunction) vkDestroyPipelineLayout;
    if (!strcmp(name, "CreateSampler"))
        return (PFN_vkVoidFunction) vkCreateSampler;
    if (!strcmp(name, "DestroySampler"))
        return (PFN_vkVoidFunction) vkDestroySampler;
    if (!strcmp(name, "CreateDescriptorSetLayout"))
        return (PFN_vkVoidFunction) vkCreateDescriptorSetLayout;
    if (!strcmp(name, "DestroyDescriptorSetLayout"))
        return (PFN_vkVoidFunction) vkDestroyDescriptorSetLayout;
    if (!strcmp(name, "CreateDescriptorPool"))
        return (PFN_vkVoidFunction) vkCreateDescriptorPool;
    if (!strcmp(name, "DestroyDescriptorPool"))
        return (PFN_vkVoidFunction) vkDestroyDescriptorPool;
    if (!strcmp(name, "ResetDescriptorPool"))
        return (PFN_vkVoidFunction) vkResetDescriptorPool;
    if (!strcmp(name, "AllocateDescriptorSets"))
        return (PFN_vkVoidFunction) vkAllocateDescriptorSets;
    if (!strcmp(name, "FreeDescriptorSets"))
        return (PFN_vkVoidFunction) vkFreeDescriptorSets;
    if (!strcmp(name, "UpdateDescriptorSets"))
        return (PFN_vkVoidFunction) vkUpdateDescriptorSets;
    if (!strcmp(name, "CreateFramebuffer"))
        return (PFN_vkVoidFunction) vkCreateFramebuffer;
    if (!strcmp(name, "DestroyFramebuffer"))
        return (PFN_vkVoidFunction) vkDestroyFramebuffer;
    if (!strcmp(name, "CreateRenderPass"))
        return (PFN_vkVoidFunction) vkCreateRenderPass;
    if (!strcmp(name, "DestroyRenderPass"))
        return (PFN_vkVoidFunction) vkDestroyRenderPass;
    if (!strcmp(name, "GetRenderAreaGranularity"))
        return (PFN_vkVoidFunction) vkGetRenderAreaGranularity;
    if (!strcmp(name, "CreateCommandPool"))
        return (PFN_vkVoidFunction) vkCreateCommandPool;
    if (!strcmp(name, "DestroyCommandPool"))
        return (PFN_vkVoidFunction) vkDestroyCommandPool;
    if (!strcmp(name, "ResetCommandPool"))
        return (PFN_vkVoidFunction) vkResetCommandPool;
    if (!strcmp(name, "AllocateCommandBuffers"))
        return (PFN_vkVoidFunction) vkAllocateCommandBuffers;
    if (!strcmp(name, "FreeCommandBuffers"))
        return (PFN_vkVoidFunction) vkFreeCommandBuffers;
    if (!strcmp(name, "BeginCommandBuffer"))
        return (PFN_vkVoidFunction) vkBeginCommandBuffer;
    if (!strcmp(name, "EndCommandBuffer"))
        return (PFN_vkVoidFunction) vkEndCommandBuffer;
    if (!strcmp(name, "ResetCommandBuffer"))
        return (PFN_vkVoidFunction) vkResetCommandBuffer;
    if (!strcmp(name, "CmdBindPipeline"))
        return (PFN_vkVoidFunction) vkCmdBindPipeline;
    if (!strcmp(name, "CmdSetViewport"))
        return (PFN_vkVoidFunction) vkCmdSetViewport;
    if (!strcmp(name, "CmdSetScissor"))
        return (PFN_vkVoidFunction) vkCmdSetScissor;
    if (!strcmp(name, "CmdSetLineWidth"))
        return (PFN_vkVoidFunction) vkCmdSetLineWidth;
    if (!strcmp(name, "CmdSetDepthBias"))
        return (PFN_vkVoidFunction) vkCmdSetDepthBias;
    if (!strcmp(name, "CmdSetBlendConstants"))
        return (PFN_vkVoidFunction) vkCmdSetBlendConstants;
    if (!strcmp(name, "CmdSetDepthBounds"))
        return (PFN_vkVoidFunction) vkCmdSetDepthBounds;
    if (!strcmp(name, "CmdSetStencilCompareMask"))
        return (PFN_vkVoidFunction) vkCmdSetStencilCompareMask;
    if (!strcmp(name, "CmdSetStencilWriteMask"))
        return (PFN_vkVoidFunction) vkCmdSetStencilWriteMask;
    if (!strcmp(name, "CmdSetStencilReference"))
        return (PFN_vkVoidFunction) vkCmdSetStencilReference;
    if (!strcmp(name, "CmdBindDescriptorSets"))
        return (PFN_vkVoidFunction) vkCmdBindDescriptorSets;
    if (!strcmp(name, "CmdBindIndexBuffer"))
        return (PFN_vkVoidFunction) vkCmdBindIndexBuffer;
    if (!strcmp(name, "CmdBindVertexBuffers"))
        return (PFN_vkVoidFunction) vkCmdBindVertexBuffers;
    if (!strcmp(name, "CmdDraw"))
        return (PFN_vkVoidFunction) vkCmdDraw;
    if (!strcmp(name, "CmdDrawIndexed"))
        return (PFN_vkVoidFunction) vkCmdDrawIndexed;
    if (!strcmp(name, "CmdDrawIndirect"))
        return (PFN_vkVoidFunction) vkCmdDrawIndirect;
    if (!strcmp(name, "CmdDrawIndexedIndirect"))
        return (PFN_vkVoidFunction) vkCmdDrawIndexedIndirect;
    if (!strcmp(name, "CmdDispatch"))
        return (PFN_vkVoidFunction) vkCmdDispatch;
    if (!strcmp(name, "CmdDispatchIndirect"))
        return (PFN_vkVoidFunction) vkCmdDispatchIndirect;
    if (!strcmp(name, "CmdCopyBuffer"))
        return (PFN_vkVoidFunction) vkCmdCopyBuffer;
    if (!strcmp(name, "CmdCopyImage"))
        return (PFN_vkVoidFunction) vkCmdCopyImage;
    if (!strcmp(name, "CmdBlitImage"))
        return (PFN_vkVoidFunction) vkCmdBlitImage;
    if (!strcmp(name, "CmdCopyBufferToImage"))
        return (PFN_vkVoidFunction) vkCmdCopyBufferToImage;
    if (!strcmp(name, "CmdCopyImageToBuffer"))
        return (PFN_vkVoidFunction) vkCmdCopyImageToBuffer;
    if (!strcmp(name, "CmdUpdateBuffer"))
        return (PFN_vkVoidFunction) vkCmdUpdateBuffer;
    if (!strcmp(name, "CmdFillBuffer"))
        return (PFN_vkVoidFunction) vkCmdFillBuffer;
    if (!strcmp(name, "CmdClearColorImage"))
        return (PFN_vkVoidFunction) vkCmdClearColorImage;
    if (!strcmp(name, "CmdClearDepthStencilImage"))
        return (PFN_vkVoidFunction) vkCmdClearDepthStencilImage;
    if (!strcmp(name, "CmdClearAttachments"))
        return (PFN_vkVoidFunction) vkCmdClearAttachments;
    if (!strcmp(name, "CmdResolveImage"))
        return (PFN_vkVoidFunction) vkCmdResolveImage;
    if (!strcmp(name, "CmdSetEvent"))
        return (PFN_vkVoidFunction) vkCmdSetEvent;
    if (!strcmp(name, "CmdResetEvent"))
        return (PFN_vkVoidFunction) vkCmdResetEvent;
    if (!strcmp(name, "CmdWaitEvents"))
        return (PFN_vkVoidFunction) vkCmdWaitEvents;
    if (!strcmp(name, "CmdPipelineBarrier"))
        return (PFN_vkVoidFunction) vkCmdPipelineBarrier;
    if (!strcmp(name, "CmdBeginQuery"))
        return (PFN_vkVoidFunction) vkCmdBeginQuery;
    if (!strcmp(name, "CmdEndQuery"))
        return (PFN_vkVoidFunction) vkCmdEndQuery;
    if (!strcmp(name, "CmdResetQueryPool"))
        return (PFN_vkVoidFunction) vkCmdResetQueryPool;
    if (!strcmp(name, "CmdWriteTimestamp"))
        return (PFN_vkVoidFunction) vkCmdWriteTimestamp;
    if (!strcmp(name, "CmdCopyQueryPoolResults"))
        return (PFN_vkVoidFunction) vkCmdCopyQueryPoolResults;
    if (!strcmp(name, "CmdPushConstants"))
        return (PFN_vkVoidFunction) vkCmdPushConstants;
    if (!strcmp(name, "CmdBeginRenderPass"))
        return (PFN_vkVoidFunction) vkCmdBeginRenderPass;
    if (!strcmp(name, "CmdNextSubpass"))
        return (PFN_vkVoidFunction) vkCmdNextSubpass;
    if (!strcmp(name, "CmdEndRenderPass"))
        return (PFN_vkVoidFunction) vkCmdEndRenderPass;
    if (!strcmp(name, "CmdExecuteCommands"))
        return (PFN_vkVoidFunction) vkCmdExecuteCommands;

    return NULL;
}

static inline PFN_vkVoidFunction layer_intercept_instance_proc(const char *name)
{
    if (!name || name[0] != 'v' || name[1] != 'k')
        return NULL;

    name += 2;
    if (!strcmp(name, "GetInstanceProcAddr"))
        return (PFN_vkVoidFunction)vkGetInstanceProcAddr;
    if (!strcmp(name, "DestroyInstance"))
        return (PFN_vkVoidFunction) vkDestroyInstance;
    if (!strcmp(name, "EnumeratePhysicalDevices"))
        return (PFN_vkVoidFunction) vkEnumeratePhysicalDevices;
    if (!strcmp(name, "GetPhysicalDeviceFeatures"))
        return (PFN_vkVoidFunction) vkGetPhysicalDeviceFeatures;
    if (!strcmp(name, "GetPhysicalDeviceFormatProperties"))
        return (PFN_vkVoidFunction) vkGetPhysicalDeviceFormatProperties;
    if (!strcmp(name, "GetPhysicalDeviceImageFormatProperties"))
        return (PFN_vkVoidFunction) vkGetPhysicalDeviceImageFormatProperties;
    if (!strcmp(name, "GetPhysicalDeviceProperties"))
        return (PFN_vkVoidFunction) vkGetPhysicalDeviceProperties;
    if (!strcmp(name, "GetPhysicalDeviceQueueFamilyProperties"))
        return (PFN_vkVoidFunction) vkGetPhysicalDeviceQueueFamilyProperties;
    if (!strcmp(name, "GetPhysicalDeviceMemoryProperties"))
        return (PFN_vkVoidFunction) vkGetPhysicalDeviceMemoryProperties;
    if (!strcmp(name, "GetPhysicalDeviceSparseImageFormatProperties"))
        return (PFN_vkVoidFunction) vkGetPhysicalDeviceSparseImageFormatProperties;
    if (!strcmp(name, "EnumerateDeviceExtensionProperties"))
        return (PFN_vkVoidFunction)vkEnumerateDeviceExtensionProperties;
    return NULL;
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice device, const char* funcName)
{
    PFN_vkVoidFunction addr;


    if (!strcmp("vkGetDeviceProcAddr", funcName)) {
        return (PFN_vkVoidFunction) vkGetDeviceProcAddr;
    }

    addr = layer_intercept_proc(funcName);
    if (addr)
        return addr;
    if (device == VK_NULL_HANDLE) {
        return NULL;
    }

    if (!strcmp("vkCreateSwapchainKHR", funcName))
        return reinterpret_cast<PFN_vkVoidFunction>(vkCreateSwapchainKHR);
    if (!strcmp("vkDestroySwapchainKHR", funcName))
        return reinterpret_cast<PFN_vkVoidFunction>(vkDestroySwapchainKHR);
    if (!strcmp("vkGetSwapchainImagesKHR", funcName))
        return reinterpret_cast<PFN_vkVoidFunction>(vkGetSwapchainImagesKHR);
    if (!strcmp("vkAcquireNextImageKHR", funcName))
        return reinterpret_cast<PFN_vkVoidFunction>(vkAcquireNextImageKHR);
    if (!strcmp("vkQueuePresentKHR", funcName))
        return reinterpret_cast<PFN_vkVoidFunction>(vkQueuePresentKHR);

    VkLayerDispatchTable *pDisp =  device_dispatch_table(device);
    if (pDisp->GetDeviceProcAddr == NULL)
    {
        return NULL;
    }

    return pDisp->GetDeviceProcAddr(device, funcName);
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char* funcName)
{
    PFN_vkVoidFunction addr;

    if (!strcmp(funcName, "vkCreateInstance"))
        return (PFN_vkVoidFunction) vkCreateInstance;
    if (!strcmp(funcName, "vkCreateDevice"))
        return (PFN_vkVoidFunction) vkCreateDevice;

    if (instance == VK_NULL_HANDLE) {
        return NULL;
    }

    addr = layer_intercept_instance_proc(funcName);
    if (addr)
        return addr;

    wrapped_inst_obj *inst;
    (void)unwrap_instance(instance, &inst);
    VkLayerInstanceDispatchTable* pTable = &inst->layer_disp;

    // EXT_debug_report
    if (!strcmp(funcName, "vkCreateDebugReportCallbackEXT"))
        return (PFN_vkVoidFunction)vkCreateDebugReportCallbackEXT;
    if (!strcmp(funcName, "vkDestroyDebugReportCallbackEXT"))
        return (PFN_vkVoidFunction)vkDestroyDebugReportCallbackEXT;
    if (!strcmp(funcName, "vkDebugReportMessageEXT"))
        return (PFN_vkVoidFunction)vkDebugReportMessageEXT;

    //KHR_surface
    if (!strcmp("vkDestroySurfaceKHR", funcName))
        return reinterpret_cast<PFN_vkVoidFunction>(vkDestroySurfaceKHR);
    if (!strcmp("vkGetPhysicalDeviceSurfaceSupportKHR", funcName))
        return reinterpret_cast<PFN_vkVoidFunction>(vkGetPhysicalDeviceSurfaceSupportKHR);
    if (!strcmp("vkGetPhysicalDeviceSurfaceCapabilitiesKHR", funcName))
        return reinterpret_cast<PFN_vkVoidFunction>(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
    if (!strcmp("vkGetPhysicalDeviceSurfaceFormatsKHR", funcName))
        return reinterpret_cast<PFN_vkVoidFunction>(vkGetPhysicalDeviceSurfaceFormatsKHR);
    if (!strcmp("vkGetPhysicalDeviceSurfacePresentModesKHR", funcName))
        return reinterpret_cast<PFN_vkVoidFunction>(vkGetPhysicalDeviceSurfacePresentModesKHR);

    // KHR_XXX_surface
#ifdef VK_USE_PLATFORM_XCB_KHR
    if (!strcmp("vkCreateXcbSurfaceKHR", funcName))
        return reinterpret_cast<PFN_vkVoidFunction>(vkCreateXcbSurfaceKHR);
    if (!strcmp("vkGetPhysicalDeviceXcbPresentationSupportKHR", funcName))
        return reinterpret_cast<PFN_vkVoidFunction>(vkGetPhysicalDeviceXcbPresentationSupportKHR);
#endif // VK_USE_PLATFORM_XCB_KHR
#ifdef VK_USE_PLATFORM_XLIB_KHR
    if (!strcmp("vkCreateXlibSurfaceKHR", funcName))
        return reinterpret_cast<PFN_vkVoidFunction>(vkCreateXlibSurfaceKHR);
    if (!strcmp("vkGetPhysicalDeviceXlibPresentationSupportKHR", funcName))
        return reinterpret_cast<PFN_vkVoidFunction>(vkGetPhysicalDeviceXlibPresentationSupportKHR);
#endif // VK_USE_PLATFORM_XLIB_KHR
#ifdef VK_USE_PLATFORM_MIR_KHR
    if (!strcmp("vkCreateMirSurfaceKHR", funcName))
        return reinterpret_cast<PFN_vkVoidFunction>(vkCreateMirSurfaceKHR);
    if (!strcmp("vkGetPhysicalDeviceMirPresentationSupportKHR", funcName))
        return reinterpret_cast<PFN_vkVoidFunction>(vkGetPhysicalDeviceMirPresentationSupportKHR);
#endif // VK_USE_PLATFORM_MIR_KHR
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    if (!strcmp("vkCreateWaylandSurfaceKHR", funcName))
        return reinterpret_cast<PFN_vkVoidFunction>(vkCreateWaylandSurfaceKHR);
    if (!strcmp("vkGetPhysicalDeviceWaylandPresentationSupportKHR", funcName))
        return reinterpret_cast<PFN_vkVoidFunction>(vkGetPhysicalDeviceWaylandPresentationSupportKHR);
#endif // VK_USE_PLATFORM_WAYLAND_KHR
#ifdef VK_USE_PLATFORM_WIN32_KHR
    if (!strcmp("vkCreateWin32SurfaceKHR", funcName))
        return reinterpret_cast<PFN_vkVoidFunction>(vkCreateWin32SurfaceKHR);
    if (!strcmp("vkGetPhysicalDeviceWin32PresentationSupportKHR", funcName))
        return reinterpret_cast<PFN_vkVoidFunction>(vkGetPhysicalDeviceWin32PresentationSupportKHR);
#endif // VK_USE_PLATFORM_WIN32_KHR

    if (pTable->GetInstanceProcAddr == NULL)
        return NULL;
    return pTable->GetInstanceProcAddr(instance, funcName);
}

} // namespace wrap_objects

// loader-layer interface v0, just wrappers since there is only a layer
VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char* funcName) {
    return wrap_objects::vkGetInstanceProcAddr(instance, funcName);
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice device, const char* funcName) {
    return wrap_objects::vkGetDeviceProcAddr(device, funcName);
}
VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL
vkEnumerateInstanceExtensionProperties(const char *pLayerName, uint32_t *pCount, VkExtensionProperties *pProperties) {
    assert(0); // TODO return wrap_objects::EnumerateInstanceExtensionProperties(pLayerName, pCount, pProperties);
    return VK_SUCCESS;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL
vkEnumerateInstanceLayerProperties(uint32_t *pCount, VkLayerProperties *pProperties) {
    assert(0); // TODO return wrap_objects::EnumerateInstanceLayerProperties(pCount, pProperties);
    return VK_SUCCESS;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL
vkEnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t *pCount, VkLayerProperties *pProperties) {
    // the layer command handles VK_NULL_HANDLE just fine internally
    assert(physicalDevice == VK_NULL_HANDLE);
    assert(0); // TODO return wrap_objects::EnumerateDeviceLayerProperties(VK_NULL_HANDLE, pCount, pProperties);
    return VK_SUCCESS;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice,
    const char *pLayerName, uint32_t *pCount,
    VkExtensionProperties *pProperties) {
    // the layer command handles VK_NULL_HANDLE just fine internally
    assert(physicalDevice == VK_NULL_HANDLE);
    return wrap_objects::vkEnumerateDeviceExtensionProperties(VK_NULL_HANDLE, pLayerName, pCount, pProperties);
}
