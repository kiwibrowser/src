#ifndef __thread_check_h_
#define __thread_check_h_ 1

namespace threading {

/*
** Copyright (c) 2015-2016 The Khronos Group Inc.
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

/*
** This header is generated from the Khronos Vulkan XML API Registry.
**
*/



// declare only
VKAPI_ATTR VkResult VKAPI_CALL CreateInstance(
    const VkInstanceCreateInfo*                 pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkInstance*                                 pInstance);

// declare only
VKAPI_ATTR void VKAPI_CALL DestroyInstance(
    VkInstance                                  instance,
    const VkAllocationCallbacks*                pAllocator);

VKAPI_ATTR VkResult VKAPI_CALL EnumeratePhysicalDevices(
    VkInstance                                  instance,
    uint32_t*                                   pPhysicalDeviceCount,
    VkPhysicalDevice*                           pPhysicalDevices)
{
    dispatch_key key = get_dispatch_key(instance);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerInstanceDispatchTable *pTable = my_data->instance_dispatch_table;
    VkResult result;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, instance);
    }
    result = pTable->EnumeratePhysicalDevices(instance,pPhysicalDeviceCount,pPhysicalDevices);
    if (threadChecks) {
        finishReadObject(my_data, instance);
    } else {
        finishMultiThread();
    }
    return result;
}

// declare only
VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL GetInstanceProcAddr(
    VkInstance                                  instance,
    const char*                                 pName);

// declare only
VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL GetDeviceProcAddr(
    VkDevice                                    device,
    const char*                                 pName);

// declare only
VKAPI_ATTR VkResult VKAPI_CALL CreateDevice(
    VkPhysicalDevice                            physicalDevice,
    const VkDeviceCreateInfo*                   pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkDevice*                                   pDevice);

// declare only
VKAPI_ATTR void VKAPI_CALL DestroyDevice(
    VkDevice                                    device,
    const VkAllocationCallbacks*                pAllocator);

