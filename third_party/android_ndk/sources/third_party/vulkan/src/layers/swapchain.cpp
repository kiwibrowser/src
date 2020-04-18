/* Copyright (c) 2015-2016 The Khronos Group Inc.
 * Copyright (c) 2015-2016 Valve Corporation
 * Copyright (c) 2015-2016 LunarG, Inc.
 * Copyright (C) 2015-2016 Google Inc.
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
 * Author: Ian Elliott <ian@lunarg.com>
 * Author: Ian Elliott <ianelliott@google.com>
 */

#include <mutex>
#include <stdio.h>
#include <string.h>
#include <vk_loader_platform.h>
#include <vulkan/vk_icd.h>
#include "swapchain.h"
#include "vk_layer_extension_utils.h"
#include "vk_enum_string_helper.h"
#include "vk_layer_utils.h"

namespace swapchain {

static std::mutex global_lock;

// The following is for logging error messages:
static std::unordered_map<void *, layer_data *> layer_data_map;

static const VkExtensionProperties instance_extensions[] = {{VK_EXT_DEBUG_REPORT_EXTENSION_NAME, VK_EXT_DEBUG_REPORT_SPEC_VERSION}};

static const VkLayerProperties swapchain_layer = {
    "VK_LAYER_LUNARG_swapchain", VK_LAYER_API_VERSION, 1, "LunarG Validation Layer",
};

static void checkDeviceRegisterExtensions(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo *pCreateInfo, VkDevice device) {
    uint32_t i;
    layer_data *my_device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    layer_data *my_instance_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);

    SwpPhysicalDevice *pPhysicalDevice = NULL;
    {
        auto it = my_instance_data->physicalDeviceMap.find(physicalDevice);
        pPhysicalDevice = (it == my_instance_data->physicalDeviceMap.end()) ? NULL : &it->second;
    }
    if (pPhysicalDevice) {
        my_device_data->deviceMap[device].pPhysicalDevice = pPhysicalDevice;
        pPhysicalDevice->pDevice = &my_device_data->deviceMap[device];
    } else {
        // TBD: Should we leave error in (since Swapchain really needs this
        // link)?
        log_msg(my_instance_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT,
                reinterpret_cast<uint64_t>(physicalDevice), __LINE__, SWAPCHAIN_INVALID_HANDLE, "Swapchain",
                "vkCreateDevice() called with a non-valid VkPhysicalDevice.");
    }
    my_device_data->deviceMap[device].device = device;
    my_device_data->deviceMap[device].swapchainExtensionEnabled = false;
    my_device_data->deviceMap[device].displaySwapchainExtensionEnabled = false;

    // Record whether the WSI device extension was enabled for this VkDevice.
    // No need to check if the extension was advertised by
    // vkEnumerateDeviceExtensionProperties(), since the loader handles that.
    for (i = 0; i < pCreateInfo->enabledExtensionCount; i++) {
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
            my_device_data->deviceMap[device].swapchainExtensionEnabled = true;
        }
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_DISPLAY_SWAPCHAIN_EXTENSION_NAME) == 0) {
            my_device_data->deviceMap[device].displaySwapchainExtensionEnabled = true;
        }
    }
}

static void checkInstanceRegisterExtensions(const VkInstanceCreateInfo *pCreateInfo, VkInstance instance) {
    uint32_t i;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);

    // Remember this instance, and whether the VK_KHR_surface extension
    // was enabled for it:
    my_data->instanceMap[instance].instance = instance;
    my_data->instanceMap[instance].surfaceExtensionEnabled = false;
    my_data->instanceMap[instance].displayExtensionEnabled = false;
#ifdef VK_USE_PLATFORM_ANDROID_KHR
    my_data->instanceMap[instance].androidSurfaceExtensionEnabled = false;
#endif // VK_USE_PLATFORM_ANDROID_KHR
#ifdef VK_USE_PLATFORM_MIR_KHR
    my_data->instanceMap[instance].mirSurfaceExtensionEnabled = false;
#endif // VK_USE_PLATFORM_MIR_KHR
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    my_data->instanceMap[instance].waylandSurfaceExtensionEnabled = false;
#endif // VK_USE_PLATFORM_WAYLAND_KHR
#ifdef VK_USE_PLATFORM_WIN32_KHR
    my_data->instanceMap[instance].win32SurfaceExtensionEnabled = false;
#endif // VK_USE_PLATFORM_WIN32_KHR
#ifdef VK_USE_PLATFORM_XCB_KHR
    my_data->instanceMap[instance].xcbSurfaceExtensionEnabled = false;
#endif // VK_USE_PLATFORM_XCB_KHR
#ifdef VK_USE_PLATFORM_XLIB_KHR
    my_data->instanceMap[instance].xlibSurfaceExtensionEnabled = false;
#endif // VK_USE_PLATFORM_XLIB_KHR

    // Look for one or more debug report create info structures, and copy the
    // callback(s) for each one found (for use by vkDestroyInstance)
    layer_copy_tmp_callbacks(pCreateInfo->pNext, &my_data->num_tmp_callbacks, &my_data->tmp_dbg_create_infos,
                             &my_data->tmp_callbacks);

    // Record whether the WSI instance extension was enabled for this
    // VkInstance.  No need to check if the extension was advertised by
    // vkEnumerateInstanceExtensionProperties(), since the loader handles that.
    for (i = 0; i < pCreateInfo->enabledExtensionCount; i++) {
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_SURFACE_EXTENSION_NAME) == 0) {

            my_data->instanceMap[instance].surfaceExtensionEnabled = true;
        }
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_DISPLAY_EXTENSION_NAME) == 0) {

            my_data->instanceMap[instance].displayExtensionEnabled = true;
        }
#ifdef VK_USE_PLATFORM_ANDROID_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_ANDROID_SURFACE_EXTENSION_NAME) == 0) {

            my_data->instanceMap[instance].androidSurfaceExtensionEnabled = true;
        }
#endif // VK_USE_PLATFORM_ANDROID_KHR
#ifdef VK_USE_PLATFORM_MIR_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_MIR_SURFACE_EXTENSION_NAME) == 0) {

            my_data->instanceMap[instance].mirSurfaceExtensionEnabled = true;
        }
#endif // VK_USE_PLATFORM_MIR_KHR
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME) == 0) {

            my_data->instanceMap[instance].waylandSurfaceExtensionEnabled = true;
        }
#endif // VK_USE_PLATFORM_WAYLAND_KHR
#ifdef VK_USE_PLATFORM_WIN32_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_WIN32_SURFACE_EXTENSION_NAME) == 0) {

            my_data->instanceMap[instance].win32SurfaceExtensionEnabled = true;
        }
#endif // VK_USE_PLATFORM_WIN32_KHR
#ifdef VK_USE_PLATFORM_XCB_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_XCB_SURFACE_EXTENSION_NAME) == 0) {

            my_data->instanceMap[instance].xcbSurfaceExtensionEnabled = true;
        }
#endif // VK_USE_PLATFORM_XCB_KHR
#ifdef VK_USE_PLATFORM_XLIB_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_XLIB_SURFACE_EXTENSION_NAME) == 0) {

            my_data->instanceMap[instance].xlibSurfaceExtensionEnabled = true;
        }
#endif // VK_USE_PLATFORM_XLIB_KHR
    }
}

#include "vk_dispatch_table_helper.h"
static void init_swapchain(layer_data *my_data, const VkAllocationCallbacks *pAllocator) {

    layer_debug_actions(my_data->report_data, my_data->logging_callback, pAllocator, "lunarg_swapchain");
}

static const char *surfaceTransformStr(VkSurfaceTransformFlagBitsKHR value) {
    // Return a string corresponding to the value:
    return string_VkSurfaceTransformFlagBitsKHR(value);
}

static const char *surfaceCompositeAlphaStr(VkCompositeAlphaFlagBitsKHR value) {
    // Return a string corresponding to the value:
    return string_VkCompositeAlphaFlagBitsKHR(value);
}

static const char *presentModeStr(VkPresentModeKHR value) {
    // Return a string corresponding to the value:
    return string_VkPresentModeKHR(value);
}

static const char *sharingModeStr(VkSharingMode value) {
    // Return a string corresponding to the value:
    return string_VkSharingMode(value);
}

static bool ValidateQueueFamilyIndex(layer_data *my_data, uint32_t queue_family_index, uint32_t queue_family_count,
                                     VkPhysicalDevice physical_device, const char *function) {
    bool skip_call = false;
    if (queue_family_index >= queue_family_count) {
        skip_call = log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT,
                            reinterpret_cast<uint64_t>(physical_device), __LINE__, SWAPCHAIN_QUEUE_FAMILY_INDEX_TOO_LARGE,
                            swapchain_layer_name,
                            "%s() called with a queueFamilyIndex that is too large (i.e. %d).  The maximum value (returned by "
                            "vkGetPhysicalDeviceQueueFamilyProperties) is only %d.",
                            function, queue_family_index, queue_family_count);
    }
    return skip_call;
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

    layer_data *my_data = get_my_data_ptr(get_dispatch_key(*pInstance), layer_data_map);
    my_data->instance = *pInstance;
    my_data->instance_dispatch_table = new VkLayerInstanceDispatchTable;
    layer_init_instance_dispatch_table(*pInstance, my_data->instance_dispatch_table, fpGetInstanceProcAddr);

    my_data->report_data = debug_report_create_instance(my_data->instance_dispatch_table, *pInstance,
                                                        pCreateInfo->enabledExtensionCount, pCreateInfo->ppEnabledExtensionNames);

    // Call the following function after my_data is initialized:
    checkInstanceRegisterExtensions(pCreateInfo, *pInstance);
    init_swapchain(my_data, pAllocator);

    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyInstance(VkInstance instance, const VkAllocationCallbacks *pAllocator) {
    dispatch_key key = get_dispatch_key(instance);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    SwpInstance *pInstance = NULL;
    {
        auto it = my_data->instanceMap.find(instance);
        pInstance = (it == my_data->instanceMap.end()) ? NULL : &it->second;
    }

    // Call down the call chain:
    my_data->instance_dispatch_table->DestroyInstance(instance, pAllocator);

    std::lock_guard<std::mutex> lock(global_lock);

    // Enable the temporary callback(s) here to catch cleanup issues:
    bool callback_setup = false;
    if (my_data->num_tmp_callbacks > 0) {
        if (!layer_enable_tmp_callbacks(my_data->report_data, my_data->num_tmp_callbacks, my_data->tmp_dbg_create_infos,
                                        my_data->tmp_callbacks)) {
            callback_setup = true;
        }
    }

    // Do additional internal cleanup:
    if (pInstance) {
        // Delete all of the SwpPhysicalDevice's, SwpSurface's, and the
        // SwpInstance associated with this instance:
        for (auto it = pInstance->physicalDevices.begin(); it != pInstance->physicalDevices.end(); it++) {

            // Free memory that was allocated for/by this SwpPhysicalDevice:
            SwpPhysicalDevice *pPhysicalDevice = it->second;
            if (pPhysicalDevice) {
                if (pPhysicalDevice->pDevice) {
                    log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                            reinterpret_cast<uint64_t>(pPhysicalDevice->pDevice->device), __LINE__,
                            SWAPCHAIN_DEL_OBJECT_BEFORE_CHILDREN, swapchain_layer_name,
                            "VkDestroyInstance() called before all of its associated VkDevices were destroyed.");
                }
                free(pPhysicalDevice->pSurfaceFormats);
                free(pPhysicalDevice->pPresentModes);
            }

            // Erase the SwpPhysicalDevice's from the my_data->physicalDeviceMap (which
            // are simply pointed to by the SwpInstance):
            my_data->physicalDeviceMap.erase(it->second->physicalDevice);
        }
        for (auto it = pInstance->surfaces.begin(); it != pInstance->surfaces.end(); it++) {

            // Free memory that was allocated for/by this SwpPhysicalDevice:
            SwpSurface *pSurface = it->second;
            if (pSurface) {
                log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
                        reinterpret_cast<uint64_t>(pInstance->instance), __LINE__, SWAPCHAIN_DEL_OBJECT_BEFORE_CHILDREN,
                        swapchain_layer_name,
                        "VkDestroyInstance() called before all of its associated VkSurfaceKHRs were destroyed.");
            }
        }
        my_data->instanceMap.erase(instance);
    }

    // Disable and cleanup the temporary callback(s):
    if (callback_setup) {
        layer_disable_tmp_callbacks(my_data->report_data, my_data->num_tmp_callbacks, my_data->tmp_callbacks);
    }
    if (my_data->num_tmp_callbacks > 0) {
        layer_free_tmp_callbacks(my_data->tmp_dbg_create_infos, my_data->tmp_callbacks);
        my_data->num_tmp_callbacks = 0;
    }

    // Clean up logging callback, if any
    while (my_data->logging_callback.size() > 0) {
        VkDebugReportCallbackEXT callback = my_data->logging_callback.back();
        layer_destroy_msg_callback(my_data->report_data, callback, pAllocator);
        my_data->logging_callback.pop_back();
    }
    layer_debug_report_destroy_instance(my_data->report_data);

    delete my_data->instance_dispatch_table;
    layer_data_map.erase(key);
}

VKAPI_ATTR void VKAPI_CALL GetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice physicalDevice,
                                                                  uint32_t *pQueueFamilyPropertyCount,
                                                                  VkQueueFamilyProperties *pQueueFamilyProperties) {
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);

    // Call down the call chain:
    my_data->instance_dispatch_table->GetPhysicalDeviceQueueFamilyProperties(physicalDevice, pQueueFamilyPropertyCount,
                                                                             pQueueFamilyProperties);

    // Record the result of this query:
    std::lock_guard<std::mutex> lock(global_lock);
    SwpPhysicalDevice *pPhysicalDevice = NULL;
    {
        auto it = my_data->physicalDeviceMap.find(physicalDevice);
        pPhysicalDevice = (it == my_data->physicalDeviceMap.end()) ? NULL : &it->second;
    }
    // Note: for poorly-written applications (e.g. that don't call this command
    // twice, the first time with pQueueFamilyProperties set to NULL, and the
    // second time with a non-NULL pQueueFamilyProperties and with the same
    // count as returned the first time), record the count when
    // pQueueFamilyProperties is non-NULL:
    if (pPhysicalDevice && pQueueFamilyPropertyCount && pQueueFamilyProperties) {
        pPhysicalDevice->gotQueueFamilyPropertyCount = true;
        pPhysicalDevice->numOfQueueFamilies = *pQueueFamilyPropertyCount;
    }
}

