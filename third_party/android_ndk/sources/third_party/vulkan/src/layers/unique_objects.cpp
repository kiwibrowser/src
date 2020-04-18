/*
 * Copyright (c) 2015-2016 The Khronos Group Inc.
 * Copyright (c) 2015-2016 Valve Corporation
 * Copyright (c) 2015-2016 LunarG, Inc.
 * Copyright (c) 2015-2016 Google, Inc.
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
 * Author: Tobin Ehlis <tobine@google.com>
 * Author: Mark Lobodzinski <mark@lunarg.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unordered_map>
#include <vector>
#include <list>
#include <memory>

#include "vk_loader_platform.h"
#include "vulkan/vk_layer.h"
#include "vk_layer_config.h"
#include "vk_layer_extension_utils.h"
#include "vk_layer_utils.h"
#include "vk_layer_table.h"
#include "vk_layer_logging.h"
#include "unique_objects.h"
#include "vk_dispatch_table_helper.h"
#include "vk_struct_string_helper_cpp.h"
#include "vk_layer_data.h"
#include "vk_layer_utils.h"

#include "unique_objects_wrappers.h"

namespace unique_objects {

static void initUniqueObjects(layer_data *instance_data, const VkAllocationCallbacks *pAllocator) {
    layer_debug_actions(instance_data->report_data, instance_data->logging_callback, pAllocator, "google_unique_objects");
}

// Handle CreateInstance Extensions
static void checkInstanceRegisterExtensions(const VkInstanceCreateInfo *pCreateInfo, VkInstance instance) {
    uint32_t i;
    layer_data *instance_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    VkLayerInstanceDispatchTable *disp_table = instance_data->instance_dispatch_table;
    instance_ext_map[disp_table] = {};

    for (i = 0; i < pCreateInfo->enabledExtensionCount; i++) {
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_SURFACE_EXTENSION_NAME) == 0) {
            instance_ext_map[disp_table].wsi_enabled = true;
        }
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_DISPLAY_EXTENSION_NAME) == 0) {
            instance_ext_map[disp_table].display_enabled = true;
        }
#ifdef VK_USE_PLATFORM_XLIB_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_XLIB_SURFACE_EXTENSION_NAME) == 0) {
            instance_ext_map[disp_table].xlib_enabled = true;
        }
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_XCB_SURFACE_EXTENSION_NAME) == 0) {
            instance_ext_map[disp_table].xcb_enabled = true;
        }
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME) == 0) {
            instance_ext_map[disp_table].wayland_enabled = true;
        }
#endif
#ifdef VK_USE_PLATFORM_MIR_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_MIR_SURFACE_EXTENSION_NAME) == 0) {
            instance_ext_map[disp_table].mir_enabled = true;
        }
#endif
#ifdef VK_USE_PLATFORM_ANDROID_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_ANDROID_SURFACE_EXTENSION_NAME) == 0) {
            instance_ext_map[disp_table].android_enabled = true;
        }
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_WIN32_SURFACE_EXTENSION_NAME) == 0) {
            instance_ext_map[disp_table].win32_enabled = true;
        }
#endif

        // Check for recognized instance extensions
        layer_data *instance_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
        if (!white_list(pCreateInfo->ppEnabledExtensionNames[i], kUniqueObjectsSupportedInstanceExtensions)) {
            log_msg(instance_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                    0, "UniqueObjects",
                    "Instance Extension %s is not supported by this layer.  Using this extension may adversely affect "
                    "validation results and/or produce undefined behavior.",
                    pCreateInfo->ppEnabledExtensionNames[i]);
        }
    }
}

// Handle CreateDevice Extensions
static void createDeviceRegisterExtensions(const VkDeviceCreateInfo *pCreateInfo, VkDevice device) {
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkLayerDispatchTable *disp_table = device_data->device_dispatch_table;
    PFN_vkGetDeviceProcAddr gpa = disp_table->GetDeviceProcAddr;

    device_data->device_dispatch_table->CreateSwapchainKHR = (PFN_vkCreateSwapchainKHR)gpa(device, "vkCreateSwapchainKHR");
    disp_table->DestroySwapchainKHR = (PFN_vkDestroySwapchainKHR)gpa(device, "vkDestroySwapchainKHR");
    disp_table->GetSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR)gpa(device, "vkGetSwapchainImagesKHR");
    disp_table->AcquireNextImageKHR = (PFN_vkAcquireNextImageKHR)gpa(device, "vkAcquireNextImageKHR");
    disp_table->QueuePresentKHR = (PFN_vkQueuePresentKHR)gpa(device, "vkQueuePresentKHR");
    device_data->wsi_enabled = false;

    for (uint32_t i = 0; i < pCreateInfo->enabledExtensionCount; i++) {
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
            device_data->wsi_enabled = true;
        }
        // Check for recognized device extensions
        if (!white_list(pCreateInfo->ppEnabledExtensionNames[i], kUniqueObjectsSupportedDeviceExtensions)) {
            log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                    0, "UniqueObjects",
                    "Device Extension %s is not supported by this layer.  Using this extension may adversely affect "
                    "validation results and/or produce undefined behavior.",
                    pCreateInfo->ppEnabledExtensionNames[i]);
        }
    }
}

VKAPI_ATTR VkResult VKAPI_CALL CreateInstance(const VkInstanceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator,
                                              VkInstance *pInstance) {
    VkLayerInstanceCreateInfo *chain_info = get_chain_info(pCreateInfo, VK_LAYER_LINK_INFO);

    assert(chain_info->u.pLayerInfo);
    PFN_vkGetInstanceProcAddr fpGetInstanceProcAddr = chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    PFN_vkCreateInstance fpCreateInstance = (PFN_vkCreateInstance)fpGetInstanceProcAddr(NULL, "vkCreateInstance");
    if (fpCreateInstance == NULL) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    // Advance the link info for the next element on the chain
    chain_info->u.pLayerInfo = chain_info->u.pLayerInfo->pNext;

    VkResult result = fpCreateInstance(pCreateInfo, pAllocator, pInstance);
    if (result != VK_SUCCESS) {
        return result;
    }

    layer_data *instance_data = get_my_data_ptr(get_dispatch_key(*pInstance), layer_data_map);
    instance_data->instance = *pInstance;
    instance_data->instance_dispatch_table = new VkLayerInstanceDispatchTable;
    layer_init_instance_dispatch_table(*pInstance, instance_data->instance_dispatch_table, fpGetInstanceProcAddr);

    instance_data->instance = *pInstance;
    instance_data->report_data =
        debug_report_create_instance(instance_data->instance_dispatch_table, *pInstance, pCreateInfo->enabledExtensionCount,
                                     pCreateInfo->ppEnabledExtensionNames);

    // Set up temporary debug callbacks to output messages at CreateInstance-time
    if (!layer_copy_tmp_callbacks(pCreateInfo->pNext, &instance_data->num_tmp_callbacks, &instance_data->tmp_dbg_create_infos,
                                  &instance_data->tmp_callbacks)) {
        if (instance_data->num_tmp_callbacks > 0) {
            if (layer_enable_tmp_callbacks(instance_data->report_data, instance_data->num_tmp_callbacks,
                                           instance_data->tmp_dbg_create_infos, instance_data->tmp_callbacks)) {
                layer_free_tmp_callbacks(instance_data->tmp_dbg_create_infos, instance_data->tmp_callbacks);
                instance_data->num_tmp_callbacks = 0;
            }
        }
    }

    initUniqueObjects(instance_data, pAllocator);
    checkInstanceRegisterExtensions(pCreateInfo, *pInstance);

    // Disable and free tmp callbacks, no longer necessary
    if (instance_data->num_tmp_callbacks > 0) {
        layer_disable_tmp_callbacks(instance_data->report_data, instance_data->num_tmp_callbacks, instance_data->tmp_callbacks);
        layer_free_tmp_callbacks(instance_data->tmp_dbg_create_infos, instance_data->tmp_callbacks);
        instance_data->num_tmp_callbacks = 0;
    }

    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyInstance(VkInstance instance, const VkAllocationCallbacks *pAllocator) {
    dispatch_key key = get_dispatch_key(instance);
    layer_data *instance_data = get_my_data_ptr(key, layer_data_map);
    VkLayerInstanceDispatchTable *disp_table = instance_data->instance_dispatch_table;
    instance_ext_map.erase(disp_table);
    disp_table->DestroyInstance(instance, pAllocator);

    // Clean up logging callback, if any
    while (instance_data->logging_callback.size() > 0) {
        VkDebugReportCallbackEXT callback = instance_data->logging_callback.back();
        layer_destroy_msg_callback(instance_data->report_data, callback, pAllocator);
        instance_data->logging_callback.pop_back();
    }

    layer_debug_report_destroy_instance(instance_data->report_data);
    layer_data_map.erase(key);
}

VKAPI_ATTR VkResult VKAPI_CALL CreateDevice(VkPhysicalDevice gpu, const VkDeviceCreateInfo *pCreateInfo,
                                            const VkAllocationCallbacks *pAllocator, VkDevice *pDevice) {
    layer_data *my_instance_data = get_my_data_ptr(get_dispatch_key(gpu), layer_data_map);
    VkLayerDeviceCreateInfo *chain_info = get_chain_info(pCreateInfo, VK_LAYER_LINK_INFO);

    assert(chain_info->u.pLayerInfo);
    PFN_vkGetInstanceProcAddr fpGetInstanceProcAddr = chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    PFN_vkGetDeviceProcAddr fpGetDeviceProcAddr = chain_info->u.pLayerInfo->pfnNextGetDeviceProcAddr;
    PFN_vkCreateDevice fpCreateDevice = (PFN_vkCreateDevice)fpGetInstanceProcAddr(my_instance_data->instance, "vkCreateDevice");
    if (fpCreateDevice == NULL) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    // Advance the link info for the next element on the chain
    chain_info->u.pLayerInfo = chain_info->u.pLayerInfo->pNext;

    VkResult result = fpCreateDevice(gpu, pCreateInfo, pAllocator, pDevice);
    if (result != VK_SUCCESS) {
        return result;
    }

    layer_data *my_device_data = get_my_data_ptr(get_dispatch_key(*pDevice), layer_data_map);
    my_device_data->report_data = layer_debug_report_create_device(my_instance_data->report_data, *pDevice);

    // Setup layer's device dispatch table
    my_device_data->device_dispatch_table = new VkLayerDispatchTable;
    layer_init_device_dispatch_table(*pDevice, my_device_data->device_dispatch_table, fpGetDeviceProcAddr);

    createDeviceRegisterExtensions(pCreateInfo, *pDevice);
    // Set gpu for this device in order to get at any objects mapped at instance level

    my_device_data->gpu = gpu;

    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyDevice(VkDevice device, const VkAllocationCallbacks *pAllocator) {
    dispatch_key key = get_dispatch_key(device);
    layer_data *dev_data = get_my_data_ptr(key, layer_data_map);

    layer_debug_report_destroy_device(device);
    dev_data->device_dispatch_table->DestroyDevice(device, pAllocator);
    layer_data_map.erase(key);
}

static const VkLayerProperties globalLayerProps = {"VK_LAYER_GOOGLE_unique_objects",
                                                   VK_LAYER_API_VERSION, // specVersion
                                                   1,                    // implementationVersion
                                                   "Google Validation Layer"};

static inline PFN_vkVoidFunction layer_intercept_proc(const char *name) {
    for (int i = 0; i < sizeof(procmap) / sizeof(procmap[0]); i++) {
        if (!strcmp(name, procmap[i].name))
            return procmap[i].pFunc;
    }
    return NULL;
}

VKAPI_ATTR VkResult VKAPI_CALL EnumerateInstanceLayerProperties(uint32_t *pCount, VkLayerProperties *pProperties) {
    return util_GetLayerProperties(1, &globalLayerProps, pCount, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL EnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t *pCount,
                                                              VkLayerProperties *pProperties) {
    return util_GetLayerProperties(1, &globalLayerProps, pCount, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL EnumerateInstanceExtensionProperties(const char *pLayerName, uint32_t *pCount,
                                                                    VkExtensionProperties *pProperties) {
    if (pLayerName && !strcmp(pLayerName, globalLayerProps.layerName))
        return util_GetExtensionProperties(0, NULL, pCount, pProperties);

    return VK_ERROR_LAYER_NOT_PRESENT;
}

VKAPI_ATTR VkResult VKAPI_CALL EnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice, const char *pLayerName,
                                                                  uint32_t *pCount, VkExtensionProperties *pProperties) {
    if (pLayerName && !strcmp(pLayerName, globalLayerProps.layerName))
        return util_GetExtensionProperties(0, nullptr, pCount, pProperties);

    assert(physicalDevice);

    dispatch_key key = get_dispatch_key(physicalDevice);
    layer_data *instance_data = get_my_data_ptr(key, layer_data_map);
    return instance_data->instance_dispatch_table->EnumerateDeviceExtensionProperties(physicalDevice, NULL, pCount, pProperties);
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL GetDeviceProcAddr(VkDevice device, const char *funcName) {
    PFN_vkVoidFunction addr;
    assert(device);
    addr = layer_intercept_proc(funcName);
    if (addr) {
        return addr;
    }

    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkLayerDispatchTable *disp_table = dev_data->device_dispatch_table;
    if (disp_table->GetDeviceProcAddr == NULL) {
        return NULL;
    }
    return disp_table->GetDeviceProcAddr(device, funcName);
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL GetInstanceProcAddr(VkInstance instance, const char *funcName) {
    PFN_vkVoidFunction addr;

    addr = layer_intercept_proc(funcName);
    if (addr) {
        return addr;
    }
    assert(instance);

    layer_data *instance_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    addr = debug_report_get_instance_proc_addr(instance_data->report_data, funcName);
    if (addr) {
        return addr;
    }

    VkLayerInstanceDispatchTable *disp_table = instance_data->instance_dispatch_table;
    if (disp_table->GetInstanceProcAddr == NULL) {
        return NULL;
    }
    return disp_table->GetInstanceProcAddr(instance, funcName);
}

VKAPI_ATTR VkResult VKAPI_CALL AllocateMemory(VkDevice device, const VkMemoryAllocateInfo *pAllocateInfo,
                                              const VkAllocationCallbacks *pAllocator, VkDeviceMemory *pMemory) {
    const VkMemoryAllocateInfo *input_allocate_info = pAllocateInfo;
    std::unique_ptr<safe_VkMemoryAllocateInfo> safe_allocate_info;
    std::unique_ptr<safe_VkDedicatedAllocationMemoryAllocateInfoNV> safe_dedicated_allocate_info;
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    if ((pAllocateInfo != nullptr) &&
        ContainsExtStruct(pAllocateInfo, VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_MEMORY_ALLOCATE_INFO_NV)) {
        // Assuming there is only one extension struct of this type in the list for now
        safe_dedicated_allocate_info =
            std::unique_ptr<safe_VkDedicatedAllocationMemoryAllocateInfoNV>(new safe_VkDedicatedAllocationMemoryAllocateInfoNV);
        safe_allocate_info = std::unique_ptr<safe_VkMemoryAllocateInfo>(new safe_VkMemoryAllocateInfo(pAllocateInfo));
        input_allocate_info = reinterpret_cast<const VkMemoryAllocateInfo *>(safe_allocate_info.get());

        const GenericHeader *orig_pnext = reinterpret_cast<const GenericHeader *>(pAllocateInfo->pNext);
        GenericHeader *input_pnext = reinterpret_cast<GenericHeader *>(safe_allocate_info.get());
        while (orig_pnext != nullptr) {
            if (orig_pnext->sType == VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_MEMORY_ALLOCATE_INFO_NV) {
                safe_dedicated_allocate_info->initialize(
                    reinterpret_cast<const VkDedicatedAllocationMemoryAllocateInfoNV *>(orig_pnext));

                std::unique_lock<std::mutex> lock(global_lock);

                if (safe_dedicated_allocate_info->buffer != VK_NULL_HANDLE) {
                    uint64_t local_buffer = reinterpret_cast<uint64_t &>(safe_dedicated_allocate_info->buffer);
                    safe_dedicated_allocate_info->buffer =
                        reinterpret_cast<VkBuffer &>(device_data->unique_id_mapping[local_buffer]);
                }

                if (safe_dedicated_allocate_info->image != VK_NULL_HANDLE) {
                    uint64_t local_image = reinterpret_cast<uint64_t &>(safe_dedicated_allocate_info->image);
                    safe_dedicated_allocate_info->image = reinterpret_cast<VkImage &>(device_data->unique_id_mapping[local_image]);
                }

                lock.unlock();

                input_pnext->pNext = reinterpret_cast<GenericHeader *>(safe_dedicated_allocate_info.get());
                input_pnext = reinterpret_cast<GenericHeader *>(input_pnext->pNext);
            } else {
                // TODO: generic handling of pNext copies
            }

            orig_pnext = reinterpret_cast<const GenericHeader *>(orig_pnext->pNext);
        }
    }

    VkResult result = device_data->device_dispatch_table->AllocateMemory(device, input_allocate_info, pAllocator, pMemory);

    if (VK_SUCCESS == result) {
        std::lock_guard<std::mutex> lock(global_lock);
        uint64_t unique_id = global_unique_id++;
        device_data->unique_id_mapping[unique_id] = reinterpret_cast<uint64_t &>(*pMemory);
        *pMemory = reinterpret_cast<VkDeviceMemory &>(unique_id);
    }

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateComputePipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount,
                                                      const VkComputePipelineCreateInfo *pCreateInfos,
                                                      const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines) {
    layer_data *my_device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    safe_VkComputePipelineCreateInfo *local_pCreateInfos = NULL;
    if (pCreateInfos) {
        std::lock_guard<std::mutex> lock(global_lock);
        local_pCreateInfos = new safe_VkComputePipelineCreateInfo[createInfoCount];
        for (uint32_t idx0 = 0; idx0 < createInfoCount; ++idx0) {
            local_pCreateInfos[idx0].initialize(&pCreateInfos[idx0]);
            if (pCreateInfos[idx0].basePipelineHandle) {
                local_pCreateInfos[idx0].basePipelineHandle =
                    (VkPipeline)my_device_data
                        ->unique_id_mapping[reinterpret_cast<const uint64_t &>(pCreateInfos[idx0].basePipelineHandle)];
            }
            if (pCreateInfos[idx0].layout) {
                local_pCreateInfos[idx0].layout =
                    (VkPipelineLayout)
                        my_device_data->unique_id_mapping[reinterpret_cast<const uint64_t &>(pCreateInfos[idx0].layout)];
            }
            if (pCreateInfos[idx0].stage.module) {
                local_pCreateInfos[idx0].stage.module =
                    (VkShaderModule)
                        my_device_data->unique_id_mapping[reinterpret_cast<const uint64_t &>(pCreateInfos[idx0].stage.module)];
            }
        }
    }
    if (pipelineCache) {
        std::lock_guard<std::mutex> lock(global_lock);
        pipelineCache = (VkPipelineCache)my_device_data->unique_id_mapping[reinterpret_cast<uint64_t &>(pipelineCache)];
    }

    VkResult result = my_device_data->device_dispatch_table->CreateComputePipelines(
        device, pipelineCache, createInfoCount, (const VkComputePipelineCreateInfo *)local_pCreateInfos, pAllocator, pPipelines);
    delete[] local_pCreateInfos;
    if (VK_SUCCESS == result) {
        uint64_t unique_id = 0;
        std::lock_guard<std::mutex> lock(global_lock);
        for (uint32_t i = 0; i < createInfoCount; ++i) {
            unique_id = global_unique_id++;
            my_device_data->unique_id_mapping[unique_id] = reinterpret_cast<uint64_t &>(pPipelines[i]);
            pPipelines[i] = reinterpret_cast<VkPipeline &>(unique_id);
        }
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount,
                                                       const VkGraphicsPipelineCreateInfo *pCreateInfos,
                                                       const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines) {
    layer_data *my_device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    safe_VkGraphicsPipelineCreateInfo *local_pCreateInfos = NULL;
    if (pCreateInfos) {
        local_pCreateInfos = new safe_VkGraphicsPipelineCreateInfo[createInfoCount];
        std::lock_guard<std::mutex> lock(global_lock);
        for (uint32_t idx0 = 0; idx0 < createInfoCount; ++idx0) {
            local_pCreateInfos[idx0].initialize(&pCreateInfos[idx0]);
            if (pCreateInfos[idx0].basePipelineHandle) {
                local_pCreateInfos[idx0].basePipelineHandle =
                    (VkPipeline)my_device_data
                        ->unique_id_mapping[reinterpret_cast<const uint64_t &>(pCreateInfos[idx0].basePipelineHandle)];
            }
            if (pCreateInfos[idx0].layout) {
                local_pCreateInfos[idx0].layout =
                    (VkPipelineLayout)
                        my_device_data->unique_id_mapping[reinterpret_cast<const uint64_t &>(pCreateInfos[idx0].layout)];
            }
            if (pCreateInfos[idx0].pStages) {
                for (uint32_t idx1 = 0; idx1 < pCreateInfos[idx0].stageCount; ++idx1) {
                    if (pCreateInfos[idx0].pStages[idx1].module) {
                        local_pCreateInfos[idx0].pStages[idx1].module =
                            (VkShaderModule)my_device_data
                                ->unique_id_mapping[reinterpret_cast<const uint64_t &>(pCreateInfos[idx0].pStages[idx1].module)];
                    }
                }
            }
            if (pCreateInfos[idx0].renderPass) {
                local_pCreateInfos[idx0].renderPass =
                    (VkRenderPass)
                        my_device_data->unique_id_mapping[reinterpret_cast<const uint64_t &>(pCreateInfos[idx0].renderPass)];
            }
        }
    }
    if (pipelineCache) {
        std::lock_guard<std::mutex> lock(global_lock);
        pipelineCache = (VkPipelineCache)my_device_data->unique_id_mapping[reinterpret_cast<uint64_t &>(pipelineCache)];
    }

    VkResult result = my_device_data->device_dispatch_table->CreateGraphicsPipelines(
        device, pipelineCache, createInfoCount, (const VkGraphicsPipelineCreateInfo *)local_pCreateInfos, pAllocator, pPipelines);
    delete[] local_pCreateInfos;
    if (VK_SUCCESS == result) {
        uint64_t unique_id = 0;
        std::lock_guard<std::mutex> lock(global_lock);
        for (uint32_t i = 0; i < createInfoCount; ++i) {
            unique_id = global_unique_id++;
            my_device_data->unique_id_mapping[unique_id] = reinterpret_cast<uint64_t &>(pPipelines[i]);
            pPipelines[i] = reinterpret_cast<VkPipeline &>(unique_id);
        }
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateDebugReportCallbackEXT(VkInstance instance,
                                                            const VkDebugReportCallbackCreateInfoEXT *pCreateInfo,
                                                            const VkAllocationCallbacks *pAllocator,
                                                            VkDebugReportCallbackEXT *pMsgCallback) {
    layer_data *instance_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    VkResult result =
        instance_data->instance_dispatch_table->CreateDebugReportCallbackEXT(instance, pCreateInfo, pAllocator, pMsgCallback);

    if (VK_SUCCESS == result) {
        result = layer_create_msg_callback(instance_data->report_data, false, pCreateInfo, pAllocator, pMsgCallback);
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback,
                                                         const VkAllocationCallbacks *pAllocator) {
    layer_data *instance_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    instance_data->instance_dispatch_table->DestroyDebugReportCallbackEXT(instance, callback, pAllocator);
    layer_destroy_msg_callback(instance_data->report_data, callback, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL DebugReportMessageEXT(VkInstance instance, VkDebugReportFlagsEXT flags,
                                                 VkDebugReportObjectTypeEXT objType, uint64_t object, size_t location,
                                                 int32_t msgCode, const char *pLayerPrefix, const char *pMsg) {
    layer_data *instance_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    instance_data->instance_dispatch_table->DebugReportMessageEXT(instance, flags, objType, object, location, msgCode, pLayerPrefix,
                                                                  pMsg);
}

VKAPI_ATTR VkResult VKAPI_CALL CreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR *pCreateInfo,
                                                  const VkAllocationCallbacks *pAllocator, VkSwapchainKHR *pSwapchain) {
    layer_data *my_map_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    safe_VkSwapchainCreateInfoKHR *local_pCreateInfo = NULL;
    if (pCreateInfo) {
        std::lock_guard<std::mutex> lock(global_lock);
        local_pCreateInfo = new safe_VkSwapchainCreateInfoKHR(pCreateInfo);
        local_pCreateInfo->oldSwapchain =
            (VkSwapchainKHR)my_map_data->unique_id_mapping[reinterpret_cast<const uint64_t &>(pCreateInfo->oldSwapchain)];
        // Need to pull surface mapping from the instance-level map
        layer_data *instance_data = get_my_data_ptr(get_dispatch_key(my_map_data->gpu), layer_data_map);
        local_pCreateInfo->surface =
            (VkSurfaceKHR)instance_data->unique_id_mapping[reinterpret_cast<const uint64_t &>(pCreateInfo->surface)];
    }

    VkResult result = my_map_data->device_dispatch_table->CreateSwapchainKHR(
        device, (const VkSwapchainCreateInfoKHR *)local_pCreateInfo, pAllocator, pSwapchain);
    if (local_pCreateInfo) {
        delete local_pCreateInfo;
    }
    if (VK_SUCCESS == result) {
        std::lock_guard<std::mutex> lock(global_lock);
        uint64_t unique_id = global_unique_id++;
        my_map_data->unique_id_mapping[unique_id] = reinterpret_cast<uint64_t &>(*pSwapchain);
        *pSwapchain = reinterpret_cast<VkSwapchainKHR &>(unique_id);
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL GetSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain, uint32_t *pSwapchainImageCount,
                                                     VkImage *pSwapchainImages) {
    layer_data *my_device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    if (VK_NULL_HANDLE != swapchain) {
        std::lock_guard<std::mutex> lock(global_lock);
        swapchain = (VkSwapchainKHR)my_device_data->unique_id_mapping[reinterpret_cast<uint64_t &>(swapchain)];
    }
    VkResult result =
        my_device_data->device_dispatch_table->GetSwapchainImagesKHR(device, swapchain, pSwapchainImageCount, pSwapchainImages);
    // TODO : Need to add corresponding code to delete these images
    if (VK_SUCCESS == result) {
        if ((*pSwapchainImageCount > 0) && pSwapchainImages) {
            uint64_t unique_id = 0;
            std::lock_guard<std::mutex> lock(global_lock);
            for (uint32_t i = 0; i < *pSwapchainImageCount; ++i) {
                unique_id = global_unique_id++;
                my_device_data->unique_id_mapping[unique_id] = reinterpret_cast<uint64_t &>(pSwapchainImages[i]);
                pSwapchainImages[i] = reinterpret_cast<VkImage &>(unique_id);
            }
        }
    }
    return result;
}

#ifndef __ANDROID__
VKAPI_ATTR VkResult VKAPI_CALL GetPhysicalDeviceDisplayPropertiesKHR(VkPhysicalDevice physicalDevice, uint32_t *pPropertyCount,
                                                                     VkDisplayPropertiesKHR *pProperties) {
    layer_data *my_map_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    safe_VkDisplayPropertiesKHR *local_pProperties = NULL;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        if (pProperties) {
            local_pProperties = new safe_VkDisplayPropertiesKHR[*pPropertyCount];
            for (uint32_t idx0 = 0; idx0 < *pPropertyCount; ++idx0) {
                local_pProperties[idx0].initialize(&pProperties[idx0]);
                if (pProperties[idx0].display) {
                    local_pProperties[idx0].display =
                        (VkDisplayKHR)my_map_data->unique_id_mapping[reinterpret_cast<const uint64_t &>(pProperties[idx0].display)];
                }
            }
        }
    }

    VkResult result = my_map_data->instance_dispatch_table->GetPhysicalDeviceDisplayPropertiesKHR(
        physicalDevice, pPropertyCount, (VkDisplayPropertiesKHR *)local_pProperties);
    if (result == VK_SUCCESS && pProperties) {
        for (uint32_t idx0 = 0; idx0 < *pPropertyCount; ++idx0) {
            std::lock_guard<std::mutex> lock(global_lock);

            uint64_t unique_id = global_unique_id++;
            my_map_data->unique_id_mapping[unique_id] = reinterpret_cast<uint64_t &>(local_pProperties[idx0].display);
            pProperties[idx0].display = reinterpret_cast<VkDisplayKHR &>(unique_id);
            pProperties[idx0].displayName = local_pProperties[idx0].displayName;
            pProperties[idx0].physicalDimensions = local_pProperties[idx0].physicalDimensions;
            pProperties[idx0].physicalResolution = local_pProperties[idx0].physicalResolution;
            pProperties[idx0].supportedTransforms = local_pProperties[idx0].supportedTransforms;
            pProperties[idx0].planeReorderPossible = local_pProperties[idx0].planeReorderPossible;
            pProperties[idx0].persistentContent = local_pProperties[idx0].persistentContent;
        }
    }
    if (local_pProperties) {
        delete[] local_pProperties;
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL GetDisplayPlaneSupportedDisplaysKHR(VkPhysicalDevice physicalDevice, uint32_t planeIndex,
                                                                   uint32_t *pDisplayCount, VkDisplayKHR *pDisplays) {
    layer_data *my_map_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    VkResult result = my_map_data->instance_dispatch_table->GetDisplayPlaneSupportedDisplaysKHR(physicalDevice, planeIndex,
                                                                                                pDisplayCount, pDisplays);
    if (VK_SUCCESS == result) {
        if ((*pDisplayCount > 0) && pDisplays) {
            std::lock_guard<std::mutex> lock(global_lock);
            for (uint32_t i = 0; i < *pDisplayCount; i++) {
                auto it = my_map_data->unique_id_mapping.find(reinterpret_cast<const uint64_t &>(pDisplays[i]));
                assert(it != my_map_data->unique_id_mapping.end());
                pDisplays[i] = reinterpret_cast<VkDisplayKHR &>(it->second);
            }
        }
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL GetDisplayModePropertiesKHR(VkPhysicalDevice physicalDevice, VkDisplayKHR display,
                                                           uint32_t *pPropertyCount, VkDisplayModePropertiesKHR *pProperties) {
    layer_data *my_map_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    safe_VkDisplayModePropertiesKHR *local_pProperties = NULL;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        display = (VkDisplayKHR)my_map_data->unique_id_mapping[reinterpret_cast<uint64_t &>(display)];
        if (pProperties) {
            local_pProperties = new safe_VkDisplayModePropertiesKHR[*pPropertyCount];
            for (uint32_t idx0 = 0; idx0 < *pPropertyCount; ++idx0) {
                local_pProperties[idx0].initialize(&pProperties[idx0]);
            }
        }
    }

    VkResult result = my_map_data->instance_dispatch_table->GetDisplayModePropertiesKHR(
        physicalDevice, display, pPropertyCount, (VkDisplayModePropertiesKHR *)local_pProperties);
    if (result == VK_SUCCESS && pProperties) {
        for (uint32_t idx0 = 0; idx0 < *pPropertyCount; ++idx0) {
            std::lock_guard<std::mutex> lock(global_lock);

            uint64_t unique_id = global_unique_id++;
            my_map_data->unique_id_mapping[unique_id] = reinterpret_cast<uint64_t &>(local_pProperties[idx0].displayMode);
            pProperties[idx0].displayMode = reinterpret_cast<VkDisplayModeKHR &>(unique_id);
            pProperties[idx0].parameters.visibleRegion.width = local_pProperties[idx0].parameters.visibleRegion.width;
            pProperties[idx0].parameters.visibleRegion.height = local_pProperties[idx0].parameters.visibleRegion.height;
            pProperties[idx0].parameters.refreshRate = local_pProperties[idx0].parameters.refreshRate;
        }
    }
    if (local_pProperties) {
        delete[] local_pProperties;
    }
    return result;
}
#endif

} // namespace unique_objects

// vk_layer_logging.h expects these to be defined
VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugReportCallbackEXT(VkInstance instance,
                                                              const VkDebugReportCallbackCreateInfoEXT *pCreateInfo,
                                                              const VkAllocationCallbacks *pAllocator,
                                                              VkDebugReportCallbackEXT *pMsgCallback) {
    return unique_objects::CreateDebugReportCallbackEXT(instance, pCreateInfo, pAllocator, pMsgCallback);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT msgCallback,
                                                           const VkAllocationCallbacks *pAllocator) {
    unique_objects::DestroyDebugReportCallbackEXT(instance, msgCallback, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL vkDebugReportMessageEXT(VkInstance instance, VkDebugReportFlagsEXT flags,
                                                   VkDebugReportObjectTypeEXT objType, uint64_t object, size_t location,
                                                   int32_t msgCode, const char *pLayerPrefix, const char *pMsg) {
    unique_objects::DebugReportMessageEXT(instance, flags, objType, object, location, msgCode, pLayerPrefix, pMsg);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceExtensionProperties(const char *pLayerName, uint32_t *pCount,
                                                                                      VkExtensionProperties *pProperties) {
    return unique_objects::EnumerateInstanceExtensionProperties(pLayerName, pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceLayerProperties(uint32_t *pCount,
                                                                                  VkLayerProperties *pProperties) {
    return unique_objects::EnumerateInstanceLayerProperties(pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t *pCount,
                                                                                VkLayerProperties *pProperties) {
    assert(physicalDevice == VK_NULL_HANDLE);
    return unique_objects::EnumerateDeviceLayerProperties(VK_NULL_HANDLE, pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice dev, const char *funcName) {
    return unique_objects::GetDeviceProcAddr(dev, funcName);
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char *funcName) {
    return unique_objects::GetInstanceProcAddr(instance, funcName);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice,
                                                                                    const char *pLayerName, uint32_t *pCount,
                                                                                    VkExtensionProperties *pProperties) {
    assert(physicalDevice == VK_NULL_HANDLE);
    return unique_objects::EnumerateDeviceExtensionProperties(VK_NULL_HANDLE, pLayerName, pCount, pProperties);
}