VKAPI_ATTR void VKAPI_CALL GetDeviceQueue(
    VkDevice                                    device,
    uint32_t                                    queueFamilyIndex,
    uint32_t                                    queueIndex,
    VkQueue*                                    pQueue)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
    }
    pTable->GetDeviceQueue(device,queueFamilyIndex,queueIndex,pQueue);
    if (threadChecks) {
        finishReadObject(my_data, device);
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR VkResult VKAPI_CALL QueueSubmit(
    VkQueue                                     queue,
    uint32_t                                    submitCount,
    const VkSubmitInfo*                         pSubmits,
    VkFence                                     fence)
{
    dispatch_key key = get_dispatch_key(queue);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, queue);
        for (uint32_t index=0;index<submitCount;index++) {
            for(uint32_t index2=0;index2<pSubmits[index].waitSemaphoreCount;index2++)
                startWriteObject(my_data, pSubmits[index].pWaitSemaphores[index2]);
            for(uint32_t index2=0;index2<pSubmits[index].signalSemaphoreCount;index2++)
                startWriteObject(my_data, pSubmits[index].pSignalSemaphores[index2]);
        }
        startWriteObject(my_data, fence);
        // Host access to queue must be externally synchronized
        // Host access to pSubmits[].pWaitSemaphores[],pSubmits[].pSignalSemaphores[] must be externally synchronized
        // Host access to fence must be externally synchronized
    }
    result = pTable->QueueSubmit(queue,submitCount,pSubmits,fence);
    if (threadChecks) {
        finishWriteObject(my_data, queue);
        for (uint32_t index=0;index<submitCount;index++) {
            for(uint32_t index2=0;index2<pSubmits[index].waitSemaphoreCount;index2++)
                finishWriteObject(my_data, pSubmits[index].pWaitSemaphores[index2]);
            for(uint32_t index2=0;index2<pSubmits[index].signalSemaphoreCount;index2++)
                finishWriteObject(my_data, pSubmits[index].pSignalSemaphores[index2]);
        }
        finishWriteObject(my_data, fence);
        // Host access to queue must be externally synchronized
        // Host access to pSubmits[].pWaitSemaphores[],pSubmits[].pSignalSemaphores[] must be externally synchronized
        // Host access to fence must be externally synchronized
    } else {
        finishMultiThread();
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL QueueWaitIdle(
    VkQueue                                     queue)
{
    dispatch_key key = get_dispatch_key(queue);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, queue);
    }
    result = pTable->QueueWaitIdle(queue);
    if (threadChecks) {
        finishReadObject(my_data, queue);
    } else {
        finishMultiThread();
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL DeviceWaitIdle(
    VkDevice                                    device)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
        // all sname:VkQueue objects created from pname:device must be externally synchronized between host accesses
    }
    result = pTable->DeviceWaitIdle(device);
    if (threadChecks) {
        finishReadObject(my_data, device);
        // all sname:VkQueue objects created from pname:device must be externally synchronized between host accesses
    } else {
        finishMultiThread();
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL AllocateMemory(
    VkDevice                                    device,
    const VkMemoryAllocateInfo*                 pAllocateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkDeviceMemory*                             pMemory)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
    }
    result = pTable->AllocateMemory(device,pAllocateInfo,pAllocator,pMemory);
    if (threadChecks) {
        finishReadObject(my_data, device);
    } else {
        finishMultiThread();
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL FreeMemory(
    VkDevice                                    device,
    VkDeviceMemory                              memory,
    const VkAllocationCallbacks*                pAllocator)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
        startWriteObject(my_data, memory);
        // Host access to memory must be externally synchronized
    }
    pTable->FreeMemory(device,memory,pAllocator);
    if (threadChecks) {
        finishReadObject(my_data, device);
        finishWriteObject(my_data, memory);
        // Host access to memory must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR VkResult VKAPI_CALL MapMemory(
    VkDevice                                    device,
    VkDeviceMemory                              memory,
    VkDeviceSize                                offset,
    VkDeviceSize                                size,
    VkMemoryMapFlags                            flags,
    void**                                      ppData)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
        startWriteObject(my_data, memory);
        // Host access to memory must be externally synchronized
    }
    result = pTable->MapMemory(device,memory,offset,size,flags,ppData);
    if (threadChecks) {
        finishReadObject(my_data, device);
        finishWriteObject(my_data, memory);
        // Host access to memory must be externally synchronized
    } else {
        finishMultiThread();
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL UnmapMemory(
    VkDevice                                    device,
    VkDeviceMemory                              memory)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
        startWriteObject(my_data, memory);
        // Host access to memory must be externally synchronized
    }
    pTable->UnmapMemory(device,memory);
    if (threadChecks) {
        finishReadObject(my_data, device);
        finishWriteObject(my_data, memory);
        // Host access to memory must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR VkResult VKAPI_CALL FlushMappedMemoryRanges(
    VkDevice                                    device,
    uint32_t                                    memoryRangeCount,
    const VkMappedMemoryRange*                  pMemoryRanges)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
    }
    result = pTable->FlushMappedMemoryRanges(device,memoryRangeCount,pMemoryRanges);
    if (threadChecks) {
        finishReadObject(my_data, device);
    } else {
        finishMultiThread();
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL InvalidateMappedMemoryRanges(
    VkDevice                                    device,
    uint32_t                                    memoryRangeCount,
    const VkMappedMemoryRange*                  pMemoryRanges)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
    }
    result = pTable->InvalidateMappedMemoryRanges(device,memoryRangeCount,pMemoryRanges);
    if (threadChecks) {
        finishReadObject(my_data, device);
    } else {
        finishMultiThread();
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL GetDeviceMemoryCommitment(
    VkDevice                                    device,
    VkDeviceMemory                              memory,
    VkDeviceSize*                               pCommittedMemoryInBytes)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
        startReadObject(my_data, memory);
    }
    pTable->GetDeviceMemoryCommitment(device,memory,pCommittedMemoryInBytes);
    if (threadChecks) {
        finishReadObject(my_data, device);
        finishReadObject(my_data, memory);
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR VkResult VKAPI_CALL BindBufferMemory(
    VkDevice                                    device,
    VkBuffer                                    buffer,
    VkDeviceMemory                              memory,
    VkDeviceSize                                memoryOffset)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
        startWriteObject(my_data, buffer);
        startReadObject(my_data, memory);
        // Host access to buffer must be externally synchronized
    }
    result = pTable->BindBufferMemory(device,buffer,memory,memoryOffset);
    if (threadChecks) {
        finishReadObject(my_data, device);
        finishWriteObject(my_data, buffer);
        finishReadObject(my_data, memory);
        // Host access to buffer must be externally synchronized
    } else {
        finishMultiThread();
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL BindImageMemory(
    VkDevice                                    device,
    VkImage                                     image,
    VkDeviceMemory                              memory,
    VkDeviceSize                                memoryOffset)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
        startWriteObject(my_data, image);
        startReadObject(my_data, memory);
        // Host access to image must be externally synchronized
    }
    result = pTable->BindImageMemory(device,image,memory,memoryOffset);
    if (threadChecks) {
        finishReadObject(my_data, device);
        finishWriteObject(my_data, image);
        finishReadObject(my_data, memory);
        // Host access to image must be externally synchronized
    } else {
        finishMultiThread();
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL GetBufferMemoryRequirements(
    VkDevice                                    device,
    VkBuffer                                    buffer,
    VkMemoryRequirements*                       pMemoryRequirements)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
        startReadObject(my_data, buffer);
    }
    pTable->GetBufferMemoryRequirements(device,buffer,pMemoryRequirements);
    if (threadChecks) {
        finishReadObject(my_data, device);
        finishReadObject(my_data, buffer);
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR void VKAPI_CALL GetImageMemoryRequirements(
    VkDevice                                    device,
    VkImage                                     image,
    VkMemoryRequirements*                       pMemoryRequirements)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
        startReadObject(my_data, image);
    }
    pTable->GetImageMemoryRequirements(device,image,pMemoryRequirements);
    if (threadChecks) {
        finishReadObject(my_data, device);
        finishReadObject(my_data, image);
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR void VKAPI_CALL GetImageSparseMemoryRequirements(
    VkDevice                                    device,
    VkImage                                     image,
    uint32_t*                                   pSparseMemoryRequirementCount,
    VkSparseImageMemoryRequirements*            pSparseMemoryRequirements)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
        startReadObject(my_data, image);
    }
    pTable->GetImageSparseMemoryRequirements(device,image,pSparseMemoryRequirementCount,pSparseMemoryRequirements);
    if (threadChecks) {
        finishReadObject(my_data, device);
        finishReadObject(my_data, image);
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR VkResult VKAPI_CALL QueueBindSparse(
    VkQueue                                     queue,
    uint32_t                                    bindInfoCount,
    const VkBindSparseInfo*                     pBindInfo,
    VkFence                                     fence)
{
    dispatch_key key = get_dispatch_key(queue);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, queue);
        for (uint32_t index=0;index<bindInfoCount;index++) {
            for(uint32_t index2=0;index2<pBindInfo[index].waitSemaphoreCount;index2++)
                startWriteObject(my_data, pBindInfo[index].pWaitSemaphores[index2]);
            for(uint32_t index2=0;index2<pBindInfo[index].signalSemaphoreCount;index2++)
                startWriteObject(my_data, pBindInfo[index].pSignalSemaphores[index2]);
            for(uint32_t index2=0;index2<pBindInfo[index].bufferBindCount;index2++)
                startWriteObject(my_data, pBindInfo[index].pBufferBinds[index2].buffer);
            for(uint32_t index2=0;index2<pBindInfo[index].imageOpaqueBindCount;index2++)
                startWriteObject(my_data, pBindInfo[index].pImageOpaqueBinds[index2].image);
            for(uint32_t index2=0;index2<pBindInfo[index].imageBindCount;index2++)
                startWriteObject(my_data, pBindInfo[index].pImageBinds[index2].image);
        }
        startWriteObject(my_data, fence);
        // Host access to queue must be externally synchronized
        // Host access to pBindInfo[].pWaitSemaphores[],pBindInfo[].pSignalSemaphores[],pBindInfo[].pBufferBinds[].buffer,pBindInfo[].pImageOpaqueBinds[].image,pBindInfo[].pImageBinds[].image must be externally synchronized
        // Host access to fence must be externally synchronized
    }
    result = pTable->QueueBindSparse(queue,bindInfoCount,pBindInfo,fence);
    if (threadChecks) {
        finishWriteObject(my_data, queue);
        for (uint32_t index=0;index<bindInfoCount;index++) {
            for(uint32_t index2=0;index2<pBindInfo[index].waitSemaphoreCount;index2++)
                finishWriteObject(my_data, pBindInfo[index].pWaitSemaphores[index2]);
            for(uint32_t index2=0;index2<pBindInfo[index].signalSemaphoreCount;index2++)
                finishWriteObject(my_data, pBindInfo[index].pSignalSemaphores[index2]);
            for(uint32_t index2=0;index2<pBindInfo[index].bufferBindCount;index2++)
                finishWriteObject(my_data, pBindInfo[index].pBufferBinds[index2].buffer);
            for(uint32_t index2=0;index2<pBindInfo[index].imageOpaqueBindCount;index2++)
                finishWriteObject(my_data, pBindInfo[index].pImageOpaqueBinds[index2].image);
            for(uint32_t index2=0;index2<pBindInfo[index].imageBindCount;index2++)
                finishWriteObject(my_data, pBindInfo[index].pImageBinds[index2].image);
        }
        finishWriteObject(my_data, fence);
        // Host access to queue must be externally synchronized
        // Host access to pBindInfo[].pWaitSemaphores[],pBindInfo[].pSignalSemaphores[],pBindInfo[].pBufferBinds[].buffer,pBindInfo[].pImageOpaqueBinds[].image,pBindInfo[].pImageBinds[].image must be externally synchronized
        // Host access to fence must be externally synchronized
    } else {
        finishMultiThread();
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateFence(
    VkDevice                                    device,
    const VkFenceCreateInfo*                    pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkFence*                                    pFence)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
    }
    result = pTable->CreateFence(device,pCreateInfo,pAllocator,pFence);
    if (threadChecks) {
        finishReadObject(my_data, device);
    } else {
        finishMultiThread();
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyFence(
    VkDevice                                    device,
    VkFence                                     fence,
    const VkAllocationCallbacks*                pAllocator)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
        startWriteObject(my_data, fence);
        // Host access to fence must be externally synchronized
    }
    pTable->DestroyFence(device,fence,pAllocator);
    if (threadChecks) {
        finishReadObject(my_data, device);
        finishWriteObject(my_data, fence);
        // Host access to fence must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR VkResult VKAPI_CALL ResetFences(
    VkDevice                                    device,
    uint32_t                                    fenceCount,
    const VkFence*                              pFences)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
        for (uint32_t index=0;index<fenceCount;index++) {
            startWriteObject(my_data, pFences[index]);
        }
        // Host access to each member of pFences must be externally synchronized
    }
    result = pTable->ResetFences(device,fenceCount,pFences);
    if (threadChecks) {
        finishReadObject(my_data, device);
        for (uint32_t index=0;index<fenceCount;index++) {
            finishWriteObject(my_data, pFences[index]);
        }
        // Host access to each member of pFences must be externally synchronized
    } else {
        finishMultiThread();
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL GetFenceStatus(
    VkDevice                                    device,
    VkFence                                     fence)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
        startReadObject(my_data, fence);
    }
    result = pTable->GetFenceStatus(device,fence);
    if (threadChecks) {
        finishReadObject(my_data, device);
        finishReadObject(my_data, fence);
    } else {
        finishMultiThread();
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL WaitForFences(
    VkDevice                                    device,
    uint32_t                                    fenceCount,
    const VkFence*                              pFences,
    VkBool32                                    waitAll,
    uint64_t                                    timeout)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
        for (uint32_t index=0;index<fenceCount;index++) {
            startReadObject(my_data, pFences[index]);
        }
    }
    result = pTable->WaitForFences(device,fenceCount,pFences,waitAll,timeout);
    if (threadChecks) {
        finishReadObject(my_data, device);
        for (uint32_t index=0;index<fenceCount;index++) {
            finishReadObject(my_data, pFences[index]);
        }
    } else {
        finishMultiThread();
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateSemaphore(
    VkDevice                                    device,
    const VkSemaphoreCreateInfo*                pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkSemaphore*                                pSemaphore)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
    }
    result = pTable->CreateSemaphore(device,pCreateInfo,pAllocator,pSemaphore);
    if (threadChecks) {
        finishReadObject(my_data, device);
    } else {
        finishMultiThread();
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroySemaphore(
    VkDevice                                    device,
    VkSemaphore                                 semaphore,
    const VkAllocationCallbacks*                pAllocator)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
        startWriteObject(my_data, semaphore);
        // Host access to semaphore must be externally synchronized
    }
    pTable->DestroySemaphore(device,semaphore,pAllocator);
    if (threadChecks) {
        finishReadObject(my_data, device);
        finishWriteObject(my_data, semaphore);
        // Host access to semaphore must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR VkResult VKAPI_CALL CreateEvent(
    VkDevice                                    device,
    const VkEventCreateInfo*                    pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkEvent*                                    pEvent)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
    }
    result = pTable->CreateEvent(device,pCreateInfo,pAllocator,pEvent);
    if (threadChecks) {
        finishReadObject(my_data, device);
    } else {
        finishMultiThread();
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyEvent(
    VkDevice                                    device,
    VkEvent                                     event,
    const VkAllocationCallbacks*                pAllocator)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
        startWriteObject(my_data, event);
        // Host access to event must be externally synchronized
    }
    pTable->DestroyEvent(device,event,pAllocator);
    if (threadChecks) {
        finishReadObject(my_data, device);
        finishWriteObject(my_data, event);
        // Host access to event must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR VkResult VKAPI_CALL GetEventStatus(
    VkDevice                                    device,
    VkEvent                                     event)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
        startReadObject(my_data, event);
    }
    result = pTable->GetEventStatus(device,event);
    if (threadChecks) {
        finishReadObject(my_data, device);
        finishReadObject(my_data, event);
    } else {
        finishMultiThread();
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL SetEvent(
    VkDevice                                    device,
    VkEvent                                     event)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
        startWriteObject(my_data, event);
        // Host access to event must be externally synchronized
    }
    result = pTable->SetEvent(device,event);
    if (threadChecks) {
        finishReadObject(my_data, device);
        finishWriteObject(my_data, event);
        // Host access to event must be externally synchronized
    } else {
        finishMultiThread();
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL ResetEvent(
    VkDevice                                    device,
    VkEvent                                     event)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
        startWriteObject(my_data, event);
        // Host access to event must be externally synchronized
    }
    result = pTable->ResetEvent(device,event);
    if (threadChecks) {
        finishReadObject(my_data, device);
        finishWriteObject(my_data, event);
        // Host access to event must be externally synchronized
    } else {
        finishMultiThread();
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateQueryPool(
    VkDevice                                    device,
    const VkQueryPoolCreateInfo*                pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkQueryPool*                                pQueryPool)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
    }
    result = pTable->CreateQueryPool(device,pCreateInfo,pAllocator,pQueryPool);
    if (threadChecks) {
        finishReadObject(my_data, device);
    } else {
        finishMultiThread();
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyQueryPool(
    VkDevice                                    device,
    VkQueryPool                                 queryPool,
    const VkAllocationCallbacks*                pAllocator)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
        startWriteObject(my_data, queryPool);
        // Host access to queryPool must be externally synchronized
    }
    pTable->DestroyQueryPool(device,queryPool,pAllocator);
    if (threadChecks) {
        finishReadObject(my_data, device);
        finishWriteObject(my_data, queryPool);
        // Host access to queryPool must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR VkResult VKAPI_CALL GetQueryPoolResults(
    VkDevice                                    device,
    VkQueryPool                                 queryPool,
    uint32_t                                    firstQuery,
    uint32_t                                    queryCount,
    size_t                                      dataSize,
    void*                                       pData,
    VkDeviceSize                                stride,
    VkQueryResultFlags                          flags)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
        startReadObject(my_data, queryPool);
    }
    result = pTable->GetQueryPoolResults(device,queryPool,firstQuery,queryCount,dataSize,pData,stride,flags);
    if (threadChecks) {
        finishReadObject(my_data, device);
        finishReadObject(my_data, queryPool);
    } else {
        finishMultiThread();
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateBuffer(
    VkDevice                                    device,
    const VkBufferCreateInfo*                   pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkBuffer*                                   pBuffer)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
    }
    result = pTable->CreateBuffer(device,pCreateInfo,pAllocator,pBuffer);
    if (threadChecks) {
        finishReadObject(my_data, device);
    } else {
        finishMultiThread();
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyBuffer(
    VkDevice                                    device,
    VkBuffer                                    buffer,
    const VkAllocationCallbacks*                pAllocator)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
        startWriteObject(my_data, buffer);
        // Host access to buffer must be externally synchronized
    }
    pTable->DestroyBuffer(device,buffer,pAllocator);
    if (threadChecks) {
        finishReadObject(my_data, device);
        finishWriteObject(my_data, buffer);
        // Host access to buffer must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR VkResult VKAPI_CALL CreateBufferView(
    VkDevice                                    device,
    const VkBufferViewCreateInfo*               pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkBufferView*                               pView)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
    }
    result = pTable->CreateBufferView(device,pCreateInfo,pAllocator,pView);
    if (threadChecks) {
        finishReadObject(my_data, device);
    } else {
        finishMultiThread();
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyBufferView(
    VkDevice                                    device,
    VkBufferView                                bufferView,
    const VkAllocationCallbacks*                pAllocator)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
        startWriteObject(my_data, bufferView);
        // Host access to bufferView must be externally synchronized
    }
    pTable->DestroyBufferView(device,bufferView,pAllocator);
    if (threadChecks) {
        finishReadObject(my_data, device);
        finishWriteObject(my_data, bufferView);
        // Host access to bufferView must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR VkResult VKAPI_CALL CreateImage(
    VkDevice                                    device,
    const VkImageCreateInfo*                    pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkImage*                                    pImage)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
    }
    result = pTable->CreateImage(device,pCreateInfo,pAllocator,pImage);
    if (threadChecks) {
        finishReadObject(my_data, device);
    } else {
        finishMultiThread();
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyImage(
    VkDevice                                    device,
    VkImage                                     image,
    const VkAllocationCallbacks*                pAllocator)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
        startWriteObject(my_data, image);
        // Host access to image must be externally synchronized
    }
    pTable->DestroyImage(device,image,pAllocator);
    if (threadChecks) {
        finishReadObject(my_data, device);
        finishWriteObject(my_data, image);
        // Host access to image must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR void VKAPI_CALL GetImageSubresourceLayout(
    VkDevice                                    device,
    VkImage                                     image,
    const VkImageSubresource*                   pSubresource,
    VkSubresourceLayout*                        pLayout)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
        startReadObject(my_data, image);
    }
    pTable->GetImageSubresourceLayout(device,image,pSubresource,pLayout);
    if (threadChecks) {
        finishReadObject(my_data, device);
        finishReadObject(my_data, image);
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR VkResult VKAPI_CALL CreateImageView(
    VkDevice                                    device,
    const VkImageViewCreateInfo*                pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkImageView*                                pView)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
    }
    result = pTable->CreateImageView(device,pCreateInfo,pAllocator,pView);
    if (threadChecks) {
        finishReadObject(my_data, device);
    } else {
        finishMultiThread();
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyImageView(
    VkDevice                                    device,
    VkImageView                                 imageView,
    const VkAllocationCallbacks*                pAllocator)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
        startWriteObject(my_data, imageView);
        // Host access to imageView must be externally synchronized
    }
    pTable->DestroyImageView(device,imageView,pAllocator);
    if (threadChecks) {
        finishReadObject(my_data, device);
        finishWriteObject(my_data, imageView);
        // Host access to imageView must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR VkResult VKAPI_CALL CreateShaderModule(
    VkDevice                                    device,
    const VkShaderModuleCreateInfo*             pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkShaderModule*                             pShaderModule)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
    }
    result = pTable->CreateShaderModule(device,pCreateInfo,pAllocator,pShaderModule);
    if (threadChecks) {
        finishReadObject(my_data, device);
    } else {
        finishMultiThread();
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyShaderModule(
    VkDevice                                    device,
    VkShaderModule                              shaderModule,
    const VkAllocationCallbacks*                pAllocator)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
        startWriteObject(my_data, shaderModule);
        // Host access to shaderModule must be externally synchronized
    }
    pTable->DestroyShaderModule(device,shaderModule,pAllocator);
    if (threadChecks) {
        finishReadObject(my_data, device);
        finishWriteObject(my_data, shaderModule);
        // Host access to shaderModule must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR VkResult VKAPI_CALL CreatePipelineCache(
    VkDevice                                    device,
    const VkPipelineCacheCreateInfo*            pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkPipelineCache*                            pPipelineCache)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
    }
    result = pTable->CreatePipelineCache(device,pCreateInfo,pAllocator,pPipelineCache);
    if (threadChecks) {
        finishReadObject(my_data, device);
    } else {
        finishMultiThread();
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyPipelineCache(
    VkDevice                                    device,
    VkPipelineCache                             pipelineCache,
    const VkAllocationCallbacks*                pAllocator)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
        startWriteObject(my_data, pipelineCache);
        // Host access to pipelineCache must be externally synchronized
    }
    pTable->DestroyPipelineCache(device,pipelineCache,pAllocator);
    if (threadChecks) {
        finishReadObject(my_data, device);
        finishWriteObject(my_data, pipelineCache);
        // Host access to pipelineCache must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR VkResult VKAPI_CALL GetPipelineCacheData(
    VkDevice                                    device,
    VkPipelineCache                             pipelineCache,
    size_t*                                     pDataSize,
    void*                                       pData)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
        startReadObject(my_data, pipelineCache);
    }
    result = pTable->GetPipelineCacheData(device,pipelineCache,pDataSize,pData);
    if (threadChecks) {
        finishReadObject(my_data, device);
        finishReadObject(my_data, pipelineCache);
    } else {
        finishMultiThread();
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL MergePipelineCaches(
    VkDevice                                    device,
    VkPipelineCache                             dstCache,
    uint32_t                                    srcCacheCount,
    const VkPipelineCache*                      pSrcCaches)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
        startWriteObject(my_data, dstCache);
        for (uint32_t index=0;index<srcCacheCount;index++) {
            startReadObject(my_data, pSrcCaches[index]);
        }
        // Host access to dstCache must be externally synchronized
    }
    result = pTable->MergePipelineCaches(device,dstCache,srcCacheCount,pSrcCaches);
    if (threadChecks) {
        finishReadObject(my_data, device);
        finishWriteObject(my_data, dstCache);
        for (uint32_t index=0;index<srcCacheCount;index++) {
            finishReadObject(my_data, pSrcCaches[index]);
        }
        // Host access to dstCache must be externally synchronized
    } else {
        finishMultiThread();
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateGraphicsPipelines(
    VkDevice                                    device,
    VkPipelineCache                             pipelineCache,
    uint32_t                                    createInfoCount,
    const VkGraphicsPipelineCreateInfo*         pCreateInfos,
    const VkAllocationCallbacks*                pAllocator,
    VkPipeline*                                 pPipelines)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
        startReadObject(my_data, pipelineCache);
    }
    result = pTable->CreateGraphicsPipelines(device,pipelineCache,createInfoCount,pCreateInfos,pAllocator,pPipelines);
    if (threadChecks) {
        finishReadObject(my_data, device);
        finishReadObject(my_data, pipelineCache);
    } else {
        finishMultiThread();
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateComputePipelines(
    VkDevice                                    device,
    VkPipelineCache                             pipelineCache,
    uint32_t                                    createInfoCount,
    const VkComputePipelineCreateInfo*          pCreateInfos,
    const VkAllocationCallbacks*                pAllocator,
    VkPipeline*                                 pPipelines)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
        startReadObject(my_data, pipelineCache);
    }
    result = pTable->CreateComputePipelines(device,pipelineCache,createInfoCount,pCreateInfos,pAllocator,pPipelines);
    if (threadChecks) {
        finishReadObject(my_data, device);
        finishReadObject(my_data, pipelineCache);
    } else {
        finishMultiThread();
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyPipeline(
    VkDevice                                    device,
    VkPipeline                                  pipeline,
    const VkAllocationCallbacks*                pAllocator)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
        startWriteObject(my_data, pipeline);
        // Host access to pipeline must be externally synchronized
    }
    pTable->DestroyPipeline(device,pipeline,pAllocator);
    if (threadChecks) {
        finishReadObject(my_data, device);
        finishWriteObject(my_data, pipeline);
        // Host access to pipeline must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR VkResult VKAPI_CALL CreatePipelineLayout(
    VkDevice                                    device,
    const VkPipelineLayoutCreateInfo*           pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkPipelineLayout*                           pPipelineLayout)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
    }
    result = pTable->CreatePipelineLayout(device,pCreateInfo,pAllocator,pPipelineLayout);
    if (threadChecks) {
        finishReadObject(my_data, device);
    } else {
        finishMultiThread();
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyPipelineLayout(
    VkDevice                                    device,
    VkPipelineLayout                            pipelineLayout,
    const VkAllocationCallbacks*                pAllocator)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
        startWriteObject(my_data, pipelineLayout);
        // Host access to pipelineLayout must be externally synchronized
    }
    pTable->DestroyPipelineLayout(device,pipelineLayout,pAllocator);
    if (threadChecks) {
        finishReadObject(my_data, device);
        finishWriteObject(my_data, pipelineLayout);
        // Host access to pipelineLayout must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR VkResult VKAPI_CALL CreateSampler(
    VkDevice                                    device,
    const VkSamplerCreateInfo*                  pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkSampler*                                  pSampler)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
    }
    result = pTable->CreateSampler(device,pCreateInfo,pAllocator,pSampler);
    if (threadChecks) {
        finishReadObject(my_data, device);
    } else {
        finishMultiThread();
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroySampler(
    VkDevice                                    device,
    VkSampler                                   sampler,
    const VkAllocationCallbacks*                pAllocator)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
        startWriteObject(my_data, sampler);
        // Host access to sampler must be externally synchronized
    }
    pTable->DestroySampler(device,sampler,pAllocator);
    if (threadChecks) {
        finishReadObject(my_data, device);
        finishWriteObject(my_data, sampler);
        // Host access to sampler must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR VkResult VKAPI_CALL CreateDescriptorSetLayout(
    VkDevice                                    device,
    const VkDescriptorSetLayoutCreateInfo*      pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkDescriptorSetLayout*                      pSetLayout)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
    }
    result = pTable->CreateDescriptorSetLayout(device,pCreateInfo,pAllocator,pSetLayout);
    if (threadChecks) {
        finishReadObject(my_data, device);
    } else {
        finishMultiThread();
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyDescriptorSetLayout(
    VkDevice                                    device,
    VkDescriptorSetLayout                       descriptorSetLayout,
    const VkAllocationCallbacks*                pAllocator)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
        startWriteObject(my_data, descriptorSetLayout);
        // Host access to descriptorSetLayout must be externally synchronized
    }
    pTable->DestroyDescriptorSetLayout(device,descriptorSetLayout,pAllocator);
    if (threadChecks) {
        finishReadObject(my_data, device);
        finishWriteObject(my_data, descriptorSetLayout);
        // Host access to descriptorSetLayout must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR VkResult VKAPI_CALL CreateDescriptorPool(
    VkDevice                                    device,
    const VkDescriptorPoolCreateInfo*           pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkDescriptorPool*                           pDescriptorPool)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
    }
    result = pTable->CreateDescriptorPool(device,pCreateInfo,pAllocator,pDescriptorPool);
    if (threadChecks) {
        finishReadObject(my_data, device);
    } else {
        finishMultiThread();
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyDescriptorPool(
    VkDevice                                    device,
    VkDescriptorPool                            descriptorPool,
    const VkAllocationCallbacks*                pAllocator)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
        startWriteObject(my_data, descriptorPool);
        // Host access to descriptorPool must be externally synchronized
    }
    pTable->DestroyDescriptorPool(device,descriptorPool,pAllocator);
    if (threadChecks) {
        finishReadObject(my_data, device);
        finishWriteObject(my_data, descriptorPool);
        // Host access to descriptorPool must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR VkResult VKAPI_CALL ResetDescriptorPool(
    VkDevice                                    device,
    VkDescriptorPool                            descriptorPool,
    VkDescriptorPoolResetFlags                  flags)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
        startWriteObject(my_data, descriptorPool);
        // Host access to descriptorPool must be externally synchronized
        // any sname:VkDescriptorSet objects allocated from pname:descriptorPool must be externally synchronized between host accesses
    }
    result = pTable->ResetDescriptorPool(device,descriptorPool,flags);
    if (threadChecks) {
        finishReadObject(my_data, device);
        finishWriteObject(my_data, descriptorPool);
        // Host access to descriptorPool must be externally synchronized
        // any sname:VkDescriptorSet objects allocated from pname:descriptorPool must be externally synchronized between host accesses
    } else {
        finishMultiThread();
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL AllocateDescriptorSets(
    VkDevice                                    device,
    const VkDescriptorSetAllocateInfo*          pAllocateInfo,
    VkDescriptorSet*                            pDescriptorSets)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
        startWriteObject(my_data, pAllocateInfo->descriptorPool);
        // Host access to pAllocateInfo::descriptorPool must be externally synchronized
    }
    result = pTable->AllocateDescriptorSets(device,pAllocateInfo,pDescriptorSets);
    if (threadChecks) {
        finishReadObject(my_data, device);
        finishWriteObject(my_data, pAllocateInfo->descriptorPool);
        // Host access to pAllocateInfo::descriptorPool must be externally synchronized
    } else {
        finishMultiThread();
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL FreeDescriptorSets(
    VkDevice                                    device,
    VkDescriptorPool                            descriptorPool,
    uint32_t                                    descriptorSetCount,
    const VkDescriptorSet*                      pDescriptorSets)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
        startWriteObject(my_data, descriptorPool);
        for (uint32_t index=0;index<descriptorSetCount;index++) {
            startWriteObject(my_data, pDescriptorSets[index]);
        }
        // Host access to descriptorPool must be externally synchronized
        // Host access to each member of pDescriptorSets must be externally synchronized
    }
    result = pTable->FreeDescriptorSets(device,descriptorPool,descriptorSetCount,pDescriptorSets);
    if (threadChecks) {
        finishReadObject(my_data, device);
        finishWriteObject(my_data, descriptorPool);
        for (uint32_t index=0;index<descriptorSetCount;index++) {
            finishWriteObject(my_data, pDescriptorSets[index]);
        }
        // Host access to descriptorPool must be externally synchronized
        // Host access to each member of pDescriptorSets must be externally synchronized
    } else {
        finishMultiThread();
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL UpdateDescriptorSets(
    VkDevice                                    device,
    uint32_t                                    descriptorWriteCount,
    const VkWriteDescriptorSet*                 pDescriptorWrites,
    uint32_t                                    descriptorCopyCount,
    const VkCopyDescriptorSet*                  pDescriptorCopies)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
        for (uint32_t index=0;index<descriptorWriteCount;index++) {
                startWriteObject(my_data, pDescriptorWrites[index].dstSet);
        }
        for (uint32_t index=0;index<descriptorCopyCount;index++) {
                startWriteObject(my_data, pDescriptorCopies[index].dstSet);
        }
        // Host access to pDescriptorWrites[].dstSet must be externally synchronized
        // Host access to pDescriptorCopies[].dstSet must be externally synchronized
    }
    pTable->UpdateDescriptorSets(device,descriptorWriteCount,pDescriptorWrites,descriptorCopyCount,pDescriptorCopies);
    if (threadChecks) {
        finishReadObject(my_data, device);
        for (uint32_t index=0;index<descriptorWriteCount;index++) {
                finishWriteObject(my_data, pDescriptorWrites[index].dstSet);
        }
        for (uint32_t index=0;index<descriptorCopyCount;index++) {
                finishWriteObject(my_data, pDescriptorCopies[index].dstSet);
        }
        // Host access to pDescriptorWrites[].dstSet must be externally synchronized
        // Host access to pDescriptorCopies[].dstSet must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR VkResult VKAPI_CALL CreateFramebuffer(
    VkDevice                                    device,
    const VkFramebufferCreateInfo*              pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkFramebuffer*                              pFramebuffer)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
    }
    result = pTable->CreateFramebuffer(device,pCreateInfo,pAllocator,pFramebuffer);
    if (threadChecks) {
        finishReadObject(my_data, device);
    } else {
        finishMultiThread();
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyFramebuffer(
    VkDevice                                    device,
    VkFramebuffer                               framebuffer,
    const VkAllocationCallbacks*                pAllocator)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
        startWriteObject(my_data, framebuffer);
        // Host access to framebuffer must be externally synchronized
    }
    pTable->DestroyFramebuffer(device,framebuffer,pAllocator);
    if (threadChecks) {
        finishReadObject(my_data, device);
        finishWriteObject(my_data, framebuffer);
        // Host access to framebuffer must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR VkResult VKAPI_CALL CreateRenderPass(
    VkDevice                                    device,
    const VkRenderPassCreateInfo*               pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkRenderPass*                               pRenderPass)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
    }
    result = pTable->CreateRenderPass(device,pCreateInfo,pAllocator,pRenderPass);
    if (threadChecks) {
        finishReadObject(my_data, device);
    } else {
        finishMultiThread();
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyRenderPass(
    VkDevice                                    device,
    VkRenderPass                                renderPass,
    const VkAllocationCallbacks*                pAllocator)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
        startWriteObject(my_data, renderPass);
        // Host access to renderPass must be externally synchronized
    }
    pTable->DestroyRenderPass(device,renderPass,pAllocator);
    if (threadChecks) {
        finishReadObject(my_data, device);
        finishWriteObject(my_data, renderPass);
        // Host access to renderPass must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR void VKAPI_CALL GetRenderAreaGranularity(
    VkDevice                                    device,
    VkRenderPass                                renderPass,
    VkExtent2D*                                 pGranularity)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
        startReadObject(my_data, renderPass);
    }
    pTable->GetRenderAreaGranularity(device,renderPass,pGranularity);
    if (threadChecks) {
        finishReadObject(my_data, device);
        finishReadObject(my_data, renderPass);
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR VkResult VKAPI_CALL CreateCommandPool(
    VkDevice                                    device,
    const VkCommandPoolCreateInfo*              pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkCommandPool*                              pCommandPool)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
    }
    result = pTable->CreateCommandPool(device,pCreateInfo,pAllocator,pCommandPool);
    if (threadChecks) {
        finishReadObject(my_data, device);
    } else {
        finishMultiThread();
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyCommandPool(
    VkDevice                                    device,
    VkCommandPool                               commandPool,
    const VkAllocationCallbacks*                pAllocator)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
        startWriteObject(my_data, commandPool);
        // Host access to commandPool must be externally synchronized
    }
    pTable->DestroyCommandPool(device,commandPool,pAllocator);
    if (threadChecks) {
        finishReadObject(my_data, device);
        finishWriteObject(my_data, commandPool);
        // Host access to commandPool must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR VkResult VKAPI_CALL ResetCommandPool(
    VkDevice                                    device,
    VkCommandPool                               commandPool,
    VkCommandPoolResetFlags                     flags)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
        startWriteObject(my_data, commandPool);
        // Host access to commandPool must be externally synchronized
    }
    result = pTable->ResetCommandPool(device,commandPool,flags);
    if (threadChecks) {
        finishReadObject(my_data, device);
        finishWriteObject(my_data, commandPool);
        // Host access to commandPool must be externally synchronized
    } else {
        finishMultiThread();
    }
    return result;
}