#ifdef VK_USE_PLATFORM_ANDROID_KHR
VKAPI_ATTR VkResult VKAPI_CALL CreateAndroidSurfaceKHR(VkInstance instance, const VkAndroidSurfaceCreateInfoKHR *pCreateInfo,
                                                       const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
    VkResult result = VK_SUCCESS;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    SwpInstance *pInstance = NULL;
    {
        auto it = my_data->instanceMap.find(instance);
        pInstance = (it == my_data->instanceMap.end()) ? NULL : &it->second;
    }

    // Validate that the platform extension was enabled:
    if (pInstance && !pInstance->androidSurfaceExtensionEnabled) {
        skip_call |=
            log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
                    reinterpret_cast<uint64_t>(instance), __LINE__, SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED, swapchain_layer_name,
                    "vkCreateAndroidSurfaceKHR() called even though the %s extension was not enabled for this VkInstance.",
                    VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
    }
    lock.unlock();

    if (!skip_call) {
        // Call down the call chain:
        result = my_data->instance_dispatch_table->CreateAndroidSurfaceKHR(instance, pCreateInfo, pAllocator, pSurface);
        lock.lock();

        // Obtain this pointer again after locking:
        {
            auto it = my_data->instanceMap.find(instance);
            pInstance = (it == my_data->instanceMap.end()) ? NULL : &it->second;
        }
        if ((result == VK_SUCCESS) && pInstance && pSurface) {
            // Record the VkSurfaceKHR returned by the ICD:
            my_data->surfaceMap[*pSurface].surface = *pSurface;
            my_data->surfaceMap[*pSurface].pInstance = pInstance;
            my_data->surfaceMap[*pSurface].numQueueFamilyIndexSupport = 0;
            my_data->surfaceMap[*pSurface].pQueueFamilyIndexSupport = NULL;
            // Point to the associated SwpInstance:
            pInstance->surfaces[*pSurface] = &my_data->surfaceMap[*pSurface];
        }
        lock.unlock();

        return result;
    }
    return VK_ERROR_VALIDATION_FAILED_EXT;
}
#endif // VK_USE_PLATFORM_ANDROID_KHR

#ifdef VK_USE_PLATFORM_MIR_KHR
VKAPI_ATTR VkResult VKAPI_CALL CreateMirSurfaceKHR(VkInstance instance, const VkMirSurfaceCreateInfoKHR *pCreateInfo,
                                                   const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
    VkResult result = VK_SUCCESS;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    SwpInstance *pInstance = NULL;
    {
        auto it = my_data->instanceMap.find(instance);
        pInstance = (it == my_data->instanceMap.end()) ? NULL : &it->second;
    }

    // Validate that the platform extension was enabled:
    if (pInstance && !pInstance->mirSurfaceExtensionEnabled) {
        skip_call |=
            log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
                    reinterpret_cast<uint64_t>(instance), __LINE__, SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED, swapchain_layer_name,
                    "vkCreateMirSurfaceKHR() called even though the %s extension was not enabled for this VkInstance.",
                    VK_KHR_MIR_SURFACE_EXTENSION_NAME);
    }
    lock.unlock();

    if (!skip_call) {
        // Call down the call chain:
        result = my_data->instance_dispatch_table->CreateMirSurfaceKHR(instance, pCreateInfo, pAllocator, pSurface);
        lock.lock();

        // Obtain this pointer again after locking:
        {
            auto it = my_data->instanceMap.find(instance);
            pInstance = (it == my_data->instanceMap.end()) ? NULL : &it->second;
        }
        if ((result == VK_SUCCESS) && pInstance && pSurface) {
            // Record the VkSurfaceKHR returned by the ICD:
            my_data->surfaceMap[*pSurface].surface = *pSurface;
            my_data->surfaceMap[*pSurface].pInstance = pInstance;
            my_data->surfaceMap[*pSurface].numQueueFamilyIndexSupport = 0;
            my_data->surfaceMap[*pSurface].pQueueFamilyIndexSupport = NULL;
            // Point to the associated SwpInstance:
            pInstance->surfaces[*pSurface] = &my_data->surfaceMap[*pSurface];
        }
        lock.unlock();

        return result;
    }
    return VK_ERROR_VALIDATION_FAILED_EXT;
}

VKAPI_ATTR VkBool32 VKAPI_CALL GetPhysicalDeviceMirPresentationSupportKHR(VkPhysicalDevice physicalDevice,
                                                                          uint32_t queueFamilyIndex, MirConnection *connection) {
    VkBool32 result = VK_FALSE;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    SwpPhysicalDevice *pPhysicalDevice = NULL;
    {
        auto it = my_data->physicalDeviceMap.find(physicalDevice);
        pPhysicalDevice = (it == my_data->physicalDeviceMap.end()) ? NULL : &it->second;
    }

    // Validate that the platform extension was enabled:
    if (pPhysicalDevice && pPhysicalDevice->pInstance && !pPhysicalDevice->pInstance->mirSurfaceExtensionEnabled) {
        skip_call |=
            log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
                    reinterpret_cast<uint64_t>(pPhysicalDevice->pInstance->instance), __LINE__, SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
                    swapchain_layer_name, "vkGetPhysicalDeviceMirPresentationSupportKHR() called even though the %s "
                                          "extension was not enabled for this VkInstance.",
                    VK_KHR_MIR_SURFACE_EXTENSION_NAME);
    }
    if (pPhysicalDevice->gotQueueFamilyPropertyCount) {
        skip_call |= ValidateQueueFamilyIndex(my_data, queueFamilyIndex, pPhysicalDevice->numOfQueueFamilies,
                                              pPhysicalDevice->physicalDevice, "vkGetPhysicalDeviceMirPresentationSupportKHR");
    }
    lock.unlock();

    if (!skip_call) {
        // Call down the call chain:
        result = my_data->instance_dispatch_table->GetPhysicalDeviceMirPresentationSupportKHR(physicalDevice, queueFamilyIndex,
                                                                                              connection);
    }
    return result;
}
#endif // VK_USE_PLATFORM_MIR_KHR

#ifdef VK_USE_PLATFORM_WAYLAND_KHR
VKAPI_ATTR VkResult VKAPI_CALL CreateWaylandSurfaceKHR(VkInstance instance, const VkWaylandSurfaceCreateInfoKHR *pCreateInfo,
                                                       const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
    VkResult result = VK_SUCCESS;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    SwpInstance *pInstance = NULL;
    {
        auto it = my_data->instanceMap.find(instance);
        pInstance = (it == my_data->instanceMap.end()) ? NULL : &it->second;
    }

    // Validate that the platform extension was enabled:
    if (pInstance && !pInstance->waylandSurfaceExtensionEnabled) {
        skip_call |=
            log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
                    reinterpret_cast<uint64_t>(instance), __LINE__, SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED, swapchain_layer_name,
                    "vkCreateWaylandSurfaceKHR() called even though the %s extension was not enabled for this VkInstance.",
                    VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
    }
    lock.unlock();

    if (!skip_call) {
        // Call down the call chain:
        result = my_data->instance_dispatch_table->CreateWaylandSurfaceKHR(instance, pCreateInfo, pAllocator, pSurface);
        lock.lock();

        // Obtain this pointer again after locking:
        {
            auto it = my_data->instanceMap.find(instance);
            pInstance = (it == my_data->instanceMap.end()) ? NULL : &it->second;
        }
        if ((result == VK_SUCCESS) && pInstance && pSurface) {
            // Record the VkSurfaceKHR returned by the ICD:
            my_data->surfaceMap[*pSurface].surface = *pSurface;
            my_data->surfaceMap[*pSurface].pInstance = pInstance;
            my_data->surfaceMap[*pSurface].numQueueFamilyIndexSupport = 0;
            my_data->surfaceMap[*pSurface].pQueueFamilyIndexSupport = NULL;
            // Point to the associated SwpInstance:
            pInstance->surfaces[*pSurface] = &my_data->surfaceMap[*pSurface];
        }
        lock.unlock();

        return result;
    }
    return VK_ERROR_VALIDATION_FAILED_EXT;
}

VKAPI_ATTR VkBool32 VKAPI_CALL GetPhysicalDeviceWaylandPresentationSupportKHR(VkPhysicalDevice physicalDevice,
                                                                              uint32_t queueFamilyIndex,
                                                                              struct wl_display *display) {
    VkBool32 result = VK_FALSE;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    SwpPhysicalDevice *pPhysicalDevice = NULL;
    {
        auto it = my_data->physicalDeviceMap.find(physicalDevice);
        pPhysicalDevice = (it == my_data->physicalDeviceMap.end()) ? NULL : &it->second;
    }

    // Validate that the platform extension was enabled:
    if (pPhysicalDevice && pPhysicalDevice->pInstance && !pPhysicalDevice->pInstance->waylandSurfaceExtensionEnabled) {
        skip_call |=
            log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
                    reinterpret_cast<uint64_t>(pPhysicalDevice->pInstance->instance), __LINE__, SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
                    swapchain_layer_name, "vkGetPhysicalDeviceWaylandPresentationSupportKHR() called even though the %s "
                                          "extension was not enabled for this VkInstance.",
                    VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
    }
    if (pPhysicalDevice->gotQueueFamilyPropertyCount) {
        skip_call |= ValidateQueueFamilyIndex(my_data, queueFamilyIndex, pPhysicalDevice->numOfQueueFamilies,
                                              pPhysicalDevice->physicalDevice, "vkGetPhysicalDeviceWaylandPresentationSupportKHR");
    }
    lock.unlock();

    if (!skip_call) {
        // Call down the call chain:
        result = my_data->instance_dispatch_table->GetPhysicalDeviceWaylandPresentationSupportKHR(physicalDevice, queueFamilyIndex,
                                                                                                  display);
    }
    return result;
}
#endif // VK_USE_PLATFORM_WAYLAND_KHR

#ifdef VK_USE_PLATFORM_WIN32_KHR
VKAPI_ATTR VkResult VKAPI_CALL CreateWin32SurfaceKHR(VkInstance instance, const VkWin32SurfaceCreateInfoKHR *pCreateInfo,
                                                     const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
    VkResult result = VK_SUCCESS;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    SwpInstance *pInstance = NULL;
    {
        auto it = my_data->instanceMap.find(instance);
        pInstance = (it == my_data->instanceMap.end()) ? NULL : &it->second;
    }

    // Validate that the platform extension was enabled:
    if (pInstance && !pInstance->win32SurfaceExtensionEnabled) {
        skip_call |=
            log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
                    reinterpret_cast<uint64_t>(instance), __LINE__, SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED, swapchain_layer_name,
                    "vkCreateWin32SurfaceKHR() called even though the %s extension was not enabled for this VkInstance.",
                    VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
    }
    lock.unlock();

    if (!skip_call) {
        // Call down the call chain:
        result = my_data->instance_dispatch_table->CreateWin32SurfaceKHR(instance, pCreateInfo, pAllocator, pSurface);
        lock.lock();

        // Obtain this pointer again after locking:
        {
            auto it = my_data->instanceMap.find(instance);
            pInstance = (it == my_data->instanceMap.end()) ? NULL : &it->second;
        }
        if ((result == VK_SUCCESS) && pInstance && pSurface) {
            // Record the VkSurfaceKHR returned by the ICD:
            my_data->surfaceMap[*pSurface].surface = *pSurface;
            my_data->surfaceMap[*pSurface].pInstance = pInstance;
            my_data->surfaceMap[*pSurface].numQueueFamilyIndexSupport = 0;
            my_data->surfaceMap[*pSurface].pQueueFamilyIndexSupport = NULL;
            // Point to the associated SwpInstance:
            pInstance->surfaces[*pSurface] = &my_data->surfaceMap[*pSurface];
        }
        lock.unlock();

        return result;
    }
    return VK_ERROR_VALIDATION_FAILED_EXT;
}

VKAPI_ATTR VkBool32 VKAPI_CALL GetPhysicalDeviceWin32PresentationSupportKHR(VkPhysicalDevice physicalDevice,
                                                                            uint32_t queueFamilyIndex) {
    VkBool32 result = VK_FALSE;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    SwpPhysicalDevice *pPhysicalDevice = NULL;
    {
        auto it = my_data->physicalDeviceMap.find(physicalDevice);
        pPhysicalDevice = (it == my_data->physicalDeviceMap.end()) ? NULL : &it->second;
    }

    // Validate that the platform extension was enabled:
    if (pPhysicalDevice && pPhysicalDevice->pInstance && !pPhysicalDevice->pInstance->win32SurfaceExtensionEnabled) {
        skip_call |=
            log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
                    reinterpret_cast<uint64_t>(pPhysicalDevice->pInstance->instance), __LINE__, SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
                    swapchain_layer_name, "vkGetPhysicalDeviceWin32PresentationSupportKHR() called even though the %s extension "
                                          "was not enabled for this VkInstance.",
                    VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
    }
    if (pPhysicalDevice->gotQueueFamilyPropertyCount) {
        skip_call |= ValidateQueueFamilyIndex(my_data, queueFamilyIndex, pPhysicalDevice->numOfQueueFamilies,
                                              pPhysicalDevice->physicalDevice, "vkGetPhysicalDeviceWin32PresentationSupportKHR");
    }
    lock.unlock();

    if (!skip_call) {
        // Call down the call chain:
        result = my_data->instance_dispatch_table->GetPhysicalDeviceWin32PresentationSupportKHR(physicalDevice, queueFamilyIndex);
    }
    return result;
}
#endif // VK_USE_PLATFORM_WIN32_KHR

#ifdef VK_USE_PLATFORM_XCB_KHR
VKAPI_ATTR VkResult VKAPI_CALL CreateXcbSurfaceKHR(VkInstance instance, const VkXcbSurfaceCreateInfoKHR *pCreateInfo,
                                                   const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
    VkResult result = VK_SUCCESS;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    SwpInstance *pInstance = NULL;
    {
        auto it = my_data->instanceMap.find(instance);
        pInstance = (it == my_data->instanceMap.end()) ? NULL : &it->second;
    }

    // Validate that the platform extension was enabled:
    if (pInstance && !pInstance->xcbSurfaceExtensionEnabled) {
        skip_call |=
            log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
                    reinterpret_cast<uint64_t>(instance), __LINE__, SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED, swapchain_layer_name,
                    "vkCreateXcbSurfaceKHR() called even though the %s extension was not enabled for this VkInstance.",
                    VK_KHR_XCB_SURFACE_EXTENSION_NAME);
    }
    lock.unlock();

    if (!skip_call) {
        // Call down the call chain:
        result = my_data->instance_dispatch_table->CreateXcbSurfaceKHR(instance, pCreateInfo, pAllocator, pSurface);
        lock.lock();

        // Obtain this pointer again after locking:
        {
            auto it = my_data->instanceMap.find(instance);
            pInstance = (it == my_data->instanceMap.end()) ? NULL : &it->second;
        }
        if ((result == VK_SUCCESS) && pInstance && pSurface) {
            // Record the VkSurfaceKHR returned by the ICD:
            my_data->surfaceMap[*pSurface].surface = *pSurface;
            my_data->surfaceMap[*pSurface].pInstance = pInstance;
            my_data->surfaceMap[*pSurface].numQueueFamilyIndexSupport = 0;
            my_data->surfaceMap[*pSurface].pQueueFamilyIndexSupport = NULL;
            // Point to the associated SwpInstance:
            pInstance->surfaces[*pSurface] = &my_data->surfaceMap[*pSurface];
        }
        lock.unlock();

        return result;
    }
    return VK_ERROR_VALIDATION_FAILED_EXT;
}

