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
#include "vk_safe_struct.h"
#include <string.h>


safe_VkApplicationInfo::safe_VkApplicationInfo(const VkApplicationInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	pApplicationName(pInStruct->pApplicationName),
	applicationVersion(pInStruct->applicationVersion),
	pEngineName(pInStruct->pEngineName),
	engineVersion(pInStruct->engineVersion),
	apiVersion(pInStruct->apiVersion)
{
}

safe_VkApplicationInfo::safe_VkApplicationInfo()
{}

safe_VkApplicationInfo::safe_VkApplicationInfo(const safe_VkApplicationInfo& src)
{
    sType = src.sType;
    pNext = src.pNext;
    pApplicationName = src.pApplicationName;
    applicationVersion = src.applicationVersion;
    pEngineName = src.pEngineName;
    engineVersion = src.engineVersion;
    apiVersion = src.apiVersion;
}

safe_VkApplicationInfo::~safe_VkApplicationInfo()
{
}

void safe_VkApplicationInfo::initialize(const VkApplicationInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    pApplicationName = pInStruct->pApplicationName;
    applicationVersion = pInStruct->applicationVersion;
    pEngineName = pInStruct->pEngineName;
    engineVersion = pInStruct->engineVersion;
    apiVersion = pInStruct->apiVersion;
}

void safe_VkApplicationInfo::initialize(const safe_VkApplicationInfo* src)
{
    sType = src->sType;
    pNext = src->pNext;
    pApplicationName = src->pApplicationName;
    applicationVersion = src->applicationVersion;
    pEngineName = src->pEngineName;
    engineVersion = src->engineVersion;
    apiVersion = src->apiVersion;
}

safe_VkInstanceCreateInfo::safe_VkInstanceCreateInfo(const VkInstanceCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	enabledLayerCount(pInStruct->enabledLayerCount),
	ppEnabledLayerNames(pInStruct->ppEnabledLayerNames),
	enabledExtensionCount(pInStruct->enabledExtensionCount),
	ppEnabledExtensionNames(pInStruct->ppEnabledExtensionNames)
{
    if (pInStruct->pApplicationInfo)
        pApplicationInfo = new safe_VkApplicationInfo(pInStruct->pApplicationInfo);
    else
        pApplicationInfo = NULL;
}

safe_VkInstanceCreateInfo::safe_VkInstanceCreateInfo()
{}

safe_VkInstanceCreateInfo::safe_VkInstanceCreateInfo(const safe_VkInstanceCreateInfo& src)
{
    sType = src.sType;
    pNext = src.pNext;
    flags = src.flags;
    enabledLayerCount = src.enabledLayerCount;
    ppEnabledLayerNames = src.ppEnabledLayerNames;
    enabledExtensionCount = src.enabledExtensionCount;
    ppEnabledExtensionNames = src.ppEnabledExtensionNames;
    if (src.pApplicationInfo)
        pApplicationInfo = new safe_VkApplicationInfo(*src.pApplicationInfo);
    else
        pApplicationInfo = NULL;
}

safe_VkInstanceCreateInfo::~safe_VkInstanceCreateInfo()
{
    if (pApplicationInfo)
        delete pApplicationInfo;
}

void safe_VkInstanceCreateInfo::initialize(const VkInstanceCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    enabledLayerCount = pInStruct->enabledLayerCount;
    ppEnabledLayerNames = pInStruct->ppEnabledLayerNames;
    enabledExtensionCount = pInStruct->enabledExtensionCount;
    ppEnabledExtensionNames = pInStruct->ppEnabledExtensionNames;
    if (pInStruct->pApplicationInfo)
        pApplicationInfo = new safe_VkApplicationInfo(pInStruct->pApplicationInfo);
    else
        pApplicationInfo = NULL;
}

void safe_VkInstanceCreateInfo::initialize(const safe_VkInstanceCreateInfo* src)
{
    sType = src->sType;
    pNext = src->pNext;
    flags = src->flags;
    enabledLayerCount = src->enabledLayerCount;
    ppEnabledLayerNames = src->ppEnabledLayerNames;
    enabledExtensionCount = src->enabledExtensionCount;
    ppEnabledExtensionNames = src->ppEnabledExtensionNames;
    if (src->pApplicationInfo)
        pApplicationInfo = new safe_VkApplicationInfo(*src->pApplicationInfo);
    else
        pApplicationInfo = NULL;
}

safe_VkAllocationCallbacks::safe_VkAllocationCallbacks(const VkAllocationCallbacks* pInStruct) : 
	pUserData(pInStruct->pUserData),
	pfnAllocation(pInStruct->pfnAllocation),
	pfnReallocation(pInStruct->pfnReallocation),
	pfnFree(pInStruct->pfnFree),
	pfnInternalAllocation(pInStruct->pfnInternalAllocation),
	pfnInternalFree(pInStruct->pfnInternalFree)
{
}

safe_VkAllocationCallbacks::safe_VkAllocationCallbacks()
{}

safe_VkAllocationCallbacks::safe_VkAllocationCallbacks(const safe_VkAllocationCallbacks& src)
{
    pUserData = src.pUserData;
    pfnAllocation = src.pfnAllocation;
    pfnReallocation = src.pfnReallocation;
    pfnFree = src.pfnFree;
    pfnInternalAllocation = src.pfnInternalAllocation;
    pfnInternalFree = src.pfnInternalFree;
}

safe_VkAllocationCallbacks::~safe_VkAllocationCallbacks()
{
}

void safe_VkAllocationCallbacks::initialize(const VkAllocationCallbacks* pInStruct)
{
    pUserData = pInStruct->pUserData;
    pfnAllocation = pInStruct->pfnAllocation;
    pfnReallocation = pInStruct->pfnReallocation;
    pfnFree = pInStruct->pfnFree;
    pfnInternalAllocation = pInStruct->pfnInternalAllocation;
    pfnInternalFree = pInStruct->pfnInternalFree;
}

void safe_VkAllocationCallbacks::initialize(const safe_VkAllocationCallbacks* src)
{
    pUserData = src->pUserData;
    pfnAllocation = src->pfnAllocation;
    pfnReallocation = src->pfnReallocation;
    pfnFree = src->pfnFree;
    pfnInternalAllocation = src->pfnInternalAllocation;
    pfnInternalFree = src->pfnInternalFree;
}

safe_VkDeviceQueueCreateInfo::safe_VkDeviceQueueCreateInfo(const VkDeviceQueueCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	queueFamilyIndex(pInStruct->queueFamilyIndex),
	queueCount(pInStruct->queueCount),
	pQueuePriorities(nullptr)
{
    if (pInStruct->pQueuePriorities) {
        pQueuePriorities = new float[pInStruct->queueCount];
        memcpy ((void *)pQueuePriorities, (void *)pInStruct->pQueuePriorities, sizeof(float)*pInStruct->queueCount);
    }
}

safe_VkDeviceQueueCreateInfo::safe_VkDeviceQueueCreateInfo() : 
	pQueuePriorities(nullptr)
{}

safe_VkDeviceQueueCreateInfo::safe_VkDeviceQueueCreateInfo(const safe_VkDeviceQueueCreateInfo& src)
{
    sType = src.sType;
    pNext = src.pNext;
    flags = src.flags;
    queueFamilyIndex = src.queueFamilyIndex;
    queueCount = src.queueCount;
    pQueuePriorities = nullptr;
    if (src.pQueuePriorities) {
        pQueuePriorities = new float[src.queueCount];
        memcpy ((void *)pQueuePriorities, (void *)src.pQueuePriorities, sizeof(float)*src.queueCount);
    }
}

safe_VkDeviceQueueCreateInfo::~safe_VkDeviceQueueCreateInfo()
{
    if (pQueuePriorities)
        delete[] pQueuePriorities;
}

void safe_VkDeviceQueueCreateInfo::initialize(const VkDeviceQueueCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    queueFamilyIndex = pInStruct->queueFamilyIndex;
    queueCount = pInStruct->queueCount;
    pQueuePriorities = nullptr;
    if (pInStruct->pQueuePriorities) {
        pQueuePriorities = new float[pInStruct->queueCount];
        memcpy ((void *)pQueuePriorities, (void *)pInStruct->pQueuePriorities, sizeof(float)*pInStruct->queueCount);
    }
}

void safe_VkDeviceQueueCreateInfo::initialize(const safe_VkDeviceQueueCreateInfo* src)
{
    sType = src->sType;
    pNext = src->pNext;
    flags = src->flags;
    queueFamilyIndex = src->queueFamilyIndex;
    queueCount = src->queueCount;
    pQueuePriorities = nullptr;
    if (src->pQueuePriorities) {
        pQueuePriorities = new float[src->queueCount];
        memcpy ((void *)pQueuePriorities, (void *)src->pQueuePriorities, sizeof(float)*src->queueCount);
    }
}

safe_VkDeviceCreateInfo::safe_VkDeviceCreateInfo(const VkDeviceCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	queueCreateInfoCount(pInStruct->queueCreateInfoCount),
	pQueueCreateInfos(nullptr),
	enabledLayerCount(pInStruct->enabledLayerCount),
	ppEnabledLayerNames(pInStruct->ppEnabledLayerNames),
	enabledExtensionCount(pInStruct->enabledExtensionCount),
	ppEnabledExtensionNames(pInStruct->ppEnabledExtensionNames),
	pEnabledFeatures(nullptr)
{
    if (queueCreateInfoCount && pInStruct->pQueueCreateInfos) {
        pQueueCreateInfos = new safe_VkDeviceQueueCreateInfo[queueCreateInfoCount];
        for (uint32_t i=0; i<queueCreateInfoCount; ++i) {
            pQueueCreateInfos[i].initialize(&pInStruct->pQueueCreateInfos[i]);
        }
    }
    if (pInStruct->pEnabledFeatures) {
        pEnabledFeatures = new VkPhysicalDeviceFeatures(*pInStruct->pEnabledFeatures);
    }
}

safe_VkDeviceCreateInfo::safe_VkDeviceCreateInfo() : 
	pQueueCreateInfos(nullptr),
	pEnabledFeatures(nullptr)
{}

safe_VkDeviceCreateInfo::safe_VkDeviceCreateInfo(const safe_VkDeviceCreateInfo& src)
{
    sType = src.sType;
    pNext = src.pNext;
    flags = src.flags;
    queueCreateInfoCount = src.queueCreateInfoCount;
    pQueueCreateInfos = nullptr;
    enabledLayerCount = src.enabledLayerCount;
    ppEnabledLayerNames = src.ppEnabledLayerNames;
    enabledExtensionCount = src.enabledExtensionCount;
    ppEnabledExtensionNames = src.ppEnabledExtensionNames;
    pEnabledFeatures = nullptr;
    if (queueCreateInfoCount && src.pQueueCreateInfos) {
        pQueueCreateInfos = new safe_VkDeviceQueueCreateInfo[queueCreateInfoCount];
        for (uint32_t i=0; i<queueCreateInfoCount; ++i) {
            pQueueCreateInfos[i].initialize(&src.pQueueCreateInfos[i]);
        }
    }
    if (src.pEnabledFeatures) {
        pEnabledFeatures = new VkPhysicalDeviceFeatures(*src.pEnabledFeatures);
    }
}

safe_VkDeviceCreateInfo::~safe_VkDeviceCreateInfo()
{
    if (pQueueCreateInfos)
        delete[] pQueueCreateInfos;
    if (pEnabledFeatures)
        delete pEnabledFeatures;
}

void safe_VkDeviceCreateInfo::initialize(const VkDeviceCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    queueCreateInfoCount = pInStruct->queueCreateInfoCount;
    pQueueCreateInfos = nullptr;
    enabledLayerCount = pInStruct->enabledLayerCount;
    ppEnabledLayerNames = pInStruct->ppEnabledLayerNames;
    enabledExtensionCount = pInStruct->enabledExtensionCount;
    ppEnabledExtensionNames = pInStruct->ppEnabledExtensionNames;
    pEnabledFeatures = nullptr;
    if (queueCreateInfoCount && pInStruct->pQueueCreateInfos) {
        pQueueCreateInfos = new safe_VkDeviceQueueCreateInfo[queueCreateInfoCount];
        for (uint32_t i=0; i<queueCreateInfoCount; ++i) {
            pQueueCreateInfos[i].initialize(&pInStruct->pQueueCreateInfos[i]);
        }
    }
    if (pInStruct->pEnabledFeatures) {
        pEnabledFeatures = new VkPhysicalDeviceFeatures(*pInStruct->pEnabledFeatures);
    }
}

void safe_VkDeviceCreateInfo::initialize(const safe_VkDeviceCreateInfo* src)
{
    sType = src->sType;
    pNext = src->pNext;
    flags = src->flags;
    queueCreateInfoCount = src->queueCreateInfoCount;
    pQueueCreateInfos = nullptr;
    enabledLayerCount = src->enabledLayerCount;
    ppEnabledLayerNames = src->ppEnabledLayerNames;
    enabledExtensionCount = src->enabledExtensionCount;
    ppEnabledExtensionNames = src->ppEnabledExtensionNames;
    pEnabledFeatures = nullptr;
    if (queueCreateInfoCount && src->pQueueCreateInfos) {
        pQueueCreateInfos = new safe_VkDeviceQueueCreateInfo[queueCreateInfoCount];
        for (uint32_t i=0; i<queueCreateInfoCount; ++i) {
            pQueueCreateInfos[i].initialize(&src->pQueueCreateInfos[i]);
        }
    }
    if (src->pEnabledFeatures) {
        pEnabledFeatures = new VkPhysicalDeviceFeatures(*src->pEnabledFeatures);
    }
}

safe_VkSubmitInfo::safe_VkSubmitInfo(const VkSubmitInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	waitSemaphoreCount(pInStruct->waitSemaphoreCount),
	pWaitSemaphores(nullptr),
	pWaitDstStageMask(nullptr),
	commandBufferCount(pInStruct->commandBufferCount),
	pCommandBuffers(nullptr),
	signalSemaphoreCount(pInStruct->signalSemaphoreCount),
	pSignalSemaphores(nullptr)
{
    if (waitSemaphoreCount && pInStruct->pWaitSemaphores) {
        pWaitSemaphores = new VkSemaphore[waitSemaphoreCount];
        for (uint32_t i=0; i<waitSemaphoreCount; ++i) {
            pWaitSemaphores[i] = pInStruct->pWaitSemaphores[i];
        }
    }
    if (pInStruct->pWaitDstStageMask) {
        pWaitDstStageMask = new VkPipelineStageFlags(*pInStruct->pWaitDstStageMask);
    }
    if (pInStruct->pCommandBuffers) {
        pCommandBuffers = new VkCommandBuffer[pInStruct->commandBufferCount];
        memcpy ((void *)pCommandBuffers, (void *)pInStruct->pCommandBuffers, sizeof(VkCommandBuffer)*pInStruct->commandBufferCount);
    }
    if (signalSemaphoreCount && pInStruct->pSignalSemaphores) {
        pSignalSemaphores = new VkSemaphore[signalSemaphoreCount];
        for (uint32_t i=0; i<signalSemaphoreCount; ++i) {
            pSignalSemaphores[i] = pInStruct->pSignalSemaphores[i];
        }
    }
}

safe_VkSubmitInfo::safe_VkSubmitInfo() : 
	pWaitSemaphores(nullptr),
	pWaitDstStageMask(nullptr),
	pCommandBuffers(nullptr),
	pSignalSemaphores(nullptr)
{}

safe_VkSubmitInfo::safe_VkSubmitInfo(const safe_VkSubmitInfo& src)
{
    sType = src.sType;
    pNext = src.pNext;
    waitSemaphoreCount = src.waitSemaphoreCount;
    pWaitSemaphores = nullptr;
    pWaitDstStageMask = nullptr;
    commandBufferCount = src.commandBufferCount;
    pCommandBuffers = nullptr;
    signalSemaphoreCount = src.signalSemaphoreCount;
    pSignalSemaphores = nullptr;
    if (waitSemaphoreCount && src.pWaitSemaphores) {
        pWaitSemaphores = new VkSemaphore[waitSemaphoreCount];
        for (uint32_t i=0; i<waitSemaphoreCount; ++i) {
            pWaitSemaphores[i] = src.pWaitSemaphores[i];
        }
    }
    if (src.pWaitDstStageMask) {
        pWaitDstStageMask = new VkPipelineStageFlags(*src.pWaitDstStageMask);
    }
    if (src.pCommandBuffers) {
        pCommandBuffers = new VkCommandBuffer[src.commandBufferCount];
        memcpy ((void *)pCommandBuffers, (void *)src.pCommandBuffers, sizeof(VkCommandBuffer)*src.commandBufferCount);
    }
    if (signalSemaphoreCount && src.pSignalSemaphores) {
        pSignalSemaphores = new VkSemaphore[signalSemaphoreCount];
        for (uint32_t i=0; i<signalSemaphoreCount; ++i) {
            pSignalSemaphores[i] = src.pSignalSemaphores[i];
        }
    }
}

safe_VkSubmitInfo::~safe_VkSubmitInfo()
{
    if (pWaitSemaphores)
        delete[] pWaitSemaphores;
    if (pWaitDstStageMask)
        delete pWaitDstStageMask;
    if (pCommandBuffers)
        delete[] pCommandBuffers;
    if (pSignalSemaphores)
        delete[] pSignalSemaphores;
}

void safe_VkSubmitInfo::initialize(const VkSubmitInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    waitSemaphoreCount = pInStruct->waitSemaphoreCount;
    pWaitSemaphores = nullptr;
    pWaitDstStageMask = nullptr;
    commandBufferCount = pInStruct->commandBufferCount;
    pCommandBuffers = nullptr;
    signalSemaphoreCount = pInStruct->signalSemaphoreCount;
    pSignalSemaphores = nullptr;
    if (waitSemaphoreCount && pInStruct->pWaitSemaphores) {
        pWaitSemaphores = new VkSemaphore[waitSemaphoreCount];
        for (uint32_t i=0; i<waitSemaphoreCount; ++i) {
            pWaitSemaphores[i] = pInStruct->pWaitSemaphores[i];
        }
    }
    if (pInStruct->pWaitDstStageMask) {
        pWaitDstStageMask = new VkPipelineStageFlags(*pInStruct->pWaitDstStageMask);
    }
    if (pInStruct->pCommandBuffers) {
        pCommandBuffers = new VkCommandBuffer[pInStruct->commandBufferCount];
        memcpy ((void *)pCommandBuffers, (void *)pInStruct->pCommandBuffers, sizeof(VkCommandBuffer)*pInStruct->commandBufferCount);
    }
    if (signalSemaphoreCount && pInStruct->pSignalSemaphores) {
        pSignalSemaphores = new VkSemaphore[signalSemaphoreCount];
        for (uint32_t i=0; i<signalSemaphoreCount; ++i) {
            pSignalSemaphores[i] = pInStruct->pSignalSemaphores[i];
        }
    }
}

void safe_VkSubmitInfo::initialize(const safe_VkSubmitInfo* src)
{
    sType = src->sType;
    pNext = src->pNext;
    waitSemaphoreCount = src->waitSemaphoreCount;
    pWaitSemaphores = nullptr;
    pWaitDstStageMask = nullptr;
    commandBufferCount = src->commandBufferCount;
    pCommandBuffers = nullptr;
    signalSemaphoreCount = src->signalSemaphoreCount;
    pSignalSemaphores = nullptr;
    if (waitSemaphoreCount && src->pWaitSemaphores) {
        pWaitSemaphores = new VkSemaphore[waitSemaphoreCount];
        for (uint32_t i=0; i<waitSemaphoreCount; ++i) {
            pWaitSemaphores[i] = src->pWaitSemaphores[i];
        }
    }
    if (src->pWaitDstStageMask) {
        pWaitDstStageMask = new VkPipelineStageFlags(*src->pWaitDstStageMask);
    }
    if (src->pCommandBuffers) {
        pCommandBuffers = new VkCommandBuffer[src->commandBufferCount];
        memcpy ((void *)pCommandBuffers, (void *)src->pCommandBuffers, sizeof(VkCommandBuffer)*src->commandBufferCount);
    }
    if (signalSemaphoreCount && src->pSignalSemaphores) {
        pSignalSemaphores = new VkSemaphore[signalSemaphoreCount];
        for (uint32_t i=0; i<signalSemaphoreCount; ++i) {
            pSignalSemaphores[i] = src->pSignalSemaphores[i];
        }
    }
}

safe_VkMemoryAllocateInfo::safe_VkMemoryAllocateInfo(const VkMemoryAllocateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	allocationSize(pInStruct->allocationSize),
	memoryTypeIndex(pInStruct->memoryTypeIndex)
{
}

safe_VkMemoryAllocateInfo::safe_VkMemoryAllocateInfo()
{}

safe_VkMemoryAllocateInfo::safe_VkMemoryAllocateInfo(const safe_VkMemoryAllocateInfo& src)
{
    sType = src.sType;
    pNext = src.pNext;
    allocationSize = src.allocationSize;
    memoryTypeIndex = src.memoryTypeIndex;
}

safe_VkMemoryAllocateInfo::~safe_VkMemoryAllocateInfo()
{
}

void safe_VkMemoryAllocateInfo::initialize(const VkMemoryAllocateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    allocationSize = pInStruct->allocationSize;
    memoryTypeIndex = pInStruct->memoryTypeIndex;
}

void safe_VkMemoryAllocateInfo::initialize(const safe_VkMemoryAllocateInfo* src)
{
    sType = src->sType;
    pNext = src->pNext;
    allocationSize = src->allocationSize;
    memoryTypeIndex = src->memoryTypeIndex;
}

safe_VkMappedMemoryRange::safe_VkMappedMemoryRange(const VkMappedMemoryRange* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	memory(pInStruct->memory),
	offset(pInStruct->offset),
	size(pInStruct->size)
{
}

safe_VkMappedMemoryRange::safe_VkMappedMemoryRange()
{}

safe_VkMappedMemoryRange::safe_VkMappedMemoryRange(const safe_VkMappedMemoryRange& src)
{
    sType = src.sType;
    pNext = src.pNext;
    memory = src.memory;
    offset = src.offset;
    size = src.size;
}

safe_VkMappedMemoryRange::~safe_VkMappedMemoryRange()
{
}

void safe_VkMappedMemoryRange::initialize(const VkMappedMemoryRange* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    memory = pInStruct->memory;
    offset = pInStruct->offset;
    size = pInStruct->size;
}

void safe_VkMappedMemoryRange::initialize(const safe_VkMappedMemoryRange* src)
{
    sType = src->sType;
    pNext = src->pNext;
    memory = src->memory;
    offset = src->offset;
    size = src->size;
}

safe_VkSparseBufferMemoryBindInfo::safe_VkSparseBufferMemoryBindInfo(const VkSparseBufferMemoryBindInfo* pInStruct) : 
	buffer(pInStruct->buffer),
	bindCount(pInStruct->bindCount),
	pBinds(nullptr)
{
    if (bindCount && pInStruct->pBinds) {
        pBinds = new VkSparseMemoryBind[bindCount];
        for (uint32_t i=0; i<bindCount; ++i) {
            pBinds[i] = pInStruct->pBinds[i];
        }
    }
}

safe_VkSparseBufferMemoryBindInfo::safe_VkSparseBufferMemoryBindInfo() : 
	pBinds(nullptr)
{}

safe_VkSparseBufferMemoryBindInfo::safe_VkSparseBufferMemoryBindInfo(const safe_VkSparseBufferMemoryBindInfo& src)
{
    buffer = src.buffer;
    bindCount = src.bindCount;
    pBinds = nullptr;
    if (bindCount && src.pBinds) {
        pBinds = new VkSparseMemoryBind[bindCount];
        for (uint32_t i=0; i<bindCount; ++i) {
            pBinds[i] = src.pBinds[i];
        }
    }
}

safe_VkSparseBufferMemoryBindInfo::~safe_VkSparseBufferMemoryBindInfo()
{
    if (pBinds)
        delete[] pBinds;
}

void safe_VkSparseBufferMemoryBindInfo::initialize(const VkSparseBufferMemoryBindInfo* pInStruct)
{
    buffer = pInStruct->buffer;
    bindCount = pInStruct->bindCount;
    pBinds = nullptr;
    if (bindCount && pInStruct->pBinds) {
        pBinds = new VkSparseMemoryBind[bindCount];
        for (uint32_t i=0; i<bindCount; ++i) {
            pBinds[i] = pInStruct->pBinds[i];
        }
    }
}

void safe_VkSparseBufferMemoryBindInfo::initialize(const safe_VkSparseBufferMemoryBindInfo* src)
{
    buffer = src->buffer;
    bindCount = src->bindCount;
    pBinds = nullptr;
    if (bindCount && src->pBinds) {
        pBinds = new VkSparseMemoryBind[bindCount];
        for (uint32_t i=0; i<bindCount; ++i) {
            pBinds[i] = src->pBinds[i];
        }
    }
}

safe_VkSparseImageOpaqueMemoryBindInfo::safe_VkSparseImageOpaqueMemoryBindInfo(const VkSparseImageOpaqueMemoryBindInfo* pInStruct) : 
	image(pInStruct->image),
	bindCount(pInStruct->bindCount),
	pBinds(nullptr)
{
    if (bindCount && pInStruct->pBinds) {
        pBinds = new VkSparseMemoryBind[bindCount];
        for (uint32_t i=0; i<bindCount; ++i) {
            pBinds[i] = pInStruct->pBinds[i];
        }
    }
}

safe_VkSparseImageOpaqueMemoryBindInfo::safe_VkSparseImageOpaqueMemoryBindInfo() : 
	pBinds(nullptr)
{}

safe_VkSparseImageOpaqueMemoryBindInfo::safe_VkSparseImageOpaqueMemoryBindInfo(const safe_VkSparseImageOpaqueMemoryBindInfo& src)
{
    image = src.image;
    bindCount = src.bindCount;
    pBinds = nullptr;
    if (bindCount && src.pBinds) {
        pBinds = new VkSparseMemoryBind[bindCount];
        for (uint32_t i=0; i<bindCount; ++i) {
            pBinds[i] = src.pBinds[i];
        }
    }
}

safe_VkSparseImageOpaqueMemoryBindInfo::~safe_VkSparseImageOpaqueMemoryBindInfo()
{
    if (pBinds)
        delete[] pBinds;
}

void safe_VkSparseImageOpaqueMemoryBindInfo::initialize(const VkSparseImageOpaqueMemoryBindInfo* pInStruct)
{
    image = pInStruct->image;
    bindCount = pInStruct->bindCount;
    pBinds = nullptr;
    if (bindCount && pInStruct->pBinds) {
        pBinds = new VkSparseMemoryBind[bindCount];
        for (uint32_t i=0; i<bindCount; ++i) {
            pBinds[i] = pInStruct->pBinds[i];
        }
    }
}

void safe_VkSparseImageOpaqueMemoryBindInfo::initialize(const safe_VkSparseImageOpaqueMemoryBindInfo* src)
{
    image = src->image;
    bindCount = src->bindCount;
    pBinds = nullptr;
    if (bindCount && src->pBinds) {
        pBinds = new VkSparseMemoryBind[bindCount];
        for (uint32_t i=0; i<bindCount; ++i) {
            pBinds[i] = src->pBinds[i];
        }
    }
}

safe_VkSparseImageMemoryBindInfo::safe_VkSparseImageMemoryBindInfo(const VkSparseImageMemoryBindInfo* pInStruct) : 
	image(pInStruct->image),
	bindCount(pInStruct->bindCount),
	pBinds(nullptr)
{
    if (bindCount && pInStruct->pBinds) {
        pBinds = new VkSparseImageMemoryBind[bindCount];
        for (uint32_t i=0; i<bindCount; ++i) {
            pBinds[i] = pInStruct->pBinds[i];
        }
    }
}

safe_VkSparseImageMemoryBindInfo::safe_VkSparseImageMemoryBindInfo() : 
	pBinds(nullptr)
{}

safe_VkSparseImageMemoryBindInfo::safe_VkSparseImageMemoryBindInfo(const safe_VkSparseImageMemoryBindInfo& src)
{
    image = src.image;
    bindCount = src.bindCount;
    pBinds = nullptr;
    if (bindCount && src.pBinds) {
        pBinds = new VkSparseImageMemoryBind[bindCount];
        for (uint32_t i=0; i<bindCount; ++i) {
            pBinds[i] = src.pBinds[i];
        }
    }
}

safe_VkSparseImageMemoryBindInfo::~safe_VkSparseImageMemoryBindInfo()
{
    if (pBinds)
        delete[] pBinds;
}

void safe_VkSparseImageMemoryBindInfo::initialize(const VkSparseImageMemoryBindInfo* pInStruct)
{
    image = pInStruct->image;
    bindCount = pInStruct->bindCount;
    pBinds = nullptr;
    if (bindCount && pInStruct->pBinds) {
        pBinds = new VkSparseImageMemoryBind[bindCount];
        for (uint32_t i=0; i<bindCount; ++i) {
            pBinds[i] = pInStruct->pBinds[i];
        }
    }
}

void safe_VkSparseImageMemoryBindInfo::initialize(const safe_VkSparseImageMemoryBindInfo* src)
{
    image = src->image;
    bindCount = src->bindCount;
    pBinds = nullptr;
    if (bindCount && src->pBinds) {
        pBinds = new VkSparseImageMemoryBind[bindCount];
        for (uint32_t i=0; i<bindCount; ++i) {
            pBinds[i] = src->pBinds[i];
        }
    }
}

safe_VkBindSparseInfo::safe_VkBindSparseInfo(const VkBindSparseInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	waitSemaphoreCount(pInStruct->waitSemaphoreCount),
	pWaitSemaphores(nullptr),
	bufferBindCount(pInStruct->bufferBindCount),
	pBufferBinds(nullptr),
	imageOpaqueBindCount(pInStruct->imageOpaqueBindCount),
	pImageOpaqueBinds(nullptr),
	imageBindCount(pInStruct->imageBindCount),
	pImageBinds(nullptr),
	signalSemaphoreCount(pInStruct->signalSemaphoreCount),
	pSignalSemaphores(nullptr)
{
    if (waitSemaphoreCount && pInStruct->pWaitSemaphores) {
        pWaitSemaphores = new VkSemaphore[waitSemaphoreCount];
        for (uint32_t i=0; i<waitSemaphoreCount; ++i) {
            pWaitSemaphores[i] = pInStruct->pWaitSemaphores[i];
        }
    }
    if (bufferBindCount && pInStruct->pBufferBinds) {
        pBufferBinds = new safe_VkSparseBufferMemoryBindInfo[bufferBindCount];
        for (uint32_t i=0; i<bufferBindCount; ++i) {
            pBufferBinds[i].initialize(&pInStruct->pBufferBinds[i]);
        }
    }
    if (imageOpaqueBindCount && pInStruct->pImageOpaqueBinds) {
        pImageOpaqueBinds = new safe_VkSparseImageOpaqueMemoryBindInfo[imageOpaqueBindCount];
        for (uint32_t i=0; i<imageOpaqueBindCount; ++i) {
            pImageOpaqueBinds[i].initialize(&pInStruct->pImageOpaqueBinds[i]);
        }
    }
    if (imageBindCount && pInStruct->pImageBinds) {
        pImageBinds = new safe_VkSparseImageMemoryBindInfo[imageBindCount];
        for (uint32_t i=0; i<imageBindCount; ++i) {
            pImageBinds[i].initialize(&pInStruct->pImageBinds[i]);
        }
    }
    if (signalSemaphoreCount && pInStruct->pSignalSemaphores) {
        pSignalSemaphores = new VkSemaphore[signalSemaphoreCount];
        for (uint32_t i=0; i<signalSemaphoreCount; ++i) {
            pSignalSemaphores[i] = pInStruct->pSignalSemaphores[i];
        }
    }
}

safe_VkBindSparseInfo::safe_VkBindSparseInfo() : 
	pWaitSemaphores(nullptr),
	pBufferBinds(nullptr),
	pImageOpaqueBinds(nullptr),
	pImageBinds(nullptr),
	pSignalSemaphores(nullptr)
{}

safe_VkBindSparseInfo::safe_VkBindSparseInfo(const safe_VkBindSparseInfo& src)
{
    sType = src.sType;
    pNext = src.pNext;
    waitSemaphoreCount = src.waitSemaphoreCount;
    pWaitSemaphores = nullptr;
    bufferBindCount = src.bufferBindCount;
    pBufferBinds = nullptr;
    imageOpaqueBindCount = src.imageOpaqueBindCount;
    pImageOpaqueBinds = nullptr;
    imageBindCount = src.imageBindCount;
    pImageBinds = nullptr;
    signalSemaphoreCount = src.signalSemaphoreCount;
    pSignalSemaphores = nullptr;
    if (waitSemaphoreCount && src.pWaitSemaphores) {
        pWaitSemaphores = new VkSemaphore[waitSemaphoreCount];
        for (uint32_t i=0; i<waitSemaphoreCount; ++i) {
            pWaitSemaphores[i] = src.pWaitSemaphores[i];
        }
    }
    if (bufferBindCount && src.pBufferBinds) {
        pBufferBinds = new safe_VkSparseBufferMemoryBindInfo[bufferBindCount];
        for (uint32_t i=0; i<bufferBindCount; ++i) {
            pBufferBinds[i].initialize(&src.pBufferBinds[i]);
        }
    }
    if (imageOpaqueBindCount && src.pImageOpaqueBinds) {
        pImageOpaqueBinds = new safe_VkSparseImageOpaqueMemoryBindInfo[imageOpaqueBindCount];
        for (uint32_t i=0; i<imageOpaqueBindCount; ++i) {
            pImageOpaqueBinds[i].initialize(&src.pImageOpaqueBinds[i]);
        }
    }
    if (imageBindCount && src.pImageBinds) {
        pImageBinds = new safe_VkSparseImageMemoryBindInfo[imageBindCount];
        for (uint32_t i=0; i<imageBindCount; ++i) {
            pImageBinds[i].initialize(&src.pImageBinds[i]);
        }
    }
    if (signalSemaphoreCount && src.pSignalSemaphores) {
        pSignalSemaphores = new VkSemaphore[signalSemaphoreCount];
        for (uint32_t i=0; i<signalSemaphoreCount; ++i) {
            pSignalSemaphores[i] = src.pSignalSemaphores[i];
        }
    }
}

safe_VkBindSparseInfo::~safe_VkBindSparseInfo()
{
    if (pWaitSemaphores)
        delete[] pWaitSemaphores;
    if (pBufferBinds)
        delete[] pBufferBinds;
    if (pImageOpaqueBinds)
        delete[] pImageOpaqueBinds;
    if (pImageBinds)
        delete[] pImageBinds;
    if (pSignalSemaphores)
        delete[] pSignalSemaphores;
}

void safe_VkBindSparseInfo::initialize(const VkBindSparseInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    waitSemaphoreCount = pInStruct->waitSemaphoreCount;
    pWaitSemaphores = nullptr;
    bufferBindCount = pInStruct->bufferBindCount;
    pBufferBinds = nullptr;
    imageOpaqueBindCount = pInStruct->imageOpaqueBindCount;
    pImageOpaqueBinds = nullptr;
    imageBindCount = pInStruct->imageBindCount;
    pImageBinds = nullptr;
    signalSemaphoreCount = pInStruct->signalSemaphoreCount;
    pSignalSemaphores = nullptr;
    if (waitSemaphoreCount && pInStruct->pWaitSemaphores) {
        pWaitSemaphores = new VkSemaphore[waitSemaphoreCount];
        for (uint32_t i=0; i<waitSemaphoreCount; ++i) {
            pWaitSemaphores[i] = pInStruct->pWaitSemaphores[i];
        }
    }
    if (bufferBindCount && pInStruct->pBufferBinds) {
        pBufferBinds = new safe_VkSparseBufferMemoryBindInfo[bufferBindCount];
        for (uint32_t i=0; i<bufferBindCount; ++i) {
            pBufferBinds[i].initialize(&pInStruct->pBufferBinds[i]);
        }
    }
    if (imageOpaqueBindCount && pInStruct->pImageOpaqueBinds) {
        pImageOpaqueBinds = new safe_VkSparseImageOpaqueMemoryBindInfo[imageOpaqueBindCount];
        for (uint32_t i=0; i<imageOpaqueBindCount; ++i) {
            pImageOpaqueBinds[i].initialize(&pInStruct->pImageOpaqueBinds[i]);
        }
    }
    if (imageBindCount && pInStruct->pImageBinds) {
        pImageBinds = new safe_VkSparseImageMemoryBindInfo[imageBindCount];
        for (uint32_t i=0; i<imageBindCount; ++i) {
            pImageBinds[i].initialize(&pInStruct->pImageBinds[i]);
        }
    }
    if (signalSemaphoreCount && pInStruct->pSignalSemaphores) {
        pSignalSemaphores = new VkSemaphore[signalSemaphoreCount];
        for (uint32_t i=0; i<signalSemaphoreCount; ++i) {
            pSignalSemaphores[i] = pInStruct->pSignalSemaphores[i];
        }
    }
}

void safe_VkBindSparseInfo::initialize(const safe_VkBindSparseInfo* src)
{
    sType = src->sType;
    pNext = src->pNext;
    waitSemaphoreCount = src->waitSemaphoreCount;
    pWaitSemaphores = nullptr;
    bufferBindCount = src->bufferBindCount;
    pBufferBinds = nullptr;
    imageOpaqueBindCount = src->imageOpaqueBindCount;
    pImageOpaqueBinds = nullptr;
    imageBindCount = src->imageBindCount;
    pImageBinds = nullptr;
    signalSemaphoreCount = src->signalSemaphoreCount;
    pSignalSemaphores = nullptr;
    if (waitSemaphoreCount && src->pWaitSemaphores) {
        pWaitSemaphores = new VkSemaphore[waitSemaphoreCount];
        for (uint32_t i=0; i<waitSemaphoreCount; ++i) {
            pWaitSemaphores[i] = src->pWaitSemaphores[i];
        }
    }
    if (bufferBindCount && src->pBufferBinds) {
        pBufferBinds = new safe_VkSparseBufferMemoryBindInfo[bufferBindCount];
        for (uint32_t i=0; i<bufferBindCount; ++i) {
            pBufferBinds[i].initialize(&src->pBufferBinds[i]);
        }
    }
    if (imageOpaqueBindCount && src->pImageOpaqueBinds) {
        pImageOpaqueBinds = new safe_VkSparseImageOpaqueMemoryBindInfo[imageOpaqueBindCount];
        for (uint32_t i=0; i<imageOpaqueBindCount; ++i) {
            pImageOpaqueBinds[i].initialize(&src->pImageOpaqueBinds[i]);
        }
    }
    if (imageBindCount && src->pImageBinds) {
        pImageBinds = new safe_VkSparseImageMemoryBindInfo[imageBindCount];
        for (uint32_t i=0; i<imageBindCount; ++i) {
            pImageBinds[i].initialize(&src->pImageBinds[i]);
        }
    }
    if (signalSemaphoreCount && src->pSignalSemaphores) {
        pSignalSemaphores = new VkSemaphore[signalSemaphoreCount];
        for (uint32_t i=0; i<signalSemaphoreCount; ++i) {
            pSignalSemaphores[i] = src->pSignalSemaphores[i];
        }
    }
}

safe_VkFenceCreateInfo::safe_VkFenceCreateInfo(const VkFenceCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags)
{
}

safe_VkFenceCreateInfo::safe_VkFenceCreateInfo()
{}

safe_VkFenceCreateInfo::safe_VkFenceCreateInfo(const safe_VkFenceCreateInfo& src)
{
    sType = src.sType;
    pNext = src.pNext;
    flags = src.flags;
}

safe_VkFenceCreateInfo::~safe_VkFenceCreateInfo()
{
}

void safe_VkFenceCreateInfo::initialize(const VkFenceCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
}

void safe_VkFenceCreateInfo::initialize(const safe_VkFenceCreateInfo* src)
{
    sType = src->sType;
    pNext = src->pNext;
    flags = src->flags;
}

safe_VkSemaphoreCreateInfo::safe_VkSemaphoreCreateInfo(const VkSemaphoreCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags)
{
}

safe_VkSemaphoreCreateInfo::safe_VkSemaphoreCreateInfo()
{}

safe_VkSemaphoreCreateInfo::safe_VkSemaphoreCreateInfo(const safe_VkSemaphoreCreateInfo& src)
{
    sType = src.sType;
    pNext = src.pNext;
    flags = src.flags;
}

safe_VkSemaphoreCreateInfo::~safe_VkSemaphoreCreateInfo()
{
}

void safe_VkSemaphoreCreateInfo::initialize(const VkSemaphoreCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
}

void safe_VkSemaphoreCreateInfo::initialize(const safe_VkSemaphoreCreateInfo* src)
{
    sType = src->sType;
    pNext = src->pNext;
    flags = src->flags;
}

safe_VkEventCreateInfo::safe_VkEventCreateInfo(const VkEventCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags)
{
}

safe_VkEventCreateInfo::safe_VkEventCreateInfo()
{}

safe_VkEventCreateInfo::safe_VkEventCreateInfo(const safe_VkEventCreateInfo& src)
{
    sType = src.sType;
    pNext = src.pNext;
    flags = src.flags;
}

safe_VkEventCreateInfo::~safe_VkEventCreateInfo()
{
}

void safe_VkEventCreateInfo::initialize(const VkEventCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
}

void safe_VkEventCreateInfo::initialize(const safe_VkEventCreateInfo* src)
{
    sType = src->sType;
    pNext = src->pNext;
    flags = src->flags;
}

safe_VkQueryPoolCreateInfo::safe_VkQueryPoolCreateInfo(const VkQueryPoolCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	queryType(pInStruct->queryType),
	queryCount(pInStruct->queryCount),
	pipelineStatistics(pInStruct->pipelineStatistics)
{
}

safe_VkQueryPoolCreateInfo::safe_VkQueryPoolCreateInfo()
{}

safe_VkQueryPoolCreateInfo::safe_VkQueryPoolCreateInfo(const safe_VkQueryPoolCreateInfo& src)
{
    sType = src.sType;
    pNext = src.pNext;
    flags = src.flags;
    queryType = src.queryType;
    queryCount = src.queryCount;
    pipelineStatistics = src.pipelineStatistics;
}

safe_VkQueryPoolCreateInfo::~safe_VkQueryPoolCreateInfo()
{
}

void safe_VkQueryPoolCreateInfo::initialize(const VkQueryPoolCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    queryType = pInStruct->queryType;
    queryCount = pInStruct->queryCount;
    pipelineStatistics = pInStruct->pipelineStatistics;
}

void safe_VkQueryPoolCreateInfo::initialize(const safe_VkQueryPoolCreateInfo* src)
{
    sType = src->sType;
    pNext = src->pNext;
    flags = src->flags;
    queryType = src->queryType;
    queryCount = src->queryCount;
    pipelineStatistics = src->pipelineStatistics;
}

safe_VkBufferCreateInfo::safe_VkBufferCreateInfo(const VkBufferCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	size(pInStruct->size),
	usage(pInStruct->usage),
	sharingMode(pInStruct->sharingMode),
	queueFamilyIndexCount(pInStruct->queueFamilyIndexCount),
	pQueueFamilyIndices(nullptr)
{
    if (pInStruct->pQueueFamilyIndices) {
        pQueueFamilyIndices = new uint32_t[pInStruct->queueFamilyIndexCount];
        memcpy ((void *)pQueueFamilyIndices, (void *)pInStruct->pQueueFamilyIndices, sizeof(uint32_t)*pInStruct->queueFamilyIndexCount);
    }
}

safe_VkBufferCreateInfo::safe_VkBufferCreateInfo() : 
	pQueueFamilyIndices(nullptr)
{}

safe_VkBufferCreateInfo::safe_VkBufferCreateInfo(const safe_VkBufferCreateInfo& src)
{
    sType = src.sType;
    pNext = src.pNext;
    flags = src.flags;
    size = src.size;
    usage = src.usage;
    sharingMode = src.sharingMode;
    queueFamilyIndexCount = src.queueFamilyIndexCount;
    pQueueFamilyIndices = nullptr;
    if (src.pQueueFamilyIndices) {
        pQueueFamilyIndices = new uint32_t[src.queueFamilyIndexCount];
        memcpy ((void *)pQueueFamilyIndices, (void *)src.pQueueFamilyIndices, sizeof(uint32_t)*src.queueFamilyIndexCount);
    }
}

safe_VkBufferCreateInfo::~safe_VkBufferCreateInfo()
{
    if (pQueueFamilyIndices)
        delete[] pQueueFamilyIndices;
}

void safe_VkBufferCreateInfo::initialize(const VkBufferCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    size = pInStruct->size;
    usage = pInStruct->usage;
    sharingMode = pInStruct->sharingMode;
    queueFamilyIndexCount = pInStruct->queueFamilyIndexCount;
    pQueueFamilyIndices = nullptr;
    if (pInStruct->pQueueFamilyIndices) {
        pQueueFamilyIndices = new uint32_t[pInStruct->queueFamilyIndexCount];
        memcpy ((void *)pQueueFamilyIndices, (void *)pInStruct->pQueueFamilyIndices, sizeof(uint32_t)*pInStruct->queueFamilyIndexCount);
    }
}

void safe_VkBufferCreateInfo::initialize(const safe_VkBufferCreateInfo* src)
{
    sType = src->sType;
    pNext = src->pNext;
    flags = src->flags;
    size = src->size;
    usage = src->usage;
    sharingMode = src->sharingMode;
    queueFamilyIndexCount = src->queueFamilyIndexCount;
    pQueueFamilyIndices = nullptr;
    if (src->pQueueFamilyIndices) {
        pQueueFamilyIndices = new uint32_t[src->queueFamilyIndexCount];
        memcpy ((void *)pQueueFamilyIndices, (void *)src->pQueueFamilyIndices, sizeof(uint32_t)*src->queueFamilyIndexCount);
    }
}

safe_VkBufferViewCreateInfo::safe_VkBufferViewCreateInfo(const VkBufferViewCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	buffer(pInStruct->buffer),
	format(pInStruct->format),
	offset(pInStruct->offset),
	range(pInStruct->range)
{
}

safe_VkBufferViewCreateInfo::safe_VkBufferViewCreateInfo()
{}

safe_VkBufferViewCreateInfo::safe_VkBufferViewCreateInfo(const safe_VkBufferViewCreateInfo& src)
{
    sType = src.sType;
    pNext = src.pNext;
    flags = src.flags;
    buffer = src.buffer;
    format = src.format;
    offset = src.offset;
    range = src.range;
}

safe_VkBufferViewCreateInfo::~safe_VkBufferViewCreateInfo()
{
}

void safe_VkBufferViewCreateInfo::initialize(const VkBufferViewCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    buffer = pInStruct->buffer;
    format = pInStruct->format;
    offset = pInStruct->offset;
    range = pInStruct->range;
}

void safe_VkBufferViewCreateInfo::initialize(const safe_VkBufferViewCreateInfo* src)
{
    sType = src->sType;
    pNext = src->pNext;
    flags = src->flags;
    buffer = src->buffer;
    format = src->format;
    offset = src->offset;
    range = src->range;
}

safe_VkImageCreateInfo::safe_VkImageCreateInfo(const VkImageCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	imageType(pInStruct->imageType),
	format(pInStruct->format),
	extent(pInStruct->extent),
	mipLevels(pInStruct->mipLevels),
	arrayLayers(pInStruct->arrayLayers),
	samples(pInStruct->samples),
	tiling(pInStruct->tiling),
	usage(pInStruct->usage),
	sharingMode(pInStruct->sharingMode),
	queueFamilyIndexCount(pInStruct->queueFamilyIndexCount),
	pQueueFamilyIndices(nullptr),
	initialLayout(pInStruct->initialLayout)
{
    if (pInStruct->pQueueFamilyIndices) {
        pQueueFamilyIndices = new uint32_t[pInStruct->queueFamilyIndexCount];
        memcpy ((void *)pQueueFamilyIndices, (void *)pInStruct->pQueueFamilyIndices, sizeof(uint32_t)*pInStruct->queueFamilyIndexCount);
    }
}

safe_VkImageCreateInfo::safe_VkImageCreateInfo() : 
	pQueueFamilyIndices(nullptr)
{}

safe_VkImageCreateInfo::safe_VkImageCreateInfo(const safe_VkImageCreateInfo& src)
{
    sType = src.sType;
    pNext = src.pNext;
    flags = src.flags;
    imageType = src.imageType;
    format = src.format;
    extent = src.extent;
    mipLevels = src.mipLevels;
    arrayLayers = src.arrayLayers;
    samples = src.samples;
    tiling = src.tiling;
    usage = src.usage;
    sharingMode = src.sharingMode;
    queueFamilyIndexCount = src.queueFamilyIndexCount;
    pQueueFamilyIndices = nullptr;
    initialLayout = src.initialLayout;
    if (src.pQueueFamilyIndices) {
        pQueueFamilyIndices = new uint32_t[src.queueFamilyIndexCount];
        memcpy ((void *)pQueueFamilyIndices, (void *)src.pQueueFamilyIndices, sizeof(uint32_t)*src.queueFamilyIndexCount);
    }
}

safe_VkImageCreateInfo::~safe_VkImageCreateInfo()
{
    if (pQueueFamilyIndices)
        delete[] pQueueFamilyIndices;
}

void safe_VkImageCreateInfo::initialize(const VkImageCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    imageType = pInStruct->imageType;
    format = pInStruct->format;
    extent = pInStruct->extent;
    mipLevels = pInStruct->mipLevels;
    arrayLayers = pInStruct->arrayLayers;
    samples = pInStruct->samples;
    tiling = pInStruct->tiling;
    usage = pInStruct->usage;
    sharingMode = pInStruct->sharingMode;
    queueFamilyIndexCount = pInStruct->queueFamilyIndexCount;
    pQueueFamilyIndices = nullptr;
    initialLayout = pInStruct->initialLayout;
    if (pInStruct->pQueueFamilyIndices) {
        pQueueFamilyIndices = new uint32_t[pInStruct->queueFamilyIndexCount];
        memcpy ((void *)pQueueFamilyIndices, (void *)pInStruct->pQueueFamilyIndices, sizeof(uint32_t)*pInStruct->queueFamilyIndexCount);
    }
}

void safe_VkImageCreateInfo::initialize(const safe_VkImageCreateInfo* src)
{
    sType = src->sType;
    pNext = src->pNext;
    flags = src->flags;
    imageType = src->imageType;
    format = src->format;
    extent = src->extent;
    mipLevels = src->mipLevels;
    arrayLayers = src->arrayLayers;
    samples = src->samples;
    tiling = src->tiling;
    usage = src->usage;
    sharingMode = src->sharingMode;
    queueFamilyIndexCount = src->queueFamilyIndexCount;
    pQueueFamilyIndices = nullptr;
    initialLayout = src->initialLayout;
    if (src->pQueueFamilyIndices) {
        pQueueFamilyIndices = new uint32_t[src->queueFamilyIndexCount];
        memcpy ((void *)pQueueFamilyIndices, (void *)src->pQueueFamilyIndices, sizeof(uint32_t)*src->queueFamilyIndexCount);
    }
}

safe_VkImageViewCreateInfo::safe_VkImageViewCreateInfo(const VkImageViewCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	image(pInStruct->image),
	viewType(pInStruct->viewType),
	format(pInStruct->format),
	components(pInStruct->components),
	subresourceRange(pInStruct->subresourceRange)
{
}

safe_VkImageViewCreateInfo::safe_VkImageViewCreateInfo()
{}

safe_VkImageViewCreateInfo::safe_VkImageViewCreateInfo(const safe_VkImageViewCreateInfo& src)
{
    sType = src.sType;
    pNext = src.pNext;
    flags = src.flags;
    image = src.image;
    viewType = src.viewType;
    format = src.format;
    components = src.components;
    subresourceRange = src.subresourceRange;
}

safe_VkImageViewCreateInfo::~safe_VkImageViewCreateInfo()
{
}

void safe_VkImageViewCreateInfo::initialize(const VkImageViewCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    image = pInStruct->image;
    viewType = pInStruct->viewType;
    format = pInStruct->format;
    components = pInStruct->components;
    subresourceRange = pInStruct->subresourceRange;
}

void safe_VkImageViewCreateInfo::initialize(const safe_VkImageViewCreateInfo* src)
{
    sType = src->sType;
    pNext = src->pNext;
    flags = src->flags;
    image = src->image;
    viewType = src->viewType;
    format = src->format;
    components = src->components;
    subresourceRange = src->subresourceRange;
}

safe_VkShaderModuleCreateInfo::safe_VkShaderModuleCreateInfo(const VkShaderModuleCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	codeSize(pInStruct->codeSize),
	pCode(nullptr)
{
    if (pInStruct->pCode) {
        pCode = new uint32_t(*pInStruct->pCode);
    }
}

safe_VkShaderModuleCreateInfo::safe_VkShaderModuleCreateInfo() : 
	pCode(nullptr)
{}

safe_VkShaderModuleCreateInfo::safe_VkShaderModuleCreateInfo(const safe_VkShaderModuleCreateInfo& src)
{
    sType = src.sType;
    pNext = src.pNext;
    flags = src.flags;
    codeSize = src.codeSize;
    pCode = nullptr;
    if (src.pCode) {
        pCode = new uint32_t(*src.pCode);
    }
}

safe_VkShaderModuleCreateInfo::~safe_VkShaderModuleCreateInfo()
{
    if (pCode)
        delete pCode;
}

void safe_VkShaderModuleCreateInfo::initialize(const VkShaderModuleCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    codeSize = pInStruct->codeSize;
    pCode = nullptr;
    if (pInStruct->pCode) {
        pCode = new uint32_t(*pInStruct->pCode);
    }
}

void safe_VkShaderModuleCreateInfo::initialize(const safe_VkShaderModuleCreateInfo* src)
{
    sType = src->sType;
    pNext = src->pNext;
    flags = src->flags;
    codeSize = src->codeSize;
    pCode = nullptr;
    if (src->pCode) {
        pCode = new uint32_t(*src->pCode);
    }
}

safe_VkPipelineCacheCreateInfo::safe_VkPipelineCacheCreateInfo(const VkPipelineCacheCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	initialDataSize(pInStruct->initialDataSize),
	pInitialData(pInStruct->pInitialData)
{
}

safe_VkPipelineCacheCreateInfo::safe_VkPipelineCacheCreateInfo()
{}

safe_VkPipelineCacheCreateInfo::safe_VkPipelineCacheCreateInfo(const safe_VkPipelineCacheCreateInfo& src)
{
    sType = src.sType;
    pNext = src.pNext;
    flags = src.flags;
    initialDataSize = src.initialDataSize;
    pInitialData = src.pInitialData;
}

safe_VkPipelineCacheCreateInfo::~safe_VkPipelineCacheCreateInfo()
{
}

void safe_VkPipelineCacheCreateInfo::initialize(const VkPipelineCacheCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    initialDataSize = pInStruct->initialDataSize;
    pInitialData = pInStruct->pInitialData;
}

void safe_VkPipelineCacheCreateInfo::initialize(const safe_VkPipelineCacheCreateInfo* src)
{
    sType = src->sType;
    pNext = src->pNext;
    flags = src->flags;
    initialDataSize = src->initialDataSize;
    pInitialData = src->pInitialData;
}

safe_VkSpecializationInfo::safe_VkSpecializationInfo(const VkSpecializationInfo* pInStruct) : 
	mapEntryCount(pInStruct->mapEntryCount),
	pMapEntries(nullptr),
	dataSize(pInStruct->dataSize),
	pData(pInStruct->pData)
{
    if (pInStruct->pMapEntries) {
        pMapEntries = new VkSpecializationMapEntry[pInStruct->mapEntryCount];
        memcpy ((void *)pMapEntries, (void *)pInStruct->pMapEntries, sizeof(VkSpecializationMapEntry)*pInStruct->mapEntryCount);
    }
}

safe_VkSpecializationInfo::safe_VkSpecializationInfo() : 
	pMapEntries(nullptr)
{}

safe_VkSpecializationInfo::safe_VkSpecializationInfo(const safe_VkSpecializationInfo& src)
{
    mapEntryCount = src.mapEntryCount;
    pMapEntries = nullptr;
    dataSize = src.dataSize;
    pData = src.pData;
    if (src.pMapEntries) {
        pMapEntries = new VkSpecializationMapEntry[src.mapEntryCount];
        memcpy ((void *)pMapEntries, (void *)src.pMapEntries, sizeof(VkSpecializationMapEntry)*src.mapEntryCount);
    }
}

safe_VkSpecializationInfo::~safe_VkSpecializationInfo()
{
    if (pMapEntries)
        delete[] pMapEntries;
}

void safe_VkSpecializationInfo::initialize(const VkSpecializationInfo* pInStruct)
{
    mapEntryCount = pInStruct->mapEntryCount;
    pMapEntries = nullptr;
    dataSize = pInStruct->dataSize;
    pData = pInStruct->pData;
    if (pInStruct->pMapEntries) {
        pMapEntries = new VkSpecializationMapEntry[pInStruct->mapEntryCount];
        memcpy ((void *)pMapEntries, (void *)pInStruct->pMapEntries, sizeof(VkSpecializationMapEntry)*pInStruct->mapEntryCount);
    }
}

void safe_VkSpecializationInfo::initialize(const safe_VkSpecializationInfo* src)
{
    mapEntryCount = src->mapEntryCount;
    pMapEntries = nullptr;
    dataSize = src->dataSize;
    pData = src->pData;
    if (src->pMapEntries) {
        pMapEntries = new VkSpecializationMapEntry[src->mapEntryCount];
        memcpy ((void *)pMapEntries, (void *)src->pMapEntries, sizeof(VkSpecializationMapEntry)*src->mapEntryCount);
    }
}

safe_VkPipelineShaderStageCreateInfo::safe_VkPipelineShaderStageCreateInfo(const VkPipelineShaderStageCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	stage(pInStruct->stage),
	module(pInStruct->module),
	pName(pInStruct->pName)
{
    if (pInStruct->pSpecializationInfo)
        pSpecializationInfo = new safe_VkSpecializationInfo(pInStruct->pSpecializationInfo);
    else
        pSpecializationInfo = NULL;
}

safe_VkPipelineShaderStageCreateInfo::safe_VkPipelineShaderStageCreateInfo()
{}

safe_VkPipelineShaderStageCreateInfo::safe_VkPipelineShaderStageCreateInfo(const safe_VkPipelineShaderStageCreateInfo& src)
{
    sType = src.sType;
    pNext = src.pNext;
    flags = src.flags;
    stage = src.stage;
    module = src.module;
    pName = src.pName;
    if (src.pSpecializationInfo)
        pSpecializationInfo = new safe_VkSpecializationInfo(*src.pSpecializationInfo);
    else
        pSpecializationInfo = NULL;
}

safe_VkPipelineShaderStageCreateInfo::~safe_VkPipelineShaderStageCreateInfo()
{
    if (pSpecializationInfo)
        delete pSpecializationInfo;
}

void safe_VkPipelineShaderStageCreateInfo::initialize(const VkPipelineShaderStageCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    stage = pInStruct->stage;
    module = pInStruct->module;
    pName = pInStruct->pName;
    if (pInStruct->pSpecializationInfo)
        pSpecializationInfo = new safe_VkSpecializationInfo(pInStruct->pSpecializationInfo);
    else
        pSpecializationInfo = NULL;
}

void safe_VkPipelineShaderStageCreateInfo::initialize(const safe_VkPipelineShaderStageCreateInfo* src)
{
    sType = src->sType;
    pNext = src->pNext;
    flags = src->flags;
    stage = src->stage;
    module = src->module;
    pName = src->pName;
    if (src->pSpecializationInfo)
        pSpecializationInfo = new safe_VkSpecializationInfo(*src->pSpecializationInfo);
    else
        pSpecializationInfo = NULL;
}

safe_VkPipelineVertexInputStateCreateInfo::safe_VkPipelineVertexInputStateCreateInfo(const VkPipelineVertexInputStateCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	vertexBindingDescriptionCount(pInStruct->vertexBindingDescriptionCount),
	pVertexBindingDescriptions(nullptr),
	vertexAttributeDescriptionCount(pInStruct->vertexAttributeDescriptionCount),
	pVertexAttributeDescriptions(nullptr)
{
    if (pInStruct->pVertexBindingDescriptions) {
        pVertexBindingDescriptions = new VkVertexInputBindingDescription[pInStruct->vertexBindingDescriptionCount];
        memcpy ((void *)pVertexBindingDescriptions, (void *)pInStruct->pVertexBindingDescriptions, sizeof(VkVertexInputBindingDescription)*pInStruct->vertexBindingDescriptionCount);
    }
    if (pInStruct->pVertexAttributeDescriptions) {
        pVertexAttributeDescriptions = new VkVertexInputAttributeDescription[pInStruct->vertexAttributeDescriptionCount];
        memcpy ((void *)pVertexAttributeDescriptions, (void *)pInStruct->pVertexAttributeDescriptions, sizeof(VkVertexInputAttributeDescription)*pInStruct->vertexAttributeDescriptionCount);
    }
}

safe_VkPipelineVertexInputStateCreateInfo::safe_VkPipelineVertexInputStateCreateInfo() : 
	pVertexBindingDescriptions(nullptr),
	pVertexAttributeDescriptions(nullptr)
{}

safe_VkPipelineVertexInputStateCreateInfo::safe_VkPipelineVertexInputStateCreateInfo(const safe_VkPipelineVertexInputStateCreateInfo& src)
{
    sType = src.sType;
    pNext = src.pNext;
    flags = src.flags;
    vertexBindingDescriptionCount = src.vertexBindingDescriptionCount;
    pVertexBindingDescriptions = nullptr;
    vertexAttributeDescriptionCount = src.vertexAttributeDescriptionCount;
    pVertexAttributeDescriptions = nullptr;
    if (src.pVertexBindingDescriptions) {
        pVertexBindingDescriptions = new VkVertexInputBindingDescription[src.vertexBindingDescriptionCount];
        memcpy ((void *)pVertexBindingDescriptions, (void *)src.pVertexBindingDescriptions, sizeof(VkVertexInputBindingDescription)*src.vertexBindingDescriptionCount);
    }
    if (src.pVertexAttributeDescriptions) {
        pVertexAttributeDescriptions = new VkVertexInputAttributeDescription[src.vertexAttributeDescriptionCount];
        memcpy ((void *)pVertexAttributeDescriptions, (void *)src.pVertexAttributeDescriptions, sizeof(VkVertexInputAttributeDescription)*src.vertexAttributeDescriptionCount);
    }
}

safe_VkPipelineVertexInputStateCreateInfo::~safe_VkPipelineVertexInputStateCreateInfo()
{
    if (pVertexBindingDescriptions)
        delete[] pVertexBindingDescriptions;
    if (pVertexAttributeDescriptions)
        delete[] pVertexAttributeDescriptions;
}

void safe_VkPipelineVertexInputStateCreateInfo::initialize(const VkPipelineVertexInputStateCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    vertexBindingDescriptionCount = pInStruct->vertexBindingDescriptionCount;
    pVertexBindingDescriptions = nullptr;
    vertexAttributeDescriptionCount = pInStruct->vertexAttributeDescriptionCount;
    pVertexAttributeDescriptions = nullptr;
    if (pInStruct->pVertexBindingDescriptions) {
        pVertexBindingDescriptions = new VkVertexInputBindingDescription[pInStruct->vertexBindingDescriptionCount];
        memcpy ((void *)pVertexBindingDescriptions, (void *)pInStruct->pVertexBindingDescriptions, sizeof(VkVertexInputBindingDescription)*pInStruct->vertexBindingDescriptionCount);
    }
    if (pInStruct->pVertexAttributeDescriptions) {
        pVertexAttributeDescriptions = new VkVertexInputAttributeDescription[pInStruct->vertexAttributeDescriptionCount];
        memcpy ((void *)pVertexAttributeDescriptions, (void *)pInStruct->pVertexAttributeDescriptions, sizeof(VkVertexInputAttributeDescription)*pInStruct->vertexAttributeDescriptionCount);
    }
}

void safe_VkPipelineVertexInputStateCreateInfo::initialize(const safe_VkPipelineVertexInputStateCreateInfo* src)
{
    sType = src->sType;
    pNext = src->pNext;
    flags = src->flags;
    vertexBindingDescriptionCount = src->vertexBindingDescriptionCount;
    pVertexBindingDescriptions = nullptr;
    vertexAttributeDescriptionCount = src->vertexAttributeDescriptionCount;
    pVertexAttributeDescriptions = nullptr;
    if (src->pVertexBindingDescriptions) {
        pVertexBindingDescriptions = new VkVertexInputBindingDescription[src->vertexBindingDescriptionCount];
        memcpy ((void *)pVertexBindingDescriptions, (void *)src->pVertexBindingDescriptions, sizeof(VkVertexInputBindingDescription)*src->vertexBindingDescriptionCount);
    }
    if (src->pVertexAttributeDescriptions) {
        pVertexAttributeDescriptions = new VkVertexInputAttributeDescription[src->vertexAttributeDescriptionCount];
        memcpy ((void *)pVertexAttributeDescriptions, (void *)src->pVertexAttributeDescriptions, sizeof(VkVertexInputAttributeDescription)*src->vertexAttributeDescriptionCount);
    }
}

safe_VkPipelineInputAssemblyStateCreateInfo::safe_VkPipelineInputAssemblyStateCreateInfo(const VkPipelineInputAssemblyStateCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	topology(pInStruct->topology),
	primitiveRestartEnable(pInStruct->primitiveRestartEnable)
{
}

safe_VkPipelineInputAssemblyStateCreateInfo::safe_VkPipelineInputAssemblyStateCreateInfo()
{}

safe_VkPipelineInputAssemblyStateCreateInfo::safe_VkPipelineInputAssemblyStateCreateInfo(const safe_VkPipelineInputAssemblyStateCreateInfo& src)
{
    sType = src.sType;
    pNext = src.pNext;
    flags = src.flags;
    topology = src.topology;
    primitiveRestartEnable = src.primitiveRestartEnable;
}

safe_VkPipelineInputAssemblyStateCreateInfo::~safe_VkPipelineInputAssemblyStateCreateInfo()
{
}

void safe_VkPipelineInputAssemblyStateCreateInfo::initialize(const VkPipelineInputAssemblyStateCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    topology = pInStruct->topology;
    primitiveRestartEnable = pInStruct->primitiveRestartEnable;
}

void safe_VkPipelineInputAssemblyStateCreateInfo::initialize(const safe_VkPipelineInputAssemblyStateCreateInfo* src)
{
    sType = src->sType;
    pNext = src->pNext;
    flags = src->flags;
    topology = src->topology;
    primitiveRestartEnable = src->primitiveRestartEnable;
}

safe_VkPipelineTessellationStateCreateInfo::safe_VkPipelineTessellationStateCreateInfo(const VkPipelineTessellationStateCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	patchControlPoints(pInStruct->patchControlPoints)
{
}

safe_VkPipelineTessellationStateCreateInfo::safe_VkPipelineTessellationStateCreateInfo()
{}

safe_VkPipelineTessellationStateCreateInfo::safe_VkPipelineTessellationStateCreateInfo(const safe_VkPipelineTessellationStateCreateInfo& src)
{
    sType = src.sType;
    pNext = src.pNext;
    flags = src.flags;
    patchControlPoints = src.patchControlPoints;
}

safe_VkPipelineTessellationStateCreateInfo::~safe_VkPipelineTessellationStateCreateInfo()
{
}

void safe_VkPipelineTessellationStateCreateInfo::initialize(const VkPipelineTessellationStateCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    patchControlPoints = pInStruct->patchControlPoints;
}

void safe_VkPipelineTessellationStateCreateInfo::initialize(const safe_VkPipelineTessellationStateCreateInfo* src)
{
    sType = src->sType;
    pNext = src->pNext;
    flags = src->flags;
    patchControlPoints = src->patchControlPoints;
}

safe_VkPipelineViewportStateCreateInfo::safe_VkPipelineViewportStateCreateInfo(const VkPipelineViewportStateCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	viewportCount(pInStruct->viewportCount),
	pViewports(nullptr),
	scissorCount(pInStruct->scissorCount),
	pScissors(nullptr)
{
    if (pInStruct->pViewports) {
        pViewports = new VkViewport[pInStruct->viewportCount];
        memcpy ((void *)pViewports, (void *)pInStruct->pViewports, sizeof(VkViewport)*pInStruct->viewportCount);
    }
    if (pInStruct->pScissors) {
        pScissors = new VkRect2D[pInStruct->scissorCount];
        memcpy ((void *)pScissors, (void *)pInStruct->pScissors, sizeof(VkRect2D)*pInStruct->scissorCount);
    }
}

safe_VkPipelineViewportStateCreateInfo::safe_VkPipelineViewportStateCreateInfo() : 
	pViewports(nullptr),
	pScissors(nullptr)
{}

safe_VkPipelineViewportStateCreateInfo::safe_VkPipelineViewportStateCreateInfo(const safe_VkPipelineViewportStateCreateInfo& src)
{
    sType = src.sType;
    pNext = src.pNext;
    flags = src.flags;
    viewportCount = src.viewportCount;
    pViewports = nullptr;
    scissorCount = src.scissorCount;
    pScissors = nullptr;
    if (src.pViewports) {
        pViewports = new VkViewport[src.viewportCount];
        memcpy ((void *)pViewports, (void *)src.pViewports, sizeof(VkViewport)*src.viewportCount);
    }
    if (src.pScissors) {
        pScissors = new VkRect2D[src.scissorCount];
        memcpy ((void *)pScissors, (void *)src.pScissors, sizeof(VkRect2D)*src.scissorCount);
    }
}

safe_VkPipelineViewportStateCreateInfo::~safe_VkPipelineViewportStateCreateInfo()
{
    if (pViewports)
        delete[] pViewports;
    if (pScissors)
        delete[] pScissors;
}

void safe_VkPipelineViewportStateCreateInfo::initialize(const VkPipelineViewportStateCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    viewportCount = pInStruct->viewportCount;
    pViewports = nullptr;
    scissorCount = pInStruct->scissorCount;
    pScissors = nullptr;
    if (pInStruct->pViewports) {
        pViewports = new VkViewport[pInStruct->viewportCount];
        memcpy ((void *)pViewports, (void *)pInStruct->pViewports, sizeof(VkViewport)*pInStruct->viewportCount);
    }
    if (pInStruct->pScissors) {
        pScissors = new VkRect2D[pInStruct->scissorCount];
        memcpy ((void *)pScissors, (void *)pInStruct->pScissors, sizeof(VkRect2D)*pInStruct->scissorCount);
    }
}

void safe_VkPipelineViewportStateCreateInfo::initialize(const safe_VkPipelineViewportStateCreateInfo* src)
{
    sType = src->sType;
    pNext = src->pNext;
    flags = src->flags;
    viewportCount = src->viewportCount;
    pViewports = nullptr;
    scissorCount = src->scissorCount;
    pScissors = nullptr;
    if (src->pViewports) {
        pViewports = new VkViewport[src->viewportCount];
        memcpy ((void *)pViewports, (void *)src->pViewports, sizeof(VkViewport)*src->viewportCount);
    }
    if (src->pScissors) {
        pScissors = new VkRect2D[src->scissorCount];
        memcpy ((void *)pScissors, (void *)src->pScissors, sizeof(VkRect2D)*src->scissorCount);
    }
}

safe_VkPipelineRasterizationStateCreateInfo::safe_VkPipelineRasterizationStateCreateInfo(const VkPipelineRasterizationStateCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	depthClampEnable(pInStruct->depthClampEnable),
	rasterizerDiscardEnable(pInStruct->rasterizerDiscardEnable),
	polygonMode(pInStruct->polygonMode),
	cullMode(pInStruct->cullMode),
	frontFace(pInStruct->frontFace),
	depthBiasEnable(pInStruct->depthBiasEnable),
	depthBiasConstantFactor(pInStruct->depthBiasConstantFactor),
	depthBiasClamp(pInStruct->depthBiasClamp),
	depthBiasSlopeFactor(pInStruct->depthBiasSlopeFactor),
	lineWidth(pInStruct->lineWidth)
{
}

safe_VkPipelineRasterizationStateCreateInfo::safe_VkPipelineRasterizationStateCreateInfo()
{}

safe_VkPipelineRasterizationStateCreateInfo::safe_VkPipelineRasterizationStateCreateInfo(const safe_VkPipelineRasterizationStateCreateInfo& src)
{
    sType = src.sType;
    pNext = src.pNext;
    flags = src.flags;
    depthClampEnable = src.depthClampEnable;
    rasterizerDiscardEnable = src.rasterizerDiscardEnable;
    polygonMode = src.polygonMode;
    cullMode = src.cullMode;
    frontFace = src.frontFace;
    depthBiasEnable = src.depthBiasEnable;
    depthBiasConstantFactor = src.depthBiasConstantFactor;
    depthBiasClamp = src.depthBiasClamp;
    depthBiasSlopeFactor = src.depthBiasSlopeFactor;
    lineWidth = src.lineWidth;
}

safe_VkPipelineRasterizationStateCreateInfo::~safe_VkPipelineRasterizationStateCreateInfo()
{
}

void safe_VkPipelineRasterizationStateCreateInfo::initialize(const VkPipelineRasterizationStateCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    depthClampEnable = pInStruct->depthClampEnable;
    rasterizerDiscardEnable = pInStruct->rasterizerDiscardEnable;
    polygonMode = pInStruct->polygonMode;
    cullMode = pInStruct->cullMode;
    frontFace = pInStruct->frontFace;
    depthBiasEnable = pInStruct->depthBiasEnable;
    depthBiasConstantFactor = pInStruct->depthBiasConstantFactor;
    depthBiasClamp = pInStruct->depthBiasClamp;
    depthBiasSlopeFactor = pInStruct->depthBiasSlopeFactor;
    lineWidth = pInStruct->lineWidth;
}

void safe_VkPipelineRasterizationStateCreateInfo::initialize(const safe_VkPipelineRasterizationStateCreateInfo* src)
{
    sType = src->sType;
    pNext = src->pNext;
    flags = src->flags;
    depthClampEnable = src->depthClampEnable;
    rasterizerDiscardEnable = src->rasterizerDiscardEnable;
    polygonMode = src->polygonMode;
    cullMode = src->cullMode;
    frontFace = src->frontFace;
    depthBiasEnable = src->depthBiasEnable;
    depthBiasConstantFactor = src->depthBiasConstantFactor;
    depthBiasClamp = src->depthBiasClamp;
    depthBiasSlopeFactor = src->depthBiasSlopeFactor;
    lineWidth = src->lineWidth;
}

safe_VkPipelineMultisampleStateCreateInfo::safe_VkPipelineMultisampleStateCreateInfo(const VkPipelineMultisampleStateCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	rasterizationSamples(pInStruct->rasterizationSamples),
	sampleShadingEnable(pInStruct->sampleShadingEnable),
	minSampleShading(pInStruct->minSampleShading),
	pSampleMask(nullptr),
	alphaToCoverageEnable(pInStruct->alphaToCoverageEnable),
	alphaToOneEnable(pInStruct->alphaToOneEnable)
{
    if (pInStruct->pSampleMask) {
        pSampleMask = new VkSampleMask(*pInStruct->pSampleMask);
    }
}

safe_VkPipelineMultisampleStateCreateInfo::safe_VkPipelineMultisampleStateCreateInfo() : 
	pSampleMask(nullptr)
{}

safe_VkPipelineMultisampleStateCreateInfo::safe_VkPipelineMultisampleStateCreateInfo(const safe_VkPipelineMultisampleStateCreateInfo& src)
{
    sType = src.sType;
    pNext = src.pNext;
    flags = src.flags;
    rasterizationSamples = src.rasterizationSamples;
    sampleShadingEnable = src.sampleShadingEnable;
    minSampleShading = src.minSampleShading;
    pSampleMask = nullptr;
    alphaToCoverageEnable = src.alphaToCoverageEnable;
    alphaToOneEnable = src.alphaToOneEnable;
    if (src.pSampleMask) {
        pSampleMask = new VkSampleMask(*src.pSampleMask);
    }
}

safe_VkPipelineMultisampleStateCreateInfo::~safe_VkPipelineMultisampleStateCreateInfo()
{
    if (pSampleMask)
        delete pSampleMask;
}

void safe_VkPipelineMultisampleStateCreateInfo::initialize(const VkPipelineMultisampleStateCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    rasterizationSamples = pInStruct->rasterizationSamples;
    sampleShadingEnable = pInStruct->sampleShadingEnable;
    minSampleShading = pInStruct->minSampleShading;
    pSampleMask = nullptr;
    alphaToCoverageEnable = pInStruct->alphaToCoverageEnable;
    alphaToOneEnable = pInStruct->alphaToOneEnable;
    if (pInStruct->pSampleMask) {
        pSampleMask = new VkSampleMask(*pInStruct->pSampleMask);
    }
}

void safe_VkPipelineMultisampleStateCreateInfo::initialize(const safe_VkPipelineMultisampleStateCreateInfo* src)
{
    sType = src->sType;
    pNext = src->pNext;
    flags = src->flags;
    rasterizationSamples = src->rasterizationSamples;
    sampleShadingEnable = src->sampleShadingEnable;
    minSampleShading = src->minSampleShading;
    pSampleMask = nullptr;
    alphaToCoverageEnable = src->alphaToCoverageEnable;
    alphaToOneEnable = src->alphaToOneEnable;
    if (src->pSampleMask) {
        pSampleMask = new VkSampleMask(*src->pSampleMask);
    }
}

safe_VkPipelineDepthStencilStateCreateInfo::safe_VkPipelineDepthStencilStateCreateInfo(const VkPipelineDepthStencilStateCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	depthTestEnable(pInStruct->depthTestEnable),
	depthWriteEnable(pInStruct->depthWriteEnable),
	depthCompareOp(pInStruct->depthCompareOp),
	depthBoundsTestEnable(pInStruct->depthBoundsTestEnable),
	stencilTestEnable(pInStruct->stencilTestEnable),
	front(pInStruct->front),
	back(pInStruct->back),
	minDepthBounds(pInStruct->minDepthBounds),
	maxDepthBounds(pInStruct->maxDepthBounds)
{
}

safe_VkPipelineDepthStencilStateCreateInfo::safe_VkPipelineDepthStencilStateCreateInfo()
{}

safe_VkPipelineDepthStencilStateCreateInfo::safe_VkPipelineDepthStencilStateCreateInfo(const safe_VkPipelineDepthStencilStateCreateInfo& src)
{
    sType = src.sType;
    pNext = src.pNext;
    flags = src.flags;
    depthTestEnable = src.depthTestEnable;
    depthWriteEnable = src.depthWriteEnable;
    depthCompareOp = src.depthCompareOp;
    depthBoundsTestEnable = src.depthBoundsTestEnable;
    stencilTestEnable = src.stencilTestEnable;
    front = src.front;
    back = src.back;
    minDepthBounds = src.minDepthBounds;
    maxDepthBounds = src.maxDepthBounds;
}

safe_VkPipelineDepthStencilStateCreateInfo::~safe_VkPipelineDepthStencilStateCreateInfo()
{
}

void safe_VkPipelineDepthStencilStateCreateInfo::initialize(const VkPipelineDepthStencilStateCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    depthTestEnable = pInStruct->depthTestEnable;
    depthWriteEnable = pInStruct->depthWriteEnable;
    depthCompareOp = pInStruct->depthCompareOp;
    depthBoundsTestEnable = pInStruct->depthBoundsTestEnable;
    stencilTestEnable = pInStruct->stencilTestEnable;
    front = pInStruct->front;
    back = pInStruct->back;
    minDepthBounds = pInStruct->minDepthBounds;
    maxDepthBounds = pInStruct->maxDepthBounds;
}

void safe_VkPipelineDepthStencilStateCreateInfo::initialize(const safe_VkPipelineDepthStencilStateCreateInfo* src)
{
    sType = src->sType;
    pNext = src->pNext;
    flags = src->flags;
    depthTestEnable = src->depthTestEnable;
    depthWriteEnable = src->depthWriteEnable;
    depthCompareOp = src->depthCompareOp;
    depthBoundsTestEnable = src->depthBoundsTestEnable;
    stencilTestEnable = src->stencilTestEnable;
    front = src->front;
    back = src->back;
    minDepthBounds = src->minDepthBounds;
    maxDepthBounds = src->maxDepthBounds;
}

safe_VkPipelineColorBlendStateCreateInfo::safe_VkPipelineColorBlendStateCreateInfo(const VkPipelineColorBlendStateCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	logicOpEnable(pInStruct->logicOpEnable),
	logicOp(pInStruct->logicOp),
	attachmentCount(pInStruct->attachmentCount),
	pAttachments(nullptr)
{
    if (pInStruct->pAttachments) {
        pAttachments = new VkPipelineColorBlendAttachmentState[pInStruct->attachmentCount];
        memcpy ((void *)pAttachments, (void *)pInStruct->pAttachments, sizeof(VkPipelineColorBlendAttachmentState)*pInStruct->attachmentCount);
    }
    for (uint32_t i=0; i<4; ++i) {
        blendConstants[i] = pInStruct->blendConstants[i];
    }
}

safe_VkPipelineColorBlendStateCreateInfo::safe_VkPipelineColorBlendStateCreateInfo() : 
	pAttachments(nullptr)
{}

safe_VkPipelineColorBlendStateCreateInfo::safe_VkPipelineColorBlendStateCreateInfo(const safe_VkPipelineColorBlendStateCreateInfo& src)
{
    sType = src.sType;
    pNext = src.pNext;
    flags = src.flags;
    logicOpEnable = src.logicOpEnable;
    logicOp = src.logicOp;
    attachmentCount = src.attachmentCount;
    pAttachments = nullptr;
    if (src.pAttachments) {
        pAttachments = new VkPipelineColorBlendAttachmentState[src.attachmentCount];
        memcpy ((void *)pAttachments, (void *)src.pAttachments, sizeof(VkPipelineColorBlendAttachmentState)*src.attachmentCount);
    }
    for (uint32_t i=0; i<4; ++i) {
        blendConstants[i] = src.blendConstants[i];
    }
}

safe_VkPipelineColorBlendStateCreateInfo::~safe_VkPipelineColorBlendStateCreateInfo()
{
    if (pAttachments)
        delete[] pAttachments;
}

void safe_VkPipelineColorBlendStateCreateInfo::initialize(const VkPipelineColorBlendStateCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    logicOpEnable = pInStruct->logicOpEnable;
    logicOp = pInStruct->logicOp;
    attachmentCount = pInStruct->attachmentCount;
    pAttachments = nullptr;
    if (pInStruct->pAttachments) {
        pAttachments = new VkPipelineColorBlendAttachmentState[pInStruct->attachmentCount];
        memcpy ((void *)pAttachments, (void *)pInStruct->pAttachments, sizeof(VkPipelineColorBlendAttachmentState)*pInStruct->attachmentCount);
    }
    for (uint32_t i=0; i<4; ++i) {
        blendConstants[i] = pInStruct->blendConstants[i];
    }
}

void safe_VkPipelineColorBlendStateCreateInfo::initialize(const safe_VkPipelineColorBlendStateCreateInfo* src)
{
    sType = src->sType;
    pNext = src->pNext;
    flags = src->flags;
    logicOpEnable = src->logicOpEnable;
    logicOp = src->logicOp;
    attachmentCount = src->attachmentCount;
    pAttachments = nullptr;
    if (src->pAttachments) {
        pAttachments = new VkPipelineColorBlendAttachmentState[src->attachmentCount];
        memcpy ((void *)pAttachments, (void *)src->pAttachments, sizeof(VkPipelineColorBlendAttachmentState)*src->attachmentCount);
    }
    for (uint32_t i=0; i<4; ++i) {
        blendConstants[i] = src->blendConstants[i];
    }
}

safe_VkPipelineDynamicStateCreateInfo::safe_VkPipelineDynamicStateCreateInfo(const VkPipelineDynamicStateCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	dynamicStateCount(pInStruct->dynamicStateCount),
	pDynamicStates(nullptr)
{
    if (pInStruct->pDynamicStates) {
        pDynamicStates = new VkDynamicState[pInStruct->dynamicStateCount];
        memcpy ((void *)pDynamicStates, (void *)pInStruct->pDynamicStates, sizeof(VkDynamicState)*pInStruct->dynamicStateCount);
    }
}

safe_VkPipelineDynamicStateCreateInfo::safe_VkPipelineDynamicStateCreateInfo() : 
	pDynamicStates(nullptr)
{}

safe_VkPipelineDynamicStateCreateInfo::safe_VkPipelineDynamicStateCreateInfo(const safe_VkPipelineDynamicStateCreateInfo& src)
{
    sType = src.sType;
    pNext = src.pNext;
    flags = src.flags;
    dynamicStateCount = src.dynamicStateCount;
    pDynamicStates = nullptr;
    if (src.pDynamicStates) {
        pDynamicStates = new VkDynamicState[src.dynamicStateCount];
        memcpy ((void *)pDynamicStates, (void *)src.pDynamicStates, sizeof(VkDynamicState)*src.dynamicStateCount);
    }
}

safe_VkPipelineDynamicStateCreateInfo::~safe_VkPipelineDynamicStateCreateInfo()
{
    if (pDynamicStates)
        delete[] pDynamicStates;
}

void safe_VkPipelineDynamicStateCreateInfo::initialize(const VkPipelineDynamicStateCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    dynamicStateCount = pInStruct->dynamicStateCount;
    pDynamicStates = nullptr;
    if (pInStruct->pDynamicStates) {
        pDynamicStates = new VkDynamicState[pInStruct->dynamicStateCount];
        memcpy ((void *)pDynamicStates, (void *)pInStruct->pDynamicStates, sizeof(VkDynamicState)*pInStruct->dynamicStateCount);
    }
}

void safe_VkPipelineDynamicStateCreateInfo::initialize(const safe_VkPipelineDynamicStateCreateInfo* src)
{
    sType = src->sType;
    pNext = src->pNext;
    flags = src->flags;
    dynamicStateCount = src->dynamicStateCount;
    pDynamicStates = nullptr;
    if (src->pDynamicStates) {
        pDynamicStates = new VkDynamicState[src->dynamicStateCount];
        memcpy ((void *)pDynamicStates, (void *)src->pDynamicStates, sizeof(VkDynamicState)*src->dynamicStateCount);
    }
}

safe_VkGraphicsPipelineCreateInfo::safe_VkGraphicsPipelineCreateInfo(const VkGraphicsPipelineCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	stageCount(pInStruct->stageCount),
	pStages(nullptr),
	layout(pInStruct->layout),
	renderPass(pInStruct->renderPass),
	subpass(pInStruct->subpass),
	basePipelineHandle(pInStruct->basePipelineHandle),
	basePipelineIndex(pInStruct->basePipelineIndex)
{
    if (stageCount && pInStruct->pStages) {
        pStages = new safe_VkPipelineShaderStageCreateInfo[stageCount];
        for (uint32_t i=0; i<stageCount; ++i) {
            pStages[i].initialize(&pInStruct->pStages[i]);
        }
    }
    if (pInStruct->pVertexInputState)
        pVertexInputState = new safe_VkPipelineVertexInputStateCreateInfo(pInStruct->pVertexInputState);
    else
        pVertexInputState = NULL;
    if (pInStruct->pInputAssemblyState)
        pInputAssemblyState = new safe_VkPipelineInputAssemblyStateCreateInfo(pInStruct->pInputAssemblyState);
    else
        pInputAssemblyState = NULL;
    if (pInStruct->pTessellationState)
        pTessellationState = new safe_VkPipelineTessellationStateCreateInfo(pInStruct->pTessellationState);
    else
        pTessellationState = NULL;
    if (pInStruct->pViewportState)
        pViewportState = new safe_VkPipelineViewportStateCreateInfo(pInStruct->pViewportState);
    else
        pViewportState = NULL;
    if (pInStruct->pRasterizationState)
        pRasterizationState = new safe_VkPipelineRasterizationStateCreateInfo(pInStruct->pRasterizationState);
    else
        pRasterizationState = NULL;
    if (pInStruct->pMultisampleState)
        pMultisampleState = new safe_VkPipelineMultisampleStateCreateInfo(pInStruct->pMultisampleState);
    else
        pMultisampleState = NULL;
    if (pInStruct->pDepthStencilState)
        pDepthStencilState = new safe_VkPipelineDepthStencilStateCreateInfo(pInStruct->pDepthStencilState);
    else
        pDepthStencilState = NULL;
    if (pInStruct->pColorBlendState)
        pColorBlendState = new safe_VkPipelineColorBlendStateCreateInfo(pInStruct->pColorBlendState);
    else
        pColorBlendState = NULL;
    if (pInStruct->pDynamicState)
        pDynamicState = new safe_VkPipelineDynamicStateCreateInfo(pInStruct->pDynamicState);
    else
        pDynamicState = NULL;
}

safe_VkGraphicsPipelineCreateInfo::safe_VkGraphicsPipelineCreateInfo() : 
	pStages(nullptr)
{}

safe_VkGraphicsPipelineCreateInfo::safe_VkGraphicsPipelineCreateInfo(const safe_VkGraphicsPipelineCreateInfo& src)
{
    sType = src.sType;
    pNext = src.pNext;
    flags = src.flags;
    stageCount = src.stageCount;
    pStages = nullptr;
    layout = src.layout;
    renderPass = src.renderPass;
    subpass = src.subpass;
    basePipelineHandle = src.basePipelineHandle;
    basePipelineIndex = src.basePipelineIndex;
    if (stageCount && src.pStages) {
        pStages = new safe_VkPipelineShaderStageCreateInfo[stageCount];
        for (uint32_t i=0; i<stageCount; ++i) {
            pStages[i].initialize(&src.pStages[i]);
        }
    }
    if (src.pVertexInputState)
        pVertexInputState = new safe_VkPipelineVertexInputStateCreateInfo(*src.pVertexInputState);
    else
        pVertexInputState = NULL;
    if (src.pInputAssemblyState)
        pInputAssemblyState = new safe_VkPipelineInputAssemblyStateCreateInfo(*src.pInputAssemblyState);
    else
        pInputAssemblyState = NULL;
    if (src.pTessellationState)
        pTessellationState = new safe_VkPipelineTessellationStateCreateInfo(*src.pTessellationState);
    else
        pTessellationState = NULL;
    if (src.pViewportState)
        pViewportState = new safe_VkPipelineViewportStateCreateInfo(*src.pViewportState);
    else
        pViewportState = NULL;
    if (src.pRasterizationState)
        pRasterizationState = new safe_VkPipelineRasterizationStateCreateInfo(*src.pRasterizationState);
    else
        pRasterizationState = NULL;
    if (src.pMultisampleState)
        pMultisampleState = new safe_VkPipelineMultisampleStateCreateInfo(*src.pMultisampleState);
    else
        pMultisampleState = NULL;
    if (src.pDepthStencilState)
        pDepthStencilState = new safe_VkPipelineDepthStencilStateCreateInfo(*src.pDepthStencilState);
    else
        pDepthStencilState = NULL;
    if (src.pColorBlendState)
        pColorBlendState = new safe_VkPipelineColorBlendStateCreateInfo(*src.pColorBlendState);
    else
        pColorBlendState = NULL;
    if (src.pDynamicState)
        pDynamicState = new safe_VkPipelineDynamicStateCreateInfo(*src.pDynamicState);
    else
        pDynamicState = NULL;
}

safe_VkGraphicsPipelineCreateInfo::~safe_VkGraphicsPipelineCreateInfo()
{
    if (pStages)
        delete[] pStages;
    if (pVertexInputState)
        delete pVertexInputState;
    if (pInputAssemblyState)
        delete pInputAssemblyState;
    if (pTessellationState)
        delete pTessellationState;
    if (pViewportState)
        delete pViewportState;
    if (pRasterizationState)
        delete pRasterizationState;
    if (pMultisampleState)
        delete pMultisampleState;
    if (pDepthStencilState)
        delete pDepthStencilState;
    if (pColorBlendState)
        delete pColorBlendState;
    if (pDynamicState)
        delete pDynamicState;
}

void safe_VkGraphicsPipelineCreateInfo::initialize(const VkGraphicsPipelineCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    stageCount = pInStruct->stageCount;
    pStages = nullptr;
    layout = pInStruct->layout;
    renderPass = pInStruct->renderPass;
    subpass = pInStruct->subpass;
    basePipelineHandle = pInStruct->basePipelineHandle;
    basePipelineIndex = pInStruct->basePipelineIndex;
    if (stageCount && pInStruct->pStages) {
        pStages = new safe_VkPipelineShaderStageCreateInfo[stageCount];
        for (uint32_t i=0; i<stageCount; ++i) {
            pStages[i].initialize(&pInStruct->pStages[i]);
        }
    }
    if (pInStruct->pVertexInputState)
        pVertexInputState = new safe_VkPipelineVertexInputStateCreateInfo(pInStruct->pVertexInputState);
    else
        pVertexInputState = NULL;
    if (pInStruct->pInputAssemblyState)
        pInputAssemblyState = new safe_VkPipelineInputAssemblyStateCreateInfo(pInStruct->pInputAssemblyState);
    else
        pInputAssemblyState = NULL;
    if (pInStruct->pTessellationState)
        pTessellationState = new safe_VkPipelineTessellationStateCreateInfo(pInStruct->pTessellationState);
    else
        pTessellationState = NULL;
    if (pInStruct->pViewportState)
        pViewportState = new safe_VkPipelineViewportStateCreateInfo(pInStruct->pViewportState);
    else
        pViewportState = NULL;
    if (pInStruct->pRasterizationState)
        pRasterizationState = new safe_VkPipelineRasterizationStateCreateInfo(pInStruct->pRasterizationState);
    else
        pRasterizationState = NULL;
    if (pInStruct->pMultisampleState)
        pMultisampleState = new safe_VkPipelineMultisampleStateCreateInfo(pInStruct->pMultisampleState);
    else
        pMultisampleState = NULL;
    if (pInStruct->pDepthStencilState)
        pDepthStencilState = new safe_VkPipelineDepthStencilStateCreateInfo(pInStruct->pDepthStencilState);
    else
        pDepthStencilState = NULL;
    if (pInStruct->pColorBlendState)
        pColorBlendState = new safe_VkPipelineColorBlendStateCreateInfo(pInStruct->pColorBlendState);
    else
        pColorBlendState = NULL;
    if (pInStruct->pDynamicState)
        pDynamicState = new safe_VkPipelineDynamicStateCreateInfo(pInStruct->pDynamicState);
    else
        pDynamicState = NULL;
}

void safe_VkGraphicsPipelineCreateInfo::initialize(const safe_VkGraphicsPipelineCreateInfo* src)
{
    sType = src->sType;
    pNext = src->pNext;
    flags = src->flags;
    stageCount = src->stageCount;
    pStages = nullptr;
    layout = src->layout;
    renderPass = src->renderPass;
    subpass = src->subpass;
    basePipelineHandle = src->basePipelineHandle;
    basePipelineIndex = src->basePipelineIndex;
    if (stageCount && src->pStages) {
        pStages = new safe_VkPipelineShaderStageCreateInfo[stageCount];
        for (uint32_t i=0; i<stageCount; ++i) {
            pStages[i].initialize(&src->pStages[i]);
        }
    }
    if (src->pVertexInputState)
        pVertexInputState = new safe_VkPipelineVertexInputStateCreateInfo(*src->pVertexInputState);
    else
        pVertexInputState = NULL;
    if (src->pInputAssemblyState)
        pInputAssemblyState = new safe_VkPipelineInputAssemblyStateCreateInfo(*src->pInputAssemblyState);
    else
        pInputAssemblyState = NULL;
    if (src->pTessellationState)
        pTessellationState = new safe_VkPipelineTessellationStateCreateInfo(*src->pTessellationState);
    else
        pTessellationState = NULL;
    if (src->pViewportState)
        pViewportState = new safe_VkPipelineViewportStateCreateInfo(*src->pViewportState);
    else
        pViewportState = NULL;
    if (src->pRasterizationState)
        pRasterizationState = new safe_VkPipelineRasterizationStateCreateInfo(*src->pRasterizationState);
    else
        pRasterizationState = NULL;
    if (src->pMultisampleState)
        pMultisampleState = new safe_VkPipelineMultisampleStateCreateInfo(*src->pMultisampleState);
    else
        pMultisampleState = NULL;
    if (src->pDepthStencilState)
        pDepthStencilState = new safe_VkPipelineDepthStencilStateCreateInfo(*src->pDepthStencilState);
    else
        pDepthStencilState = NULL;
    if (src->pColorBlendState)
        pColorBlendState = new safe_VkPipelineColorBlendStateCreateInfo(*src->pColorBlendState);
    else
        pColorBlendState = NULL;
    if (src->pDynamicState)
        pDynamicState = new safe_VkPipelineDynamicStateCreateInfo(*src->pDynamicState);
    else
        pDynamicState = NULL;
}

safe_VkComputePipelineCreateInfo::safe_VkComputePipelineCreateInfo(const VkComputePipelineCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	stage(&pInStruct->stage),
	layout(pInStruct->layout),
	basePipelineHandle(pInStruct->basePipelineHandle),
	basePipelineIndex(pInStruct->basePipelineIndex)
{
}

safe_VkComputePipelineCreateInfo::safe_VkComputePipelineCreateInfo()
{}

safe_VkComputePipelineCreateInfo::safe_VkComputePipelineCreateInfo(const safe_VkComputePipelineCreateInfo& src)
{
    sType = src.sType;
    pNext = src.pNext;
    flags = src.flags;
        stage.initialize(&src.stage);
    layout = src.layout;
    basePipelineHandle = src.basePipelineHandle;
    basePipelineIndex = src.basePipelineIndex;
}

safe_VkComputePipelineCreateInfo::~safe_VkComputePipelineCreateInfo()
{
}

void safe_VkComputePipelineCreateInfo::initialize(const VkComputePipelineCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
        stage.initialize(&pInStruct->stage);
    layout = pInStruct->layout;
    basePipelineHandle = pInStruct->basePipelineHandle;
    basePipelineIndex = pInStruct->basePipelineIndex;
}

void safe_VkComputePipelineCreateInfo::initialize(const safe_VkComputePipelineCreateInfo* src)
{
    sType = src->sType;
    pNext = src->pNext;
    flags = src->flags;
        stage.initialize(&src->stage);
    layout = src->layout;
    basePipelineHandle = src->basePipelineHandle;
    basePipelineIndex = src->basePipelineIndex;
}

safe_VkPipelineLayoutCreateInfo::safe_VkPipelineLayoutCreateInfo(const VkPipelineLayoutCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	setLayoutCount(pInStruct->setLayoutCount),
	pSetLayouts(nullptr),
	pushConstantRangeCount(pInStruct->pushConstantRangeCount),
	pPushConstantRanges(nullptr)
{
    if (setLayoutCount && pInStruct->pSetLayouts) {
        pSetLayouts = new VkDescriptorSetLayout[setLayoutCount];
        for (uint32_t i=0; i<setLayoutCount; ++i) {
            pSetLayouts[i] = pInStruct->pSetLayouts[i];
        }
    }
    if (pInStruct->pPushConstantRanges) {
        pPushConstantRanges = new VkPushConstantRange[pInStruct->pushConstantRangeCount];
        memcpy ((void *)pPushConstantRanges, (void *)pInStruct->pPushConstantRanges, sizeof(VkPushConstantRange)*pInStruct->pushConstantRangeCount);
    }
}

safe_VkPipelineLayoutCreateInfo::safe_VkPipelineLayoutCreateInfo() : 
	pSetLayouts(nullptr),
	pPushConstantRanges(nullptr)
{}

safe_VkPipelineLayoutCreateInfo::safe_VkPipelineLayoutCreateInfo(const safe_VkPipelineLayoutCreateInfo& src)
{
    sType = src.sType;
    pNext = src.pNext;
    flags = src.flags;
    setLayoutCount = src.setLayoutCount;
    pSetLayouts = nullptr;
    pushConstantRangeCount = src.pushConstantRangeCount;
    pPushConstantRanges = nullptr;
    if (setLayoutCount && src.pSetLayouts) {
        pSetLayouts = new VkDescriptorSetLayout[setLayoutCount];
        for (uint32_t i=0; i<setLayoutCount; ++i) {
            pSetLayouts[i] = src.pSetLayouts[i];
        }
    }
    if (src.pPushConstantRanges) {
        pPushConstantRanges = new VkPushConstantRange[src.pushConstantRangeCount];
        memcpy ((void *)pPushConstantRanges, (void *)src.pPushConstantRanges, sizeof(VkPushConstantRange)*src.pushConstantRangeCount);
    }
}

safe_VkPipelineLayoutCreateInfo::~safe_VkPipelineLayoutCreateInfo()
{
    if (pSetLayouts)
        delete[] pSetLayouts;
    if (pPushConstantRanges)
        delete[] pPushConstantRanges;
}

void safe_VkPipelineLayoutCreateInfo::initialize(const VkPipelineLayoutCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    setLayoutCount = pInStruct->setLayoutCount;
    pSetLayouts = nullptr;
    pushConstantRangeCount = pInStruct->pushConstantRangeCount;
    pPushConstantRanges = nullptr;
    if (setLayoutCount && pInStruct->pSetLayouts) {
        pSetLayouts = new VkDescriptorSetLayout[setLayoutCount];
        for (uint32_t i=0; i<setLayoutCount; ++i) {
            pSetLayouts[i] = pInStruct->pSetLayouts[i];
        }
    }
    if (pInStruct->pPushConstantRanges) {
        pPushConstantRanges = new VkPushConstantRange[pInStruct->pushConstantRangeCount];
        memcpy ((void *)pPushConstantRanges, (void *)pInStruct->pPushConstantRanges, sizeof(VkPushConstantRange)*pInStruct->pushConstantRangeCount);
    }
}

void safe_VkPipelineLayoutCreateInfo::initialize(const safe_VkPipelineLayoutCreateInfo* src)
{
    sType = src->sType;
    pNext = src->pNext;
    flags = src->flags;
    setLayoutCount = src->setLayoutCount;
    pSetLayouts = nullptr;
    pushConstantRangeCount = src->pushConstantRangeCount;
    pPushConstantRanges = nullptr;
    if (setLayoutCount && src->pSetLayouts) {
        pSetLayouts = new VkDescriptorSetLayout[setLayoutCount];
        for (uint32_t i=0; i<setLayoutCount; ++i) {
            pSetLayouts[i] = src->pSetLayouts[i];
        }
    }
    if (src->pPushConstantRanges) {
        pPushConstantRanges = new VkPushConstantRange[src->pushConstantRangeCount];
        memcpy ((void *)pPushConstantRanges, (void *)src->pPushConstantRanges, sizeof(VkPushConstantRange)*src->pushConstantRangeCount);
    }
}

safe_VkSamplerCreateInfo::safe_VkSamplerCreateInfo(const VkSamplerCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	magFilter(pInStruct->magFilter),
	minFilter(pInStruct->minFilter),
	mipmapMode(pInStruct->mipmapMode),
	addressModeU(pInStruct->addressModeU),
	addressModeV(pInStruct->addressModeV),
	addressModeW(pInStruct->addressModeW),
	mipLodBias(pInStruct->mipLodBias),
	anisotropyEnable(pInStruct->anisotropyEnable),
	maxAnisotropy(pInStruct->maxAnisotropy),
	compareEnable(pInStruct->compareEnable),
	compareOp(pInStruct->compareOp),
	minLod(pInStruct->minLod),
	maxLod(pInStruct->maxLod),
	borderColor(pInStruct->borderColor),
	unnormalizedCoordinates(pInStruct->unnormalizedCoordinates)
{
}

safe_VkSamplerCreateInfo::safe_VkSamplerCreateInfo()
{}

safe_VkSamplerCreateInfo::safe_VkSamplerCreateInfo(const safe_VkSamplerCreateInfo& src)
{
    sType = src.sType;
    pNext = src.pNext;
    flags = src.flags;
    magFilter = src.magFilter;
    minFilter = src.minFilter;
    mipmapMode = src.mipmapMode;
    addressModeU = src.addressModeU;
    addressModeV = src.addressModeV;
    addressModeW = src.addressModeW;
    mipLodBias = src.mipLodBias;
    anisotropyEnable = src.anisotropyEnable;
    maxAnisotropy = src.maxAnisotropy;
    compareEnable = src.compareEnable;
    compareOp = src.compareOp;
    minLod = src.minLod;
    maxLod = src.maxLod;
    borderColor = src.borderColor;
    unnormalizedCoordinates = src.unnormalizedCoordinates;
}

safe_VkSamplerCreateInfo::~safe_VkSamplerCreateInfo()
{
}

void safe_VkSamplerCreateInfo::initialize(const VkSamplerCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    magFilter = pInStruct->magFilter;
    minFilter = pInStruct->minFilter;
    mipmapMode = pInStruct->mipmapMode;
    addressModeU = pInStruct->addressModeU;
    addressModeV = pInStruct->addressModeV;
    addressModeW = pInStruct->addressModeW;
    mipLodBias = pInStruct->mipLodBias;
    anisotropyEnable = pInStruct->anisotropyEnable;
    maxAnisotropy = pInStruct->maxAnisotropy;
    compareEnable = pInStruct->compareEnable;
    compareOp = pInStruct->compareOp;
    minLod = pInStruct->minLod;
    maxLod = pInStruct->maxLod;
    borderColor = pInStruct->borderColor;
    unnormalizedCoordinates = pInStruct->unnormalizedCoordinates;
}

void safe_VkSamplerCreateInfo::initialize(const safe_VkSamplerCreateInfo* src)
{
    sType = src->sType;
    pNext = src->pNext;
    flags = src->flags;
    magFilter = src->magFilter;
    minFilter = src->minFilter;
    mipmapMode = src->mipmapMode;
    addressModeU = src->addressModeU;
    addressModeV = src->addressModeV;
    addressModeW = src->addressModeW;
    mipLodBias = src->mipLodBias;
    anisotropyEnable = src->anisotropyEnable;
    maxAnisotropy = src->maxAnisotropy;
    compareEnable = src->compareEnable;
    compareOp = src->compareOp;
    minLod = src->minLod;
    maxLod = src->maxLod;
    borderColor = src->borderColor;
    unnormalizedCoordinates = src->unnormalizedCoordinates;
}

safe_VkDescriptorSetLayoutBinding::safe_VkDescriptorSetLayoutBinding(const VkDescriptorSetLayoutBinding* pInStruct) : 
	binding(pInStruct->binding),
	descriptorType(pInStruct->descriptorType),
	descriptorCount(pInStruct->descriptorCount),
	stageFlags(pInStruct->stageFlags),
	pImmutableSamplers(nullptr)
{
    if (descriptorCount && pInStruct->pImmutableSamplers) {
        pImmutableSamplers = new VkSampler[descriptorCount];
        for (uint32_t i=0; i<descriptorCount; ++i) {
            pImmutableSamplers[i] = pInStruct->pImmutableSamplers[i];
        }
    }
}

safe_VkDescriptorSetLayoutBinding::safe_VkDescriptorSetLayoutBinding() : 
	pImmutableSamplers(nullptr)
{}

safe_VkDescriptorSetLayoutBinding::safe_VkDescriptorSetLayoutBinding(const safe_VkDescriptorSetLayoutBinding& src)
{
    binding = src.binding;
    descriptorType = src.descriptorType;
    descriptorCount = src.descriptorCount;
    stageFlags = src.stageFlags;
    pImmutableSamplers = nullptr;
    if (descriptorCount && src.pImmutableSamplers) {
        pImmutableSamplers = new VkSampler[descriptorCount];
        for (uint32_t i=0; i<descriptorCount; ++i) {
            pImmutableSamplers[i] = src.pImmutableSamplers[i];
        }
    }
}

safe_VkDescriptorSetLayoutBinding::~safe_VkDescriptorSetLayoutBinding()
{
    if (pImmutableSamplers)
        delete[] pImmutableSamplers;
}

void safe_VkDescriptorSetLayoutBinding::initialize(const VkDescriptorSetLayoutBinding* pInStruct)
{
    binding = pInStruct->binding;
    descriptorType = pInStruct->descriptorType;
    descriptorCount = pInStruct->descriptorCount;
    stageFlags = pInStruct->stageFlags;
    pImmutableSamplers = nullptr;
    if (descriptorCount && pInStruct->pImmutableSamplers) {
        pImmutableSamplers = new VkSampler[descriptorCount];
        for (uint32_t i=0; i<descriptorCount; ++i) {
            pImmutableSamplers[i] = pInStruct->pImmutableSamplers[i];
        }
    }
}

void safe_VkDescriptorSetLayoutBinding::initialize(const safe_VkDescriptorSetLayoutBinding* src)
{
    binding = src->binding;
    descriptorType = src->descriptorType;
    descriptorCount = src->descriptorCount;
    stageFlags = src->stageFlags;
    pImmutableSamplers = nullptr;
    if (descriptorCount && src->pImmutableSamplers) {
        pImmutableSamplers = new VkSampler[descriptorCount];
        for (uint32_t i=0; i<descriptorCount; ++i) {
            pImmutableSamplers[i] = src->pImmutableSamplers[i];
        }
    }
}

safe_VkDescriptorSetLayoutCreateInfo::safe_VkDescriptorSetLayoutCreateInfo(const VkDescriptorSetLayoutCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	bindingCount(pInStruct->bindingCount),
	pBindings(nullptr)
{
    if (bindingCount && pInStruct->pBindings) {
        pBindings = new safe_VkDescriptorSetLayoutBinding[bindingCount];
        for (uint32_t i=0; i<bindingCount; ++i) {
            pBindings[i].initialize(&pInStruct->pBindings[i]);
        }
    }
}

safe_VkDescriptorSetLayoutCreateInfo::safe_VkDescriptorSetLayoutCreateInfo() : 
	pBindings(nullptr)
{}

safe_VkDescriptorSetLayoutCreateInfo::safe_VkDescriptorSetLayoutCreateInfo(const safe_VkDescriptorSetLayoutCreateInfo& src)
{
    sType = src.sType;
    pNext = src.pNext;
    flags = src.flags;
    bindingCount = src.bindingCount;
    pBindings = nullptr;
    if (bindingCount && src.pBindings) {
        pBindings = new safe_VkDescriptorSetLayoutBinding[bindingCount];
        for (uint32_t i=0; i<bindingCount; ++i) {
            pBindings[i].initialize(&src.pBindings[i]);
        }
    }
}

safe_VkDescriptorSetLayoutCreateInfo::~safe_VkDescriptorSetLayoutCreateInfo()
{
    if (pBindings)
        delete[] pBindings;
}

void safe_VkDescriptorSetLayoutCreateInfo::initialize(const VkDescriptorSetLayoutCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    bindingCount = pInStruct->bindingCount;
    pBindings = nullptr;
    if (bindingCount && pInStruct->pBindings) {
        pBindings = new safe_VkDescriptorSetLayoutBinding[bindingCount];
        for (uint32_t i=0; i<bindingCount; ++i) {
            pBindings[i].initialize(&pInStruct->pBindings[i]);
        }
    }
}

void safe_VkDescriptorSetLayoutCreateInfo::initialize(const safe_VkDescriptorSetLayoutCreateInfo* src)
{
    sType = src->sType;
    pNext = src->pNext;
    flags = src->flags;
    bindingCount = src->bindingCount;
    pBindings = nullptr;
    if (bindingCount && src->pBindings) {
        pBindings = new safe_VkDescriptorSetLayoutBinding[bindingCount];
        for (uint32_t i=0; i<bindingCount; ++i) {
            pBindings[i].initialize(&src->pBindings[i]);
        }
    }
}

safe_VkDescriptorPoolCreateInfo::safe_VkDescriptorPoolCreateInfo(const VkDescriptorPoolCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	maxSets(pInStruct->maxSets),
	poolSizeCount(pInStruct->poolSizeCount),
	pPoolSizes(nullptr)
{
    if (pInStruct->pPoolSizes) {
        pPoolSizes = new VkDescriptorPoolSize[pInStruct->poolSizeCount];
        memcpy ((void *)pPoolSizes, (void *)pInStruct->pPoolSizes, sizeof(VkDescriptorPoolSize)*pInStruct->poolSizeCount);
    }
}

safe_VkDescriptorPoolCreateInfo::safe_VkDescriptorPoolCreateInfo() : 
	pPoolSizes(nullptr)
{}

safe_VkDescriptorPoolCreateInfo::safe_VkDescriptorPoolCreateInfo(const safe_VkDescriptorPoolCreateInfo& src)
{
    sType = src.sType;
    pNext = src.pNext;
    flags = src.flags;
    maxSets = src.maxSets;
    poolSizeCount = src.poolSizeCount;
    pPoolSizes = nullptr;
    if (src.pPoolSizes) {
        pPoolSizes = new VkDescriptorPoolSize[src.poolSizeCount];
        memcpy ((void *)pPoolSizes, (void *)src.pPoolSizes, sizeof(VkDescriptorPoolSize)*src.poolSizeCount);
    }
}

safe_VkDescriptorPoolCreateInfo::~safe_VkDescriptorPoolCreateInfo()
{
    if (pPoolSizes)
        delete[] pPoolSizes;
}

void safe_VkDescriptorPoolCreateInfo::initialize(const VkDescriptorPoolCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    maxSets = pInStruct->maxSets;
    poolSizeCount = pInStruct->poolSizeCount;
    pPoolSizes = nullptr;
    if (pInStruct->pPoolSizes) {
        pPoolSizes = new VkDescriptorPoolSize[pInStruct->poolSizeCount];
        memcpy ((void *)pPoolSizes, (void *)pInStruct->pPoolSizes, sizeof(VkDescriptorPoolSize)*pInStruct->poolSizeCount);
    }
}

void safe_VkDescriptorPoolCreateInfo::initialize(const safe_VkDescriptorPoolCreateInfo* src)
{
    sType = src->sType;
    pNext = src->pNext;
    flags = src->flags;
    maxSets = src->maxSets;
    poolSizeCount = src->poolSizeCount;
    pPoolSizes = nullptr;
    if (src->pPoolSizes) {
        pPoolSizes = new VkDescriptorPoolSize[src->poolSizeCount];
        memcpy ((void *)pPoolSizes, (void *)src->pPoolSizes, sizeof(VkDescriptorPoolSize)*src->poolSizeCount);
    }
}

safe_VkDescriptorSetAllocateInfo::safe_VkDescriptorSetAllocateInfo(const VkDescriptorSetAllocateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	descriptorPool(pInStruct->descriptorPool),
	descriptorSetCount(pInStruct->descriptorSetCount),
	pSetLayouts(nullptr)
{
    if (descriptorSetCount && pInStruct->pSetLayouts) {
        pSetLayouts = new VkDescriptorSetLayout[descriptorSetCount];
        for (uint32_t i=0; i<descriptorSetCount; ++i) {
            pSetLayouts[i] = pInStruct->pSetLayouts[i];
        }
    }
}

safe_VkDescriptorSetAllocateInfo::safe_VkDescriptorSetAllocateInfo() : 
	pSetLayouts(nullptr)
{}

safe_VkDescriptorSetAllocateInfo::safe_VkDescriptorSetAllocateInfo(const safe_VkDescriptorSetAllocateInfo& src)
{
    sType = src.sType;
    pNext = src.pNext;
    descriptorPool = src.descriptorPool;
    descriptorSetCount = src.descriptorSetCount;
    pSetLayouts = nullptr;
    if (descriptorSetCount && src.pSetLayouts) {
        pSetLayouts = new VkDescriptorSetLayout[descriptorSetCount];
        for (uint32_t i=0; i<descriptorSetCount; ++i) {
            pSetLayouts[i] = src.pSetLayouts[i];
        }
    }
}

safe_VkDescriptorSetAllocateInfo::~safe_VkDescriptorSetAllocateInfo()
{
    if (pSetLayouts)
        delete[] pSetLayouts;
}

void safe_VkDescriptorSetAllocateInfo::initialize(const VkDescriptorSetAllocateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    descriptorPool = pInStruct->descriptorPool;
    descriptorSetCount = pInStruct->descriptorSetCount;
    pSetLayouts = nullptr;
    if (descriptorSetCount && pInStruct->pSetLayouts) {
        pSetLayouts = new VkDescriptorSetLayout[descriptorSetCount];
        for (uint32_t i=0; i<descriptorSetCount; ++i) {
            pSetLayouts[i] = pInStruct->pSetLayouts[i];
        }
    }
}

void safe_VkDescriptorSetAllocateInfo::initialize(const safe_VkDescriptorSetAllocateInfo* src)
{
    sType = src->sType;
    pNext = src->pNext;
    descriptorPool = src->descriptorPool;
    descriptorSetCount = src->descriptorSetCount;
    pSetLayouts = nullptr;
    if (descriptorSetCount && src->pSetLayouts) {
        pSetLayouts = new VkDescriptorSetLayout[descriptorSetCount];
        for (uint32_t i=0; i<descriptorSetCount; ++i) {
            pSetLayouts[i] = src->pSetLayouts[i];
        }
    }
}

safe_VkWriteDescriptorSet::safe_VkWriteDescriptorSet(const VkWriteDescriptorSet* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	dstSet(pInStruct->dstSet),
	dstBinding(pInStruct->dstBinding),
	dstArrayElement(pInStruct->dstArrayElement),
	descriptorCount(pInStruct->descriptorCount),
	descriptorType(pInStruct->descriptorType),
	pImageInfo(nullptr),
	pBufferInfo(nullptr),
	pTexelBufferView(nullptr)
{
    switch (descriptorType) {
        case VK_DESCRIPTOR_TYPE_SAMPLER:
        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
        case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
        if (descriptorCount && pInStruct->pImageInfo) {
            pImageInfo = new VkDescriptorImageInfo[descriptorCount];
            for (uint32_t i=0; i<descriptorCount; ++i) {
                pImageInfo[i] = pInStruct->pImageInfo[i];
            }
        }
        break;
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
        if (descriptorCount && pInStruct->pBufferInfo) {
            pBufferInfo = new VkDescriptorBufferInfo[descriptorCount];
            for (uint32_t i=0; i<descriptorCount; ++i) {
                pBufferInfo[i] = pInStruct->pBufferInfo[i];
            }
        }
        break;
        case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
        if (descriptorCount && pInStruct->pTexelBufferView) {
            pTexelBufferView = new VkBufferView[descriptorCount];
            for (uint32_t i=0; i<descriptorCount; ++i) {
                pTexelBufferView[i] = pInStruct->pTexelBufferView[i];
            }
        }
        break;
        default:
        break;
    }
}

safe_VkWriteDescriptorSet::safe_VkWriteDescriptorSet() : 
	pImageInfo(nullptr),
	pBufferInfo(nullptr),
	pTexelBufferView(nullptr)
{}

safe_VkWriteDescriptorSet::safe_VkWriteDescriptorSet(const safe_VkWriteDescriptorSet& src)
{
    sType = src.sType;
    pNext = src.pNext;
    dstSet = src.dstSet;
    dstBinding = src.dstBinding;
    dstArrayElement = src.dstArrayElement;
    descriptorCount = src.descriptorCount;
    descriptorType = src.descriptorType;
    pImageInfo = nullptr;
    pBufferInfo = nullptr;
    pTexelBufferView = nullptr;
    switch (descriptorType) {
        case VK_DESCRIPTOR_TYPE_SAMPLER:
        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
        case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
        if (descriptorCount && src.pImageInfo) {
            pImageInfo = new VkDescriptorImageInfo[descriptorCount];
            for (uint32_t i=0; i<descriptorCount; ++i) {
                pImageInfo[i] = src.pImageInfo[i];
            }
        }
        break;
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
        if (descriptorCount && src.pBufferInfo) {
            pBufferInfo = new VkDescriptorBufferInfo[descriptorCount];
            for (uint32_t i=0; i<descriptorCount; ++i) {
                pBufferInfo[i] = src.pBufferInfo[i];
            }
        }
        break;
        case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
        if (descriptorCount && src.pTexelBufferView) {
            pTexelBufferView = new VkBufferView[descriptorCount];
            for (uint32_t i=0; i<descriptorCount; ++i) {
                pTexelBufferView[i] = src.pTexelBufferView[i];
            }
        }
        break;
        default:
        break;
    }
}

safe_VkWriteDescriptorSet::~safe_VkWriteDescriptorSet()
{
    if (pImageInfo)
        delete[] pImageInfo;
    if (pBufferInfo)
        delete[] pBufferInfo;
    if (pTexelBufferView)
        delete[] pTexelBufferView;
}

void safe_VkWriteDescriptorSet::initialize(const VkWriteDescriptorSet* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    dstSet = pInStruct->dstSet;
    dstBinding = pInStruct->dstBinding;
    dstArrayElement = pInStruct->dstArrayElement;
    descriptorCount = pInStruct->descriptorCount;
    descriptorType = pInStruct->descriptorType;
    pImageInfo = nullptr;
    pBufferInfo = nullptr;
    pTexelBufferView = nullptr;
    switch (descriptorType) {
        case VK_DESCRIPTOR_TYPE_SAMPLER:
        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
        case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
        if (descriptorCount && pInStruct->pImageInfo) {
            pImageInfo = new VkDescriptorImageInfo[descriptorCount];
            for (uint32_t i=0; i<descriptorCount; ++i) {
                pImageInfo[i] = pInStruct->pImageInfo[i];
            }
        }
        break;
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
        if (descriptorCount && pInStruct->pBufferInfo) {
            pBufferInfo = new VkDescriptorBufferInfo[descriptorCount];
            for (uint32_t i=0; i<descriptorCount; ++i) {
                pBufferInfo[i] = pInStruct->pBufferInfo[i];
            }
        }
        break;
        case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
        if (descriptorCount && pInStruct->pTexelBufferView) {
            pTexelBufferView = new VkBufferView[descriptorCount];
            for (uint32_t i=0; i<descriptorCount; ++i) {
                pTexelBufferView[i] = pInStruct->pTexelBufferView[i];
            }
        }
        break;
        default:
        break;
    }
}

void safe_VkWriteDescriptorSet::initialize(const safe_VkWriteDescriptorSet* src)
{
    sType = src->sType;
    pNext = src->pNext;
    dstSet = src->dstSet;
    dstBinding = src->dstBinding;
    dstArrayElement = src->dstArrayElement;
    descriptorCount = src->descriptorCount;
    descriptorType = src->descriptorType;
    pImageInfo = nullptr;
    pBufferInfo = nullptr;
    pTexelBufferView = nullptr;
    switch (descriptorType) {
        case VK_DESCRIPTOR_TYPE_SAMPLER:
        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
        case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
        if (descriptorCount && src->pImageInfo) {
            pImageInfo = new VkDescriptorImageInfo[descriptorCount];
            for (uint32_t i=0; i<descriptorCount; ++i) {
                pImageInfo[i] = src->pImageInfo[i];
            }
        }
        break;
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
        if (descriptorCount && src->pBufferInfo) {
            pBufferInfo = new VkDescriptorBufferInfo[descriptorCount];
            for (uint32_t i=0; i<descriptorCount; ++i) {
                pBufferInfo[i] = src->pBufferInfo[i];
            }
        }
        break;
        case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
        if (descriptorCount && src->pTexelBufferView) {
            pTexelBufferView = new VkBufferView[descriptorCount];
            for (uint32_t i=0; i<descriptorCount; ++i) {
                pTexelBufferView[i] = src->pTexelBufferView[i];
            }
        }
        break;
        default:
        break;
    }
}

safe_VkCopyDescriptorSet::safe_VkCopyDescriptorSet(const VkCopyDescriptorSet* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	srcSet(pInStruct->srcSet),
	srcBinding(pInStruct->srcBinding),
	srcArrayElement(pInStruct->srcArrayElement),
	dstSet(pInStruct->dstSet),
	dstBinding(pInStruct->dstBinding),
	dstArrayElement(pInStruct->dstArrayElement),
	descriptorCount(pInStruct->descriptorCount)
{
}

safe_VkCopyDescriptorSet::safe_VkCopyDescriptorSet()
{}

safe_VkCopyDescriptorSet::safe_VkCopyDescriptorSet(const safe_VkCopyDescriptorSet& src)
{
    sType = src.sType;
    pNext = src.pNext;
    srcSet = src.srcSet;
    srcBinding = src.srcBinding;
    srcArrayElement = src.srcArrayElement;
    dstSet = src.dstSet;
    dstBinding = src.dstBinding;
    dstArrayElement = src.dstArrayElement;
    descriptorCount = src.descriptorCount;
}

safe_VkCopyDescriptorSet::~safe_VkCopyDescriptorSet()
{
}

void safe_VkCopyDescriptorSet::initialize(const VkCopyDescriptorSet* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    srcSet = pInStruct->srcSet;
    srcBinding = pInStruct->srcBinding;
    srcArrayElement = pInStruct->srcArrayElement;
    dstSet = pInStruct->dstSet;
    dstBinding = pInStruct->dstBinding;
    dstArrayElement = pInStruct->dstArrayElement;
    descriptorCount = pInStruct->descriptorCount;
}

void safe_VkCopyDescriptorSet::initialize(const safe_VkCopyDescriptorSet* src)
{
    sType = src->sType;
    pNext = src->pNext;
    srcSet = src->srcSet;
    srcBinding = src->srcBinding;
    srcArrayElement = src->srcArrayElement;
    dstSet = src->dstSet;
    dstBinding = src->dstBinding;
    dstArrayElement = src->dstArrayElement;
    descriptorCount = src->descriptorCount;
}

safe_VkFramebufferCreateInfo::safe_VkFramebufferCreateInfo(const VkFramebufferCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	renderPass(pInStruct->renderPass),
	attachmentCount(pInStruct->attachmentCount),
	pAttachments(nullptr),
	width(pInStruct->width),
	height(pInStruct->height),
	layers(pInStruct->layers)
{
    if (attachmentCount && pInStruct->pAttachments) {
        pAttachments = new VkImageView[attachmentCount];
        for (uint32_t i=0; i<attachmentCount; ++i) {
            pAttachments[i] = pInStruct->pAttachments[i];
        }
    }
}

safe_VkFramebufferCreateInfo::safe_VkFramebufferCreateInfo() : 
	pAttachments(nullptr)
{}

safe_VkFramebufferCreateInfo::safe_VkFramebufferCreateInfo(const safe_VkFramebufferCreateInfo& src)
{
    sType = src.sType;
    pNext = src.pNext;
    flags = src.flags;
    renderPass = src.renderPass;
    attachmentCount = src.attachmentCount;
    pAttachments = nullptr;
    width = src.width;
    height = src.height;
    layers = src.layers;
    if (attachmentCount && src.pAttachments) {
        pAttachments = new VkImageView[attachmentCount];
        for (uint32_t i=0; i<attachmentCount; ++i) {
            pAttachments[i] = src.pAttachments[i];
        }
    }
}

safe_VkFramebufferCreateInfo::~safe_VkFramebufferCreateInfo()
{
    if (pAttachments)
        delete[] pAttachments;
}

void safe_VkFramebufferCreateInfo::initialize(const VkFramebufferCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    renderPass = pInStruct->renderPass;
    attachmentCount = pInStruct->attachmentCount;
    pAttachments = nullptr;
    width = pInStruct->width;
    height = pInStruct->height;
    layers = pInStruct->layers;
    if (attachmentCount && pInStruct->pAttachments) {
        pAttachments = new VkImageView[attachmentCount];
        for (uint32_t i=0; i<attachmentCount; ++i) {
            pAttachments[i] = pInStruct->pAttachments[i];
        }
    }
}

void safe_VkFramebufferCreateInfo::initialize(const safe_VkFramebufferCreateInfo* src)
{
    sType = src->sType;
    pNext = src->pNext;
    flags = src->flags;
    renderPass = src->renderPass;
    attachmentCount = src->attachmentCount;
    pAttachments = nullptr;
    width = src->width;
    height = src->height;
    layers = src->layers;
    if (attachmentCount && src->pAttachments) {
        pAttachments = new VkImageView[attachmentCount];
        for (uint32_t i=0; i<attachmentCount; ++i) {
            pAttachments[i] = src->pAttachments[i];
        }
    }
}

safe_VkSubpassDescription::safe_VkSubpassDescription(const VkSubpassDescription* pInStruct) : 
	flags(pInStruct->flags),
	pipelineBindPoint(pInStruct->pipelineBindPoint),
	inputAttachmentCount(pInStruct->inputAttachmentCount),
	pInputAttachments(nullptr),
	colorAttachmentCount(pInStruct->colorAttachmentCount),
	pColorAttachments(nullptr),
	pResolveAttachments(nullptr),
	pDepthStencilAttachment(nullptr),
	preserveAttachmentCount(pInStruct->preserveAttachmentCount),
	pPreserveAttachments(nullptr)
{
    if (pInStruct->pInputAttachments) {
        pInputAttachments = new VkAttachmentReference[pInStruct->inputAttachmentCount];
        memcpy ((void *)pInputAttachments, (void *)pInStruct->pInputAttachments, sizeof(VkAttachmentReference)*pInStruct->inputAttachmentCount);
    }
    if (pInStruct->pColorAttachments) {
        pColorAttachments = new VkAttachmentReference[pInStruct->colorAttachmentCount];
        memcpy ((void *)pColorAttachments, (void *)pInStruct->pColorAttachments, sizeof(VkAttachmentReference)*pInStruct->colorAttachmentCount);
    }
    if (pInStruct->pResolveAttachments) {
        pResolveAttachments = new VkAttachmentReference[pInStruct->colorAttachmentCount];
        memcpy ((void *)pResolveAttachments, (void *)pInStruct->pResolveAttachments, sizeof(VkAttachmentReference)*pInStruct->colorAttachmentCount);
    }
    if (pInStruct->pDepthStencilAttachment) {
        pDepthStencilAttachment = new VkAttachmentReference(*pInStruct->pDepthStencilAttachment);
    }
    if (pInStruct->pPreserveAttachments) {
        pPreserveAttachments = new uint32_t[pInStruct->preserveAttachmentCount];
        memcpy ((void *)pPreserveAttachments, (void *)pInStruct->pPreserveAttachments, sizeof(uint32_t)*pInStruct->preserveAttachmentCount);
    }
}

safe_VkSubpassDescription::safe_VkSubpassDescription() : 
	pInputAttachments(nullptr),
	pColorAttachments(nullptr),
	pResolveAttachments(nullptr),
	pDepthStencilAttachment(nullptr),
	pPreserveAttachments(nullptr)
{}

safe_VkSubpassDescription::safe_VkSubpassDescription(const safe_VkSubpassDescription& src)
{
    flags = src.flags;
    pipelineBindPoint = src.pipelineBindPoint;
    inputAttachmentCount = src.inputAttachmentCount;
    pInputAttachments = nullptr;
    colorAttachmentCount = src.colorAttachmentCount;
    pColorAttachments = nullptr;
    pResolveAttachments = nullptr;
    pDepthStencilAttachment = nullptr;
    preserveAttachmentCount = src.preserveAttachmentCount;
    pPreserveAttachments = nullptr;
    if (src.pInputAttachments) {
        pInputAttachments = new VkAttachmentReference[src.inputAttachmentCount];
        memcpy ((void *)pInputAttachments, (void *)src.pInputAttachments, sizeof(VkAttachmentReference)*src.inputAttachmentCount);
    }
    if (src.pColorAttachments) {
        pColorAttachments = new VkAttachmentReference[src.colorAttachmentCount];
        memcpy ((void *)pColorAttachments, (void *)src.pColorAttachments, sizeof(VkAttachmentReference)*src.colorAttachmentCount);
    }
    if (src.pResolveAttachments) {
        pResolveAttachments = new VkAttachmentReference[src.colorAttachmentCount];
        memcpy ((void *)pResolveAttachments, (void *)src.pResolveAttachments, sizeof(VkAttachmentReference)*src.colorAttachmentCount);
    }
    if (src.pDepthStencilAttachment) {
        pDepthStencilAttachment = new VkAttachmentReference(*src.pDepthStencilAttachment);
    }
    if (src.pPreserveAttachments) {
        pPreserveAttachments = new uint32_t[src.preserveAttachmentCount];
        memcpy ((void *)pPreserveAttachments, (void *)src.pPreserveAttachments, sizeof(uint32_t)*src.preserveAttachmentCount);
    }
}

safe_VkSubpassDescription::~safe_VkSubpassDescription()
{
    if (pInputAttachments)
        delete[] pInputAttachments;
    if (pColorAttachments)
        delete[] pColorAttachments;
    if (pResolveAttachments)
        delete[] pResolveAttachments;
    if (pDepthStencilAttachment)
        delete pDepthStencilAttachment;
    if (pPreserveAttachments)
        delete[] pPreserveAttachments;
}

void safe_VkSubpassDescription::initialize(const VkSubpassDescription* pInStruct)
{
    flags = pInStruct->flags;
    pipelineBindPoint = pInStruct->pipelineBindPoint;
    inputAttachmentCount = pInStruct->inputAttachmentCount;
    pInputAttachments = nullptr;
    colorAttachmentCount = pInStruct->colorAttachmentCount;
    pColorAttachments = nullptr;
    pResolveAttachments = nullptr;
    pDepthStencilAttachment = nullptr;
    preserveAttachmentCount = pInStruct->preserveAttachmentCount;
    pPreserveAttachments = nullptr;
    if (pInStruct->pInputAttachments) {
        pInputAttachments = new VkAttachmentReference[pInStruct->inputAttachmentCount];
        memcpy ((void *)pInputAttachments, (void *)pInStruct->pInputAttachments, sizeof(VkAttachmentReference)*pInStruct->inputAttachmentCount);
    }
    if (pInStruct->pColorAttachments) {
        pColorAttachments = new VkAttachmentReference[pInStruct->colorAttachmentCount];
        memcpy ((void *)pColorAttachments, (void *)pInStruct->pColorAttachments, sizeof(VkAttachmentReference)*pInStruct->colorAttachmentCount);
    }
    if (pInStruct->pResolveAttachments) {
        pResolveAttachments = new VkAttachmentReference[pInStruct->colorAttachmentCount];
        memcpy ((void *)pResolveAttachments, (void *)pInStruct->pResolveAttachments, sizeof(VkAttachmentReference)*pInStruct->colorAttachmentCount);
    }
    if (pInStruct->pDepthStencilAttachment) {
        pDepthStencilAttachment = new VkAttachmentReference(*pInStruct->pDepthStencilAttachment);
    }
    if (pInStruct->pPreserveAttachments) {
        pPreserveAttachments = new uint32_t[pInStruct->preserveAttachmentCount];
        memcpy ((void *)pPreserveAttachments, (void *)pInStruct->pPreserveAttachments, sizeof(uint32_t)*pInStruct->preserveAttachmentCount);
    }
}

void safe_VkSubpassDescription::initialize(const safe_VkSubpassDescription* src)
{
    flags = src->flags;
    pipelineBindPoint = src->pipelineBindPoint;
    inputAttachmentCount = src->inputAttachmentCount;
    pInputAttachments = nullptr;
    colorAttachmentCount = src->colorAttachmentCount;
    pColorAttachments = nullptr;
    pResolveAttachments = nullptr;
    pDepthStencilAttachment = nullptr;
    preserveAttachmentCount = src->preserveAttachmentCount;
    pPreserveAttachments = nullptr;
    if (src->pInputAttachments) {
        pInputAttachments = new VkAttachmentReference[src->inputAttachmentCount];
        memcpy ((void *)pInputAttachments, (void *)src->pInputAttachments, sizeof(VkAttachmentReference)*src->inputAttachmentCount);
    }
    if (src->pColorAttachments) {
        pColorAttachments = new VkAttachmentReference[src->colorAttachmentCount];
        memcpy ((void *)pColorAttachments, (void *)src->pColorAttachments, sizeof(VkAttachmentReference)*src->colorAttachmentCount);
    }
    if (src->pResolveAttachments) {
        pResolveAttachments = new VkAttachmentReference[src->colorAttachmentCount];
        memcpy ((void *)pResolveAttachments, (void *)src->pResolveAttachments, sizeof(VkAttachmentReference)*src->colorAttachmentCount);
    }
    if (src->pDepthStencilAttachment) {
        pDepthStencilAttachment = new VkAttachmentReference(*src->pDepthStencilAttachment);
    }
    if (src->pPreserveAttachments) {
        pPreserveAttachments = new uint32_t[src->preserveAttachmentCount];
        memcpy ((void *)pPreserveAttachments, (void *)src->pPreserveAttachments, sizeof(uint32_t)*src->preserveAttachmentCount);
    }
}

safe_VkRenderPassCreateInfo::safe_VkRenderPassCreateInfo(const VkRenderPassCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	attachmentCount(pInStruct->attachmentCount),
	pAttachments(nullptr),
	subpassCount(pInStruct->subpassCount),
	pSubpasses(nullptr),
	dependencyCount(pInStruct->dependencyCount),
	pDependencies(nullptr)
{
    if (pInStruct->pAttachments) {
        pAttachments = new VkAttachmentDescription[pInStruct->attachmentCount];
        memcpy ((void *)pAttachments, (void *)pInStruct->pAttachments, sizeof(VkAttachmentDescription)*pInStruct->attachmentCount);
    }
    if (subpassCount && pInStruct->pSubpasses) {
        pSubpasses = new safe_VkSubpassDescription[subpassCount];
        for (uint32_t i=0; i<subpassCount; ++i) {
            pSubpasses[i].initialize(&pInStruct->pSubpasses[i]);
        }
    }
    if (pInStruct->pDependencies) {
        pDependencies = new VkSubpassDependency[pInStruct->dependencyCount];
        memcpy ((void *)pDependencies, (void *)pInStruct->pDependencies, sizeof(VkSubpassDependency)*pInStruct->dependencyCount);
    }
}

safe_VkRenderPassCreateInfo::safe_VkRenderPassCreateInfo() : 
	pAttachments(nullptr),
	pSubpasses(nullptr),
	pDependencies(nullptr)
{}

safe_VkRenderPassCreateInfo::safe_VkRenderPassCreateInfo(const safe_VkRenderPassCreateInfo& src)
{
    sType = src.sType;
    pNext = src.pNext;
    flags = src.flags;
    attachmentCount = src.attachmentCount;
    pAttachments = nullptr;
    subpassCount = src.subpassCount;
    pSubpasses = nullptr;
    dependencyCount = src.dependencyCount;
    pDependencies = nullptr;
    if (src.pAttachments) {
        pAttachments = new VkAttachmentDescription[src.attachmentCount];
        memcpy ((void *)pAttachments, (void *)src.pAttachments, sizeof(VkAttachmentDescription)*src.attachmentCount);
    }
    if (subpassCount && src.pSubpasses) {
        pSubpasses = new safe_VkSubpassDescription[subpassCount];
        for (uint32_t i=0; i<subpassCount; ++i) {
            pSubpasses[i].initialize(&src.pSubpasses[i]);
        }
    }
    if (src.pDependencies) {
        pDependencies = new VkSubpassDependency[src.dependencyCount];
        memcpy ((void *)pDependencies, (void *)src.pDependencies, sizeof(VkSubpassDependency)*src.dependencyCount);
    }
}

safe_VkRenderPassCreateInfo::~safe_VkRenderPassCreateInfo()
{
    if (pAttachments)
        delete[] pAttachments;
    if (pSubpasses)
        delete[] pSubpasses;
    if (pDependencies)
        delete[] pDependencies;
}

void safe_VkRenderPassCreateInfo::initialize(const VkRenderPassCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    attachmentCount = pInStruct->attachmentCount;
    pAttachments = nullptr;
    subpassCount = pInStruct->subpassCount;
    pSubpasses = nullptr;
    dependencyCount = pInStruct->dependencyCount;
    pDependencies = nullptr;
    if (pInStruct->pAttachments) {
        pAttachments = new VkAttachmentDescription[pInStruct->attachmentCount];
        memcpy ((void *)pAttachments, (void *)pInStruct->pAttachments, sizeof(VkAttachmentDescription)*pInStruct->attachmentCount);
    }
    if (subpassCount && pInStruct->pSubpasses) {
        pSubpasses = new safe_VkSubpassDescription[subpassCount];
        for (uint32_t i=0; i<subpassCount; ++i) {
            pSubpasses[i].initialize(&pInStruct->pSubpasses[i]);
        }
    }
    if (pInStruct->pDependencies) {
        pDependencies = new VkSubpassDependency[pInStruct->dependencyCount];
        memcpy ((void *)pDependencies, (void *)pInStruct->pDependencies, sizeof(VkSubpassDependency)*pInStruct->dependencyCount);
    }
}

void safe_VkRenderPassCreateInfo::initialize(const safe_VkRenderPassCreateInfo* src)
{
    sType = src->sType;
    pNext = src->pNext;
    flags = src->flags;
    attachmentCount = src->attachmentCount;
    pAttachments = nullptr;
    subpassCount = src->subpassCount;
    pSubpasses = nullptr;
    dependencyCount = src->dependencyCount;
    pDependencies = nullptr;
    if (src->pAttachments) {
        pAttachments = new VkAttachmentDescription[src->attachmentCount];
        memcpy ((void *)pAttachments, (void *)src->pAttachments, sizeof(VkAttachmentDescription)*src->attachmentCount);
    }
    if (subpassCount && src->pSubpasses) {
        pSubpasses = new safe_VkSubpassDescription[subpassCount];
        for (uint32_t i=0; i<subpassCount; ++i) {
            pSubpasses[i].initialize(&src->pSubpasses[i]);
        }
    }
    if (src->pDependencies) {
        pDependencies = new VkSubpassDependency[src->dependencyCount];
        memcpy ((void *)pDependencies, (void *)src->pDependencies, sizeof(VkSubpassDependency)*src->dependencyCount);
    }
}

safe_VkCommandPoolCreateInfo::safe_VkCommandPoolCreateInfo(const VkCommandPoolCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	queueFamilyIndex(pInStruct->queueFamilyIndex)
{
}

safe_VkCommandPoolCreateInfo::safe_VkCommandPoolCreateInfo()
{}

safe_VkCommandPoolCreateInfo::safe_VkCommandPoolCreateInfo(const safe_VkCommandPoolCreateInfo& src)
{
    sType = src.sType;
    pNext = src.pNext;
    flags = src.flags;
    queueFamilyIndex = src.queueFamilyIndex;
}

safe_VkCommandPoolCreateInfo::~safe_VkCommandPoolCreateInfo()
{
}

void safe_VkCommandPoolCreateInfo::initialize(const VkCommandPoolCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    queueFamilyIndex = pInStruct->queueFamilyIndex;
}

void safe_VkCommandPoolCreateInfo::initialize(const safe_VkCommandPoolCreateInfo* src)
{
    sType = src->sType;
    pNext = src->pNext;
    flags = src->flags;
    queueFamilyIndex = src->queueFamilyIndex;
}

safe_VkCommandBufferAllocateInfo::safe_VkCommandBufferAllocateInfo(const VkCommandBufferAllocateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	commandPool(pInStruct->commandPool),
	level(pInStruct->level),
	commandBufferCount(pInStruct->commandBufferCount)
{
}

safe_VkCommandBufferAllocateInfo::safe_VkCommandBufferAllocateInfo()
{}

safe_VkCommandBufferAllocateInfo::safe_VkCommandBufferAllocateInfo(const safe_VkCommandBufferAllocateInfo& src)
{
    sType = src.sType;
    pNext = src.pNext;
    commandPool = src.commandPool;
    level = src.level;
    commandBufferCount = src.commandBufferCount;
}

safe_VkCommandBufferAllocateInfo::~safe_VkCommandBufferAllocateInfo()
{
}

void safe_VkCommandBufferAllocateInfo::initialize(const VkCommandBufferAllocateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    commandPool = pInStruct->commandPool;
    level = pInStruct->level;
    commandBufferCount = pInStruct->commandBufferCount;
}

void safe_VkCommandBufferAllocateInfo::initialize(const safe_VkCommandBufferAllocateInfo* src)
{
    sType = src->sType;
    pNext = src->pNext;
    commandPool = src->commandPool;
    level = src->level;
    commandBufferCount = src->commandBufferCount;
}

safe_VkCommandBufferInheritanceInfo::safe_VkCommandBufferInheritanceInfo(const VkCommandBufferInheritanceInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	renderPass(pInStruct->renderPass),
	subpass(pInStruct->subpass),
	framebuffer(pInStruct->framebuffer),
	occlusionQueryEnable(pInStruct->occlusionQueryEnable),
	queryFlags(pInStruct->queryFlags),
	pipelineStatistics(pInStruct->pipelineStatistics)
{
}

safe_VkCommandBufferInheritanceInfo::safe_VkCommandBufferInheritanceInfo()
{}

safe_VkCommandBufferInheritanceInfo::safe_VkCommandBufferInheritanceInfo(const safe_VkCommandBufferInheritanceInfo& src)
{
    sType = src.sType;
    pNext = src.pNext;
    renderPass = src.renderPass;
    subpass = src.subpass;
    framebuffer = src.framebuffer;
    occlusionQueryEnable = src.occlusionQueryEnable;
    queryFlags = src.queryFlags;
    pipelineStatistics = src.pipelineStatistics;
}

safe_VkCommandBufferInheritanceInfo::~safe_VkCommandBufferInheritanceInfo()
{
}

void safe_VkCommandBufferInheritanceInfo::initialize(const VkCommandBufferInheritanceInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    renderPass = pInStruct->renderPass;
    subpass = pInStruct->subpass;
    framebuffer = pInStruct->framebuffer;
    occlusionQueryEnable = pInStruct->occlusionQueryEnable;
    queryFlags = pInStruct->queryFlags;
    pipelineStatistics = pInStruct->pipelineStatistics;
}

void safe_VkCommandBufferInheritanceInfo::initialize(const safe_VkCommandBufferInheritanceInfo* src)
{
    sType = src->sType;
    pNext = src->pNext;
    renderPass = src->renderPass;
    subpass = src->subpass;
    framebuffer = src->framebuffer;
    occlusionQueryEnable = src->occlusionQueryEnable;
    queryFlags = src->queryFlags;
    pipelineStatistics = src->pipelineStatistics;
}

safe_VkCommandBufferBeginInfo::safe_VkCommandBufferBeginInfo(const VkCommandBufferBeginInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags)
{
    if (pInStruct->pInheritanceInfo)
        pInheritanceInfo = new safe_VkCommandBufferInheritanceInfo(pInStruct->pInheritanceInfo);
    else
        pInheritanceInfo = NULL;
}

safe_VkCommandBufferBeginInfo::safe_VkCommandBufferBeginInfo()
{}

safe_VkCommandBufferBeginInfo::safe_VkCommandBufferBeginInfo(const safe_VkCommandBufferBeginInfo& src)
{
    sType = src.sType;
    pNext = src.pNext;
    flags = src.flags;
    if (src.pInheritanceInfo)
        pInheritanceInfo = new safe_VkCommandBufferInheritanceInfo(*src.pInheritanceInfo);
    else
        pInheritanceInfo = NULL;
}

safe_VkCommandBufferBeginInfo::~safe_VkCommandBufferBeginInfo()
{
    if (pInheritanceInfo)
        delete pInheritanceInfo;
}

void safe_VkCommandBufferBeginInfo::initialize(const VkCommandBufferBeginInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    if (pInStruct->pInheritanceInfo)
        pInheritanceInfo = new safe_VkCommandBufferInheritanceInfo(pInStruct->pInheritanceInfo);
    else
        pInheritanceInfo = NULL;
}

void safe_VkCommandBufferBeginInfo::initialize(const safe_VkCommandBufferBeginInfo* src)
{
    sType = src->sType;
    pNext = src->pNext;
    flags = src->flags;
    if (src->pInheritanceInfo)
        pInheritanceInfo = new safe_VkCommandBufferInheritanceInfo(*src->pInheritanceInfo);
    else
        pInheritanceInfo = NULL;
}

safe_VkMemoryBarrier::safe_VkMemoryBarrier(const VkMemoryBarrier* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	srcAccessMask(pInStruct->srcAccessMask),
	dstAccessMask(pInStruct->dstAccessMask)
{
}

safe_VkMemoryBarrier::safe_VkMemoryBarrier()
{}

safe_VkMemoryBarrier::safe_VkMemoryBarrier(const safe_VkMemoryBarrier& src)
{
    sType = src.sType;
    pNext = src.pNext;
    srcAccessMask = src.srcAccessMask;
    dstAccessMask = src.dstAccessMask;
}

safe_VkMemoryBarrier::~safe_VkMemoryBarrier()
{
}

void safe_VkMemoryBarrier::initialize(const VkMemoryBarrier* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    srcAccessMask = pInStruct->srcAccessMask;
    dstAccessMask = pInStruct->dstAccessMask;
}

void safe_VkMemoryBarrier::initialize(const safe_VkMemoryBarrier* src)
{
    sType = src->sType;
    pNext = src->pNext;
    srcAccessMask = src->srcAccessMask;
    dstAccessMask = src->dstAccessMask;
}

safe_VkBufferMemoryBarrier::safe_VkBufferMemoryBarrier(const VkBufferMemoryBarrier* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	srcAccessMask(pInStruct->srcAccessMask),
	dstAccessMask(pInStruct->dstAccessMask),
	srcQueueFamilyIndex(pInStruct->srcQueueFamilyIndex),
	dstQueueFamilyIndex(pInStruct->dstQueueFamilyIndex),
	buffer(pInStruct->buffer),
	offset(pInStruct->offset),
	size(pInStruct->size)
{
}

safe_VkBufferMemoryBarrier::safe_VkBufferMemoryBarrier()
{}

safe_VkBufferMemoryBarrier::safe_VkBufferMemoryBarrier(const safe_VkBufferMemoryBarrier& src)
{
    sType = src.sType;
    pNext = src.pNext;
    srcAccessMask = src.srcAccessMask;
    dstAccessMask = src.dstAccessMask;
    srcQueueFamilyIndex = src.srcQueueFamilyIndex;
    dstQueueFamilyIndex = src.dstQueueFamilyIndex;
    buffer = src.buffer;
    offset = src.offset;
    size = src.size;
}

safe_VkBufferMemoryBarrier::~safe_VkBufferMemoryBarrier()
{
}

void safe_VkBufferMemoryBarrier::initialize(const VkBufferMemoryBarrier* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    srcAccessMask = pInStruct->srcAccessMask;
    dstAccessMask = pInStruct->dstAccessMask;
    srcQueueFamilyIndex = pInStruct->srcQueueFamilyIndex;
    dstQueueFamilyIndex = pInStruct->dstQueueFamilyIndex;
    buffer = pInStruct->buffer;
    offset = pInStruct->offset;
    size = pInStruct->size;
}

void safe_VkBufferMemoryBarrier::initialize(const safe_VkBufferMemoryBarrier* src)
{
    sType = src->sType;
    pNext = src->pNext;
    srcAccessMask = src->srcAccessMask;
    dstAccessMask = src->dstAccessMask;
    srcQueueFamilyIndex = src->srcQueueFamilyIndex;
    dstQueueFamilyIndex = src->dstQueueFamilyIndex;
    buffer = src->buffer;
    offset = src->offset;
    size = src->size;
}

safe_VkImageMemoryBarrier::safe_VkImageMemoryBarrier(const VkImageMemoryBarrier* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	srcAccessMask(pInStruct->srcAccessMask),
	dstAccessMask(pInStruct->dstAccessMask),
	oldLayout(pInStruct->oldLayout),
	newLayout(pInStruct->newLayout),
	srcQueueFamilyIndex(pInStruct->srcQueueFamilyIndex),
	dstQueueFamilyIndex(pInStruct->dstQueueFamilyIndex),
	image(pInStruct->image),
	subresourceRange(pInStruct->subresourceRange)
{
}

safe_VkImageMemoryBarrier::safe_VkImageMemoryBarrier()
{}

safe_VkImageMemoryBarrier::safe_VkImageMemoryBarrier(const safe_VkImageMemoryBarrier& src)
{
    sType = src.sType;
    pNext = src.pNext;
    srcAccessMask = src.srcAccessMask;
    dstAccessMask = src.dstAccessMask;
    oldLayout = src.oldLayout;
    newLayout = src.newLayout;
    srcQueueFamilyIndex = src.srcQueueFamilyIndex;
    dstQueueFamilyIndex = src.dstQueueFamilyIndex;
    image = src.image;
    subresourceRange = src.subresourceRange;
}

safe_VkImageMemoryBarrier::~safe_VkImageMemoryBarrier()
{
}

void safe_VkImageMemoryBarrier::initialize(const VkImageMemoryBarrier* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    srcAccessMask = pInStruct->srcAccessMask;
    dstAccessMask = pInStruct->dstAccessMask;
    oldLayout = pInStruct->oldLayout;
    newLayout = pInStruct->newLayout;
    srcQueueFamilyIndex = pInStruct->srcQueueFamilyIndex;
    dstQueueFamilyIndex = pInStruct->dstQueueFamilyIndex;
    image = pInStruct->image;
    subresourceRange = pInStruct->subresourceRange;
}

void safe_VkImageMemoryBarrier::initialize(const safe_VkImageMemoryBarrier* src)
{
    sType = src->sType;
    pNext = src->pNext;
    srcAccessMask = src->srcAccessMask;
    dstAccessMask = src->dstAccessMask;
    oldLayout = src->oldLayout;
    newLayout = src->newLayout;
    srcQueueFamilyIndex = src->srcQueueFamilyIndex;
    dstQueueFamilyIndex = src->dstQueueFamilyIndex;
    image = src->image;
    subresourceRange = src->subresourceRange;
}

safe_VkRenderPassBeginInfo::safe_VkRenderPassBeginInfo(const VkRenderPassBeginInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	renderPass(pInStruct->renderPass),
	framebuffer(pInStruct->framebuffer),
	renderArea(pInStruct->renderArea),
	clearValueCount(pInStruct->clearValueCount),
	pClearValues(nullptr)
{
    if (pInStruct->pClearValues) {
        pClearValues = new VkClearValue[pInStruct->clearValueCount];
        memcpy ((void *)pClearValues, (void *)pInStruct->pClearValues, sizeof(VkClearValue)*pInStruct->clearValueCount);
    }
}

safe_VkRenderPassBeginInfo::safe_VkRenderPassBeginInfo() : 
	pClearValues(nullptr)
{}

safe_VkRenderPassBeginInfo::safe_VkRenderPassBeginInfo(const safe_VkRenderPassBeginInfo& src)
{
    sType = src.sType;
    pNext = src.pNext;
    renderPass = src.renderPass;
    framebuffer = src.framebuffer;
    renderArea = src.renderArea;
    clearValueCount = src.clearValueCount;
    pClearValues = nullptr;
    if (src.pClearValues) {
        pClearValues = new VkClearValue[src.clearValueCount];
        memcpy ((void *)pClearValues, (void *)src.pClearValues, sizeof(VkClearValue)*src.clearValueCount);
    }
}

safe_VkRenderPassBeginInfo::~safe_VkRenderPassBeginInfo()
{
    if (pClearValues)
        delete[] pClearValues;
}

void safe_VkRenderPassBeginInfo::initialize(const VkRenderPassBeginInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    renderPass = pInStruct->renderPass;
    framebuffer = pInStruct->framebuffer;
    renderArea = pInStruct->renderArea;
    clearValueCount = pInStruct->clearValueCount;
    pClearValues = nullptr;
    if (pInStruct->pClearValues) {
        pClearValues = new VkClearValue[pInStruct->clearValueCount];
        memcpy ((void *)pClearValues, (void *)pInStruct->pClearValues, sizeof(VkClearValue)*pInStruct->clearValueCount);
    }
}

void safe_VkRenderPassBeginInfo::initialize(const safe_VkRenderPassBeginInfo* src)
{
    sType = src->sType;
    pNext = src->pNext;
    renderPass = src->renderPass;
    framebuffer = src->framebuffer;
    renderArea = src->renderArea;
    clearValueCount = src->clearValueCount;
    pClearValues = nullptr;
    if (src->pClearValues) {
        pClearValues = new VkClearValue[src->clearValueCount];
        memcpy ((void *)pClearValues, (void *)src->pClearValues, sizeof(VkClearValue)*src->clearValueCount);
    }
}

safe_VkSwapchainCreateInfoKHR::safe_VkSwapchainCreateInfoKHR(const VkSwapchainCreateInfoKHR* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	surface(pInStruct->surface),
	minImageCount(pInStruct->minImageCount),
	imageFormat(pInStruct->imageFormat),
	imageColorSpace(pInStruct->imageColorSpace),
	imageExtent(pInStruct->imageExtent),
	imageArrayLayers(pInStruct->imageArrayLayers),
	imageUsage(pInStruct->imageUsage),
	imageSharingMode(pInStruct->imageSharingMode),
	queueFamilyIndexCount(pInStruct->queueFamilyIndexCount),
	pQueueFamilyIndices(pInStruct->pQueueFamilyIndices),
	preTransform(pInStruct->preTransform),
	compositeAlpha(pInStruct->compositeAlpha),
	presentMode(pInStruct->presentMode),
	clipped(pInStruct->clipped),
	oldSwapchain(pInStruct->oldSwapchain)
{
}

safe_VkSwapchainCreateInfoKHR::safe_VkSwapchainCreateInfoKHR()
{}

safe_VkSwapchainCreateInfoKHR::safe_VkSwapchainCreateInfoKHR(const safe_VkSwapchainCreateInfoKHR& src)
{
    sType = src.sType;
    pNext = src.pNext;
    flags = src.flags;
    surface = src.surface;
    minImageCount = src.minImageCount;
    imageFormat = src.imageFormat;
    imageColorSpace = src.imageColorSpace;
    imageExtent = src.imageExtent;
    imageArrayLayers = src.imageArrayLayers;
    imageUsage = src.imageUsage;
    imageSharingMode = src.imageSharingMode;
    queueFamilyIndexCount = src.queueFamilyIndexCount;
    pQueueFamilyIndices = src.pQueueFamilyIndices;
    preTransform = src.preTransform;
    compositeAlpha = src.compositeAlpha;
    presentMode = src.presentMode;
    clipped = src.clipped;
    oldSwapchain = src.oldSwapchain;
}

safe_VkSwapchainCreateInfoKHR::~safe_VkSwapchainCreateInfoKHR()
{
}

void safe_VkSwapchainCreateInfoKHR::initialize(const VkSwapchainCreateInfoKHR* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    surface = pInStruct->surface;
    minImageCount = pInStruct->minImageCount;
    imageFormat = pInStruct->imageFormat;
    imageColorSpace = pInStruct->imageColorSpace;
    imageExtent = pInStruct->imageExtent;
    imageArrayLayers = pInStruct->imageArrayLayers;
    imageUsage = pInStruct->imageUsage;
    imageSharingMode = pInStruct->imageSharingMode;
    queueFamilyIndexCount = pInStruct->queueFamilyIndexCount;
    pQueueFamilyIndices = pInStruct->pQueueFamilyIndices;
    preTransform = pInStruct->preTransform;
    compositeAlpha = pInStruct->compositeAlpha;
    presentMode = pInStruct->presentMode;
    clipped = pInStruct->clipped;
    oldSwapchain = pInStruct->oldSwapchain;
}

void safe_VkSwapchainCreateInfoKHR::initialize(const safe_VkSwapchainCreateInfoKHR* src)
{
    sType = src->sType;
    pNext = src->pNext;
    flags = src->flags;
    surface = src->surface;
    minImageCount = src->minImageCount;
    imageFormat = src->imageFormat;
    imageColorSpace = src->imageColorSpace;
    imageExtent = src->imageExtent;
    imageArrayLayers = src->imageArrayLayers;
    imageUsage = src->imageUsage;
    imageSharingMode = src->imageSharingMode;
    queueFamilyIndexCount = src->queueFamilyIndexCount;
    pQueueFamilyIndices = src->pQueueFamilyIndices;
    preTransform = src->preTransform;
    compositeAlpha = src->compositeAlpha;
    presentMode = src->presentMode;
    clipped = src->clipped;
    oldSwapchain = src->oldSwapchain;
}

safe_VkPresentInfoKHR::safe_VkPresentInfoKHR(const VkPresentInfoKHR* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	waitSemaphoreCount(pInStruct->waitSemaphoreCount),
	pWaitSemaphores(nullptr),
	swapchainCount(pInStruct->swapchainCount),
	pSwapchains(nullptr),
	pImageIndices(pInStruct->pImageIndices),
	pResults(pInStruct->pResults)
{
    if (waitSemaphoreCount && pInStruct->pWaitSemaphores) {
        pWaitSemaphores = new VkSemaphore[waitSemaphoreCount];
        for (uint32_t i=0; i<waitSemaphoreCount; ++i) {
            pWaitSemaphores[i] = pInStruct->pWaitSemaphores[i];
        }
    }
    if (swapchainCount && pInStruct->pSwapchains) {
        pSwapchains = new VkSwapchainKHR[swapchainCount];
        for (uint32_t i=0; i<swapchainCount; ++i) {
            pSwapchains[i] = pInStruct->pSwapchains[i];
        }
    }
}

safe_VkPresentInfoKHR::safe_VkPresentInfoKHR() : 
	pWaitSemaphores(nullptr),
	pSwapchains(nullptr)
{}

safe_VkPresentInfoKHR::safe_VkPresentInfoKHR(const safe_VkPresentInfoKHR& src)
{
    sType = src.sType;
    pNext = src.pNext;
    waitSemaphoreCount = src.waitSemaphoreCount;
    pWaitSemaphores = nullptr;
    swapchainCount = src.swapchainCount;
    pSwapchains = nullptr;
    pImageIndices = src.pImageIndices;
    pResults = src.pResults;
    if (waitSemaphoreCount && src.pWaitSemaphores) {
        pWaitSemaphores = new VkSemaphore[waitSemaphoreCount];
        for (uint32_t i=0; i<waitSemaphoreCount; ++i) {
            pWaitSemaphores[i] = src.pWaitSemaphores[i];
        }
    }
    if (swapchainCount && src.pSwapchains) {
        pSwapchains = new VkSwapchainKHR[swapchainCount];
        for (uint32_t i=0; i<swapchainCount; ++i) {
            pSwapchains[i] = src.pSwapchains[i];
        }
    }
}

safe_VkPresentInfoKHR::~safe_VkPresentInfoKHR()
{
    if (pWaitSemaphores)
        delete[] pWaitSemaphores;
    if (pSwapchains)
        delete[] pSwapchains;
}

void safe_VkPresentInfoKHR::initialize(const VkPresentInfoKHR* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    waitSemaphoreCount = pInStruct->waitSemaphoreCount;
    pWaitSemaphores = nullptr;
    swapchainCount = pInStruct->swapchainCount;
    pSwapchains = nullptr;
    pImageIndices = pInStruct->pImageIndices;
    pResults = pInStruct->pResults;
    if (waitSemaphoreCount && pInStruct->pWaitSemaphores) {
        pWaitSemaphores = new VkSemaphore[waitSemaphoreCount];
        for (uint32_t i=0; i<waitSemaphoreCount; ++i) {
            pWaitSemaphores[i] = pInStruct->pWaitSemaphores[i];
        }
    }
    if (swapchainCount && pInStruct->pSwapchains) {
        pSwapchains = new VkSwapchainKHR[swapchainCount];
        for (uint32_t i=0; i<swapchainCount; ++i) {
            pSwapchains[i] = pInStruct->pSwapchains[i];
        }
    }
}

void safe_VkPresentInfoKHR::initialize(const safe_VkPresentInfoKHR* src)
{
    sType = src->sType;
    pNext = src->pNext;
    waitSemaphoreCount = src->waitSemaphoreCount;
    pWaitSemaphores = nullptr;
    swapchainCount = src->swapchainCount;
    pSwapchains = nullptr;
    pImageIndices = src->pImageIndices;
    pResults = src->pResults;
    if (waitSemaphoreCount && src->pWaitSemaphores) {
        pWaitSemaphores = new VkSemaphore[waitSemaphoreCount];
        for (uint32_t i=0; i<waitSemaphoreCount; ++i) {
            pWaitSemaphores[i] = src->pWaitSemaphores[i];
        }
    }
    if (swapchainCount && src->pSwapchains) {
        pSwapchains = new VkSwapchainKHR[swapchainCount];
        for (uint32_t i=0; i<swapchainCount; ++i) {
            pSwapchains[i] = src->pSwapchains[i];
        }
    }
}

safe_VkDisplayPropertiesKHR::safe_VkDisplayPropertiesKHR(const VkDisplayPropertiesKHR* pInStruct) : 
	display(pInStruct->display),
	displayName(pInStruct->displayName),
	physicalDimensions(pInStruct->physicalDimensions),
	physicalResolution(pInStruct->physicalResolution),
	supportedTransforms(pInStruct->supportedTransforms),
	planeReorderPossible(pInStruct->planeReorderPossible),
	persistentContent(pInStruct->persistentContent)
{
}

safe_VkDisplayPropertiesKHR::safe_VkDisplayPropertiesKHR()
{}

safe_VkDisplayPropertiesKHR::safe_VkDisplayPropertiesKHR(const safe_VkDisplayPropertiesKHR& src)
{
    display = src.display;
    displayName = src.displayName;
    physicalDimensions = src.physicalDimensions;
    physicalResolution = src.physicalResolution;
    supportedTransforms = src.supportedTransforms;
    planeReorderPossible = src.planeReorderPossible;
    persistentContent = src.persistentContent;
}

safe_VkDisplayPropertiesKHR::~safe_VkDisplayPropertiesKHR()
{
}

void safe_VkDisplayPropertiesKHR::initialize(const VkDisplayPropertiesKHR* pInStruct)
{
    display = pInStruct->display;
    displayName = pInStruct->displayName;
    physicalDimensions = pInStruct->physicalDimensions;
    physicalResolution = pInStruct->physicalResolution;
    supportedTransforms = pInStruct->supportedTransforms;
    planeReorderPossible = pInStruct->planeReorderPossible;
    persistentContent = pInStruct->persistentContent;
}

void safe_VkDisplayPropertiesKHR::initialize(const safe_VkDisplayPropertiesKHR* src)
{
    display = src->display;
    displayName = src->displayName;
    physicalDimensions = src->physicalDimensions;
    physicalResolution = src->physicalResolution;
    supportedTransforms = src->supportedTransforms;
    planeReorderPossible = src->planeReorderPossible;
    persistentContent = src->persistentContent;
}

safe_VkDisplayModePropertiesKHR::safe_VkDisplayModePropertiesKHR(const VkDisplayModePropertiesKHR* pInStruct) : 
	displayMode(pInStruct->displayMode),
	parameters(pInStruct->parameters)
{
}

safe_VkDisplayModePropertiesKHR::safe_VkDisplayModePropertiesKHR()
{}

safe_VkDisplayModePropertiesKHR::safe_VkDisplayModePropertiesKHR(const safe_VkDisplayModePropertiesKHR& src)
{
    displayMode = src.displayMode;
    parameters = src.parameters;
}

safe_VkDisplayModePropertiesKHR::~safe_VkDisplayModePropertiesKHR()
{
}

void safe_VkDisplayModePropertiesKHR::initialize(const VkDisplayModePropertiesKHR* pInStruct)
{
    displayMode = pInStruct->displayMode;
    parameters = pInStruct->parameters;
}

void safe_VkDisplayModePropertiesKHR::initialize(const safe_VkDisplayModePropertiesKHR* src)
{
    displayMode = src->displayMode;
    parameters = src->parameters;
}

safe_VkDisplayModeCreateInfoKHR::safe_VkDisplayModeCreateInfoKHR(const VkDisplayModeCreateInfoKHR* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	parameters(pInStruct->parameters)
{
}

safe_VkDisplayModeCreateInfoKHR::safe_VkDisplayModeCreateInfoKHR()
{}

safe_VkDisplayModeCreateInfoKHR::safe_VkDisplayModeCreateInfoKHR(const safe_VkDisplayModeCreateInfoKHR& src)
{
    sType = src.sType;
    pNext = src.pNext;
    flags = src.flags;
    parameters = src.parameters;
}

safe_VkDisplayModeCreateInfoKHR::~safe_VkDisplayModeCreateInfoKHR()
{
}

void safe_VkDisplayModeCreateInfoKHR::initialize(const VkDisplayModeCreateInfoKHR* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    parameters = pInStruct->parameters;
}

void safe_VkDisplayModeCreateInfoKHR::initialize(const safe_VkDisplayModeCreateInfoKHR* src)
{
    sType = src->sType;
    pNext = src->pNext;
    flags = src->flags;
    parameters = src->parameters;
}

safe_VkDisplayPlanePropertiesKHR::safe_VkDisplayPlanePropertiesKHR(const VkDisplayPlanePropertiesKHR* pInStruct) : 
	currentDisplay(pInStruct->currentDisplay),
	currentStackIndex(pInStruct->currentStackIndex)
{
}

safe_VkDisplayPlanePropertiesKHR::safe_VkDisplayPlanePropertiesKHR()
{}

safe_VkDisplayPlanePropertiesKHR::safe_VkDisplayPlanePropertiesKHR(const safe_VkDisplayPlanePropertiesKHR& src)
{
    currentDisplay = src.currentDisplay;
    currentStackIndex = src.currentStackIndex;
}

safe_VkDisplayPlanePropertiesKHR::~safe_VkDisplayPlanePropertiesKHR()
{
}

void safe_VkDisplayPlanePropertiesKHR::initialize(const VkDisplayPlanePropertiesKHR* pInStruct)
{
    currentDisplay = pInStruct->currentDisplay;
    currentStackIndex = pInStruct->currentStackIndex;
}

void safe_VkDisplayPlanePropertiesKHR::initialize(const safe_VkDisplayPlanePropertiesKHR* src)
{
    currentDisplay = src->currentDisplay;
    currentStackIndex = src->currentStackIndex;
}

safe_VkDisplaySurfaceCreateInfoKHR::safe_VkDisplaySurfaceCreateInfoKHR(const VkDisplaySurfaceCreateInfoKHR* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	displayMode(pInStruct->displayMode),
	planeIndex(pInStruct->planeIndex),
	planeStackIndex(pInStruct->planeStackIndex),
	transform(pInStruct->transform),
	globalAlpha(pInStruct->globalAlpha),
	alphaMode(pInStruct->alphaMode),
	imageExtent(pInStruct->imageExtent)
{
}

safe_VkDisplaySurfaceCreateInfoKHR::safe_VkDisplaySurfaceCreateInfoKHR()
{}

safe_VkDisplaySurfaceCreateInfoKHR::safe_VkDisplaySurfaceCreateInfoKHR(const safe_VkDisplaySurfaceCreateInfoKHR& src)
{
    sType = src.sType;
    pNext = src.pNext;
    flags = src.flags;
    displayMode = src.displayMode;
    planeIndex = src.planeIndex;
    planeStackIndex = src.planeStackIndex;
    transform = src.transform;
    globalAlpha = src.globalAlpha;
    alphaMode = src.alphaMode;
    imageExtent = src.imageExtent;
}

safe_VkDisplaySurfaceCreateInfoKHR::~safe_VkDisplaySurfaceCreateInfoKHR()
{
}

void safe_VkDisplaySurfaceCreateInfoKHR::initialize(const VkDisplaySurfaceCreateInfoKHR* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    displayMode = pInStruct->displayMode;
    planeIndex = pInStruct->planeIndex;
    planeStackIndex = pInStruct->planeStackIndex;
    transform = pInStruct->transform;
    globalAlpha = pInStruct->globalAlpha;
    alphaMode = pInStruct->alphaMode;
    imageExtent = pInStruct->imageExtent;
}

void safe_VkDisplaySurfaceCreateInfoKHR::initialize(const safe_VkDisplaySurfaceCreateInfoKHR* src)
{
    sType = src->sType;
    pNext = src->pNext;
    flags = src->flags;
    displayMode = src->displayMode;
    planeIndex = src->planeIndex;
    planeStackIndex = src->planeStackIndex;
    transform = src->transform;
    globalAlpha = src->globalAlpha;
    alphaMode = src->alphaMode;
    imageExtent = src->imageExtent;
}

safe_VkDisplayPresentInfoKHR::safe_VkDisplayPresentInfoKHR(const VkDisplayPresentInfoKHR* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	srcRect(pInStruct->srcRect),
	dstRect(pInStruct->dstRect),
	persistent(pInStruct->persistent)
{
}

safe_VkDisplayPresentInfoKHR::safe_VkDisplayPresentInfoKHR()
{}

safe_VkDisplayPresentInfoKHR::safe_VkDisplayPresentInfoKHR(const safe_VkDisplayPresentInfoKHR& src)
{
    sType = src.sType;
    pNext = src.pNext;
    srcRect = src.srcRect;
    dstRect = src.dstRect;
    persistent = src.persistent;
}

safe_VkDisplayPresentInfoKHR::~safe_VkDisplayPresentInfoKHR()
{
}

void safe_VkDisplayPresentInfoKHR::initialize(const VkDisplayPresentInfoKHR* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    srcRect = pInStruct->srcRect;
    dstRect = pInStruct->dstRect;
    persistent = pInStruct->persistent;
}

void safe_VkDisplayPresentInfoKHR::initialize(const safe_VkDisplayPresentInfoKHR* src)
{
    sType = src->sType;
    pNext = src->pNext;
    srcRect = src->srcRect;
    dstRect = src->dstRect;
    persistent = src->persistent;
}
#ifdef VK_USE_PLATFORM_XLIB_KHR

safe_VkXlibSurfaceCreateInfoKHR::safe_VkXlibSurfaceCreateInfoKHR(const VkXlibSurfaceCreateInfoKHR* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	dpy(pInStruct->dpy),
	window(pInStruct->window)
{
}

safe_VkXlibSurfaceCreateInfoKHR::safe_VkXlibSurfaceCreateInfoKHR()
{}

safe_VkXlibSurfaceCreateInfoKHR::safe_VkXlibSurfaceCreateInfoKHR(const safe_VkXlibSurfaceCreateInfoKHR& src)
{
    sType = src.sType;
    pNext = src.pNext;
    flags = src.flags;
    dpy = src.dpy;
    window = src.window;
}

safe_VkXlibSurfaceCreateInfoKHR::~safe_VkXlibSurfaceCreateInfoKHR()
{
}

void safe_VkXlibSurfaceCreateInfoKHR::initialize(const VkXlibSurfaceCreateInfoKHR* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    dpy = pInStruct->dpy;
    window = pInStruct->window;
}

void safe_VkXlibSurfaceCreateInfoKHR::initialize(const safe_VkXlibSurfaceCreateInfoKHR* src)
{
    sType = src->sType;
    pNext = src->pNext;
    flags = src->flags;
    dpy = src->dpy;
    window = src->window;
}
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR

safe_VkXcbSurfaceCreateInfoKHR::safe_VkXcbSurfaceCreateInfoKHR(const VkXcbSurfaceCreateInfoKHR* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	connection(pInStruct->connection),
	window(pInStruct->window)
{
}

safe_VkXcbSurfaceCreateInfoKHR::safe_VkXcbSurfaceCreateInfoKHR()
{}

safe_VkXcbSurfaceCreateInfoKHR::safe_VkXcbSurfaceCreateInfoKHR(const safe_VkXcbSurfaceCreateInfoKHR& src)
{
    sType = src.sType;
    pNext = src.pNext;
    flags = src.flags;
    connection = src.connection;
    window = src.window;
}

safe_VkXcbSurfaceCreateInfoKHR::~safe_VkXcbSurfaceCreateInfoKHR()
{
}

void safe_VkXcbSurfaceCreateInfoKHR::initialize(const VkXcbSurfaceCreateInfoKHR* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    connection = pInStruct->connection;
    window = pInStruct->window;
}

void safe_VkXcbSurfaceCreateInfoKHR::initialize(const safe_VkXcbSurfaceCreateInfoKHR* src)
{
    sType = src->sType;
    pNext = src->pNext;
    flags = src->flags;
    connection = src->connection;
    window = src->window;
}
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR

safe_VkWaylandSurfaceCreateInfoKHR::safe_VkWaylandSurfaceCreateInfoKHR(const VkWaylandSurfaceCreateInfoKHR* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	display(pInStruct->display),
	surface(pInStruct->surface)
{
}

safe_VkWaylandSurfaceCreateInfoKHR::safe_VkWaylandSurfaceCreateInfoKHR()
{}

safe_VkWaylandSurfaceCreateInfoKHR::safe_VkWaylandSurfaceCreateInfoKHR(const safe_VkWaylandSurfaceCreateInfoKHR& src)
{
    sType = src.sType;
    pNext = src.pNext;
    flags = src.flags;
    display = src.display;
    surface = src.surface;
}

safe_VkWaylandSurfaceCreateInfoKHR::~safe_VkWaylandSurfaceCreateInfoKHR()
{
}

void safe_VkWaylandSurfaceCreateInfoKHR::initialize(const VkWaylandSurfaceCreateInfoKHR* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    display = pInStruct->display;
    surface = pInStruct->surface;
}

void safe_VkWaylandSurfaceCreateInfoKHR::initialize(const safe_VkWaylandSurfaceCreateInfoKHR* src)
{
    sType = src->sType;
    pNext = src->pNext;
    flags = src->flags;
    display = src->display;
    surface = src->surface;
}
#endif
#ifdef VK_USE_PLATFORM_MIR_KHR

safe_VkMirSurfaceCreateInfoKHR::safe_VkMirSurfaceCreateInfoKHR(const VkMirSurfaceCreateInfoKHR* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	connection(pInStruct->connection),
	mirSurface(pInStruct->mirSurface)
{
}

safe_VkMirSurfaceCreateInfoKHR::safe_VkMirSurfaceCreateInfoKHR()
{}

safe_VkMirSurfaceCreateInfoKHR::safe_VkMirSurfaceCreateInfoKHR(const safe_VkMirSurfaceCreateInfoKHR& src)
{
    sType = src.sType;
    pNext = src.pNext;
    flags = src.flags;
    connection = src.connection;
    mirSurface = src.mirSurface;
}

safe_VkMirSurfaceCreateInfoKHR::~safe_VkMirSurfaceCreateInfoKHR()
{
}

void safe_VkMirSurfaceCreateInfoKHR::initialize(const VkMirSurfaceCreateInfoKHR* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    connection = pInStruct->connection;
    mirSurface = pInStruct->mirSurface;
}

void safe_VkMirSurfaceCreateInfoKHR::initialize(const safe_VkMirSurfaceCreateInfoKHR* src)
{
    sType = src->sType;
    pNext = src->pNext;
    flags = src->flags;
    connection = src->connection;
    mirSurface = src->mirSurface;
}
#endif
#ifdef VK_USE_PLATFORM_ANDROID_KHR

safe_VkAndroidSurfaceCreateInfoKHR::safe_VkAndroidSurfaceCreateInfoKHR(const VkAndroidSurfaceCreateInfoKHR* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	window(pInStruct->window)
{
}

safe_VkAndroidSurfaceCreateInfoKHR::safe_VkAndroidSurfaceCreateInfoKHR()
{}

safe_VkAndroidSurfaceCreateInfoKHR::safe_VkAndroidSurfaceCreateInfoKHR(const safe_VkAndroidSurfaceCreateInfoKHR& src)
{
    sType = src.sType;
    pNext = src.pNext;
    flags = src.flags;
    window = src.window;
}

safe_VkAndroidSurfaceCreateInfoKHR::~safe_VkAndroidSurfaceCreateInfoKHR()
{
}

void safe_VkAndroidSurfaceCreateInfoKHR::initialize(const VkAndroidSurfaceCreateInfoKHR* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    window = pInStruct->window;
}

void safe_VkAndroidSurfaceCreateInfoKHR::initialize(const safe_VkAndroidSurfaceCreateInfoKHR* src)
{
    sType = src->sType;
    pNext = src->pNext;
    flags = src->flags;
    window = src->window;
}
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR

safe_VkWin32SurfaceCreateInfoKHR::safe_VkWin32SurfaceCreateInfoKHR(const VkWin32SurfaceCreateInfoKHR* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	hinstance(pInStruct->hinstance),
	hwnd(pInStruct->hwnd)
{
}

safe_VkWin32SurfaceCreateInfoKHR::safe_VkWin32SurfaceCreateInfoKHR()
{}

safe_VkWin32SurfaceCreateInfoKHR::safe_VkWin32SurfaceCreateInfoKHR(const safe_VkWin32SurfaceCreateInfoKHR& src)
{
    sType = src.sType;
    pNext = src.pNext;
    flags = src.flags;
    hinstance = src.hinstance;
    hwnd = src.hwnd;
}

safe_VkWin32SurfaceCreateInfoKHR::~safe_VkWin32SurfaceCreateInfoKHR()
{
}

void safe_VkWin32SurfaceCreateInfoKHR::initialize(const VkWin32SurfaceCreateInfoKHR* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    hinstance = pInStruct->hinstance;
    hwnd = pInStruct->hwnd;
}

void safe_VkWin32SurfaceCreateInfoKHR::initialize(const safe_VkWin32SurfaceCreateInfoKHR* src)
{
    sType = src->sType;
    pNext = src->pNext;
    flags = src->flags;
    hinstance = src->hinstance;
    hwnd = src->hwnd;
}
#endif

safe_VkDebugReportCallbackCreateInfoEXT::safe_VkDebugReportCallbackCreateInfoEXT(const VkDebugReportCallbackCreateInfoEXT* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	pfnCallback(pInStruct->pfnCallback),
	pUserData(pInStruct->pUserData)
{
}

safe_VkDebugReportCallbackCreateInfoEXT::safe_VkDebugReportCallbackCreateInfoEXT()
{}

safe_VkDebugReportCallbackCreateInfoEXT::safe_VkDebugReportCallbackCreateInfoEXT(const safe_VkDebugReportCallbackCreateInfoEXT& src)
{
    sType = src.sType;
    pNext = src.pNext;
    flags = src.flags;
    pfnCallback = src.pfnCallback;
    pUserData = src.pUserData;
}

safe_VkDebugReportCallbackCreateInfoEXT::~safe_VkDebugReportCallbackCreateInfoEXT()
{
}

void safe_VkDebugReportCallbackCreateInfoEXT::initialize(const VkDebugReportCallbackCreateInfoEXT* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    pfnCallback = pInStruct->pfnCallback;
    pUserData = pInStruct->pUserData;
}

void safe_VkDebugReportCallbackCreateInfoEXT::initialize(const safe_VkDebugReportCallbackCreateInfoEXT* src)
{
    sType = src->sType;
    pNext = src->pNext;
    flags = src->flags;
    pfnCallback = src->pfnCallback;
    pUserData = src->pUserData;
}

safe_VkPipelineRasterizationStateRasterizationOrderAMD::safe_VkPipelineRasterizationStateRasterizationOrderAMD(const VkPipelineRasterizationStateRasterizationOrderAMD* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	rasterizationOrder(pInStruct->rasterizationOrder)
{
}

safe_VkPipelineRasterizationStateRasterizationOrderAMD::safe_VkPipelineRasterizationStateRasterizationOrderAMD()
{}

safe_VkPipelineRasterizationStateRasterizationOrderAMD::safe_VkPipelineRasterizationStateRasterizationOrderAMD(const safe_VkPipelineRasterizationStateRasterizationOrderAMD& src)
{
    sType = src.sType;
    pNext = src.pNext;
    rasterizationOrder = src.rasterizationOrder;
}

safe_VkPipelineRasterizationStateRasterizationOrderAMD::~safe_VkPipelineRasterizationStateRasterizationOrderAMD()
{
}

void safe_VkPipelineRasterizationStateRasterizationOrderAMD::initialize(const VkPipelineRasterizationStateRasterizationOrderAMD* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    rasterizationOrder = pInStruct->rasterizationOrder;
}

void safe_VkPipelineRasterizationStateRasterizationOrderAMD::initialize(const safe_VkPipelineRasterizationStateRasterizationOrderAMD* src)
{
    sType = src->sType;
    pNext = src->pNext;
    rasterizationOrder = src->rasterizationOrder;
}

safe_VkDebugMarkerObjectNameInfoEXT::safe_VkDebugMarkerObjectNameInfoEXT(const VkDebugMarkerObjectNameInfoEXT* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	objectType(pInStruct->objectType),
	object(pInStruct->object),
	pObjectName(pInStruct->pObjectName)
{
}

safe_VkDebugMarkerObjectNameInfoEXT::safe_VkDebugMarkerObjectNameInfoEXT()
{}

safe_VkDebugMarkerObjectNameInfoEXT::safe_VkDebugMarkerObjectNameInfoEXT(const safe_VkDebugMarkerObjectNameInfoEXT& src)
{
    sType = src.sType;
    pNext = src.pNext;
    objectType = src.objectType;
    object = src.object;
    pObjectName = src.pObjectName;
}

safe_VkDebugMarkerObjectNameInfoEXT::~safe_VkDebugMarkerObjectNameInfoEXT()
{
}

void safe_VkDebugMarkerObjectNameInfoEXT::initialize(const VkDebugMarkerObjectNameInfoEXT* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    objectType = pInStruct->objectType;
    object = pInStruct->object;
    pObjectName = pInStruct->pObjectName;
}

void safe_VkDebugMarkerObjectNameInfoEXT::initialize(const safe_VkDebugMarkerObjectNameInfoEXT* src)
{
    sType = src->sType;
    pNext = src->pNext;
    objectType = src->objectType;
    object = src->object;
    pObjectName = src->pObjectName;
}

safe_VkDebugMarkerObjectTagInfoEXT::safe_VkDebugMarkerObjectTagInfoEXT(const VkDebugMarkerObjectTagInfoEXT* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	objectType(pInStruct->objectType),
	object(pInStruct->object),
	tagName(pInStruct->tagName),
	tagSize(pInStruct->tagSize),
	pTag(pInStruct->pTag)
{
}

safe_VkDebugMarkerObjectTagInfoEXT::safe_VkDebugMarkerObjectTagInfoEXT()
{}

safe_VkDebugMarkerObjectTagInfoEXT::safe_VkDebugMarkerObjectTagInfoEXT(const safe_VkDebugMarkerObjectTagInfoEXT& src)
{
    sType = src.sType;
    pNext = src.pNext;
    objectType = src.objectType;
    object = src.object;
    tagName = src.tagName;
    tagSize = src.tagSize;
    pTag = src.pTag;
}

safe_VkDebugMarkerObjectTagInfoEXT::~safe_VkDebugMarkerObjectTagInfoEXT()
{
}

void safe_VkDebugMarkerObjectTagInfoEXT::initialize(const VkDebugMarkerObjectTagInfoEXT* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    objectType = pInStruct->objectType;
    object = pInStruct->object;
    tagName = pInStruct->tagName;
    tagSize = pInStruct->tagSize;
    pTag = pInStruct->pTag;
}

void safe_VkDebugMarkerObjectTagInfoEXT::initialize(const safe_VkDebugMarkerObjectTagInfoEXT* src)
{
    sType = src->sType;
    pNext = src->pNext;
    objectType = src->objectType;
    object = src->object;
    tagName = src->tagName;
    tagSize = src->tagSize;
    pTag = src->pTag;
}

safe_VkDebugMarkerMarkerInfoEXT::safe_VkDebugMarkerMarkerInfoEXT(const VkDebugMarkerMarkerInfoEXT* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	pMarkerName(pInStruct->pMarkerName)
{
    for (uint32_t i=0; i<4; ++i) {
        color[i] = pInStruct->color[i];
    }
}

safe_VkDebugMarkerMarkerInfoEXT::safe_VkDebugMarkerMarkerInfoEXT()
{}

safe_VkDebugMarkerMarkerInfoEXT::safe_VkDebugMarkerMarkerInfoEXT(const safe_VkDebugMarkerMarkerInfoEXT& src)
{
    sType = src.sType;
    pNext = src.pNext;
    pMarkerName = src.pMarkerName;
    for (uint32_t i=0; i<4; ++i) {
        color[i] = src.color[i];
    }
}

safe_VkDebugMarkerMarkerInfoEXT::~safe_VkDebugMarkerMarkerInfoEXT()
{
}

void safe_VkDebugMarkerMarkerInfoEXT::initialize(const VkDebugMarkerMarkerInfoEXT* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    pMarkerName = pInStruct->pMarkerName;
    for (uint32_t i=0; i<4; ++i) {
        color[i] = pInStruct->color[i];
    }
}

void safe_VkDebugMarkerMarkerInfoEXT::initialize(const safe_VkDebugMarkerMarkerInfoEXT* src)
{
    sType = src->sType;
    pNext = src->pNext;
    pMarkerName = src->pMarkerName;
    for (uint32_t i=0; i<4; ++i) {
        color[i] = src->color[i];
    }
}

safe_VkDedicatedAllocationImageCreateInfoNV::safe_VkDedicatedAllocationImageCreateInfoNV(const VkDedicatedAllocationImageCreateInfoNV* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	dedicatedAllocation(pInStruct->dedicatedAllocation)
{
}

safe_VkDedicatedAllocationImageCreateInfoNV::safe_VkDedicatedAllocationImageCreateInfoNV()
{}

safe_VkDedicatedAllocationImageCreateInfoNV::safe_VkDedicatedAllocationImageCreateInfoNV(const safe_VkDedicatedAllocationImageCreateInfoNV& src)
{
    sType = src.sType;
    pNext = src.pNext;
    dedicatedAllocation = src.dedicatedAllocation;
}

safe_VkDedicatedAllocationImageCreateInfoNV::~safe_VkDedicatedAllocationImageCreateInfoNV()
{
}

void safe_VkDedicatedAllocationImageCreateInfoNV::initialize(const VkDedicatedAllocationImageCreateInfoNV* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    dedicatedAllocation = pInStruct->dedicatedAllocation;
}

void safe_VkDedicatedAllocationImageCreateInfoNV::initialize(const safe_VkDedicatedAllocationImageCreateInfoNV* src)
{
    sType = src->sType;
    pNext = src->pNext;
    dedicatedAllocation = src->dedicatedAllocation;
}

safe_VkDedicatedAllocationBufferCreateInfoNV::safe_VkDedicatedAllocationBufferCreateInfoNV(const VkDedicatedAllocationBufferCreateInfoNV* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	dedicatedAllocation(pInStruct->dedicatedAllocation)
{
}

safe_VkDedicatedAllocationBufferCreateInfoNV::safe_VkDedicatedAllocationBufferCreateInfoNV()
{}

safe_VkDedicatedAllocationBufferCreateInfoNV::safe_VkDedicatedAllocationBufferCreateInfoNV(const safe_VkDedicatedAllocationBufferCreateInfoNV& src)
{
    sType = src.sType;
    pNext = src.pNext;
    dedicatedAllocation = src.dedicatedAllocation;
}

safe_VkDedicatedAllocationBufferCreateInfoNV::~safe_VkDedicatedAllocationBufferCreateInfoNV()
{
}

void safe_VkDedicatedAllocationBufferCreateInfoNV::initialize(const VkDedicatedAllocationBufferCreateInfoNV* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    dedicatedAllocation = pInStruct->dedicatedAllocation;
}

void safe_VkDedicatedAllocationBufferCreateInfoNV::initialize(const safe_VkDedicatedAllocationBufferCreateInfoNV* src)
{
    sType = src->sType;
    pNext = src->pNext;
    dedicatedAllocation = src->dedicatedAllocation;
}

safe_VkDedicatedAllocationMemoryAllocateInfoNV::safe_VkDedicatedAllocationMemoryAllocateInfoNV(const VkDedicatedAllocationMemoryAllocateInfoNV* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	image(pInStruct->image),
	buffer(pInStruct->buffer)
{
}

safe_VkDedicatedAllocationMemoryAllocateInfoNV::safe_VkDedicatedAllocationMemoryAllocateInfoNV()
{}

safe_VkDedicatedAllocationMemoryAllocateInfoNV::safe_VkDedicatedAllocationMemoryAllocateInfoNV(const safe_VkDedicatedAllocationMemoryAllocateInfoNV& src)
{
    sType = src.sType;
    pNext = src.pNext;
    image = src.image;
    buffer = src.buffer;
}

safe_VkDedicatedAllocationMemoryAllocateInfoNV::~safe_VkDedicatedAllocationMemoryAllocateInfoNV()
{
}

void safe_VkDedicatedAllocationMemoryAllocateInfoNV::initialize(const VkDedicatedAllocationMemoryAllocateInfoNV* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    image = pInStruct->image;
    buffer = pInStruct->buffer;
}

void safe_VkDedicatedAllocationMemoryAllocateInfoNV::initialize(const safe_VkDedicatedAllocationMemoryAllocateInfoNV* src)
{
    sType = src->sType;
    pNext = src->pNext;
    image = src->image;
    buffer = src->buffer;
}

safe_VkExternalMemoryImageCreateInfoNV::safe_VkExternalMemoryImageCreateInfoNV(const VkExternalMemoryImageCreateInfoNV* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	handleTypes(pInStruct->handleTypes)
{
}

safe_VkExternalMemoryImageCreateInfoNV::safe_VkExternalMemoryImageCreateInfoNV()
{}

safe_VkExternalMemoryImageCreateInfoNV::safe_VkExternalMemoryImageCreateInfoNV(const safe_VkExternalMemoryImageCreateInfoNV& src)
{
    sType = src.sType;
    pNext = src.pNext;
    handleTypes = src.handleTypes;
}

safe_VkExternalMemoryImageCreateInfoNV::~safe_VkExternalMemoryImageCreateInfoNV()
{
}

void safe_VkExternalMemoryImageCreateInfoNV::initialize(const VkExternalMemoryImageCreateInfoNV* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    handleTypes = pInStruct->handleTypes;
}

void safe_VkExternalMemoryImageCreateInfoNV::initialize(const safe_VkExternalMemoryImageCreateInfoNV* src)
{
    sType = src->sType;
    pNext = src->pNext;
    handleTypes = src->handleTypes;
}

safe_VkExportMemoryAllocateInfoNV::safe_VkExportMemoryAllocateInfoNV(const VkExportMemoryAllocateInfoNV* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	handleTypes(pInStruct->handleTypes)
{
}

safe_VkExportMemoryAllocateInfoNV::safe_VkExportMemoryAllocateInfoNV()
{}

safe_VkExportMemoryAllocateInfoNV::safe_VkExportMemoryAllocateInfoNV(const safe_VkExportMemoryAllocateInfoNV& src)
{
    sType = src.sType;
    pNext = src.pNext;
    handleTypes = src.handleTypes;
}

safe_VkExportMemoryAllocateInfoNV::~safe_VkExportMemoryAllocateInfoNV()
{
}

void safe_VkExportMemoryAllocateInfoNV::initialize(const VkExportMemoryAllocateInfoNV* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    handleTypes = pInStruct->handleTypes;
}

void safe_VkExportMemoryAllocateInfoNV::initialize(const safe_VkExportMemoryAllocateInfoNV* src)
{
    sType = src->sType;
    pNext = src->pNext;
    handleTypes = src->handleTypes;
}
#ifdef VK_USE_PLATFORM_WIN32_KHR

safe_VkImportMemoryWin32HandleInfoNV::safe_VkImportMemoryWin32HandleInfoNV(const VkImportMemoryWin32HandleInfoNV* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	handleType(pInStruct->handleType),
	handle(pInStruct->handle)
{
}

safe_VkImportMemoryWin32HandleInfoNV::safe_VkImportMemoryWin32HandleInfoNV()
{}

safe_VkImportMemoryWin32HandleInfoNV::safe_VkImportMemoryWin32HandleInfoNV(const safe_VkImportMemoryWin32HandleInfoNV& src)
{
    sType = src.sType;
    pNext = src.pNext;
    handleType = src.handleType;
    handle = src.handle;
}

safe_VkImportMemoryWin32HandleInfoNV::~safe_VkImportMemoryWin32HandleInfoNV()
{
}

void safe_VkImportMemoryWin32HandleInfoNV::initialize(const VkImportMemoryWin32HandleInfoNV* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    handleType = pInStruct->handleType;
    handle = pInStruct->handle;
}

void safe_VkImportMemoryWin32HandleInfoNV::initialize(const safe_VkImportMemoryWin32HandleInfoNV* src)
{
    sType = src->sType;
    pNext = src->pNext;
    handleType = src->handleType;
    handle = src->handle;
}
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR

safe_VkExportMemoryWin32HandleInfoNV::safe_VkExportMemoryWin32HandleInfoNV(const VkExportMemoryWin32HandleInfoNV* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	pAttributes(nullptr),
	dwAccess(pInStruct->dwAccess)
{
    if (pInStruct->pAttributes) {
        pAttributes = new SECURITY_ATTRIBUTES(*pInStruct->pAttributes);
    }
}

safe_VkExportMemoryWin32HandleInfoNV::safe_VkExportMemoryWin32HandleInfoNV() : 
	pAttributes(nullptr)
{}

safe_VkExportMemoryWin32HandleInfoNV::safe_VkExportMemoryWin32HandleInfoNV(const safe_VkExportMemoryWin32HandleInfoNV& src)
{
    sType = src.sType;
    pNext = src.pNext;
    pAttributes = nullptr;
    dwAccess = src.dwAccess;
    if (src.pAttributes) {
        pAttributes = new SECURITY_ATTRIBUTES(*src.pAttributes);
    }
}

safe_VkExportMemoryWin32HandleInfoNV::~safe_VkExportMemoryWin32HandleInfoNV()
{
    if (pAttributes)
        delete pAttributes;
}

void safe_VkExportMemoryWin32HandleInfoNV::initialize(const VkExportMemoryWin32HandleInfoNV* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    pAttributes = nullptr;
    dwAccess = pInStruct->dwAccess;
    if (pInStruct->pAttributes) {
        pAttributes = new SECURITY_ATTRIBUTES(*pInStruct->pAttributes);
    }
}

void safe_VkExportMemoryWin32HandleInfoNV::initialize(const safe_VkExportMemoryWin32HandleInfoNV* src)
{
    sType = src->sType;
    pNext = src->pNext;
    pAttributes = nullptr;
    dwAccess = src->dwAccess;
    if (src->pAttributes) {
        pAttributes = new SECURITY_ATTRIBUTES(*src->pAttributes);
    }
}
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR

safe_VkWin32KeyedMutexAcquireReleaseInfoNV::safe_VkWin32KeyedMutexAcquireReleaseInfoNV(const VkWin32KeyedMutexAcquireReleaseInfoNV* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	acquireCount(pInStruct->acquireCount),
	pAcquireSyncs(nullptr),
	pAcquireKeys(nullptr),
	pAcquireTimeoutMilliseconds(nullptr),
	releaseCount(pInStruct->releaseCount),
	pReleaseSyncs(nullptr),
	pReleaseKeys(nullptr)
{
    if (acquireCount && pInStruct->pAcquireSyncs) {
        pAcquireSyncs = new VkDeviceMemory[acquireCount];
        for (uint32_t i=0; i<acquireCount; ++i) {
            pAcquireSyncs[i] = pInStruct->pAcquireSyncs[i];
        }
    }
    if (pInStruct->pAcquireKeys) {
        pAcquireKeys = new uint64_t[pInStruct->acquireCount];
        memcpy ((void *)pAcquireKeys, (void *)pInStruct->pAcquireKeys, sizeof(uint64_t)*pInStruct->acquireCount);
    }
    if (pInStruct->pAcquireTimeoutMilliseconds) {
        pAcquireTimeoutMilliseconds = new uint32_t[pInStruct->acquireCount];
        memcpy ((void *)pAcquireTimeoutMilliseconds, (void *)pInStruct->pAcquireTimeoutMilliseconds, sizeof(uint32_t)*pInStruct->acquireCount);
    }
    if (releaseCount && pInStruct->pReleaseSyncs) {
        pReleaseSyncs = new VkDeviceMemory[releaseCount];
        for (uint32_t i=0; i<releaseCount; ++i) {
            pReleaseSyncs[i] = pInStruct->pReleaseSyncs[i];
        }
    }
    if (pInStruct->pReleaseKeys) {
        pReleaseKeys = new uint64_t[pInStruct->releaseCount];
        memcpy ((void *)pReleaseKeys, (void *)pInStruct->pReleaseKeys, sizeof(uint64_t)*pInStruct->releaseCount);
    }
}

safe_VkWin32KeyedMutexAcquireReleaseInfoNV::safe_VkWin32KeyedMutexAcquireReleaseInfoNV() : 
	pAcquireSyncs(nullptr),
	pAcquireKeys(nullptr),
	pAcquireTimeoutMilliseconds(nullptr),
	pReleaseSyncs(nullptr),
	pReleaseKeys(nullptr)
{}

safe_VkWin32KeyedMutexAcquireReleaseInfoNV::safe_VkWin32KeyedMutexAcquireReleaseInfoNV(const safe_VkWin32KeyedMutexAcquireReleaseInfoNV& src)
{
    sType = src.sType;
    pNext = src.pNext;
    acquireCount = src.acquireCount;
    pAcquireSyncs = nullptr;
    pAcquireKeys = nullptr;
    pAcquireTimeoutMilliseconds = nullptr;
    releaseCount = src.releaseCount;
    pReleaseSyncs = nullptr;
    pReleaseKeys = nullptr;
    if (acquireCount && src.pAcquireSyncs) {
        pAcquireSyncs = new VkDeviceMemory[acquireCount];
        for (uint32_t i=0; i<acquireCount; ++i) {
            pAcquireSyncs[i] = src.pAcquireSyncs[i];
        }
    }
    if (src.pAcquireKeys) {
        pAcquireKeys = new uint64_t[src.acquireCount];
        memcpy ((void *)pAcquireKeys, (void *)src.pAcquireKeys, sizeof(uint64_t)*src.acquireCount);
    }
    if (src.pAcquireTimeoutMilliseconds) {
        pAcquireTimeoutMilliseconds = new uint32_t[src.acquireCount];
        memcpy ((void *)pAcquireTimeoutMilliseconds, (void *)src.pAcquireTimeoutMilliseconds, sizeof(uint32_t)*src.acquireCount);
    }
    if (releaseCount && src.pReleaseSyncs) {
        pReleaseSyncs = new VkDeviceMemory[releaseCount];
        for (uint32_t i=0; i<releaseCount; ++i) {
            pReleaseSyncs[i] = src.pReleaseSyncs[i];
        }
    }
    if (src.pReleaseKeys) {
        pReleaseKeys = new uint64_t[src.releaseCount];
        memcpy ((void *)pReleaseKeys, (void *)src.pReleaseKeys, sizeof(uint64_t)*src.releaseCount);
    }
}

safe_VkWin32KeyedMutexAcquireReleaseInfoNV::~safe_VkWin32KeyedMutexAcquireReleaseInfoNV()
{
    if (pAcquireSyncs)
        delete[] pAcquireSyncs;
    if (pAcquireKeys)
        delete[] pAcquireKeys;
    if (pAcquireTimeoutMilliseconds)
        delete[] pAcquireTimeoutMilliseconds;
    if (pReleaseSyncs)
        delete[] pReleaseSyncs;
    if (pReleaseKeys)
        delete[] pReleaseKeys;
}

void safe_VkWin32KeyedMutexAcquireReleaseInfoNV::initialize(const VkWin32KeyedMutexAcquireReleaseInfoNV* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    acquireCount = pInStruct->acquireCount;
    pAcquireSyncs = nullptr;
    pAcquireKeys = nullptr;
    pAcquireTimeoutMilliseconds = nullptr;
    releaseCount = pInStruct->releaseCount;
    pReleaseSyncs = nullptr;
    pReleaseKeys = nullptr;
    if (acquireCount && pInStruct->pAcquireSyncs) {
        pAcquireSyncs = new VkDeviceMemory[acquireCount];
        for (uint32_t i=0; i<acquireCount; ++i) {
            pAcquireSyncs[i] = pInStruct->pAcquireSyncs[i];
        }
    }
    if (pInStruct->pAcquireKeys) {
        pAcquireKeys = new uint64_t[pInStruct->acquireCount];
        memcpy ((void *)pAcquireKeys, (void *)pInStruct->pAcquireKeys, sizeof(uint64_t)*pInStruct->acquireCount);
    }
    if (pInStruct->pAcquireTimeoutMilliseconds) {
        pAcquireTimeoutMilliseconds = new uint32_t[pInStruct->acquireCount];
        memcpy ((void *)pAcquireTimeoutMilliseconds, (void *)pInStruct->pAcquireTimeoutMilliseconds, sizeof(uint32_t)*pInStruct->acquireCount);
    }
    if (releaseCount && pInStruct->pReleaseSyncs) {
        pReleaseSyncs = new VkDeviceMemory[releaseCount];
        for (uint32_t i=0; i<releaseCount; ++i) {
            pReleaseSyncs[i] = pInStruct->pReleaseSyncs[i];
        }
    }
    if (pInStruct->pReleaseKeys) {
        pReleaseKeys = new uint64_t[pInStruct->releaseCount];
        memcpy ((void *)pReleaseKeys, (void *)pInStruct->pReleaseKeys, sizeof(uint64_t)*pInStruct->releaseCount);
    }
}

void safe_VkWin32KeyedMutexAcquireReleaseInfoNV::initialize(const safe_VkWin32KeyedMutexAcquireReleaseInfoNV* src)
{
    sType = src->sType;
    pNext = src->pNext;
    acquireCount = src->acquireCount;
    pAcquireSyncs = nullptr;
    pAcquireKeys = nullptr;
    pAcquireTimeoutMilliseconds = nullptr;
    releaseCount = src->releaseCount;
    pReleaseSyncs = nullptr;
    pReleaseKeys = nullptr;
    if (acquireCount && src->pAcquireSyncs) {
        pAcquireSyncs = new VkDeviceMemory[acquireCount];
        for (uint32_t i=0; i<acquireCount; ++i) {
            pAcquireSyncs[i] = src->pAcquireSyncs[i];
        }
    }
    if (src->pAcquireKeys) {
        pAcquireKeys = new uint64_t[src->acquireCount];
        memcpy ((void *)pAcquireKeys, (void *)src->pAcquireKeys, sizeof(uint64_t)*src->acquireCount);
    }
    if (src->pAcquireTimeoutMilliseconds) {
        pAcquireTimeoutMilliseconds = new uint32_t[src->acquireCount];
        memcpy ((void *)pAcquireTimeoutMilliseconds, (void *)src->pAcquireTimeoutMilliseconds, sizeof(uint32_t)*src->acquireCount);
    }
    if (releaseCount && src->pReleaseSyncs) {
        pReleaseSyncs = new VkDeviceMemory[releaseCount];
        for (uint32_t i=0; i<releaseCount; ++i) {
            pReleaseSyncs[i] = src->pReleaseSyncs[i];
        }
    }
    if (src->pReleaseKeys) {
        pReleaseKeys = new uint64_t[src->releaseCount];
        memcpy ((void *)pReleaseKeys, (void *)src->pReleaseKeys, sizeof(uint64_t)*src->releaseCount);
    }
}
#endif

safe_VkValidationFlagsEXT::safe_VkValidationFlagsEXT(const VkValidationFlagsEXT* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	disabledValidationCheckCount(pInStruct->disabledValidationCheckCount),
	pDisabledValidationChecks(nullptr)
{
    if (pInStruct->pDisabledValidationChecks) {
        pDisabledValidationChecks = new VkValidationCheckEXT(*pInStruct->pDisabledValidationChecks);
    }
}

safe_VkValidationFlagsEXT::safe_VkValidationFlagsEXT() : 
	pDisabledValidationChecks(nullptr)
{}

safe_VkValidationFlagsEXT::safe_VkValidationFlagsEXT(const safe_VkValidationFlagsEXT& src)
{
    sType = src.sType;
    pNext = src.pNext;
    disabledValidationCheckCount = src.disabledValidationCheckCount;
    pDisabledValidationChecks = nullptr;
    if (src.pDisabledValidationChecks) {
        pDisabledValidationChecks = new VkValidationCheckEXT(*src.pDisabledValidationChecks);
    }
}

safe_VkValidationFlagsEXT::~safe_VkValidationFlagsEXT()
{
    if (pDisabledValidationChecks)
        delete pDisabledValidationChecks;
}

void safe_VkValidationFlagsEXT::initialize(const VkValidationFlagsEXT* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    disabledValidationCheckCount = pInStruct->disabledValidationCheckCount;
    pDisabledValidationChecks = nullptr;
    if (pInStruct->pDisabledValidationChecks) {
        pDisabledValidationChecks = new VkValidationCheckEXT(*pInStruct->pDisabledValidationChecks);
    }
}

void safe_VkValidationFlagsEXT::initialize(const safe_VkValidationFlagsEXT* src)
{
    sType = src->sType;
    pNext = src->pNext;
    disabledValidationCheckCount = src->disabledValidationCheckCount;
    pDisabledValidationChecks = nullptr;
    if (src->pDisabledValidationChecks) {
        pDisabledValidationChecks = new VkValidationCheckEXT(*src->pDisabledValidationChecks);
    }
}