// declare only
VKAPI_ATTR VkResult VKAPI_CALL AllocateCommandBuffers(
    VkDevice                                    device,
    const VkCommandBufferAllocateInfo*          pAllocateInfo,
    VkCommandBuffer*                            pCommandBuffers);

// declare only
VKAPI_ATTR void VKAPI_CALL FreeCommandBuffers(
    VkDevice                                    device,
    VkCommandPool                               commandPool,
    uint32_t                                    commandBufferCount,
    const VkCommandBuffer*                      pCommandBuffers);

VKAPI_ATTR VkResult VKAPI_CALL BeginCommandBuffer(
    VkCommandBuffer                             commandBuffer,
    const VkCommandBufferBeginInfo*             pBeginInfo)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, commandBuffer);
        // Host access to commandBuffer must be externally synchronized
    }
    result = pTable->BeginCommandBuffer(commandBuffer,pBeginInfo);
    if (threadChecks) {
        finishWriteObject(my_data, commandBuffer);
        // Host access to commandBuffer must be externally synchronized
    } else {
        finishMultiThread();
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL EndCommandBuffer(
    VkCommandBuffer                             commandBuffer)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, commandBuffer);
        // Host access to commandBuffer must be externally synchronized
    }
    result = pTable->EndCommandBuffer(commandBuffer);
    if (threadChecks) {
        finishWriteObject(my_data, commandBuffer);
        // Host access to commandBuffer must be externally synchronized
    } else {
        finishMultiThread();
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL ResetCommandBuffer(
    VkCommandBuffer                             commandBuffer,
    VkCommandBufferResetFlags                   flags)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, commandBuffer);
        // Host access to commandBuffer must be externally synchronized
    }
    result = pTable->ResetCommandBuffer(commandBuffer,flags);
    if (threadChecks) {
        finishWriteObject(my_data, commandBuffer);
        // Host access to commandBuffer must be externally synchronized
    } else {
        finishMultiThread();
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL CmdBindPipeline(
    VkCommandBuffer                             commandBuffer,
    VkPipelineBindPoint                         pipelineBindPoint,
    VkPipeline                                  pipeline)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, commandBuffer);
        startReadObject(my_data, pipeline);
        // Host access to commandBuffer must be externally synchronized
    }
    pTable->CmdBindPipeline(commandBuffer,pipelineBindPoint,pipeline);
    if (threadChecks) {
        finishWriteObject(my_data, commandBuffer);
        finishReadObject(my_data, pipeline);
        // Host access to commandBuffer must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR void VKAPI_CALL CmdSetViewport(
    VkCommandBuffer                             commandBuffer,
    uint32_t                                    firstViewport,
    uint32_t                                    viewportCount,
    const VkViewport*                           pViewports)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, commandBuffer);
        // Host access to commandBuffer must be externally synchronized
    }
    pTable->CmdSetViewport(commandBuffer,firstViewport,viewportCount,pViewports);
    if (threadChecks) {
        finishWriteObject(my_data, commandBuffer);
        // Host access to commandBuffer must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR void VKAPI_CALL CmdSetScissor(
    VkCommandBuffer                             commandBuffer,
    uint32_t                                    firstScissor,
    uint32_t                                    scissorCount,
    const VkRect2D*                             pScissors)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, commandBuffer);
        // Host access to commandBuffer must be externally synchronized
    }
    pTable->CmdSetScissor(commandBuffer,firstScissor,scissorCount,pScissors);
    if (threadChecks) {
        finishWriteObject(my_data, commandBuffer);
        // Host access to commandBuffer must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR void VKAPI_CALL CmdSetLineWidth(
    VkCommandBuffer                             commandBuffer,
    float                                       lineWidth)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, commandBuffer);
        // Host access to commandBuffer must be externally synchronized
    }
    pTable->CmdSetLineWidth(commandBuffer,lineWidth);
    if (threadChecks) {
        finishWriteObject(my_data, commandBuffer);
        // Host access to commandBuffer must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR void VKAPI_CALL CmdSetDepthBias(
    VkCommandBuffer                             commandBuffer,
    float                                       depthBiasConstantFactor,
    float                                       depthBiasClamp,
    float                                       depthBiasSlopeFactor)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, commandBuffer);
        // Host access to commandBuffer must be externally synchronized
    }
    pTable->CmdSetDepthBias(commandBuffer,depthBiasConstantFactor,depthBiasClamp,depthBiasSlopeFactor);
    if (threadChecks) {
        finishWriteObject(my_data, commandBuffer);
        // Host access to commandBuffer must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR void VKAPI_CALL CmdSetBlendConstants(
    VkCommandBuffer                             commandBuffer,
    const float                                 blendConstants[4])
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, commandBuffer);
        // Host access to commandBuffer must be externally synchronized
    }
    pTable->CmdSetBlendConstants(commandBuffer,blendConstants);
    if (threadChecks) {
        finishWriteObject(my_data, commandBuffer);
        // Host access to commandBuffer must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR void VKAPI_CALL CmdSetDepthBounds(
    VkCommandBuffer                             commandBuffer,
    float                                       minDepthBounds,
    float                                       maxDepthBounds)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, commandBuffer);
        // Host access to commandBuffer must be externally synchronized
    }
    pTable->CmdSetDepthBounds(commandBuffer,minDepthBounds,maxDepthBounds);
    if (threadChecks) {
        finishWriteObject(my_data, commandBuffer);
        // Host access to commandBuffer must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR void VKAPI_CALL CmdSetStencilCompareMask(
    VkCommandBuffer                             commandBuffer,
    VkStencilFaceFlags                          faceMask,
    uint32_t                                    compareMask)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, commandBuffer);
        // Host access to commandBuffer must be externally synchronized
    }
    pTable->CmdSetStencilCompareMask(commandBuffer,faceMask,compareMask);
    if (threadChecks) {
        finishWriteObject(my_data, commandBuffer);
        // Host access to commandBuffer must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR void VKAPI_CALL CmdSetStencilWriteMask(
    VkCommandBuffer                             commandBuffer,
    VkStencilFaceFlags                          faceMask,
    uint32_t                                    writeMask)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, commandBuffer);
        // Host access to commandBuffer must be externally synchronized
    }
    pTable->CmdSetStencilWriteMask(commandBuffer,faceMask,writeMask);
    if (threadChecks) {
        finishWriteObject(my_data, commandBuffer);
        // Host access to commandBuffer must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR void VKAPI_CALL CmdSetStencilReference(
    VkCommandBuffer                             commandBuffer,
    VkStencilFaceFlags                          faceMask,
    uint32_t                                    reference)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, commandBuffer);
        // Host access to commandBuffer must be externally synchronized
    }
    pTable->CmdSetStencilReference(commandBuffer,faceMask,reference);
    if (threadChecks) {
        finishWriteObject(my_data, commandBuffer);
        // Host access to commandBuffer must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR void VKAPI_CALL CmdBindDescriptorSets(
    VkCommandBuffer                             commandBuffer,
    VkPipelineBindPoint                         pipelineBindPoint,
    VkPipelineLayout                            layout,
    uint32_t                                    firstSet,
    uint32_t                                    descriptorSetCount,
    const VkDescriptorSet*                      pDescriptorSets,
    uint32_t                                    dynamicOffsetCount,
    const uint32_t*                             pDynamicOffsets)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, commandBuffer);
        startReadObject(my_data, layout);
        // Host access to commandBuffer must be externally synchronized
    }
    pTable->CmdBindDescriptorSets(commandBuffer,pipelineBindPoint,layout,firstSet,descriptorSetCount,pDescriptorSets,dynamicOffsetCount,pDynamicOffsets);
    if (threadChecks) {
        finishWriteObject(my_data, commandBuffer);
        finishReadObject(my_data, layout);
        // Host access to commandBuffer must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR void VKAPI_CALL CmdBindIndexBuffer(
    VkCommandBuffer                             commandBuffer,
    VkBuffer                                    buffer,
    VkDeviceSize                                offset,
    VkIndexType                                 indexType)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, commandBuffer);
        startReadObject(my_data, buffer);
        // Host access to commandBuffer must be externally synchronized
    }
    pTable->CmdBindIndexBuffer(commandBuffer,buffer,offset,indexType);
    if (threadChecks) {
        finishWriteObject(my_data, commandBuffer);
        finishReadObject(my_data, buffer);
        // Host access to commandBuffer must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR void VKAPI_CALL CmdBindVertexBuffers(
    VkCommandBuffer                             commandBuffer,
    uint32_t                                    firstBinding,
    uint32_t                                    bindingCount,
    const VkBuffer*                             pBuffers,
    const VkDeviceSize*                         pOffsets)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, commandBuffer);
        for (uint32_t index=0;index<bindingCount;index++) {
            startReadObject(my_data, pBuffers[index]);
        }
        // Host access to commandBuffer must be externally synchronized
    }
    pTable->CmdBindVertexBuffers(commandBuffer,firstBinding,bindingCount,pBuffers,pOffsets);
    if (threadChecks) {
        finishWriteObject(my_data, commandBuffer);
        for (uint32_t index=0;index<bindingCount;index++) {
            finishReadObject(my_data, pBuffers[index]);
        }
        // Host access to commandBuffer must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR void VKAPI_CALL CmdDraw(
    VkCommandBuffer                             commandBuffer,
    uint32_t                                    vertexCount,
    uint32_t                                    instanceCount,
    uint32_t                                    firstVertex,
    uint32_t                                    firstInstance)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, commandBuffer);
        // Host access to commandBuffer must be externally synchronized
    }
    pTable->CmdDraw(commandBuffer,vertexCount,instanceCount,firstVertex,firstInstance);
    if (threadChecks) {
        finishWriteObject(my_data, commandBuffer);
        // Host access to commandBuffer must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR void VKAPI_CALL CmdDrawIndexed(
    VkCommandBuffer                             commandBuffer,
    uint32_t                                    indexCount,
    uint32_t                                    instanceCount,
    uint32_t                                    firstIndex,
    int32_t                                     vertexOffset,
    uint32_t                                    firstInstance)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, commandBuffer);
        // Host access to commandBuffer must be externally synchronized
    }
    pTable->CmdDrawIndexed(commandBuffer,indexCount,instanceCount,firstIndex,vertexOffset,firstInstance);
    if (threadChecks) {
        finishWriteObject(my_data, commandBuffer);
        // Host access to commandBuffer must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR void VKAPI_CALL CmdDrawIndirect(
    VkCommandBuffer                             commandBuffer,
    VkBuffer                                    buffer,
    VkDeviceSize                                offset,
    uint32_t                                    drawCount,
    uint32_t                                    stride)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, commandBuffer);
        startReadObject(my_data, buffer);
        // Host access to commandBuffer must be externally synchronized
    }
    pTable->CmdDrawIndirect(commandBuffer,buffer,offset,drawCount,stride);
    if (threadChecks) {
        finishWriteObject(my_data, commandBuffer);
        finishReadObject(my_data, buffer);
        // Host access to commandBuffer must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR void VKAPI_CALL CmdDrawIndexedIndirect(
    VkCommandBuffer                             commandBuffer,
    VkBuffer                                    buffer,
    VkDeviceSize                                offset,
    uint32_t                                    drawCount,
    uint32_t                                    stride)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, commandBuffer);
        startReadObject(my_data, buffer);
        // Host access to commandBuffer must be externally synchronized
    }
    pTable->CmdDrawIndexedIndirect(commandBuffer,buffer,offset,drawCount,stride);
    if (threadChecks) {
        finishWriteObject(my_data, commandBuffer);
        finishReadObject(my_data, buffer);
        // Host access to commandBuffer must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR void VKAPI_CALL CmdDispatch(
    VkCommandBuffer                             commandBuffer,
    uint32_t                                    x,
    uint32_t                                    y,
    uint32_t                                    z)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, commandBuffer);
        // Host access to commandBuffer must be externally synchronized
    }
    pTable->CmdDispatch(commandBuffer,x,y,z);
    if (threadChecks) {
        finishWriteObject(my_data, commandBuffer);
        // Host access to commandBuffer must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR void VKAPI_CALL CmdDispatchIndirect(
    VkCommandBuffer                             commandBuffer,
    VkBuffer                                    buffer,
    VkDeviceSize                                offset)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, commandBuffer);
        startReadObject(my_data, buffer);
        // Host access to commandBuffer must be externally synchronized
    }
    pTable->CmdDispatchIndirect(commandBuffer,buffer,offset);
    if (threadChecks) {
        finishWriteObject(my_data, commandBuffer);
        finishReadObject(my_data, buffer);
        // Host access to commandBuffer must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR void VKAPI_CALL CmdCopyBuffer(
    VkCommandBuffer                             commandBuffer,
    VkBuffer                                    srcBuffer,
    VkBuffer                                    dstBuffer,
    uint32_t                                    regionCount,
    const VkBufferCopy*                         pRegions)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, commandBuffer);
        startReadObject(my_data, srcBuffer);
        startReadObject(my_data, dstBuffer);
        // Host access to commandBuffer must be externally synchronized
    }
    pTable->CmdCopyBuffer(commandBuffer,srcBuffer,dstBuffer,regionCount,pRegions);
    if (threadChecks) {
        finishWriteObject(my_data, commandBuffer);
        finishReadObject(my_data, srcBuffer);
        finishReadObject(my_data, dstBuffer);
        // Host access to commandBuffer must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR void VKAPI_CALL CmdCopyImage(
    VkCommandBuffer                             commandBuffer,
    VkImage                                     srcImage,
    VkImageLayout                               srcImageLayout,
    VkImage                                     dstImage,
    VkImageLayout                               dstImageLayout,
    uint32_t                                    regionCount,
    const VkImageCopy*                          pRegions)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, commandBuffer);
        startReadObject(my_data, srcImage);
        startReadObject(my_data, dstImage);
        // Host access to commandBuffer must be externally synchronized
    }
    pTable->CmdCopyImage(commandBuffer,srcImage,srcImageLayout,dstImage,dstImageLayout,regionCount,pRegions);
    if (threadChecks) {
        finishWriteObject(my_data, commandBuffer);
        finishReadObject(my_data, srcImage);
        finishReadObject(my_data, dstImage);
        // Host access to commandBuffer must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR void VKAPI_CALL CmdBlitImage(
    VkCommandBuffer                             commandBuffer,
    VkImage                                     srcImage,
    VkImageLayout                               srcImageLayout,
    VkImage                                     dstImage,
    VkImageLayout                               dstImageLayout,
    uint32_t                                    regionCount,
    const VkImageBlit*                          pRegions,
    VkFilter                                    filter)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, commandBuffer);
        startReadObject(my_data, srcImage);
        startReadObject(my_data, dstImage);
        // Host access to commandBuffer must be externally synchronized
    }
    pTable->CmdBlitImage(commandBuffer,srcImage,srcImageLayout,dstImage,dstImageLayout,regionCount,pRegions,filter);
    if (threadChecks) {
        finishWriteObject(my_data, commandBuffer);
        finishReadObject(my_data, srcImage);
        finishReadObject(my_data, dstImage);
        // Host access to commandBuffer must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR void VKAPI_CALL CmdCopyBufferToImage(
    VkCommandBuffer                             commandBuffer,
    VkBuffer                                    srcBuffer,
    VkImage                                     dstImage,
    VkImageLayout                               dstImageLayout,
    uint32_t                                    regionCount,
    const VkBufferImageCopy*                    pRegions)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, commandBuffer);
        startReadObject(my_data, srcBuffer);
        startReadObject(my_data, dstImage);
        // Host access to commandBuffer must be externally synchronized
    }
    pTable->CmdCopyBufferToImage(commandBuffer,srcBuffer,dstImage,dstImageLayout,regionCount,pRegions);
    if (threadChecks) {
        finishWriteObject(my_data, commandBuffer);
        finishReadObject(my_data, srcBuffer);
        finishReadObject(my_data, dstImage);
        // Host access to commandBuffer must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR void VKAPI_CALL CmdCopyImageToBuffer(
    VkCommandBuffer                             commandBuffer,
    VkImage                                     srcImage,
    VkImageLayout                               srcImageLayout,
    VkBuffer                                    dstBuffer,
    uint32_t                                    regionCount,
    const VkBufferImageCopy*                    pRegions)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, commandBuffer);
        startReadObject(my_data, srcImage);
        startReadObject(my_data, dstBuffer);
        // Host access to commandBuffer must be externally synchronized
    }
    pTable->CmdCopyImageToBuffer(commandBuffer,srcImage,srcImageLayout,dstBuffer,regionCount,pRegions);
    if (threadChecks) {
        finishWriteObject(my_data, commandBuffer);
        finishReadObject(my_data, srcImage);
        finishReadObject(my_data, dstBuffer);
        // Host access to commandBuffer must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR void VKAPI_CALL CmdUpdateBuffer(
    VkCommandBuffer                             commandBuffer,
    VkBuffer                                    dstBuffer,
    VkDeviceSize                                dstOffset,
    VkDeviceSize                                dataSize,
    const void*                                 pData)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, commandBuffer);
        startReadObject(my_data, dstBuffer);
        // Host access to commandBuffer must be externally synchronized
    }
    pTable->CmdUpdateBuffer(commandBuffer,dstBuffer,dstOffset,dataSize,pData);
    if (threadChecks) {
        finishWriteObject(my_data, commandBuffer);
        finishReadObject(my_data, dstBuffer);
        // Host access to commandBuffer must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR void VKAPI_CALL CmdFillBuffer(
    VkCommandBuffer                             commandBuffer,
    VkBuffer                                    dstBuffer,
    VkDeviceSize                                dstOffset,
    VkDeviceSize                                size,
    uint32_t                                    data)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, commandBuffer);
        startReadObject(my_data, dstBuffer);
        // Host access to commandBuffer must be externally synchronized
    }
    pTable->CmdFillBuffer(commandBuffer,dstBuffer,dstOffset,size,data);
    if (threadChecks) {
        finishWriteObject(my_data, commandBuffer);
        finishReadObject(my_data, dstBuffer);
        // Host access to commandBuffer must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR void VKAPI_CALL CmdClearColorImage(
    VkCommandBuffer                             commandBuffer,
    VkImage                                     image,
    VkImageLayout                               imageLayout,
    const VkClearColorValue*                    pColor,
    uint32_t                                    rangeCount,
    const VkImageSubresourceRange*              pRanges)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, commandBuffer);
        startReadObject(my_data, image);
        // Host access to commandBuffer must be externally synchronized
    }
    pTable->CmdClearColorImage(commandBuffer,image,imageLayout,pColor,rangeCount,pRanges);
    if (threadChecks) {
        finishWriteObject(my_data, commandBuffer);
        finishReadObject(my_data, image);
        // Host access to commandBuffer must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR void VKAPI_CALL CmdClearDepthStencilImage(
    VkCommandBuffer                             commandBuffer,
    VkImage                                     image,
    VkImageLayout                               imageLayout,
    const VkClearDepthStencilValue*             pDepthStencil,
    uint32_t                                    rangeCount,
    const VkImageSubresourceRange*              pRanges)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, commandBuffer);
        startReadObject(my_data, image);
        // Host access to commandBuffer must be externally synchronized
    }
    pTable->CmdClearDepthStencilImage(commandBuffer,image,imageLayout,pDepthStencil,rangeCount,pRanges);
    if (threadChecks) {
        finishWriteObject(my_data, commandBuffer);
        finishReadObject(my_data, image);
        // Host access to commandBuffer must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR void VKAPI_CALL CmdClearAttachments(
    VkCommandBuffer                             commandBuffer,
    uint32_t                                    attachmentCount,
    const VkClearAttachment*                    pAttachments,
    uint32_t                                    rectCount,
    const VkClearRect*                          pRects)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, commandBuffer);
        // Host access to commandBuffer must be externally synchronized
    }
    pTable->CmdClearAttachments(commandBuffer,attachmentCount,pAttachments,rectCount,pRects);
    if (threadChecks) {
        finishWriteObject(my_data, commandBuffer);
        // Host access to commandBuffer must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR void VKAPI_CALL CmdResolveImage(
    VkCommandBuffer                             commandBuffer,
    VkImage                                     srcImage,
    VkImageLayout                               srcImageLayout,
    VkImage                                     dstImage,
    VkImageLayout                               dstImageLayout,
    uint32_t                                    regionCount,
    const VkImageResolve*                       pRegions)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, commandBuffer);
        startReadObject(my_data, srcImage);
        startReadObject(my_data, dstImage);
        // Host access to commandBuffer must be externally synchronized
    }
    pTable->CmdResolveImage(commandBuffer,srcImage,srcImageLayout,dstImage,dstImageLayout,regionCount,pRegions);
    if (threadChecks) {
        finishWriteObject(my_data, commandBuffer);
        finishReadObject(my_data, srcImage);
        finishReadObject(my_data, dstImage);
        // Host access to commandBuffer must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR void VKAPI_CALL CmdSetEvent(
    VkCommandBuffer                             commandBuffer,
    VkEvent                                     event,
    VkPipelineStageFlags                        stageMask)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, commandBuffer);
        startReadObject(my_data, event);
        // Host access to commandBuffer must be externally synchronized
    }
    pTable->CmdSetEvent(commandBuffer,event,stageMask);
    if (threadChecks) {
        finishWriteObject(my_data, commandBuffer);
        finishReadObject(my_data, event);
        // Host access to commandBuffer must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR void VKAPI_CALL CmdResetEvent(
    VkCommandBuffer                             commandBuffer,
    VkEvent                                     event,
    VkPipelineStageFlags                        stageMask)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, commandBuffer);
        startReadObject(my_data, event);
        // Host access to commandBuffer must be externally synchronized
    }
    pTable->CmdResetEvent(commandBuffer,event,stageMask);
    if (threadChecks) {
        finishWriteObject(my_data, commandBuffer);
        finishReadObject(my_data, event);
        // Host access to commandBuffer must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR void VKAPI_CALL CmdWaitEvents(
    VkCommandBuffer                             commandBuffer,
    uint32_t                                    eventCount,
    const VkEvent*                              pEvents,
    VkPipelineStageFlags                        srcStageMask,
    VkPipelineStageFlags                        dstStageMask,
    uint32_t                                    memoryBarrierCount,
    const VkMemoryBarrier*                      pMemoryBarriers,
    uint32_t                                    bufferMemoryBarrierCount,
    const VkBufferMemoryBarrier*                pBufferMemoryBarriers,
    uint32_t                                    imageMemoryBarrierCount,
    const VkImageMemoryBarrier*                 pImageMemoryBarriers)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, commandBuffer);
        for (uint32_t index=0;index<eventCount;index++) {
            startReadObject(my_data, pEvents[index]);
        }
        // Host access to commandBuffer must be externally synchronized
    }
    pTable->CmdWaitEvents(commandBuffer,eventCount,pEvents,srcStageMask,dstStageMask,memoryBarrierCount,pMemoryBarriers,bufferMemoryBarrierCount,pBufferMemoryBarriers,imageMemoryBarrierCount,pImageMemoryBarriers);
    if (threadChecks) {
        finishWriteObject(my_data, commandBuffer);
        for (uint32_t index=0;index<eventCount;index++) {
            finishReadObject(my_data, pEvents[index]);
        }
        // Host access to commandBuffer must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR void VKAPI_CALL CmdPipelineBarrier(
    VkCommandBuffer                             commandBuffer,
    VkPipelineStageFlags                        srcStageMask,
    VkPipelineStageFlags                        dstStageMask,
    VkDependencyFlags                           dependencyFlags,
    uint32_t                                    memoryBarrierCount,
    const VkMemoryBarrier*                      pMemoryBarriers,
    uint32_t                                    bufferMemoryBarrierCount,
    const VkBufferMemoryBarrier*                pBufferMemoryBarriers,
    uint32_t                                    imageMemoryBarrierCount,
    const VkImageMemoryBarrier*                 pImageMemoryBarriers)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, commandBuffer);
        // Host access to commandBuffer must be externally synchronized
    }
    pTable->CmdPipelineBarrier(commandBuffer,srcStageMask,dstStageMask,dependencyFlags,memoryBarrierCount,pMemoryBarriers,bufferMemoryBarrierCount,pBufferMemoryBarriers,imageMemoryBarrierCount,pImageMemoryBarriers);
    if (threadChecks) {
        finishWriteObject(my_data, commandBuffer);
        // Host access to commandBuffer must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR void VKAPI_CALL CmdBeginQuery(
    VkCommandBuffer                             commandBuffer,
    VkQueryPool                                 queryPool,
    uint32_t                                    query,
    VkQueryControlFlags                         flags)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, commandBuffer);
        startReadObject(my_data, queryPool);
        // Host access to commandBuffer must be externally synchronized
    }
    pTable->CmdBeginQuery(commandBuffer,queryPool,query,flags);
    if (threadChecks) {
        finishWriteObject(my_data, commandBuffer);
        finishReadObject(my_data, queryPool);
        // Host access to commandBuffer must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR void VKAPI_CALL CmdEndQuery(
    VkCommandBuffer                             commandBuffer,
    VkQueryPool                                 queryPool,
    uint32_t                                    query)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, commandBuffer);
        startReadObject(my_data, queryPool);
        // Host access to commandBuffer must be externally synchronized
    }
    pTable->CmdEndQuery(commandBuffer,queryPool,query);
    if (threadChecks) {
        finishWriteObject(my_data, commandBuffer);
        finishReadObject(my_data, queryPool);
        // Host access to commandBuffer must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR void VKAPI_CALL CmdResetQueryPool(
    VkCommandBuffer                             commandBuffer,
    VkQueryPool                                 queryPool,
    uint32_t                                    firstQuery,
    uint32_t                                    queryCount)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, commandBuffer);
        startReadObject(my_data, queryPool);
        // Host access to commandBuffer must be externally synchronized
    }
    pTable->CmdResetQueryPool(commandBuffer,queryPool,firstQuery,queryCount);
    if (threadChecks) {
        finishWriteObject(my_data, commandBuffer);
        finishReadObject(my_data, queryPool);
        // Host access to commandBuffer must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR void VKAPI_CALL CmdWriteTimestamp(
    VkCommandBuffer                             commandBuffer,
    VkPipelineStageFlagBits                     pipelineStage,
    VkQueryPool                                 queryPool,
    uint32_t                                    query)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, commandBuffer);
        startReadObject(my_data, queryPool);
        // Host access to commandBuffer must be externally synchronized
    }
    pTable->CmdWriteTimestamp(commandBuffer,pipelineStage,queryPool,query);
    if (threadChecks) {
        finishWriteObject(my_data, commandBuffer);
        finishReadObject(my_data, queryPool);
        // Host access to commandBuffer must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR void VKAPI_CALL CmdCopyQueryPoolResults(
    VkCommandBuffer                             commandBuffer,
    VkQueryPool                                 queryPool,
    uint32_t                                    firstQuery,
    uint32_t                                    queryCount,
    VkBuffer                                    dstBuffer,
    VkDeviceSize                                dstOffset,
    VkDeviceSize                                stride,
    VkQueryResultFlags                          flags)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, commandBuffer);
        startReadObject(my_data, queryPool);
        startReadObject(my_data, dstBuffer);
        // Host access to commandBuffer must be externally synchronized
    }
    pTable->CmdCopyQueryPoolResults(commandBuffer,queryPool,firstQuery,queryCount,dstBuffer,dstOffset,stride,flags);
    if (threadChecks) {
        finishWriteObject(my_data, commandBuffer);
        finishReadObject(my_data, queryPool);
        finishReadObject(my_data, dstBuffer);
        // Host access to commandBuffer must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR void VKAPI_CALL CmdPushConstants(
    VkCommandBuffer                             commandBuffer,
    VkPipelineLayout                            layout,
    VkShaderStageFlags                          stageFlags,
    uint32_t                                    offset,
    uint32_t                                    size,
    const void*                                 pValues)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, commandBuffer);
        startReadObject(my_data, layout);
        // Host access to commandBuffer must be externally synchronized
    }
    pTable->CmdPushConstants(commandBuffer,layout,stageFlags,offset,size,pValues);
    if (threadChecks) {
        finishWriteObject(my_data, commandBuffer);
        finishReadObject(my_data, layout);
        // Host access to commandBuffer must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR void VKAPI_CALL CmdBeginRenderPass(
    VkCommandBuffer                             commandBuffer,
    const VkRenderPassBeginInfo*                pRenderPassBegin,
    VkSubpassContents                           contents)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, commandBuffer);
        // Host access to commandBuffer must be externally synchronized
    }
    pTable->CmdBeginRenderPass(commandBuffer,pRenderPassBegin,contents);
    if (threadChecks) {
        finishWriteObject(my_data, commandBuffer);
        // Host access to commandBuffer must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR void VKAPI_CALL CmdNextSubpass(
    VkCommandBuffer                             commandBuffer,
    VkSubpassContents                           contents)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, commandBuffer);
        // Host access to commandBuffer must be externally synchronized
    }
    pTable->CmdNextSubpass(commandBuffer,contents);
    if (threadChecks) {
        finishWriteObject(my_data, commandBuffer);
        // Host access to commandBuffer must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR void VKAPI_CALL CmdEndRenderPass(
    VkCommandBuffer                             commandBuffer)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, commandBuffer);
        // Host access to commandBuffer must be externally synchronized
    }
    pTable->CmdEndRenderPass(commandBuffer);
    if (threadChecks) {
        finishWriteObject(my_data, commandBuffer);
        // Host access to commandBuffer must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR void VKAPI_CALL CmdExecuteCommands(
    VkCommandBuffer                             commandBuffer,
    uint32_t                                    commandBufferCount,
    const VkCommandBuffer*                      pCommandBuffers)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, commandBuffer);
        for (uint32_t index=0;index<commandBufferCount;index++) {
            startReadObject(my_data, pCommandBuffers[index]);
        }
        // Host access to commandBuffer must be externally synchronized
    }
    pTable->CmdExecuteCommands(commandBuffer,commandBufferCount,pCommandBuffers);
    if (threadChecks) {
        finishWriteObject(my_data, commandBuffer);
        for (uint32_t index=0;index<commandBufferCount;index++) {
            finishReadObject(my_data, pCommandBuffers[index]);
        }
        // Host access to commandBuffer must be externally synchronized
    } else {
        finishMultiThread();
    }
}