VKAPI_ATTR VkBool32 VKAPI_CALL GetPhysicalDeviceXcbPresentationSupportKHR(VkPhysicalDevice physicalDevice,
                                                                          uint32_t queueFamilyIndex, xcb_connection_t *connection,
                                                                          xcb_visualid_t visual_id) {
    VkBool32 result = VK_FALSE;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    SwpPhysicalDevice *pPhysicalDevice = NULL;
    {
        auto it = my_data->physicalDeviceMap.find(physicalDevice);
        pPhysicalDevice = (it == my_data->physicalDeviceMap.end()) ? NULL : &it->second;
    }

    // Validate that the platform extension was enabled:
    if (pPhysicalDevice && pPhysicalDevice->pInstance && !pPhysicalDevice->pInstance->xcbSurfaceExtensionEnabled) {
        skip_call |=
            log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
                    reinterpret_cast<uint64_t>(pPhysicalDevice->pInstance->instance), __LINE__, SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
                    swapchain_layer_name, "vkGetPhysicalDeviceXcbPresentationSupportKHR() called even though the %s "
                                          "extension was not enabled for this VkInstance.",
                    VK_KHR_XCB_SURFACE_EXTENSION_NAME);
    }
    if (pPhysicalDevice->gotQueueFamilyPropertyCount) {
        skip_call |= ValidateQueueFamilyIndex(my_data, queueFamilyIndex, pPhysicalDevice->numOfQueueFamilies,
                                              pPhysicalDevice->physicalDevice, "vkGetPhysicalDeviceXcbPresentationSupportKHR");
    }
    lock.unlock();

    if (!skip_call) {
        // Call down the call chain:
        result = my_data->instance_dispatch_table->GetPhysicalDeviceXcbPresentationSupportKHR(physicalDevice, queueFamilyIndex,
                                                                                              connection, visual_id);
    }
    return result;
}
#endif // VK_USE_PLATFORM_XCB_KHR

#ifdef VK_USE_PLATFORM_XLIB_KHR
VKAPI_ATTR VkResult VKAPI_CALL CreateXlibSurfaceKHR(VkInstance instance, const VkXlibSurfaceCreateInfoKHR *pCreateInfo,
                                                    const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
    VkResult result = VK_SUCCESS;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    SwpInstance *pInstance = NULL;
    {
        auto it = my_data->instanceMap.find(instance);
        pInstance = (it == my_data->instanceMap.end()) ? NULL : &it->second;
    }

    // Validate that the platform extension was enabled:
    if (pInstance && !pInstance->xlibSurfaceExtensionEnabled) {
        skip_call |=
            log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
                    reinterpret_cast<uint64_t>(instance), __LINE__, SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED, swapchain_layer_name,
                    "vkCreateXlibSurfaceKHR() called even though the %s extension was not enabled for this VkInstance.",
                    VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
    }
    lock.unlock();

    if (!skip_call) {
        // Call down the call chain:
        result = my_data->instance_dispatch_table->CreateXlibSurfaceKHR(instance, pCreateInfo, pAllocator, pSurface);
        lock.lock();

        // Obtain this pointer again after locking:
        {
            auto it = my_data->instanceMap.find(instance);
            pInstance = (it == my_data->instanceMap.end()) ? NULL : &it->second;
        }
        if ((result == VK_SUCCESS) && pInstance && pSurface) {
            // Record the VkSurfaceKHR returned by the ICD:
            my_data->surfaceMap[*pSurface].surface = *pSurface;
            my_data->surfaceMap[*pSurface].pInstance = pInstance;
            my_data->surfaceMap[*pSurface].numQueueFamilyIndexSupport = 0;
            my_data->surfaceMap[*pSurface].pQueueFamilyIndexSupport = NULL;
            // Point to the associated SwpInstance:
            pInstance->surfaces[*pSurface] = &my_data->surfaceMap[*pSurface];
        }
        lock.unlock();

        return result;
    }
    return VK_ERROR_VALIDATION_FAILED_EXT;
}

VKAPI_ATTR VkBool32 VKAPI_CALL GetPhysicalDeviceXlibPresentationSupportKHR(VkPhysicalDevice physicalDevice,
                                                                           uint32_t queueFamilyIndex, Display *dpy,
                                                                           VisualID visualID) {
    VkBool32 result = VK_FALSE;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    SwpPhysicalDevice *pPhysicalDevice = NULL;
    {
        auto it = my_data->physicalDeviceMap.find(physicalDevice);
        pPhysicalDevice = (it == my_data->physicalDeviceMap.end()) ? NULL : &it->second;
    }

    // Validate that the platform extension was enabled:
    if (pPhysicalDevice && pPhysicalDevice->pInstance && !pPhysicalDevice->pInstance->xlibSurfaceExtensionEnabled) {
        skip_call |=
            log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
                    reinterpret_cast<uint64_t>(pPhysicalDevice->pInstance->instance), __LINE__, SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
                    swapchain_layer_name, "vkGetPhysicalDeviceXlibPresentationSupportKHR() called even though the %s "
                                          "extension was not enabled for this VkInstance.",
                    VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
    }
    if (pPhysicalDevice->gotQueueFamilyPropertyCount) {
        skip_call |= ValidateQueueFamilyIndex(my_data, queueFamilyIndex, pPhysicalDevice->numOfQueueFamilies,
                                              pPhysicalDevice->physicalDevice, "vkGetPhysicalDeviceXlibPresentationSupportKHR");
    }
    lock.unlock();

    if (!skip_call) {
        // Call down the call chain:
        result = my_data->instance_dispatch_table->GetPhysicalDeviceXlibPresentationSupportKHR(physicalDevice, queueFamilyIndex,
                                                                                               dpy, visualID);
    }
    return result;
}
#endif // VK_USE_PLATFORM_XLIB_KHR

VKAPI_ATTR VkResult VKAPI_CALL GetPhysicalDeviceDisplayPropertiesKHR(VkPhysicalDevice physicalDevice, uint32_t *pPropertyCount,
                                                                     VkDisplayPropertiesKHR *pProperties) {
    VkResult result = VK_SUCCESS;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    SwpPhysicalDevice *pPhysicalDevice = NULL;
    {
        auto it = my_data->physicalDeviceMap.find(physicalDevice);
        pPhysicalDevice = (it == my_data->physicalDeviceMap.end()) ? NULL : &it->second;
    }

    if (pPhysicalDevice && pPhysicalDevice->pInstance && !pPhysicalDevice->pInstance->displayExtensionEnabled) {
        skip_call |=
            log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
                    reinterpret_cast<uint64_t>(pPhysicalDevice->pInstance->instance), __LINE__, SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
                    swapchain_layer_name, "vkGetPhysicalDeviceDisplayPropertiesKHR() called even though the %s "
                                          "extension was not enabled for this VkInstance.",
                    VK_KHR_DISPLAY_EXTENSION_NAME);
    }
    lock.unlock();

    if (!skip_call) {
        result =
            my_data->instance_dispatch_table->GetPhysicalDeviceDisplayPropertiesKHR(physicalDevice, pPropertyCount, pProperties);
        return result;
    }
    return VK_ERROR_VALIDATION_FAILED_EXT;
}

VKAPI_ATTR VkResult VKAPI_CALL GetPhysicalDeviceDisplayPlanePropertiesKHR(VkPhysicalDevice physicalDevice, uint32_t *pPropertyCount,
                                                                          VkDisplayPlanePropertiesKHR *pProperties) {
    VkResult result = VK_SUCCESS;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    SwpPhysicalDevice *pPhysicalDevice = NULL;
    {
        auto it = my_data->physicalDeviceMap.find(physicalDevice);
        pPhysicalDevice = (it == my_data->physicalDeviceMap.end()) ? NULL : &it->second;
    }

    if (pPhysicalDevice && pPhysicalDevice->pInstance && !pPhysicalDevice->pInstance->displayExtensionEnabled) {
        skip_call |=
            log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
                    reinterpret_cast<uint64_t>(pPhysicalDevice->pInstance->instance), __LINE__, SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
                    swapchain_layer_name, "vkGetPhysicalDeviceDisplayPlanePropertiesKHR() called even though the %s "
                                          "extension was not enabled for this VkInstance.",
                    VK_KHR_DISPLAY_EXTENSION_NAME);
    }
    lock.unlock();

    if (!skip_call) {
        result = my_data->instance_dispatch_table->GetPhysicalDeviceDisplayPlanePropertiesKHR(physicalDevice, pPropertyCount,
                                                                                              pProperties);

        lock.lock();
        if (!pPhysicalDevice->gotDisplayPlanePropertyCount) {
            pPhysicalDevice->displayPlanePropertyCount = *pPropertyCount;
            pPhysicalDevice->gotDisplayPlanePropertyCount = true;
        }
        // TODO store the properties for later checks
        lock.unlock();

        return result;
    }
    return VK_ERROR_VALIDATION_FAILED_EXT;
}

VKAPI_ATTR VkResult VKAPI_CALL GetDisplayPlaneSupportedDisplaysKHR(VkPhysicalDevice physicalDevice, uint32_t planeIndex,
                                                                   uint32_t *pDisplayCount, VkDisplayKHR *pDisplays) {
    VkResult result = VK_SUCCESS;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    SwpPhysicalDevice *pPhysicalDevice = NULL;
    {
        auto it = my_data->physicalDeviceMap.find(physicalDevice);
        pPhysicalDevice = (it == my_data->physicalDeviceMap.end()) ? NULL : &it->second;
    }

    if (pPhysicalDevice && pPhysicalDevice->pInstance && !pPhysicalDevice->pInstance->displayExtensionEnabled) {
        skip_call |=
            log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
                    reinterpret_cast<uint64_t>(pPhysicalDevice->pInstance->instance), __LINE__, SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
                    swapchain_layer_name, "vkGetDisplayPlaneSupportedDisplaysKHR() called even though the %s "
                                          "extension was not enabled for this VkInstance.",
                    VK_KHR_DISPLAY_EXTENSION_NAME);
    }

    if (!pPhysicalDevice->gotDisplayPlanePropertyCount) {
        skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
                             reinterpret_cast<uint64_t>(pPhysicalDevice->pInstance->instance), __LINE__,
                             SWAPCHAIN_GET_SUPPORTED_DISPLAYS_WITHOUT_QUERY, swapchain_layer_name,
                             "Potential problem with calling vkGetDisplayPlaneSupportedDisplaysKHR() without first "
                             "querying vkGetPhysicalDeviceDisplayPlanePropertiesKHR.");
    }

    if (pPhysicalDevice->gotDisplayPlanePropertyCount && planeIndex >= pPhysicalDevice->displayPlanePropertyCount) {
        skip_call |=
            log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
                    reinterpret_cast<uint64_t>(pPhysicalDevice->pInstance->instance), __LINE__, SWAPCHAIN_PLANE_INDEX_TOO_LARGE,
                    swapchain_layer_name,
                    "vkGetDisplayPlaneSupportedDisplaysKHR(): planeIndex must be in the range [0, %d] that was returned by "
                    "vkGetPhysicalDeviceDisplayPlanePropertiesKHR. Do you have the plane index hardcoded?",
                    pPhysicalDevice->displayPlanePropertyCount - 1);
    }
    lock.unlock();

    if (!skip_call) {
        result = my_data->instance_dispatch_table->GetDisplayPlaneSupportedDisplaysKHR(physicalDevice, planeIndex, pDisplayCount,
                                                                                       pDisplays);

        return result;
    }
    // TODO validate the returned display objects
    return VK_ERROR_VALIDATION_FAILED_EXT;
}

VKAPI_ATTR VkResult VKAPI_CALL GetDisplayModePropertiesKHR(VkPhysicalDevice physicalDevice, VkDisplayKHR display,
                                                           uint32_t *pPropertyCount, VkDisplayModePropertiesKHR *pProperties) {
    VkResult result = VK_SUCCESS;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    SwpPhysicalDevice *pPhysicalDevice = NULL;
    {
        auto it = my_data->physicalDeviceMap.find(physicalDevice);
        pPhysicalDevice = (it == my_data->physicalDeviceMap.end()) ? NULL : &it->second;
    }

    if (pPhysicalDevice && pPhysicalDevice->pInstance && !pPhysicalDevice->pInstance->displayExtensionEnabled) {
        skip_call |=
            log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
                    reinterpret_cast<uint64_t>(pPhysicalDevice->pInstance->instance), __LINE__, SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
                    swapchain_layer_name,
                    "vkGetDisplayModePropertiesKHR() called even though the %s extension was not enabled for this VkInstance.",
                    VK_KHR_DISPLAY_EXTENSION_NAME);
    }

    lock.unlock();

    if (!skip_call) {
        result =
            my_data->instance_dispatch_table->GetDisplayModePropertiesKHR(physicalDevice, display, pPropertyCount, pProperties);
        return result;
    }
    // TODO store the displayMode for later checking
    return VK_ERROR_VALIDATION_FAILED_EXT;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateDisplayModeKHR(VkPhysicalDevice physicalDevice, VkDisplayKHR display,
                                                    const VkDisplayModeCreateInfoKHR *pCreateInfo,
                                                    const VkAllocationCallbacks *pAllocator, VkDisplayModeKHR *pMode) {
    VkResult result = VK_SUCCESS;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    SwpPhysicalDevice *pPhysicalDevice = NULL;
    {
        auto it = my_data->physicalDeviceMap.find(physicalDevice);
        pPhysicalDevice = (it == my_data->physicalDeviceMap.end()) ? NULL : &it->second;
    }

    if (pPhysicalDevice && pPhysicalDevice->pInstance && !pPhysicalDevice->pInstance->displayExtensionEnabled) {
        skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
                             reinterpret_cast<uint64_t>(pPhysicalDevice->pInstance->instance), __LINE__,
                             SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED, swapchain_layer_name,
                             "vkCreateDisplayModeKHR() called even though the %s extension was not enabled for this VkInstance.",
                             VK_KHR_DISPLAY_EXTENSION_NAME);
    }

    lock.unlock();

    // TODO more validation checks needed
    if (!skip_call) {
        result = my_data->instance_dispatch_table->CreateDisplayModeKHR(physicalDevice, display, pCreateInfo, pAllocator, pMode);
        return result;
    }

    return VK_ERROR_VALIDATION_FAILED_EXT;
}

VKAPI_ATTR VkResult VKAPI_CALL GetDisplayPlaneCapabilitiesKHR(VkPhysicalDevice physicalDevice, VkDisplayModeKHR mode,
                                                              uint32_t planeIndex, VkDisplayPlaneCapabilitiesKHR *pCapabilities) {
    VkResult result = VK_SUCCESS;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    SwpPhysicalDevice *pPhysicalDevice = NULL;
    {
        auto it = my_data->physicalDeviceMap.find(physicalDevice);
        pPhysicalDevice = (it == my_data->physicalDeviceMap.end()) ? NULL : &it->second;
    }

    if (pPhysicalDevice && pPhysicalDevice->pInstance && !pPhysicalDevice->pInstance->displayExtensionEnabled) {
        skip_call |=
            log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
                    reinterpret_cast<uint64_t>(pPhysicalDevice->pInstance->instance), __LINE__, SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
                    swapchain_layer_name,
                    "vkGetDisplayPlaneCapabilitiesKHR) called even though the %s extension was not enabled for this VkInstance.",
                    VK_KHR_DISPLAY_EXTENSION_NAME);
    }

    if (!pPhysicalDevice->gotDisplayPlanePropertyCount) {
        skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
                             reinterpret_cast<uint64_t>(pPhysicalDevice->pInstance->instance), __LINE__,
                             SWAPCHAIN_GET_SUPPORTED_DISPLAYS_WITHOUT_QUERY, swapchain_layer_name,
                             "Potential problem with calling vkGetDisplayPlaneCapabilitiesKHR() without first "
                             "querying vkGetPhysicalDeviceDisplayPlanePropertiesKHR.");
    }

    if (pPhysicalDevice->gotDisplayPlanePropertyCount && planeIndex >= pPhysicalDevice->displayPlanePropertyCount) {
        skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
                             reinterpret_cast<uint64_t>(pPhysicalDevice->pInstance->instance), __LINE__,
                             SWAPCHAIN_PLANE_INDEX_TOO_LARGE, swapchain_layer_name,
                             "vkGetDisplayPlaneCapabilitiesKHR(): planeIndex must be in the range [0, %d] that was returned by "
                             "vkGetPhysicalDeviceDisplayPlanePropertiesKHR. Do you have the plane index hardcoded?",
                             pPhysicalDevice->displayPlanePropertyCount - 1);
    }

    lock.unlock();

    if (!skip_call) {
        result = my_data->instance_dispatch_table->GetDisplayPlaneCapabilitiesKHR(physicalDevice, mode, planeIndex, pCapabilities);
        return result;
    }

    return VK_ERROR_VALIDATION_FAILED_EXT;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateDisplayPlaneSurfaceKHR(VkInstance instance, const VkDisplaySurfaceCreateInfoKHR *pCreateInfo,
                                                            const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
    VkResult result = VK_SUCCESS;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    SwpInstance *pInstance = &(my_data->instanceMap[instance]);

    // Validate that the platform extension was enabled:
    if (pInstance && !pInstance->displayExtensionEnabled) {
        skip_call |=
            log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
                    reinterpret_cast<uint64_t>(instance), __LINE__, SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED, swapchain_layer_name,
                    "vkCreateDisplayPlaneSurfaceKHR() called even though the %s extension was not enabled for this VkInstance.",
                    VK_KHR_DISPLAY_EXTENSION_NAME);
    }

    // TODO more validation checks
    if (!skip_call) {
        // Call down the call chain:
        lock.unlock();
        result = my_data->instance_dispatch_table->CreateDisplayPlaneSurfaceKHR(instance, pCreateInfo, pAllocator, pSurface);
        lock.lock();

        // Obtain this pointer again after locking:
        pInstance = &(my_data->instanceMap[instance]);
        if ((result == VK_SUCCESS) && pInstance && pSurface) {
            // Record the VkSurfaceKHR returned by the ICD:
            my_data->surfaceMap[*pSurface].surface = *pSurface;
            my_data->surfaceMap[*pSurface].pInstance = pInstance;
            my_data->surfaceMap[*pSurface].numQueueFamilyIndexSupport = 0;
            my_data->surfaceMap[*pSurface].pQueueFamilyIndexSupport = NULL;
            // Point to the associated SwpInstance:
            pInstance->surfaces[*pSurface] = &my_data->surfaceMap[*pSurface];
        }
        lock.unlock();
        return result;
    }
    return VK_ERROR_VALIDATION_FAILED_EXT;
}

VKAPI_ATTR void VKAPI_CALL DestroySurfaceKHR(VkInstance instance, VkSurfaceKHR surface, const VkAllocationCallbacks *pAllocator) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    SwpSurface *pSurface = NULL;
    {
        auto it = my_data->surfaceMap.find(surface);
        pSurface = (it == my_data->surfaceMap.end()) ? NULL : &it->second;
    }
    SwpInstance *pInstance = NULL;
    {
        auto it = my_data->instanceMap.find(instance);
        pInstance = (it == my_data->instanceMap.end()) ? NULL : &it->second;
    }

    // Validate that the platform extension was enabled:
    if (pInstance && !pInstance->surfaceExtensionEnabled) {
        skip_call |=
            log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
                    reinterpret_cast<uint64_t>(instance), __LINE__, SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED, swapchain_layer_name,
                    "vkDestroySurfaceKHR() called even though the %s extension was not enabled for this VkInstance.",
                    VK_KHR_DISPLAY_EXTENSION_NAME);
    }

    // Regardless of skip_call value, do some internal cleanup:
    if (pSurface) {
        // Delete the SwpSurface associated with this surface:
        if (pSurface->pInstance) {
            pSurface->pInstance->surfaces.erase(surface);
        }
        if (!pSurface->swapchains.empty()) {
            skip_call |=
                log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
                        reinterpret_cast<uint64_t>(instance), __LINE__, SWAPCHAIN_DEL_OBJECT_BEFORE_CHILDREN, swapchain_layer_name,
                        "vkDestroySurfaceKHR() called before all of its associated VkSwapchainKHRs were destroyed.");

            // Empty and then delete all SwpSwapchains
            for (auto it = pSurface->swapchains.begin(); it != pSurface->swapchains.end(); it++) {
                // Delete all SwpImage's
                it->second->images.clear();
                // In case the swapchain's device hasn't been destroyed yet
                // (which isn't likely, but is possible), delete its
                // association with this swapchain (i.e. so we can't point to
                // this swpchain from that device, later on):
                if (it->second->pDevice) {
                    it->second->pDevice->swapchains.clear();
                }
            }
            pSurface->swapchains.clear();
        }
        my_data->surfaceMap.erase(surface);
    }
    lock.unlock();

    if (!skip_call) {
        // Call down the call chain:
        my_data->instance_dispatch_table->DestroySurfaceKHR(instance, surface, pAllocator);
    }
}

VKAPI_ATTR VkResult VKAPI_CALL EnumeratePhysicalDevices(VkInstance instance, uint32_t *pPhysicalDeviceCount,
                                                        VkPhysicalDevice *pPhysicalDevices) {
    VkResult result = VK_SUCCESS;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);

    // Call down the call chain:
    result = my_data->instance_dispatch_table->EnumeratePhysicalDevices(instance, pPhysicalDeviceCount, pPhysicalDevices);

    std::lock_guard<std::mutex> lock(global_lock);
    SwpInstance *pInstance = NULL;
    {
        auto it = my_data->instanceMap.find(instance);
        pInstance = (it == my_data->instanceMap.end()) ? NULL : &it->second;
    }
    if ((result == VK_SUCCESS) && pInstance && pPhysicalDevices && (*pPhysicalDeviceCount > 0)) {
        // Record the VkPhysicalDevices returned by the ICD:
        for (uint32_t i = 0; i < *pPhysicalDeviceCount; i++) {
            my_data->physicalDeviceMap[pPhysicalDevices[i]].physicalDevice = pPhysicalDevices[i];
            my_data->physicalDeviceMap[pPhysicalDevices[i]].pInstance = pInstance;
            my_data->physicalDeviceMap[pPhysicalDevices[i]].pDevice = NULL;
            my_data->physicalDeviceMap[pPhysicalDevices[i]].gotQueueFamilyPropertyCount = false;
            my_data->physicalDeviceMap[pPhysicalDevices[i]].gotSurfaceCapabilities = false;
            my_data->physicalDeviceMap[pPhysicalDevices[i]].surfaceFormatCount = 0;
            my_data->physicalDeviceMap[pPhysicalDevices[i]].pSurfaceFormats = NULL;
            my_data->physicalDeviceMap[pPhysicalDevices[i]].presentModeCount = 0;
            my_data->physicalDeviceMap[pPhysicalDevices[i]].pPresentModes = NULL;
            // Point to the associated SwpInstance:
            if (pInstance) {
                pInstance->physicalDevices[pPhysicalDevices[i]] = &my_data->physicalDeviceMap[pPhysicalDevices[i]];
            }
        }
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo *pCreateInfo,
                                            const VkAllocationCallbacks *pAllocator, VkDevice *pDevice) {
    layer_data *my_instance_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
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

    VkResult result = fpCreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);
    if (result != VK_SUCCESS) {
        return result;
    }

    std::lock_guard<std::mutex> lock(global_lock);
    layer_data *my_device_data = get_my_data_ptr(get_dispatch_key(*pDevice), layer_data_map);

    // Setup device dispatch table
    my_device_data->device_dispatch_table = new VkLayerDispatchTable;
    layer_init_device_dispatch_table(*pDevice, my_device_data->device_dispatch_table, fpGetDeviceProcAddr);

    my_device_data->report_data = layer_debug_report_create_device(my_instance_data->report_data, *pDevice);
    checkDeviceRegisterExtensions(physicalDevice, pCreateInfo, *pDevice);

    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyDevice(VkDevice device, const VkAllocationCallbacks *pAllocator) {
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);

    // Call down the call chain:
    my_data->device_dispatch_table->DestroyDevice(device, pAllocator);

    // Do some internal cleanup:
    std::lock_guard<std::mutex> lock(global_lock);
    SwpDevice *pDevice = NULL;
    {
        auto it = my_data->deviceMap.find(device);
        pDevice = (it == my_data->deviceMap.end()) ? NULL : &it->second;
    }
    if (pDevice) {
        // Delete the SwpDevice associated with this device:
        if (pDevice->pPhysicalDevice) {
            pDevice->pPhysicalDevice->pDevice = NULL;
        }
        if (!pDevice->swapchains.empty()) {
            log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                    reinterpret_cast<uint64_t>(device), __LINE__, SWAPCHAIN_DEL_OBJECT_BEFORE_CHILDREN, swapchain_layer_name,
                    "vkDestroyDevice() called before all of its associated VkSwapchainKHRs were destroyed.");

            // Empty and then delete all SwpSwapchain's
            for (auto it = pDevice->swapchains.begin(); it != pDevice->swapchains.end(); it++) {
                // Delete all SwpImage's
                it->second->images.clear();
                // In case the swapchain's surface hasn't been destroyed yet
                // (which is likely) delete its association with this swapchain
                // (i.e. so we can't point to this swpchain from that surface,
                // later on):
                if (it->second->pSurface) {
                    it->second->pSurface->swapchains.clear();
                }
            }
            pDevice->swapchains.clear();
        }
        my_data->deviceMap.erase(device);
    }
    delete my_data->device_dispatch_table;
    layer_data_map.erase(key);
}

VKAPI_ATTR VkResult VKAPI_CALL GetPhysicalDeviceSurfaceSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex,
                                                                  VkSurfaceKHR surface, VkBool32 *pSupported) {
    VkResult result = VK_SUCCESS;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    SwpPhysicalDevice *pPhysicalDevice = NULL;
    {
        auto it = my_data->physicalDeviceMap.find(physicalDevice);
        pPhysicalDevice = (it == my_data->physicalDeviceMap.end()) ? NULL : &it->second;
    }

    // Validate that the surface extension was enabled:
    if (pPhysicalDevice && pPhysicalDevice->pInstance && !pPhysicalDevice->pInstance->surfaceExtensionEnabled) {
        skip_call |= log_msg(
            my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
            reinterpret_cast<uint64_t>(pPhysicalDevice->pInstance->instance), __LINE__, SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
            swapchain_layer_name,
            "vkGetPhysicalDeviceSurfaceSupportKHR() called even though the %s extension was not enabled for this VkInstance.",
            VK_KHR_SURFACE_EXTENSION_NAME);
    }
    if (!pPhysicalDevice->gotQueueFamilyPropertyCount) {
        skip_call |= log_msg(
            my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT,
            reinterpret_cast<uint64_t>(pPhysicalDevice->physicalDevice), __LINE__, SWAPCHAIN_DID_NOT_QUERY_QUEUE_FAMILIES,
            swapchain_layer_name,
            "vkGetPhysicalDeviceSurfaceSupportKHR() called before calling the vkGetPhysicalDeviceQueueFamilyProperties function.");
    } else if (pPhysicalDevice->gotQueueFamilyPropertyCount) {
        skip_call |= ValidateQueueFamilyIndex(my_data, queueFamilyIndex, pPhysicalDevice->numOfQueueFamilies,
                                              pPhysicalDevice->physicalDevice, "vkGetPhysicalDeviceSurfaceSupportKHR");
    }

    lock.unlock();

    if (!skip_call) {
        // Call down the call chain:
        result = my_data->instance_dispatch_table->GetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIndex, surface,
                                                                                      pSupported);
        lock.lock();

        // Obtain this pointer again after locking:
        {
            auto it = my_data->physicalDeviceMap.find(physicalDevice);
            pPhysicalDevice = (it == my_data->physicalDeviceMap.end()) ? NULL : &it->second;
        }
        if ((result == VK_SUCCESS) && pSupported && pPhysicalDevice) {
            // Record the result of this query:
            SwpInstance *pInstance = pPhysicalDevice->pInstance;
            SwpSurface *pSurface = (pInstance) ? pInstance->surfaces[surface] : NULL;
            if (pSurface) {
                pPhysicalDevice->supportedSurfaces[surface] = pSurface;
                if (!pSurface->numQueueFamilyIndexSupport) {
                    if (pPhysicalDevice->gotQueueFamilyPropertyCount) {
                        pSurface->pQueueFamilyIndexSupport =
                            (VkBool32 *)malloc(pPhysicalDevice->numOfQueueFamilies * sizeof(VkBool32));
                        if (pSurface->pQueueFamilyIndexSupport != NULL) {
                            pSurface->numQueueFamilyIndexSupport = pPhysicalDevice->numOfQueueFamilies;
                        }
                    }
                }
                if (pSurface->numQueueFamilyIndexSupport) {
                    pSurface->pQueueFamilyIndexSupport[queueFamilyIndex] = *pSupported;
                }
            }
        }
        lock.unlock();

        return result;
    }
    return VK_ERROR_VALIDATION_FAILED_EXT;
}

VKAPI_ATTR VkResult VKAPI_CALL GetPhysicalDeviceSurfaceCapabilitiesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
                                                                       VkSurfaceCapabilitiesKHR *pSurfaceCapabilities) {
    VkResult result = VK_SUCCESS;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    SwpPhysicalDevice *pPhysicalDevice = NULL;
    {
        auto it = my_data->physicalDeviceMap.find(physicalDevice);
        pPhysicalDevice = (it == my_data->physicalDeviceMap.end()) ? NULL : &it->second;
    }

    // Validate that the surface extension was enabled:
    if (pPhysicalDevice && pPhysicalDevice->pInstance && !pPhysicalDevice->pInstance->surfaceExtensionEnabled) {
        skip_call |= log_msg(
            my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
            reinterpret_cast<uint64_t>(pPhysicalDevice->pInstance->instance), __LINE__, SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
            swapchain_layer_name,
            "vkGetPhysicalDeviceSurfaceCapabilitiesKHR() called even though the %s extension was not enabled for this VkInstance.",
            VK_KHR_DISPLAY_EXTENSION_NAME);
    }
    lock.unlock();

    if (!skip_call) {
        // Call down the call chain:
        result = my_data->instance_dispatch_table->GetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface,
                                                                                           pSurfaceCapabilities);
        lock.lock();

        // Obtain this pointer again after locking:
        {
            auto it = my_data->physicalDeviceMap.find(physicalDevice);
            pPhysicalDevice = (it == my_data->physicalDeviceMap.end()) ? NULL : &it->second;
        }
        if ((result == VK_SUCCESS) && pPhysicalDevice) {
            // Record the result of this query:
            pPhysicalDevice->gotSurfaceCapabilities = true;
            pPhysicalDevice->surfaceCapabilities = *pSurfaceCapabilities;
        }
        lock.unlock();

        return result;
    }
    return VK_ERROR_VALIDATION_FAILED_EXT;
}