// TODO - not wrapping KHR function vkDestroySurfaceKHR
// TODO - not wrapping KHR function vkGetPhysicalDeviceSurfaceSupportKHR
// TODO - not wrapping KHR function vkGetPhysicalDeviceSurfaceCapabilitiesKHR
// TODO - not wrapping KHR function vkGetPhysicalDeviceSurfaceFormatsKHR
// TODO - not wrapping KHR function vkGetPhysicalDeviceSurfacePresentModesKHR

// TODO - not wrapping KHR function vkCreateSwapchainKHR
// TODO - not wrapping KHR function vkDestroySwapchainKHR
// TODO - not wrapping KHR function vkGetSwapchainImagesKHR
// TODO - not wrapping KHR function vkAcquireNextImageKHR
// TODO - not wrapping KHR function vkQueuePresentKHR

// TODO - not wrapping KHR function vkGetPhysicalDeviceDisplayPropertiesKHR
// TODO - not wrapping KHR function vkGetPhysicalDeviceDisplayPlanePropertiesKHR
// TODO - not wrapping KHR function vkGetDisplayPlaneSupportedDisplaysKHR
// TODO - not wrapping KHR function vkGetDisplayModePropertiesKHR
// TODO - not wrapping KHR function vkCreateDisplayModeKHR
// TODO - not wrapping KHR function vkGetDisplayPlaneCapabilitiesKHR
// TODO - not wrapping KHR function vkCreateDisplayPlaneSurfaceKHR