VKAPI_ATTR VkResult VKAPI_CALL GetPhysicalDeviceSurfaceFormatsKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
                                                                  uint32_t *pSurfaceFormatCount,
                                                                  VkSurfaceFormatKHR *pSurfaceFormats) {
    VkResult result = VK_SUCCESS;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    SwpPhysicalDevice *pPhysicalDevice = NULL;
    {
        auto it = my_data->physicalDeviceMap.find(physicalDevice);
        pPhysicalDevice = (it == my_data->physicalDeviceMap.end()) ? NULL : &it->second;
    }

    // Validate that the surface extension was enabled:
    if (pPhysicalDevice && pPhysicalDevice->pInstance && !pPhysicalDevice->pInstance->surfaceExtensionEnabled) {
        skip_call |= log_msg(
            my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
            reinterpret_cast<uint64_t>(pPhysicalDevice->pInstance->instance), __LINE__, SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
            swapchain_layer_name,
            "vkGetPhysicalDeviceSurfaceFormatsKHR() called even though the %s extension was not enabled for this VkInstance.",
            VK_KHR_DISPLAY_EXTENSION_NAME);
    }
    if (pPhysicalDevice && pSurfaceFormats) {
        // Compare the preliminary value of *pSurfaceFormatCount with the value this time:
        if (pPhysicalDevice->surfaceFormatCount == 0) {
            // Since we haven't recorded a preliminary value of *pSurfaceFormatCount, that likely means that the application didn't
            // previously call this function with a NULL value of pSurfaceFormats:
            skip_call |= log_msg(
                my_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT,
                reinterpret_cast<uint64_t>(pPhysicalDevice->physicalDevice), __LINE__, SWAPCHAIN_PRIOR_COUNT, swapchain_layer_name,
                "vkGetPhysicalDeviceSurfaceFormatsKHR() called with non-NULL pSurfaceFormatCount; but no prior positive "
                "value has been seen for pSurfaceFormats.");
        } else if (*pSurfaceFormatCount > pPhysicalDevice->surfaceFormatCount) {
            skip_call |= log_msg(
                my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT,
                reinterpret_cast<uint64_t>(pPhysicalDevice->physicalDevice), __LINE__, SWAPCHAIN_INVALID_COUNT,
                swapchain_layer_name,
                "vkGetPhysicalDeviceSurfaceFormatsKHR() called with non-NULL pSurfaceFormatCount, and with pSurfaceFormats set to "
                "a value (%d) that is greater than the value (%d) that was returned when pSurfaceFormatCount was NULL.",
                *pSurfaceFormatCount, pPhysicalDevice->surfaceFormatCount);
        }
    }
    lock.unlock();

    if (!skip_call) {
        // Call down the call chain:
        result = my_data->instance_dispatch_table->GetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, pSurfaceFormatCount,
                                                                                      pSurfaceFormats);
        lock.lock();

        // Obtain this pointer again after locking:
        {
            auto it = my_data->physicalDeviceMap.find(physicalDevice);
            pPhysicalDevice = (it == my_data->physicalDeviceMap.end()) ? NULL : &it->second;
        }
        if ((result == VK_SUCCESS) && pPhysicalDevice && !pSurfaceFormats && pSurfaceFormatCount) {
            // Record the result of this preliminary query:
            pPhysicalDevice->surfaceFormatCount = *pSurfaceFormatCount;
        } else if (((result == VK_SUCCESS) || (result == VK_INCOMPLETE)) && pPhysicalDevice && pSurfaceFormats &&
                   pSurfaceFormatCount && (*pSurfaceFormatCount > 0)) {
            // Record the result of this query:

            // Note: for poorly-written applications (e.g. that don't call this command
            // twice, the first time with pSurfaceFormats set to NULL, and the second time
            // with a non-NULL pSurfaceFormats and with the same count as returned the
            // first time), record again the count when pSurfaceFormats is non-NULL:
            pPhysicalDevice->surfaceFormatCount = *pSurfaceFormatCount;
            pPhysicalDevice->pSurfaceFormats = (VkSurfaceFormatKHR *)malloc(*pSurfaceFormatCount * sizeof(VkSurfaceFormatKHR));
            if (pPhysicalDevice->pSurfaceFormats) {
                for (uint32_t i = 0; i < *pSurfaceFormatCount; i++) {
                    pPhysicalDevice->pSurfaceFormats[i] = pSurfaceFormats[i];
                }
            } else {
                pPhysicalDevice->surfaceFormatCount = 0;
            }
        }
        lock.unlock();

        return result;
    }
    return VK_ERROR_VALIDATION_FAILED_EXT;
}

VKAPI_ATTR VkResult VKAPI_CALL GetPhysicalDeviceSurfacePresentModesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
                                                                       uint32_t *pPresentModeCount,
                                                                       VkPresentModeKHR *pPresentModes) {
    VkResult result = VK_SUCCESS;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    SwpPhysicalDevice *pPhysicalDevice = NULL;
    {
        auto it = my_data->physicalDeviceMap.find(physicalDevice);
        pPhysicalDevice = (it == my_data->physicalDeviceMap.end()) ? NULL : &it->second;
    }

    // Validate that the surface extension was enabled:
    if (pPhysicalDevice && pPhysicalDevice->pInstance && !pPhysicalDevice->pInstance->surfaceExtensionEnabled) {
        skip_call |= log_msg(
            my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
            reinterpret_cast<uint64_t>(pPhysicalDevice->pInstance->instance), __LINE__, SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
            swapchain_layer_name,
            "vkGetPhysicalDeviceSurfacePresentModesKHR() called even though the %s extension was not enabled for this VkInstance.",
            VK_KHR_DISPLAY_EXTENSION_NAME);
    }
    if (pPhysicalDevice && pPresentModes) {
        // Compare the preliminary value of *pPresentModeCount with the value this time:
        if (pPhysicalDevice->presentModeCount == 0) {
            // Since we haven't recorded a preliminary value of *pPresentModeCount, that likely means that the application didn't
            // previously call this function with a NULL value of pPresentModes:
            skip_call |= log_msg(
                my_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT,
                reinterpret_cast<uint64_t>(pPhysicalDevice->physicalDevice), __LINE__, SWAPCHAIN_PRIOR_COUNT, swapchain_layer_name,
                "vkGetPhysicalDeviceSurfacePresentModesKHR() called with non-NULL pPresentModeCount; but no prior positive "
                "value has been seen for pPresentModes.");
        } else if (*pPresentModeCount > pPhysicalDevice->presentModeCount) {
            skip_call |= log_msg(
                my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT,
                reinterpret_cast<uint64_t>(pPhysicalDevice->physicalDevice), __LINE__, SWAPCHAIN_INVALID_COUNT,
                swapchain_layer_name,
                "vkGetPhysicalDeviceSurfacePresentModesKHR() called with non-NULL pPresentModeCount, and with pPresentModes set to "
                "a value (%d) that is greater than the value (%d) that was returned when pPresentModeCount was NULL.",
                *pPresentModeCount, pPhysicalDevice->presentModeCount);
        }
    }
    lock.unlock();

    if (!skip_call) {
        // Call down the call chain:
        result = my_data->instance_dispatch_table->GetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface,
                                                                                           pPresentModeCount, pPresentModes);
        lock.lock();

        // Obtain this pointer again after locking:
        {
            auto it = my_data->physicalDeviceMap.find(physicalDevice);
            pPhysicalDevice = (it == my_data->physicalDeviceMap.end()) ? NULL : &it->second;
        }
        if ((result == VK_SUCCESS) && pPhysicalDevice && !pPresentModes && pPresentModeCount) {
            // Record the result of this preliminary query:
            pPhysicalDevice->presentModeCount = *pPresentModeCount;
        } else if (((result == VK_SUCCESS) || (result == VK_INCOMPLETE)) && pPhysicalDevice &&
                   pPresentModes && pPresentModeCount && (*pPresentModeCount > 0)) {
            // Record the result of this query:

            // Note: for poorly-written applications (e.g. that don't call this command
            // twice, the first time with pPresentModes set to NULL, and the second time
            // with a non-NULL pPresentModes and with the same count as returned the
            // first time), record again the count when pPresentModes is non-NULL:
            pPhysicalDevice->presentModeCount = *pPresentModeCount;
            pPhysicalDevice->pPresentModes = (VkPresentModeKHR *)malloc(*pPresentModeCount * sizeof(VkPresentModeKHR));
            if (pPhysicalDevice->pPresentModes) {
                for (uint32_t i = 0; i < *pPresentModeCount; i++) {
                    pPhysicalDevice->pPresentModes[i] = pPresentModes[i];
                }
            } else {
                pPhysicalDevice->presentModeCount = 0;
            }
        }
        lock.unlock();

        return result;
    }
    return VK_ERROR_VALIDATION_FAILED_EXT;
}

// This function does the up-front validation work for vkCreateSwapchainKHR(),
// and returns true if a logging callback indicates that the call down the
// chain should be skipped:
static bool validateCreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR *pCreateInfo, VkSwapchainKHR *pSwapchain) {
    // TODO: Validate cases of re-creating a swapchain (the current code
    // assumes a new swapchain is being created).
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    char fn[] = "vkCreateSwapchainKHR";
    SwpDevice *pDevice = NULL;
    {
        auto it = my_data->deviceMap.find(device);
        pDevice = (it == my_data->deviceMap.end()) ? NULL : &it->second;
    }

    // Validate that the swapchain extension was enabled:
    if (pDevice && !pDevice->swapchainExtensionEnabled) {
        return log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                       reinterpret_cast<uint64_t>(device), __LINE__, SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED, swapchain_layer_name,
                       "vkCreateSwapchainKHR() called even though the %s extension was not enabled for this VkDevice.",
                       VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    }

    // Keep around a useful pointer to pPhysicalDevice:
    SwpPhysicalDevice *pPhysicalDevice = pDevice->pPhysicalDevice;

    // Validate pCreateInfo values with result of
    // vkGetPhysicalDeviceQueueFamilyProperties
    if (pPhysicalDevice && pPhysicalDevice->gotQueueFamilyPropertyCount) {
        for (uint32_t i = 0; i < pCreateInfo->queueFamilyIndexCount; i++) {
            skip_call |= ValidateQueueFamilyIndex(my_data, pCreateInfo->pQueueFamilyIndices[i], pPhysicalDevice->numOfQueueFamilies,
                                                  pPhysicalDevice->physicalDevice, "vkCreateSwapchainKHR");
        }
    }

    // Validate pCreateInfo values with the results of
    // vkGetPhysicalDeviceSurfaceCapabilitiesKHR():
    if (!pPhysicalDevice || !pPhysicalDevice->gotSurfaceCapabilities) {
        skip_call |=
            log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                    reinterpret_cast<uint64_t>(device), __LINE__, SWAPCHAIN_CREATE_SWAP_WITHOUT_QUERY, swapchain_layer_name,
                    "vkCreateSwapchainKHR() called before calling vkGetPhysicalDeviceSurfaceCapabilitiesKHR().");
    } else if (pCreateInfo) {
        // Validate pCreateInfo->surface to make sure that
        // vkGetPhysicalDeviceSurfaceSupportKHR() reported this as a supported
        // surface:
        SwpSurface *pSurface = ((pPhysicalDevice) ? pPhysicalDevice->supportedSurfaces[pCreateInfo->surface] : NULL);
        if (!pSurface) {
            skip_call |=
                log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                        reinterpret_cast<uint64_t>(device), __LINE__, SWAPCHAIN_CREATE_UNSUPPORTED_SURFACE, swapchain_layer_name,
                        "The surface in pCreateInfo->surface, that was given to vkCreateSwapchainKHR(), must be a surface "
                        "that is supported by the device as determined by vkGetPhysicalDeviceSurfaceSupportKHR().  "
                        "However, vkGetPhysicalDeviceSurfaceSupportKHR() was never called with this surface.");
        }

        // Validate pCreateInfo->minImageCount against
        // VkSurfaceCapabilitiesKHR::{min|max}ImageCount:
        VkSurfaceCapabilitiesKHR *pCapabilities = &pPhysicalDevice->surfaceCapabilities;
        if ((pCreateInfo->minImageCount < pCapabilities->minImageCount) ||
            ((pCapabilities->maxImageCount > 0) && (pCreateInfo->minImageCount > pCapabilities->maxImageCount))) {
            skip_call |=
                log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                        reinterpret_cast<uint64_t>(device), __LINE__, SWAPCHAIN_CREATE_SWAP_BAD_MIN_IMG_COUNT, swapchain_layer_name,
                        "vkCreateSwapchainKHR() called with pCreateInfo->minImageCount = %d, which is outside the bounds returned "
                        "by vkGetPhysicalDeviceSurfaceCapabilitiesKHR() (i.e. minImageCount = %d, maxImageCount = %d).",
                        pCreateInfo->minImageCount, pCapabilities->minImageCount, pCapabilities->maxImageCount);
        }
        // Validate pCreateInfo->imageExtent against
        // VkSurfaceCapabilitiesKHR::{current|min|max}ImageExtent:
        if ((pCapabilities->currentExtent.width == -1) &&
            ((pCreateInfo->imageExtent.width < pCapabilities->minImageExtent.width) ||
             (pCreateInfo->imageExtent.width > pCapabilities->maxImageExtent.width) ||
             (pCreateInfo->imageExtent.height < pCapabilities->minImageExtent.height) ||
             (pCreateInfo->imageExtent.height > pCapabilities->maxImageExtent.height))) {
            skip_call |= log_msg(
                my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                reinterpret_cast<uint64_t>(device), __LINE__, SWAPCHAIN_CREATE_SWAP_OUT_OF_BOUNDS_EXTENTS, swapchain_layer_name,
                "vkCreateSwapchainKHR() called with pCreateInfo->imageExtent = (%d,%d), which is outside the "
                "bounds returned by vkGetPhysicalDeviceSurfaceCapabilitiesKHR(): currentExtent = (%d,%d), "
                "minImageExtent = (%d,%d), maxImageExtent = (%d,%d).",
                pCreateInfo->imageExtent.width, pCreateInfo->imageExtent.height, pCapabilities->currentExtent.width,
                pCapabilities->currentExtent.height, pCapabilities->minImageExtent.width, pCapabilities->minImageExtent.height,
                pCapabilities->maxImageExtent.width, pCapabilities->maxImageExtent.height);
        }
        if ((pCapabilities->currentExtent.width != -1) &&
            ((pCreateInfo->imageExtent.width != pCapabilities->currentExtent.width) ||
             (pCreateInfo->imageExtent.height != pCapabilities->currentExtent.height))) {
            skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                 reinterpret_cast<uint64_t>(device), __LINE__, SWAPCHAIN_CREATE_SWAP_EXTENTS_NO_MATCH_WIN,
                                 swapchain_layer_name,
                                 "vkCreateSwapchainKHR() called with pCreateInfo->imageExtent = (%d,%d), which is not equal to the "
                                 "currentExtent = (%d,%d) returned by vkGetPhysicalDeviceSurfaceCapabilitiesKHR().",
                                 pCreateInfo->imageExtent.width, pCreateInfo->imageExtent.height,
                                 pCapabilities->currentExtent.width, pCapabilities->currentExtent.height);
        }
        // Validate pCreateInfo->preTransform has one bit set (1st two
        // lines of if-statement), which bit is also set in
        // VkSurfaceCapabilitiesKHR::supportedTransforms (3rd line of if-statement):
        if (!pCreateInfo->preTransform || (pCreateInfo->preTransform & (pCreateInfo->preTransform - 1)) ||
            !(pCreateInfo->preTransform & pCapabilities->supportedTransforms)) {
            // This is an error situation; one for which we'd like to give
            // the developer a helpful, multi-line error message.  Build it
            // up a little at a time, and then log it:
            std::string errorString = "";
            char str[1024];
            // Here's the first part of the message:
            sprintf(str, "%s() called with a non-supported "
                         "pCreateInfo->preTransform (i.e. %s).  "
                         "Supported values are:\n",
                    fn, surfaceTransformStr(pCreateInfo->preTransform));
            errorString += str;
            for (int i = 0; i < 32; i++) {
                // Build up the rest of the message:
                if ((1 << i) & pCapabilities->supportedTransforms) {
                    const char *newStr = surfaceTransformStr((VkSurfaceTransformFlagBitsKHR)(1 << i));
                    sprintf(str, "    %s\n", newStr);
                    errorString += str;
                }
            }
            // Log the message that we've built up:
            skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                 reinterpret_cast<uint64_t &>(device), __LINE__, SWAPCHAIN_CREATE_SWAP_BAD_PRE_TRANSFORM,
                                 LAYER_NAME, "%s", errorString.c_str());
        }
        // Validate pCreateInfo->compositeAlpha has one bit set (1st two
        // lines of if-statement), which bit is also set in
        // VkSurfaceCapabilitiesKHR::supportedCompositeAlpha (3rd line of if-statement):
        if (!pCreateInfo->compositeAlpha || (pCreateInfo->compositeAlpha & (pCreateInfo->compositeAlpha - 1)) ||
            !((pCreateInfo->compositeAlpha) & pCapabilities->supportedCompositeAlpha)) {
            // This is an error situation; one for which we'd like to give
            // the developer a helpful, multi-line error message.  Build it
            // up a little at a time, and then log it:
            std::string errorString = "";
            char str[1024];
            // Here's the first part of the message:
            sprintf(str, "%s() called with a non-supported "
                         "pCreateInfo->compositeAlpha (i.e. %s).  "
                         "Supported values are:\n",
                    fn, surfaceCompositeAlphaStr(pCreateInfo->compositeAlpha));
            errorString += str;
            for (int i = 0; i < 32; i++) {
                // Build up the rest of the message:
                if ((1 << i) & pCapabilities->supportedCompositeAlpha) {
                    const char *newStr = surfaceCompositeAlphaStr((VkCompositeAlphaFlagBitsKHR)(1 << i));
                    sprintf(str, "    %s\n", newStr);
                    errorString += str;
                }
            }
            // Log the message that we've built up:
            skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                 reinterpret_cast<uint64_t &>(device), __LINE__, SWAPCHAIN_CREATE_SWAP_BAD_COMPOSITE_ALPHA,
                                 LAYER_NAME, "%s", errorString.c_str());
        }
        // Validate pCreateInfo->imageArrayLayers against
        // VkSurfaceCapabilitiesKHR::maxImageArrayLayers:
        if ((pCreateInfo->imageArrayLayers < 1) || (pCreateInfo->imageArrayLayers > pCapabilities->maxImageArrayLayers)) {
            skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                 reinterpret_cast<uint64_t>(device), __LINE__, SWAPCHAIN_CREATE_SWAP_BAD_IMG_ARRAY_LAYERS,
                                 swapchain_layer_name,
                                 "vkCreateSwapchainKHR() called with a non-supported pCreateInfo->imageArrayLayers (i.e. %d).  "
                                 "Minimum value is 1, maximum value is %d.",
                                 pCreateInfo->imageArrayLayers, pCapabilities->maxImageArrayLayers);
        }
        // Validate pCreateInfo->imageUsage against
        // VkSurfaceCapabilitiesKHR::supportedUsageFlags:
        if (pCreateInfo->imageUsage != (pCreateInfo->imageUsage & pCapabilities->supportedUsageFlags)) {
            skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                 reinterpret_cast<uint64_t>(device), __LINE__, SWAPCHAIN_CREATE_SWAP_BAD_IMG_USAGE_FLAGS,
                                 swapchain_layer_name,
                                 "vkCreateSwapchainKHR() called with a non-supported pCreateInfo->imageUsage (i.e. 0x%08x).  "
                                 "Supported flag bits are 0x%08x.",
                                 pCreateInfo->imageUsage, pCapabilities->supportedUsageFlags);
        }
    }

    // Validate pCreateInfo values with the results of
    // vkGetPhysicalDeviceSurfaceFormatsKHR():
    if (!pPhysicalDevice || !pPhysicalDevice->surfaceFormatCount) {
        skip_call |=
            log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                    reinterpret_cast<uint64_t>(device), __LINE__, SWAPCHAIN_CREATE_SWAP_WITHOUT_QUERY, swapchain_layer_name,
                    "vkCreateSwapchainKHR() called before calling vkGetPhysicalDeviceSurfaceFormatsKHR().");
    } else if (pCreateInfo) {
        // Validate pCreateInfo->imageFormat against
        // VkSurfaceFormatKHR::format:
        bool foundFormat = false;
        bool foundColorSpace = false;
        bool foundMatch = false;
        for (uint32_t i = 0; i < pPhysicalDevice->surfaceFormatCount; i++) {
            if (pCreateInfo->imageFormat == pPhysicalDevice->pSurfaceFormats[i].format) {
                // Validate pCreateInfo->imageColorSpace against
                // VkSurfaceFormatKHR::colorSpace:
                foundFormat = true;
                if (pCreateInfo->imageColorSpace == pPhysicalDevice->pSurfaceFormats[i].colorSpace) {
                    foundMatch = true;
                    break;
                }
            } else {
                if (pCreateInfo->imageColorSpace == pPhysicalDevice->pSurfaceFormats[i].colorSpace) {
                    foundColorSpace = true;
                }
            }
        }
        if (!foundMatch) {
            if (!foundFormat) {
                if (!foundColorSpace) {
                    skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, reinterpret_cast<uint64_t>(device), __LINE__,
                                         SWAPCHAIN_CREATE_SWAP_BAD_IMG_FMT_CLR_SP, swapchain_layer_name,
                                         "vkCreateSwapchainKHR() called with neither a supported pCreateInfo->imageFormat "
                                         "(i.e. %d) nor a supported "
                                         "pCreateInfo->imageColorSpace (i.e. %d).",
                                         pCreateInfo->imageFormat, pCreateInfo->imageColorSpace);
                } else {
                    skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, reinterpret_cast<uint64_t>(device), __LINE__,
                                         SWAPCHAIN_CREATE_SWAP_BAD_IMG_FORMAT, swapchain_layer_name,
                                         "vkCreateSwapchainKHR() called with a non-supported pCreateInfo->imageFormat (i.e. %d)",
                                         pCreateInfo->imageFormat);
                }
            } else if (!foundColorSpace) {
                skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                     reinterpret_cast<uint64_t>(device), __LINE__, SWAPCHAIN_CREATE_SWAP_BAD_IMG_COLOR_SPACE,
                                     swapchain_layer_name,
                                     "vkCreateSwapchainKHR() called with a non-supported pCreateInfo->imageColorSpace (i.e. %d).",
                                     pCreateInfo->imageColorSpace);
            }
        }
    }

    // Validate pCreateInfo values with the results of
    // vkGetPhysicalDeviceSurfacePresentModesKHR():
    if (!pPhysicalDevice || !pPhysicalDevice->presentModeCount) {
        if (!pCreateInfo || (pCreateInfo->presentMode != VK_PRESENT_MODE_FIFO_KHR)) {
            skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                 reinterpret_cast<uint64_t>(device), __LINE__, SWAPCHAIN_CREATE_SWAP_WITHOUT_QUERY,
                                 swapchain_layer_name, "vkCreateSwapchainKHR() called before calling "
                                                       "vkGetPhysicalDeviceSurfacePresentModesKHR().");
        }
    } else if (pCreateInfo) {
        // Validate pCreateInfo->presentMode against
        // vkGetPhysicalDeviceSurfacePresentModesKHR():
        bool foundMatch = false;
        for (uint32_t i = 0; i < pPhysicalDevice->presentModeCount; i++) {
            if (pPhysicalDevice->pPresentModes[i] == pCreateInfo->presentMode) {
                foundMatch = true;
                break;
            }
        }
        if (!foundMatch) {
            skip_call |=
                log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                        reinterpret_cast<uint64_t>(device), __LINE__, SWAPCHAIN_CREATE_SWAP_BAD_PRESENT_MODE, swapchain_layer_name,
                        "vkCreateSwapchainKHR() called with a non-supported pCreateInfo->presentMode (i.e. %s).",
                        presentModeStr(pCreateInfo->presentMode));
        }
    }

    // Validate pCreateInfo->imageSharingMode and related values:
    if (pCreateInfo->imageSharingMode == VK_SHARING_MODE_CONCURRENT) {
        if ((pCreateInfo->queueFamilyIndexCount <= 1) || !pCreateInfo->pQueueFamilyIndices) {
            skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                 reinterpret_cast<uint64_t>(device), __LINE__, SWAPCHAIN_CREATE_SWAP_BAD_SHARING_VALUES,
                                 swapchain_layer_name,
                                 "vkCreateSwapchainKHR() called with a supported pCreateInfo->sharingMode of (i.e. %s), but with a "
                                 "bad value(s) for pCreateInfo->queueFamilyIndexCount or pCreateInfo->pQueueFamilyIndices).",
                                 sharingModeStr(pCreateInfo->imageSharingMode));
        }
    } else if (pCreateInfo->imageSharingMode != VK_SHARING_MODE_EXCLUSIVE) {
        skip_call |=
            log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                    reinterpret_cast<uint64_t>(device), __LINE__, SWAPCHAIN_CREATE_SWAP_BAD_SHARING_MODE, swapchain_layer_name,
                    "vkCreateSwapchainKHR() called with a non-supported pCreateInfo->imageSharingMode (i.e. %s).",
                    sharingModeStr(pCreateInfo->imageSharingMode));
    }

    // Validate pCreateInfo->clipped:
    if (pCreateInfo && (pCreateInfo->clipped != VK_FALSE) && (pCreateInfo->clipped != VK_TRUE)) {
        skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                             reinterpret_cast<uint64_t>(device), __LINE__, SWAPCHAIN_BAD_BOOL, swapchain_layer_name,
                             "vkCreateSwapchainKHR() called with a VkBool32 value that is neither VK_TRUE nor VK_FALSE, but "
                             "has the numeric value of %d.",
                             pCreateInfo->clipped);
    }

    return skip_call;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR *pCreateInfo,
                                                  const VkAllocationCallbacks *pAllocator, VkSwapchainKHR *pSwapchain) {
    VkResult result = VK_SUCCESS;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip_call = validateCreateSwapchainKHR(device, pCreateInfo, pSwapchain);
    lock.unlock();

    if (!skip_call) {
        // Call down the call chain:
        result = my_data->device_dispatch_table->CreateSwapchainKHR(device, pCreateInfo, pAllocator, pSwapchain);
        lock.lock();

        if (result == VK_SUCCESS) {
            // Remember the swapchain's handle, and link it to the device:
            SwpDevice *pDevice = NULL;
            {
                auto it = my_data->deviceMap.find(device);
                pDevice = (it == my_data->deviceMap.end()) ? NULL : &it->second;
            }

            my_data->swapchainMap[*pSwapchain].swapchain = *pSwapchain;
            if (pDevice) {
                pDevice->swapchains[*pSwapchain] = &my_data->swapchainMap[*pSwapchain];
            }
            my_data->swapchainMap[*pSwapchain].pDevice = pDevice;
            my_data->swapchainMap[*pSwapchain].imageCount = 0;
            // Store a pointer to the surface
            SwpPhysicalDevice *pPhysicalDevice = pDevice->pPhysicalDevice;
            SwpInstance *pInstance = (pPhysicalDevice) ? pPhysicalDevice->pInstance : NULL;
            layer_data *my_instance_data =
                ((pInstance) ? get_my_data_ptr(get_dispatch_key(pInstance->instance), layer_data_map) : NULL);
            SwpSurface *pSurface = ((my_data && pCreateInfo) ? &my_instance_data->surfaceMap[pCreateInfo->surface] : NULL);
            my_data->swapchainMap[*pSwapchain].pSurface = pSurface;
            if (pSurface) {
                pSurface->swapchains[*pSwapchain] = &my_data->swapchainMap[*pSwapchain];
            }
        }
        lock.unlock();

        return result;
    }
    return VK_ERROR_VALIDATION_FAILED_EXT;
}

VKAPI_ATTR void VKAPI_CALL DestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain, const VkAllocationCallbacks *pAllocator) {
    // TODOs:
    //
    // - Implement a check for validity language that reads: All uses of
    //   presentable images acquired from pname:swapchain must: have completed
    //   execution
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    SwpDevice *pDevice = NULL;
    {
        auto it = my_data->deviceMap.find(device);
        pDevice = (it == my_data->deviceMap.end()) ? NULL : &it->second;
    }

    // Validate that the swapchain extension was enabled:
    if (pDevice && !pDevice->swapchainExtensionEnabled) {
        skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                             reinterpret_cast<uint64_t>(device), __LINE__, SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED, swapchain_layer_name,
                             "vkDestroySwapchainKHR() called even though the %s extension was not enabled for this VkDevice.",
                             VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    }

    // Regardless of skip_call value, do some internal cleanup:
    SwpSwapchain *pSwapchain = NULL;
    {
        auto it = my_data->swapchainMap.find(swapchain);
        pSwapchain = (it == my_data->swapchainMap.end()) ? NULL : &it->second;
    }
    if (pSwapchain) {
        // Delete the SwpSwapchain associated with this swapchain:
        if (pSwapchain->pDevice) {
            pSwapchain->pDevice->swapchains.erase(swapchain);
        }
        if (pSwapchain->pSurface) {
            pSwapchain->pSurface->swapchains.erase(swapchain);
        }
        if (pSwapchain->imageCount) {
            pSwapchain->images.clear();
        }
        my_data->swapchainMap.erase(swapchain);
    }
    lock.unlock();

    if (!skip_call) {
        // Call down the call chain:
        my_data->device_dispatch_table->DestroySwapchainKHR(device, swapchain, pAllocator);
    }
}