// TODO - not wrapping KHR function vkCreateSharedSwapchainsKHR

#ifdef VK_USE_PLATFORM_XLIB_KHR
// TODO - not wrapping KHR function vkCreateXlibSurfaceKHR
// TODO - not wrapping KHR function vkGetPhysicalDeviceXlibPresentationSupportKHR
#endif /* VK_USE_PLATFORM_XLIB_KHR */

#ifdef VK_USE_PLATFORM_XCB_KHR
// TODO - not wrapping KHR function vkCreateXcbSurfaceKHR
// TODO - not wrapping KHR function vkGetPhysicalDeviceXcbPresentationSupportKHR
#endif /* VK_USE_PLATFORM_XCB_KHR */

#ifdef VK_USE_PLATFORM_WAYLAND_KHR
// TODO - not wrapping KHR function vkCreateWaylandSurfaceKHR
// TODO - not wrapping KHR function vkGetPhysicalDeviceWaylandPresentationSupportKHR
#endif /* VK_USE_PLATFORM_WAYLAND_KHR */

#ifdef VK_USE_PLATFORM_MIR_KHR
// TODO - not wrapping KHR function vkCreateMirSurfaceKHR
// TODO - not wrapping KHR function vkGetPhysicalDeviceMirPresentationSupportKHR
#endif /* VK_USE_PLATFORM_MIR_KHR */