VKAPI_ATTR VkResult VKAPI_CALL GetSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain, uint32_t *pSwapchainImageCount,
                                                     VkImage *pSwapchainImages) {
    VkResult result = VK_SUCCESS;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    SwpDevice *pDevice = NULL;
    {
        auto it = my_data->deviceMap.find(device);
        pDevice = (it == my_data->deviceMap.end()) ? NULL : &it->second;
    }

    // Validate that the swapchain extension was enabled:
    if (pDevice && !pDevice->swapchainExtensionEnabled) {
        skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                             reinterpret_cast<uint64_t>(device), __LINE__, SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED, swapchain_layer_name,
                             "vkGetSwapchainImagesKHR() called even though the %s extension was not enabled for this VkDevice.",
                             VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    }
    SwpSwapchain *pSwapchain = NULL;
    {
        auto it = my_data->swapchainMap.find(swapchain);
        pSwapchain = (it == my_data->swapchainMap.end()) ? NULL : &it->second;
    }
    if (pSwapchain && pSwapchainImages) {
        // Compare the preliminary value of *pSwapchainImageCount with the value this time:
        if (pSwapchain->imageCount == 0) {
            // Since we haven't recorded a preliminary value of *pSwapchainImageCount, that likely means that the application didn't
            // previously call this function with a NULL value of pSwapchainImages:
            skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                 reinterpret_cast<uint64_t>(device), __LINE__, SWAPCHAIN_PRIOR_COUNT, swapchain_layer_name,
                                 "vkGetSwapchainImagesKHR() called with non-NULL pSwapchainImageCount; but no prior positive "
                                 "value has been seen for pSwapchainImages.");
        } else if (*pSwapchainImageCount > pSwapchain->imageCount) {
            skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                 reinterpret_cast<uint64_t>(device), __LINE__, SWAPCHAIN_INVALID_COUNT, swapchain_layer_name,
                                 "vkGetSwapchainImagesKHR() called with non-NULL pSwapchainImageCount, and with "
                                 "pSwapchainImages set to a value (%d) that is greater than the value (%d) that was returned when "
                                 "pSwapchainImageCount was NULL.",
                                 *pSwapchainImageCount, pSwapchain->imageCount);
        }
    }
    lock.unlock();

    if (!skip_call) {
        // Call down the call chain:
        result = my_data->device_dispatch_table->GetSwapchainImagesKHR(device, swapchain, pSwapchainImageCount, pSwapchainImages);
        lock.lock();

        // Obtain this pointer again after locking:
        {
            auto it = my_data->swapchainMap.find(swapchain);
            pSwapchain = (it == my_data->swapchainMap.end()) ? NULL : &it->second;
        }
        if ((result == VK_SUCCESS) && pSwapchain && !pSwapchainImages && pSwapchainImageCount) {
            // Record the result of this preliminary query:
            pSwapchain->imageCount = *pSwapchainImageCount;
        } else if ((result == VK_SUCCESS) && pSwapchain && pSwapchainImages && pSwapchainImageCount &&
                   (*pSwapchainImageCount > 0)) {
            // Record the images and their state:
            pSwapchain->imageCount = *pSwapchainImageCount;
            for (uint32_t i = 0; i < *pSwapchainImageCount; i++) {
                pSwapchain->images[i].image = pSwapchainImages[i];
                pSwapchain->images[i].pSwapchain = pSwapchain;
                pSwapchain->images[i].acquiredByApp = false;
            }
        }
        lock.unlock();

        return result;
    }
    return VK_ERROR_VALIDATION_FAILED_EXT;
}

VKAPI_ATTR VkResult VKAPI_CALL AcquireNextImageKHR(VkDevice device, VkSwapchainKHR swapchain, uint64_t timeout,
                                                   VkSemaphore semaphore, VkFence fence, uint32_t *pImageIndex) {
    // TODOs:
    //
    // - Address the timeout.  Possibilities include looking at the state of the
    //   swapchain's images, depending on the timeout value.
    // - Implement a check for validity language that reads: If pname:semaphore is
    //   not sname:VK_NULL_HANDLE it must: be unsignalled
    // - Implement a check for validity language that reads: If pname:fence is not
    //   sname:VK_NULL_HANDLE it must: be unsignalled and mustnot: be associated
    //   with any other queue command that has not yet completed execution on that
    //   queue
    // - Record/update the state of the swapchain, in case an error occurs
    //   (e.g. VK_ERROR_OUT_OF_DATE_KHR).
    VkResult result = VK_SUCCESS;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    SwpDevice *pDevice = NULL;
    {
        auto it = my_data->deviceMap.find(device);
        pDevice = (it == my_data->deviceMap.end()) ? NULL : &it->second;
    }

    // Validate that the swapchain extension was enabled:
    if (pDevice && !pDevice->swapchainExtensionEnabled) {
        skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                             reinterpret_cast<uint64_t>(device), __LINE__, SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED, swapchain_layer_name,
                             "vkAcquireNextImageKHR() called even though the %s extension was not enabled for this VkDevice.",
                             VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    }
    SwpSwapchain *pSwapchain = NULL;
    {
        auto it = my_data->swapchainMap.find(swapchain);
        pSwapchain = (it == my_data->swapchainMap.end()) ? NULL : &it->second;
    }
    SwpPhysicalDevice *pPhysicalDevice = pDevice->pPhysicalDevice;
    if (pSwapchain && pPhysicalDevice && pPhysicalDevice->gotSurfaceCapabilities) {
        // Look to see if the application has already acquired the maximum
        // number of images, and this will push it past the spec-defined
        // limits:
        uint32_t minImageCount = pPhysicalDevice->surfaceCapabilities.minImageCount;
        uint32_t imagesAcquiredByApp = 0;
        for (uint32_t i = 0; i < pSwapchain->imageCount; i++) {
            if (pSwapchain->images[i].acquiredByApp) {
                imagesAcquiredByApp++;
            }
        }
        if (imagesAcquiredByApp > (pSwapchain->imageCount - minImageCount)) {
            skip_call |= log_msg(
                my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                reinterpret_cast<uint64_t>(device), __LINE__, SWAPCHAIN_APP_ACQUIRES_TOO_MANY_IMAGES, swapchain_layer_name,
                "vkAcquireNextImageKHR() called when it cannot succeed.  The application has acquired %d image(s) that have not "
                "yet "
                "been presented.  The maximum number of images that the application can simultaneously acquire from this swapchain "
                "(including this call to vkCreateSwapchainKHR()) is %d.  That value is derived by subtracting "
                "VkSurfaceCapabilitiesKHR::minImageCount (%d) from the number of images in the swapchain (%d) and adding 1.",
                imagesAcquiredByApp, (pSwapchain->imageCount - minImageCount + 1), minImageCount, pSwapchain->imageCount);
        }
    }
    lock.unlock();

    if (!skip_call) {
        // Call down the call chain:
        result = my_data->device_dispatch_table->AcquireNextImageKHR(device, swapchain, timeout, semaphore, fence, pImageIndex);
        lock.lock();

        // Obtain this pointer again after locking:
        {
            auto it = my_data->swapchainMap.find(swapchain);
            pSwapchain = (it == my_data->swapchainMap.end()) ? NULL : &it->second;
        }
        if (((result == VK_SUCCESS) || (result == VK_SUBOPTIMAL_KHR)) && pSwapchain) {
            // Change the state of the image (now acquired by the application):
            pSwapchain->images[*pImageIndex].acquiredByApp = true;
        }
        lock.unlock();

        return result;
    }
    return VK_ERROR_VALIDATION_FAILED_EXT;
}

VKAPI_ATTR VkResult VKAPI_CALL QueuePresentKHR(VkQueue queue, const VkPresentInfoKHR *pPresentInfo) {
    // TODOs:
    //
    // - Implement a check for validity language that reads: Any given element of
    //   sname:VkSemaphore in pname:pWaitSemaphores must: refer to a prior signal
    //   of that sname:VkSemaphore that won't be consumed by any other wait on that
    //   semaphore
    // - Record/update the state of the swapchain, in case an error occurs
    //   (e.g. VK_ERROR_OUT_OF_DATE_KHR).
    VkResult result = VK_SUCCESS;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(queue), layer_data_map);

    // Note: pPresentInfo->pResults is allowed to be NULL

    std::unique_lock<std::mutex> lock(global_lock);
    for (uint32_t i = 0; pPresentInfo && (i < pPresentInfo->swapchainCount); i++) {
        SwpSwapchain *pSwapchain = NULL;
        {
            auto it = my_data->swapchainMap.find(pPresentInfo->pSwapchains[i]);
            pSwapchain = (it == my_data->swapchainMap.end()) ? NULL : &it->second;
        }
        if (pSwapchain) {
            if (!pSwapchain->pDevice->swapchainExtensionEnabled) {
                skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                     reinterpret_cast<uint64_t>(pSwapchain->pDevice->device), __LINE__,
                                     SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED, swapchain_layer_name,
                                     "vkQueuePresentKHR() called even though the %s extension was not enabled for this VkDevice.",
                                     VK_KHR_SWAPCHAIN_EXTENSION_NAME);
            }
            SwpQueue *pQueue = NULL;
            {
                auto it = my_data->queueMap.find(queue);
                pQueue = (it == my_data->queueMap.end()) ? NULL : &it->second;
            }
            SwpSurface *pSurface = pSwapchain->pSurface;
            if (pQueue && pSurface && pSurface->numQueueFamilyIndexSupport) {
                uint32_t queueFamilyIndex = pQueue->queueFamilyIndex;
                // Note: the 1st test is to ensure queueFamilyIndex is in range,
                // and the 2nd test is the validation check:
                if ((pSurface->numQueueFamilyIndexSupport > queueFamilyIndex) &&
                    (!pSurface->pQueueFamilyIndexSupport[queueFamilyIndex])) {
                    skip_call |=
                        log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT,
                                reinterpret_cast<const uint64_t &>(pPresentInfo->pSwapchains[i]), __LINE__,
                                SWAPCHAIN_SURFACE_NOT_SUPPORTED_WITH_QUEUE, swapchain_layer_name,
                                "vkQueuePresentKHR() called with a swapchain whose surface is not supported for presention "
                                "on this device with the queueFamilyIndex (i.e. %d) of the given queue.",
                                queueFamilyIndex);
                }
            }
        }
    }
    lock.unlock();

    if (!skip_call) {
        // Call down the call chain:
        result = my_data->device_dispatch_table->QueuePresentKHR(queue, pPresentInfo);
        lock.lock();

        if (pPresentInfo && ((result == VK_SUCCESS) || (result == VK_SUBOPTIMAL_KHR))) {
            for (uint32_t i = 0; i < pPresentInfo->swapchainCount; i++) {
                int index = pPresentInfo->pImageIndices[i];
                SwpSwapchain *pSwapchain = NULL;
                {
                    auto it = my_data->swapchainMap.find(pPresentInfo->pSwapchains[i]);
                    pSwapchain = (it == my_data->swapchainMap.end()) ? NULL : &it->second;
                }
                if (pSwapchain) {
                    // Change the state of the image (no longer acquired by the
                    // application):
                    pSwapchain->images[index].acquiredByApp = false;
                }
            }
        }
        lock.unlock();

        return result;
    }
    return VK_ERROR_VALIDATION_FAILED_EXT;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateSharedSwapchainsKHR(VkDevice device, uint32_t swapchainCount,
                                                         const VkSwapchainCreateInfoKHR *pCreateInfos,
                                                         const VkAllocationCallbacks *pAllocator, VkSwapchainKHR *pSwapchains) {
    VkResult result = VK_SUCCESS;
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    SwpDevice *pDevice = nullptr;
    {
        auto it = my_data->deviceMap.find(device);
        pDevice = (it == my_data->deviceMap.end()) ? nullptr : &it->second;
    }

    // Validate that the swapchain extension was enabled:
    if (pDevice && !pDevice->displaySwapchainExtensionEnabled) {
        skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                             reinterpret_cast<uint64_t>(device), __LINE__, SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED, swapchain_layer_name,
                             "vkCreateSharedSwapchainsKHR() called even though the %s extension was not enabled for this VkDevice.",
                             VK_KHR_DISPLAY_SWAPCHAIN_EXTENSION_NAME);
    }
    if (!pCreateInfos || !pSwapchains) {
        skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                             reinterpret_cast<uint64_t>(device), __LINE__, SWAPCHAIN_NULL_POINTER, swapchain_layer_name,
                             "vkCreateSharedSwapchainsKHR() called with NULL pointer");
    }
    if (swapchainCount == 0) {
        skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                             reinterpret_cast<uint64_t>(device), __LINE__, SWAPCHAIN_INVALID_COUNT, swapchain_layer_name,
                             "vkCreateSharedSwapchainsKHR() called with invalid swapchain count of %d.", swapchainCount);
    } else {
        SwpSwapchain *pSwapchain = nullptr;
        for (uint32_t iii = 0; iii < swapchainCount; iii++) {
            if (pCreateInfos[iii].sType != VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR) {
                skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                     reinterpret_cast<uint64_t>(device), __LINE__, SWAPCHAIN_WRONG_STYPE, swapchain_layer_name,
                                     "vkCreateSharedSwapchainsKHR() called with invalid stype in pCreateInfos[%d].", iii);
            }
            auto it = my_data->swapchainMap.find(pSwapchains[iii]);
            pSwapchain = (it == my_data->swapchainMap.end()) ? nullptr : &it->second;
            if (nullptr == pSwapchain) {
                skip_call |=
                    log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                            reinterpret_cast<uint64_t>(device), __LINE__, SWAPCHAIN_INVALID_HANDLE, swapchain_layer_name,
                            "vkCreateSharedSwapchainsKHR() called with invalid Swapchain Handle in pCreateInfos[%d].", iii);
            }
        }
    }
    lock.unlock();

    if (!skip_call) {
        // Call down the call chain:
        result = my_data->device_dispatch_table->CreateSharedSwapchainsKHR(device, swapchainCount, pCreateInfos, pAllocator,
                                                                           pSwapchains);
        return result;
    }
    return VK_ERROR_VALIDATION_FAILED_EXT;
}

VKAPI_ATTR void VKAPI_CALL
GetDeviceQueue(VkDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, VkQueue *pQueue) {
    bool skip_call = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    if (!skip_call) {
        // Call down the call chain:
        my_data->device_dispatch_table->GetDeviceQueue(device, queueFamilyIndex, queueIndex, pQueue);

        // Remember the queue's handle, and link it to the device:
        std::lock_guard<std::mutex> lock(global_lock);
        SwpDevice *pDevice = NULL;
        {
            auto it = my_data->deviceMap.find(device);
            pDevice = (it == my_data->deviceMap.end()) ? NULL : &it->second;
        }
        my_data->queueMap[&pQueue].queue = *pQueue;
        if (pDevice) {
            pDevice->queues[*pQueue] = &my_data->queueMap[*pQueue];
        }
        my_data->queueMap[&pQueue].pDevice = pDevice;
        my_data->queueMap[&pQueue].queueFamilyIndex = queueFamilyIndex;
    }
}