#ifdef VK_USE_PLATFORM_ANDROID_KHR
// TODO - not wrapping KHR function vkCreateAndroidSurfaceKHR
#endif /* VK_USE_PLATFORM_ANDROID_KHR */

#ifdef VK_USE_PLATFORM_WIN32_KHR
// TODO - not wrapping KHR function vkCreateWin32SurfaceKHR
// TODO - not wrapping KHR function vkGetPhysicalDeviceWin32PresentationSupportKHR
#endif /* VK_USE_PLATFORM_WIN32_KHR */



// declare only
VKAPI_ATTR VkResult VKAPI_CALL CreateDebugReportCallbackEXT(
    VkInstance                                  instance,
    const VkDebugReportCallbackCreateInfoEXT*   pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkDebugReportCallbackEXT*                   pCallback);

// declare only
VKAPI_ATTR void VKAPI_CALL DestroyDebugReportCallbackEXT(
    VkInstance                                  instance,
    VkDebugReportCallbackEXT                    callback,
    const VkAllocationCallbacks*                pAllocator);

VKAPI_ATTR void VKAPI_CALL DebugReportMessageEXT(
    VkInstance                                  instance,
    VkDebugReportFlagsEXT                       flags,
    VkDebugReportObjectTypeEXT                  objectType,
    uint64_t                                    object,
    size_t                                      location,
    int32_t                                     messageCode,
    const char*                                 pLayerPrefix,
    const char*                                 pMessage)
{
    dispatch_key key = get_dispatch_key(instance);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerInstanceDispatchTable *pTable = my_data->instance_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, instance);
    }
    pTable->DebugReportMessageEXT(instance,flags,objectType,object,location,messageCode,pLayerPrefix,pMessage);
    if (threadChecks) {
        finishReadObject(my_data, instance);
    } else {
        finishMultiThread();
    }
}






// TODO - not wrapping EXT function vkDebugMarkerSetObjectTagEXT
// TODO - not wrapping EXT function vkDebugMarkerSetObjectNameEXT
// TODO - not wrapping EXT function vkCmdDebugMarkerBeginEXT
// TODO - not wrapping EXT function vkCmdDebugMarkerEndEXT
// TODO - not wrapping EXT function vkCmdDebugMarkerInsertEXT




VKAPI_ATTR void VKAPI_CALL CmdDrawIndirectCountAMD(
    VkCommandBuffer                             commandBuffer,
    VkBuffer                                    buffer,
    VkDeviceSize                                offset,
    VkBuffer                                    countBuffer,
    VkDeviceSize                                countBufferOffset,
    uint32_t                                    maxDrawCount,
    uint32_t                                    stride)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, commandBuffer);
        startReadObject(my_data, buffer);
        startReadObject(my_data, countBuffer);
        // Host access to commandBuffer must be externally synchronized
    }
    pTable->CmdDrawIndirectCountAMD(commandBuffer,buffer,offset,countBuffer,countBufferOffset,maxDrawCount,stride);
    if (threadChecks) {
        finishWriteObject(my_data, commandBuffer);
        finishReadObject(my_data, buffer);
        finishReadObject(my_data, countBuffer);
        // Host access to commandBuffer must be externally synchronized
    } else {
        finishMultiThread();
    }
}

VKAPI_ATTR void VKAPI_CALL CmdDrawIndexedIndirectCountAMD(
    VkCommandBuffer                             commandBuffer,
    VkBuffer                                    buffer,
    VkDeviceSize                                offset,
    VkBuffer                                    countBuffer,
    VkDeviceSize                                countBufferOffset,
    uint32_t                                    maxDrawCount,
    uint32_t                                    stride)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startWriteObject(my_data, commandBuffer);
        startReadObject(my_data, buffer);
        startReadObject(my_data, countBuffer);
        // Host access to commandBuffer must be externally synchronized
    }
    pTable->CmdDrawIndexedIndirectCountAMD(commandBuffer,buffer,offset,countBuffer,countBufferOffset,maxDrawCount,stride);
    if (threadChecks) {
        finishWriteObject(my_data, commandBuffer);
        finishReadObject(my_data, buffer);
        finishReadObject(my_data, countBuffer);
        // Host access to commandBuffer must be externally synchronized
    } else {
        finishMultiThread();
    }
}







#ifdef VK_USE_PLATFORM_WIN32_KHR

VKAPI_ATTR VkResult VKAPI_CALL GetMemoryWin32HandleNV(
    VkDevice                                    device,
    VkDeviceMemory                              memory,
    VkExternalMemoryHandleTypeFlagsNV           handleType,
    HANDLE*                                     pHandle)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    bool threadChecks = startMultiThread();
    if (threadChecks) {
        startReadObject(my_data, device);
        startReadObject(my_data, memory);
    }
    result = pTable->GetMemoryWin32HandleNV(device,memory,handleType,pHandle);
    if (threadChecks) {
        finishReadObject(my_data, device);
        finishReadObject(my_data, memory);
    } else {
        finishMultiThread();
    }
    return result;
}
#endif /* VK_USE_PLATFORM_WIN32_KHR */

#ifdef VK_USE_PLATFORM_WIN32_KHR
#endif /* VK_USE_PLATFORM_WIN32_KHR */