VKAPI_ATTR VkResult VKAPI_CALL CreateDebugReportCallbackEXT(VkInstance instance,
                                                            const VkDebugReportCallbackCreateInfoEXT *pCreateInfo,
                                                            const VkAllocationCallbacks *pAllocator,
                                                            VkDebugReportCallbackEXT *pMsgCallback) {
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    VkResult result =
        my_data->instance_dispatch_table->CreateDebugReportCallbackEXT(instance, pCreateInfo, pAllocator, pMsgCallback);
    if (VK_SUCCESS == result) {
        std::lock_guard<std::mutex> lock(global_lock);
        result = layer_create_msg_callback(my_data->report_data, false, pCreateInfo, pAllocator, pMsgCallback);
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT msgCallback,
                                                         const VkAllocationCallbacks *pAllocator) {
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    my_data->instance_dispatch_table->DestroyDebugReportCallbackEXT(instance, msgCallback, pAllocator);
    std::lock_guard<std::mutex> lock(global_lock);
    layer_destroy_msg_callback(my_data->report_data, msgCallback, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL DebugReportMessageEXT(VkInstance instance, VkDebugReportFlagsEXT flags,
                                                 VkDebugReportObjectTypeEXT objType, uint64_t object, size_t location,
                                                 int32_t msgCode, const char *pLayerPrefix, const char *pMsg) {
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    my_data->instance_dispatch_table->DebugReportMessageEXT(instance, flags, objType, object, location, msgCode, pLayerPrefix,
                                                            pMsg);
}

VKAPI_ATTR VkResult VKAPI_CALL EnumerateInstanceLayerProperties(uint32_t *pCount, VkLayerProperties *pProperties) {
    return util_GetLayerProperties(1, &swapchain_layer, pCount, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL EnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t *pCount,
                                                              VkLayerProperties *pProperties) {
    return util_GetLayerProperties(1, &swapchain_layer, pCount, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL EnumerateInstanceExtensionProperties(const char *pLayerName, uint32_t *pCount,
                                                                    VkExtensionProperties *pProperties) {
    if (pLayerName && !strcmp(pLayerName, swapchain_layer.layerName))
        return util_GetExtensionProperties(1, instance_extensions, pCount, pProperties);

    return VK_ERROR_LAYER_NOT_PRESENT;
}

VKAPI_ATTR VkResult VKAPI_CALL EnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice, const char *pLayerName,
                                                                  uint32_t *pCount, VkExtensionProperties *pProperties) {
    if (pLayerName && !strcmp(pLayerName, swapchain_layer.layerName))
        return util_GetExtensionProperties(0, nullptr, pCount, pProperties);

    assert(physicalDevice);

    dispatch_key key = get_dispatch_key(physicalDevice);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    return my_data->instance_dispatch_table->EnumerateDeviceExtensionProperties(physicalDevice, NULL, pCount, pProperties);
}

static PFN_vkVoidFunction intercept_core_instance_command(const char *name);

static PFN_vkVoidFunction intercept_khr_surface_command(const char *name, VkInstance instance);

static PFN_vkVoidFunction intercept_core_device_command(const char *name);

static PFN_vkVoidFunction intercept_khr_swapchain_command(const char *name, VkDevice dev);

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL GetDeviceProcAddr(VkDevice device, const char *funcName) {
    PFN_vkVoidFunction proc = intercept_core_device_command(funcName);
    if (proc)
        return proc;

    assert(device);

    layer_data *my_data;

    my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkLayerDispatchTable *pDisp = my_data->device_dispatch_table;

    proc = intercept_khr_swapchain_command(funcName, device);
    if (proc)
        return proc;

    if (pDisp->GetDeviceProcAddr == NULL)
        return NULL;
    return pDisp->GetDeviceProcAddr(device, funcName);
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL GetInstanceProcAddr(VkInstance instance, const char *funcName) {
    PFN_vkVoidFunction proc = intercept_core_instance_command(funcName);
    if (!proc)
        proc = intercept_core_device_command(funcName);
    if (!proc)
        proc = intercept_khr_swapchain_command(funcName, VK_NULL_HANDLE);
    if (proc)
        return proc;

    assert(instance);

    layer_data *my_data;
    my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    VkLayerInstanceDispatchTable *pTable = my_data->instance_dispatch_table;

    proc = debug_report_get_instance_proc_addr(my_data->report_data, funcName);
    if (!proc)
        proc = intercept_khr_surface_command(funcName, instance);
    if (proc)
        return proc;

    if (pTable->GetInstanceProcAddr == NULL)
        return NULL;
    return pTable->GetInstanceProcAddr(instance, funcName);
}

static PFN_vkVoidFunction intercept_core_instance_command(const char *name) {
    static const struct {
        const char *name;
        PFN_vkVoidFunction proc;
    } core_instance_commands[] = {
        {"vkGetInstanceProcAddr", reinterpret_cast<PFN_vkVoidFunction>(GetInstanceProcAddr)},
        {"vkCreateInstance", reinterpret_cast<PFN_vkVoidFunction>(CreateInstance)},
        {"vkDestroyInstance", reinterpret_cast<PFN_vkVoidFunction>(DestroyInstance)},
        {"vkCreateDevice", reinterpret_cast<PFN_vkVoidFunction>(CreateDevice)},
        {"vkEnumeratePhysicalDevices", reinterpret_cast<PFN_vkVoidFunction>(EnumeratePhysicalDevices)},
        {"vkEnumerateInstanceLayerProperties", reinterpret_cast<PFN_vkVoidFunction>(EnumerateInstanceLayerProperties)},
        {"vkEnumerateDeviceLayerProperties", reinterpret_cast<PFN_vkVoidFunction>(EnumerateDeviceLayerProperties)},
        {"vkEnumerateInstanceExtensionProperties", reinterpret_cast<PFN_vkVoidFunction>(EnumerateInstanceExtensionProperties)},
        {"vkEnumerateDeviceExtensionProperties", reinterpret_cast<PFN_vkVoidFunction>(EnumerateDeviceExtensionProperties)},
        {"vkGetPhysicalDeviceQueueFamilyProperties", reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceQueueFamilyProperties)},
    };

    for (size_t i = 0; i < ARRAY_SIZE(core_instance_commands); i++) {
        if (!strcmp(core_instance_commands[i].name, name))
            return core_instance_commands[i].proc;
    }

    return nullptr;
}

static PFN_vkVoidFunction intercept_khr_surface_command(const char *name, VkInstance instance) {
    static const struct {
        const char *name;
        PFN_vkVoidFunction proc;
    } khr_surface_commands[] = {
#ifdef VK_USE_PLATFORM_ANDROID_KHR
        {"vkCreateAndroidSurfaceKHR", reinterpret_cast<PFN_vkVoidFunction>(CreateAndroidSurfaceKHR)},
#endif // VK_USE_PLATFORM_ANDROID_KHR
#ifdef VK_USE_PLATFORM_MIR_KHR
        {"vkCreateMirSurfaceKHR", reinterpret_cast<PFN_vkVoidFunction>(CreateMirSurfaceKHR)},
        {"vkGetPhysicalDeviceMirPresentationSupportKHR",
         reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceMirPresentationSupportKHR)},
#endif // VK_USE_PLATFORM_MIR_KHR
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
        {"vkCreateWaylandSurfaceKHR", reinterpret_cast<PFN_vkVoidFunction>(CreateWaylandSurfaceKHR)},
        {"vkGetPhysicalDeviceWaylandPresentationSupportKHR",
         reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceWaylandPresentationSupportKHR)},
#endif // VK_USE_PLATFORM_WAYLAND_KHR
#ifdef VK_USE_PLATFORM_WIN32_KHR
        {"vkCreateWin32SurfaceKHR", reinterpret_cast<PFN_vkVoidFunction>(CreateWin32SurfaceKHR)},
        {"vkGetPhysicalDeviceWin32PresentationSupportKHR",
         reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceWin32PresentationSupportKHR)},
#endif // VK_USE_PLATFORM_WIN32_KHR
#ifdef VK_USE_PLATFORM_XCB_KHR
        {"vkCreateXcbSurfaceKHR", reinterpret_cast<PFN_vkVoidFunction>(CreateXcbSurfaceKHR)},
        {"vkGetPhysicalDeviceXcbPresentationSupportKHR",
         reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceXcbPresentationSupportKHR)},
#endif // VK_USE_PLATFORM_XCB_KHR
#ifdef VK_USE_PLATFORM_XLIB_KHR
        {"vkCreateXlibSurfaceKHR", reinterpret_cast<PFN_vkVoidFunction>(CreateXlibSurfaceKHR)},
        {"vkGetPhysicalDeviceXlibPresentationSupportKHR",
         reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceXlibPresentationSupportKHR)},
#endif // VK_USE_PLATFORM_XLIB_KHR
        {"vkDestroySurfaceKHR", reinterpret_cast<PFN_vkVoidFunction>(DestroySurfaceKHR)},
        {"vkGetPhysicalDeviceSurfaceSupportKHR", reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceSurfaceSupportKHR)},
        {"vkGetPhysicalDeviceSurfaceCapabilitiesKHR",
         reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceSurfaceCapabilitiesKHR)},
        {"vkGetPhysicalDeviceSurfaceFormatsKHR", reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceSurfaceFormatsKHR)},
        {"vkGetPhysicalDeviceSurfacePresentModesKHR",
         reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceSurfacePresentModesKHR)},
        {"vkGetPhysicalDeviceDisplayPropertiesKHR", reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceDisplayPropertiesKHR)},
        {"vkGetPhysicalDeviceDisplayPlanePropertiesKHR",
         reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceDisplayPlanePropertiesKHR)},
        {"vkGetDisplayPlaneSupportedDisplaysKHR", reinterpret_cast<PFN_vkVoidFunction>(GetDisplayPlaneSupportedDisplaysKHR)},
        {"vkGetDisplayModePropertiesKHR", reinterpret_cast<PFN_vkVoidFunction>(GetDisplayModePropertiesKHR)},
        {"vkCreateDisplayModeKHR", reinterpret_cast<PFN_vkVoidFunction>(CreateDisplayModeKHR)},
        {"vkGetDisplayPlaneCapabilitiesKHR", reinterpret_cast<PFN_vkVoidFunction>(GetDisplayPlaneCapabilitiesKHR)},
        {"vkCreateDisplayPlaneSurfaceKHR", reinterpret_cast<PFN_vkVoidFunction>(CreateDisplayPlaneSurfaceKHR)},
    };

    // do not check if VK_KHR_*_surface is enabled (why?)

    for (size_t i = 0; i < ARRAY_SIZE(khr_surface_commands); i++) {
        if (!strcmp(khr_surface_commands[i].name, name))
            return khr_surface_commands[i].proc;
    }

    return nullptr;
}

static PFN_vkVoidFunction intercept_core_device_command(const char *name) {
    static const struct {
        const char *name;
        PFN_vkVoidFunction proc;
    } core_device_commands[] = {
        {"vkGetDeviceProcAddr", reinterpret_cast<PFN_vkVoidFunction>(GetDeviceProcAddr)},
        {"vkDestroyDevice", reinterpret_cast<PFN_vkVoidFunction>(DestroyDevice)},
        {"vkGetDeviceQueue", reinterpret_cast<PFN_vkVoidFunction>(GetDeviceQueue)},
    };

    for (size_t i = 0; i < ARRAY_SIZE(core_device_commands); i++) {
        if (!strcmp(core_device_commands[i].name, name))
            return core_device_commands[i].proc;
    }

    return nullptr;
}

static PFN_vkVoidFunction intercept_khr_swapchain_command(const char *name, VkDevice dev) {
    static const struct {
        const char *name;
        PFN_vkVoidFunction proc;
    } khr_swapchain_commands[] = {
        {"vkCreateSwapchainKHR", reinterpret_cast<PFN_vkVoidFunction>(CreateSwapchainKHR)},
        {"vkDestroySwapchainKHR", reinterpret_cast<PFN_vkVoidFunction>(DestroySwapchainKHR)},
        {"vkGetSwapchainImagesKHR", reinterpret_cast<PFN_vkVoidFunction>(GetSwapchainImagesKHR)},
        {"vkAcquireNextImageKHR", reinterpret_cast<PFN_vkVoidFunction>(AcquireNextImageKHR)},
        {"vkQueuePresentKHR", reinterpret_cast<PFN_vkVoidFunction>(QueuePresentKHR)},
    };

    // do not check if VK_KHR_swapchain is enabled (why?)

    for (size_t i = 0; i < ARRAY_SIZE(khr_swapchain_commands); i++) {
        if (!strcmp(khr_swapchain_commands[i].name, name))
            return khr_swapchain_commands[i].proc;
    }

    return nullptr;
}

} // namespace swapchain

// vk_layer_logging.h expects these to be defined

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugReportCallbackEXT(VkInstance instance,
                                                              const VkDebugReportCallbackCreateInfoEXT *pCreateInfo,
                                                              const VkAllocationCallbacks *pAllocator,
                                                              VkDebugReportCallbackEXT *pMsgCallback) {
    return swapchain::CreateDebugReportCallbackEXT(instance, pCreateInfo, pAllocator, pMsgCallback);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT msgCallback,
                                                           const VkAllocationCallbacks *pAllocator) {
    swapchain::DestroyDebugReportCallbackEXT(instance, msgCallback, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL vkDebugReportMessageEXT(VkInstance instance, VkDebugReportFlagsEXT flags,
                                                   VkDebugReportObjectTypeEXT objType, uint64_t object, size_t location,
                                                   int32_t msgCode, const char *pLayerPrefix, const char *pMsg) {
    swapchain::DebugReportMessageEXT(instance, flags, objType, object, location, msgCode, pLayerPrefix, pMsg);
}

// loader-layer interface v0, just wrappers since there is only a layer

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceExtensionProperties(const char *pLayerName, uint32_t *pCount,
                                                                                      VkExtensionProperties *pProperties) {
    return swapchain::EnumerateInstanceExtensionProperties(pLayerName, pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceLayerProperties(uint32_t *pCount,
                                                                                  VkLayerProperties *pProperties) {
    return swapchain::EnumerateInstanceLayerProperties(pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t *pCount,
                                                                                VkLayerProperties *pProperties) {
    // the layer command handles VK_NULL_HANDLE just fine internally
    assert(physicalDevice == VK_NULL_HANDLE);
    return swapchain::EnumerateDeviceLayerProperties(VK_NULL_HANDLE, pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice,
                                                                                    const char *pLayerName, uint32_t *pCount,
                                                                                    VkExtensionProperties *pProperties) {
    // the layer command handles VK_NULL_HANDLE just fine internally
    assert(physicalDevice == VK_NULL_HANDLE);
    return swapchain::EnumerateDeviceExtensionProperties(VK_NULL_HANDLE, pLayerName, pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice dev, const char *funcName) {
    return swapchain::GetDeviceProcAddr(dev, funcName);
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char *funcName) {
    return swapchain::GetInstanceProcAddr(instance, funcName);
}