// intercepts
struct { const char* name; PFN_vkVoidFunction pFunc;} procmap[] = {
    {"vkCreateInstance", reinterpret_cast<PFN_vkVoidFunction>(CreateInstance)},
    {"vkDestroyInstance", reinterpret_cast<PFN_vkVoidFunction>(DestroyInstance)},
    {"vkEnumeratePhysicalDevices", reinterpret_cast<PFN_vkVoidFunction>(EnumeratePhysicalDevices)},
    {"vkGetInstanceProcAddr", reinterpret_cast<PFN_vkVoidFunction>(GetInstanceProcAddr)},
    {"vkGetDeviceProcAddr", reinterpret_cast<PFN_vkVoidFunction>(GetDeviceProcAddr)},
    {"vkCreateDevice", reinterpret_cast<PFN_vkVoidFunction>(CreateDevice)},
    {"vkDestroyDevice", reinterpret_cast<PFN_vkVoidFunction>(DestroyDevice)},
    {"vkGetDeviceQueue", reinterpret_cast<PFN_vkVoidFunction>(GetDeviceQueue)},
    {"vkQueueSubmit", reinterpret_cast<PFN_vkVoidFunction>(QueueSubmit)},
    {"vkQueueWaitIdle", reinterpret_cast<PFN_vkVoidFunction>(QueueWaitIdle)},
    {"vkDeviceWaitIdle", reinterpret_cast<PFN_vkVoidFunction>(DeviceWaitIdle)},
    {"vkAllocateMemory", reinterpret_cast<PFN_vkVoidFunction>(AllocateMemory)},
    {"vkFreeMemory", reinterpret_cast<PFN_vkVoidFunction>(FreeMemory)},
    {"vkMapMemory", reinterpret_cast<PFN_vkVoidFunction>(MapMemory)},
    {"vkUnmapMemory", reinterpret_cast<PFN_vkVoidFunction>(UnmapMemory)},
    {"vkFlushMappedMemoryRanges", reinterpret_cast<PFN_vkVoidFunction>(FlushMappedMemoryRanges)},
    {"vkInvalidateMappedMemoryRanges", reinterpret_cast<PFN_vkVoidFunction>(InvalidateMappedMemoryRanges)},
    {"vkGetDeviceMemoryCommitment", reinterpret_cast<PFN_vkVoidFunction>(GetDeviceMemoryCommitment)},
    {"vkBindBufferMemory", reinterpret_cast<PFN_vkVoidFunction>(BindBufferMemory)},
    {"vkBindImageMemory", reinterpret_cast<PFN_vkVoidFunction>(BindImageMemory)},
    {"vkGetBufferMemoryRequirements", reinterpret_cast<PFN_vkVoidFunction>(GetBufferMemoryRequirements)},
    {"vkGetImageMemoryRequirements", reinterpret_cast<PFN_vkVoidFunction>(GetImageMemoryRequirements)},
    {"vkGetImageSparseMemoryRequirements", reinterpret_cast<PFN_vkVoidFunction>(GetImageSparseMemoryRequirements)},
    {"vkQueueBindSparse", reinterpret_cast<PFN_vkVoidFunction>(QueueBindSparse)},
    {"vkCreateFence", reinterpret_cast<PFN_vkVoidFunction>(CreateFence)},
    {"vkDestroyFence", reinterpret_cast<PFN_vkVoidFunction>(DestroyFence)},
    {"vkResetFences", reinterpret_cast<PFN_vkVoidFunction>(ResetFences)},
    {"vkGetFenceStatus", reinterpret_cast<PFN_vkVoidFunction>(GetFenceStatus)},
    {"vkWaitForFences", reinterpret_cast<PFN_vkVoidFunction>(WaitForFences)},
    {"vkCreateSemaphore", reinterpret_cast<PFN_vkVoidFunction>(CreateSemaphore)},
    {"vkDestroySemaphore", reinterpret_cast<PFN_vkVoidFunction>(DestroySemaphore)},
    {"vkCreateEvent", reinterpret_cast<PFN_vkVoidFunction>(CreateEvent)},
    {"vkDestroyEvent", reinterpret_cast<PFN_vkVoidFunction>(DestroyEvent)},
    {"vkGetEventStatus", reinterpret_cast<PFN_vkVoidFunction>(GetEventStatus)},
    {"vkSetEvent", reinterpret_cast<PFN_vkVoidFunction>(SetEvent)},
    {"vkResetEvent", reinterpret_cast<PFN_vkVoidFunction>(ResetEvent)},
    {"vkCreateQueryPool", reinterpret_cast<PFN_vkVoidFunction>(CreateQueryPool)},
    {"vkDestroyQueryPool", reinterpret_cast<PFN_vkVoidFunction>(DestroyQueryPool)},
    {"vkGetQueryPoolResults", reinterpret_cast<PFN_vkVoidFunction>(GetQueryPoolResults)},
    {"vkCreateBuffer", reinterpret_cast<PFN_vkVoidFunction>(CreateBuffer)},
    {"vkDestroyBuffer", reinterpret_cast<PFN_vkVoidFunction>(DestroyBuffer)},
    {"vkCreateBufferView", reinterpret_cast<PFN_vkVoidFunction>(CreateBufferView)},
    {"vkDestroyBufferView", reinterpret_cast<PFN_vkVoidFunction>(DestroyBufferView)},
    {"vkCreateImage", reinterpret_cast<PFN_vkVoidFunction>(CreateImage)},
    {"vkDestroyImage", reinterpret_cast<PFN_vkVoidFunction>(DestroyImage)},
    {"vkGetImageSubresourceLayout", reinterpret_cast<PFN_vkVoidFunction>(GetImageSubresourceLayout)},
    {"vkCreateImageView", reinterpret_cast<PFN_vkVoidFunction>(CreateImageView)},
    {"vkDestroyImageView", reinterpret_cast<PFN_vkVoidFunction>(DestroyImageView)},
    {"vkCreateShaderModule", reinterpret_cast<PFN_vkVoidFunction>(CreateShaderModule)},
    {"vkDestroyShaderModule", reinterpret_cast<PFN_vkVoidFunction>(DestroyShaderModule)},
    {"vkCreatePipelineCache", reinterpret_cast<PFN_vkVoidFunction>(CreatePipelineCache)},
    {"vkDestroyPipelineCache", reinterpret_cast<PFN_vkVoidFunction>(DestroyPipelineCache)},
    {"vkGetPipelineCacheData", reinterpret_cast<PFN_vkVoidFunction>(GetPipelineCacheData)},
    {"vkMergePipelineCaches", reinterpret_cast<PFN_vkVoidFunction>(MergePipelineCaches)},
    {"vkCreateGraphicsPipelines", reinterpret_cast<PFN_vkVoidFunction>(CreateGraphicsPipelines)},
    {"vkCreateComputePipelines", reinterpret_cast<PFN_vkVoidFunction>(CreateComputePipelines)},
    {"vkDestroyPipeline", reinterpret_cast<PFN_vkVoidFunction>(DestroyPipeline)},
    {"vkCreatePipelineLayout", reinterpret_cast<PFN_vkVoidFunction>(CreatePipelineLayout)},
    {"vkDestroyPipelineLayout", reinterpret_cast<PFN_vkVoidFunction>(DestroyPipelineLayout)},
    {"vkCreateSampler", reinterpret_cast<PFN_vkVoidFunction>(CreateSampler)},
    {"vkDestroySampler", reinterpret_cast<PFN_vkVoidFunction>(DestroySampler)},
    {"vkCreateDescriptorSetLayout", reinterpret_cast<PFN_vkVoidFunction>(CreateDescriptorSetLayout)},
    {"vkDestroyDescriptorSetLayout", reinterpret_cast<PFN_vkVoidFunction>(DestroyDescriptorSetLayout)},
    {"vkCreateDescriptorPool", reinterpret_cast<PFN_vkVoidFunction>(CreateDescriptorPool)},
    {"vkDestroyDescriptorPool", reinterpret_cast<PFN_vkVoidFunction>(DestroyDescriptorPool)},
    {"vkResetDescriptorPool", reinterpret_cast<PFN_vkVoidFunction>(ResetDescriptorPool)},
    {"vkAllocateDescriptorSets", reinterpret_cast<PFN_vkVoidFunction>(AllocateDescriptorSets)},
    {"vkFreeDescriptorSets", reinterpret_cast<PFN_vkVoidFunction>(FreeDescriptorSets)},
    {"vkUpdateDescriptorSets", reinterpret_cast<PFN_vkVoidFunction>(UpdateDescriptorSets)},
    {"vkCreateFramebuffer", reinterpret_cast<PFN_vkVoidFunction>(CreateFramebuffer)},
    {"vkDestroyFramebuffer", reinterpret_cast<PFN_vkVoidFunction>(DestroyFramebuffer)},
    {"vkCreateRenderPass", reinterpret_cast<PFN_vkVoidFunction>(CreateRenderPass)},
    {"vkDestroyRenderPass", reinterpret_cast<PFN_vkVoidFunction>(DestroyRenderPass)},
    {"vkGetRenderAreaGranularity", reinterpret_cast<PFN_vkVoidFunction>(GetRenderAreaGranularity)},
    {"vkCreateCommandPool", reinterpret_cast<PFN_vkVoidFunction>(CreateCommandPool)},
    {"vkDestroyCommandPool", reinterpret_cast<PFN_vkVoidFunction>(DestroyCommandPool)},
    {"vkResetCommandPool", reinterpret_cast<PFN_vkVoidFunction>(ResetCommandPool)},
    {"vkAllocateCommandBuffers", reinterpret_cast<PFN_vkVoidFunction>(AllocateCommandBuffers)},
    {"vkFreeCommandBuffers", reinterpret_cast<PFN_vkVoidFunction>(FreeCommandBuffers)},
    {"vkBeginCommandBuffer", reinterpret_cast<PFN_vkVoidFunction>(BeginCommandBuffer)},
    {"vkEndCommandBuffer", reinterpret_cast<PFN_vkVoidFunction>(EndCommandBuffer)},
    {"vkResetCommandBuffer", reinterpret_cast<PFN_vkVoidFunction>(ResetCommandBuffer)},
    {"vkCmdBindPipeline", reinterpret_cast<PFN_vkVoidFunction>(CmdBindPipeline)},
    {"vkCmdSetViewport", reinterpret_cast<PFN_vkVoidFunction>(CmdSetViewport)},
    {"vkCmdSetScissor", reinterpret_cast<PFN_vkVoidFunction>(CmdSetScissor)},
    {"vkCmdSetLineWidth", reinterpret_cast<PFN_vkVoidFunction>(CmdSetLineWidth)},
    {"vkCmdSetDepthBias", reinterpret_cast<PFN_vkVoidFunction>(CmdSetDepthBias)},
    {"vkCmdSetBlendConstants", reinterpret_cast<PFN_vkVoidFunction>(CmdSetBlendConstants)},
    {"vkCmdSetDepthBounds", reinterpret_cast<PFN_vkVoidFunction>(CmdSetDepthBounds)},
    {"vkCmdSetStencilCompareMask", reinterpret_cast<PFN_vkVoidFunction>(CmdSetStencilCompareMask)},
    {"vkCmdSetStencilWriteMask", reinterpret_cast<PFN_vkVoidFunction>(CmdSetStencilWriteMask)},
    {"vkCmdSetStencilReference", reinterpret_cast<PFN_vkVoidFunction>(CmdSetStencilReference)},
    {"vkCmdBindDescriptorSets", reinterpret_cast<PFN_vkVoidFunction>(CmdBindDescriptorSets)},
    {"vkCmdBindIndexBuffer", reinterpret_cast<PFN_vkVoidFunction>(CmdBindIndexBuffer)},
    {"vkCmdBindVertexBuffers", reinterpret_cast<PFN_vkVoidFunction>(CmdBindVertexBuffers)},
    {"vkCmdDraw", reinterpret_cast<PFN_vkVoidFunction>(CmdDraw)},
    {"vkCmdDrawIndexed", reinterpret_cast<PFN_vkVoidFunction>(CmdDrawIndexed)},
    {"vkCmdDrawIndirect", reinterpret_cast<PFN_vkVoidFunction>(CmdDrawIndirect)},
    {"vkCmdDrawIndexedIndirect", reinterpret_cast<PFN_vkVoidFunction>(CmdDrawIndexedIndirect)},
    {"vkCmdDispatch", reinterpret_cast<PFN_vkVoidFunction>(CmdDispatch)},
    {"vkCmdDispatchIndirect", reinterpret_cast<PFN_vkVoidFunction>(CmdDispatchIndirect)},
    {"vkCmdCopyBuffer", reinterpret_cast<PFN_vkVoidFunction>(CmdCopyBuffer)},
    {"vkCmdCopyImage", reinterpret_cast<PFN_vkVoidFunction>(CmdCopyImage)},
    {"vkCmdBlitImage", reinterpret_cast<PFN_vkVoidFunction>(CmdBlitImage)},
    {"vkCmdCopyBufferToImage", reinterpret_cast<PFN_vkVoidFunction>(CmdCopyBufferToImage)},
    {"vkCmdCopyImageToBuffer", reinterpret_cast<PFN_vkVoidFunction>(CmdCopyImageToBuffer)},
    {"vkCmdUpdateBuffer", reinterpret_cast<PFN_vkVoidFunction>(CmdUpdateBuffer)},
    {"vkCmdFillBuffer", reinterpret_cast<PFN_vkVoidFunction>(CmdFillBuffer)},
    {"vkCmdClearColorImage", reinterpret_cast<PFN_vkVoidFunction>(CmdClearColorImage)},
    {"vkCmdClearDepthStencilImage", reinterpret_cast<PFN_vkVoidFunction>(CmdClearDepthStencilImage)},
    {"vkCmdClearAttachments", reinterpret_cast<PFN_vkVoidFunction>(CmdClearAttachments)},
    {"vkCmdResolveImage", reinterpret_cast<PFN_vkVoidFunction>(CmdResolveImage)},
    {"vkCmdSetEvent", reinterpret_cast<PFN_vkVoidFunction>(CmdSetEvent)},
    {"vkCmdResetEvent", reinterpret_cast<PFN_vkVoidFunction>(CmdResetEvent)},
    {"vkCmdWaitEvents", reinterpret_cast<PFN_vkVoidFunction>(CmdWaitEvents)},
    {"vkCmdPipelineBarrier", reinterpret_cast<PFN_vkVoidFunction>(CmdPipelineBarrier)},
    {"vkCmdBeginQuery", reinterpret_cast<PFN_vkVoidFunction>(CmdBeginQuery)},
    {"vkCmdEndQuery", reinterpret_cast<PFN_vkVoidFunction>(CmdEndQuery)},
    {"vkCmdResetQueryPool", reinterpret_cast<PFN_vkVoidFunction>(CmdResetQueryPool)},
    {"vkCmdWriteTimestamp", reinterpret_cast<PFN_vkVoidFunction>(CmdWriteTimestamp)},
    {"vkCmdCopyQueryPoolResults", reinterpret_cast<PFN_vkVoidFunction>(CmdCopyQueryPoolResults)},
    {"vkCmdPushConstants", reinterpret_cast<PFN_vkVoidFunction>(CmdPushConstants)},
    {"vkCmdBeginRenderPass", reinterpret_cast<PFN_vkVoidFunction>(CmdBeginRenderPass)},
    {"vkCmdNextSubpass", reinterpret_cast<PFN_vkVoidFunction>(CmdNextSubpass)},
    {"vkCmdEndRenderPass", reinterpret_cast<PFN_vkVoidFunction>(CmdEndRenderPass)},
    {"vkCmdExecuteCommands", reinterpret_cast<PFN_vkVoidFunction>(CmdExecuteCommands)},
    {"vkCreateDebugReportCallbackEXT", reinterpret_cast<PFN_vkVoidFunction>(CreateDebugReportCallbackEXT)},
    {"vkDestroyDebugReportCallbackEXT", reinterpret_cast<PFN_vkVoidFunction>(DestroyDebugReportCallbackEXT)},
    {"vkDebugReportMessageEXT", reinterpret_cast<PFN_vkVoidFunction>(DebugReportMessageEXT)},
    {"vkCmdDrawIndirectCountAMD", reinterpret_cast<PFN_vkVoidFunction>(CmdDrawIndirectCountAMD)},
    {"vkCmdDrawIndexedIndirectCountAMD", reinterpret_cast<PFN_vkVoidFunction>(CmdDrawIndexedIndirectCountAMD)},
#ifdef VK_USE_PLATFORM_WIN32_KHR
    {"vkGetMemoryWin32HandleNV", reinterpret_cast<PFN_vkVoidFunction>(GetMemoryWin32HandleNV)},
#endif
};


} // namespace threading

#endif
