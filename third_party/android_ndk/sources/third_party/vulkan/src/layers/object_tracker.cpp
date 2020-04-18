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
 * Author: Mark Lobodzinski <mark@lunarg.com>
 * Author: Tobin Ehlis <tobine@google.com>
 * Author: Courtney Goeltzenleuchter <courtneygo@google.com>
 * Author: Jon Ashburn <jon@lunarg.com>
 * Author: Mike Stroyan <stroyan@google.com>
 * Author: Tony Barbour <tony@LunarG.com>
 */

#include "vk_loader_platform.h"
#include "vulkan/vulkan.h"

#include <cinttypes>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unordered_map>

#include "vk_layer_config.h"
#include "vk_layer_data.h"
#include "vk_layer_logging.h"
#include "vk_layer_table.h"
#include "vulkan/vk_layer.h"

#include "object_tracker.h"

#include "vk_validation_error_messages.h"

namespace object_tracker {

static void InitObjectTracker(layer_data *my_data, const VkAllocationCallbacks *pAllocator) {

    layer_debug_actions(my_data->report_data, my_data->logging_callback, pAllocator, "lunarg_object_tracker");
}

// Add new queue to head of global queue list
static void AddQueueInfo(VkDevice device, uint32_t queue_node_index, VkQueue queue) {
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    auto queueItem = device_data->queue_info_map.find(queue);
    if (queueItem == device_data->queue_info_map.end()) {
        OT_QUEUE_INFO *p_queue_info = new OT_QUEUE_INFO;
        if (p_queue_info != NULL) {
            memset(p_queue_info, 0, sizeof(OT_QUEUE_INFO));
            p_queue_info->queue = queue;
            p_queue_info->queue_node_index = queue_node_index;
            device_data->queue_info_map[queue] = p_queue_info;
        } else {
            log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT,
                    reinterpret_cast<uint64_t>(queue), __LINE__, OBJTRACK_INTERNAL_ERROR, LayerName,
                    "ERROR:  VK_ERROR_OUT_OF_HOST_MEMORY -- could not allocate memory for Queue Information");
        }
    }
}

// Destroy memRef lists and free all memory
static void DestroyQueueDataStructures(VkDevice device) {
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    for (auto queue_item : device_data->queue_info_map) {
        delete queue_item.second;
    }
    device_data->queue_info_map.clear();

    // Destroy the items in the queue map
    auto queue = device_data->object_map[VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT].begin();
    while (queue != device_data->object_map[VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT].end()) {
        uint32_t obj_index = queue->second->object_type;
        assert(device_data->num_total_objects > 0);
        device_data->num_total_objects--;
        assert(device_data->num_objects[obj_index] > 0);
        device_data->num_objects[obj_index]--;
        log_msg(device_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, queue->second->object_type, queue->second->handle,
                __LINE__, OBJTRACK_NONE, LayerName,
                "OBJ_STAT Destroy Queue obj 0x%" PRIxLEAST64 " (%" PRIu64 " total objs remain & %" PRIu64 " Queue objs).",
                queue->second->handle, device_data->num_total_objects, device_data->num_objects[obj_index]);
        delete queue->second;
        queue = device_data->object_map[VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT].erase(queue);
    }
}

// Check Queue type flags for selected queue operations
static void ValidateQueueFlags(VkQueue queue, const char *function) {
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(queue), layer_data_map);
    auto queue_item = device_data->queue_info_map.find(queue);
    if (queue_item != device_data->queue_info_map.end()) {
        OT_QUEUE_INFO *pQueueInfo = queue_item->second;
        if (pQueueInfo != NULL) {
            layer_data *instance_data = get_my_data_ptr(get_dispatch_key(device_data->physical_device), layer_data_map);
            if ((instance_data->queue_family_properties[pQueueInfo->queue_node_index].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) ==
                0) {
                log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT,
                        reinterpret_cast<uint64_t>(queue), __LINE__, OBJTRACK_UNKNOWN_OBJECT, LayerName,
                        "Attempting %s on a non-memory-management capable queue -- VK_QUEUE_SPARSE_BINDING_BIT not set", function);
            }
        }
    }
}

static void AllocateCommandBuffer(VkDevice device, const VkCommandPool command_pool, const VkCommandBuffer command_buffer,
                                  VkDebugReportObjectTypeEXT object_type, VkCommandBufferLevel level) {
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    log_msg(device_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, object_type, reinterpret_cast<const uint64_t>(command_buffer),
            __LINE__, OBJTRACK_NONE, LayerName, "OBJ[0x%" PRIxLEAST64 "] : CREATE %s object 0x%" PRIxLEAST64, object_track_index++,
            string_VkDebugReportObjectTypeEXT(object_type), reinterpret_cast<const uint64_t>(command_buffer));

    OBJTRACK_NODE *pNewObjNode = new OBJTRACK_NODE;
    pNewObjNode->object_type = object_type;
    pNewObjNode->handle = reinterpret_cast<const uint64_t>(command_buffer);
    pNewObjNode->parent_object = reinterpret_cast<const uint64_t &>(command_pool);
    if (level == VK_COMMAND_BUFFER_LEVEL_SECONDARY) {
        pNewObjNode->status = OBJSTATUS_COMMAND_BUFFER_SECONDARY;
    } else {
        pNewObjNode->status = OBJSTATUS_NONE;
    }
    device_data->object_map[object_type][reinterpret_cast<const uint64_t>(command_buffer)] = pNewObjNode;
    device_data->num_objects[object_type]++;
    device_data->num_total_objects++;
}

static bool ValidateCommandBuffer(VkDevice device, VkCommandPool command_pool, VkCommandBuffer command_buffer) {
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    bool skip_call = false;
    uint64_t object_handle = reinterpret_cast<uint64_t>(command_buffer);
    if (device_data->object_map[VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT].find(object_handle) !=
        device_data->object_map[VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT].end()) {
        OBJTRACK_NODE *pNode =
            device_data->object_map[VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT][reinterpret_cast<uint64_t>(command_buffer)];

        if (pNode->parent_object != reinterpret_cast<uint64_t &>(command_pool)) {
            skip_call |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->object_type, object_handle,
                                 __LINE__, OBJTRACK_COMMAND_POOL_MISMATCH, LayerName,
                                 "FreeCommandBuffers is attempting to free Command Buffer 0x%" PRIxLEAST64
                                 " belonging to Command Pool 0x%" PRIxLEAST64 " from pool 0x%" PRIxLEAST64 ").",
                                 reinterpret_cast<uint64_t>(command_buffer), pNode->parent_object,
                                 reinterpret_cast<uint64_t &>(command_pool));
        }
    } else {
        skip_call |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, object_handle,
                             __LINE__, OBJTRACK_NONE, LayerName, "Unable to remove command buffer obj 0x%" PRIxLEAST64
                                                                 ". Was it created? Has it already been destroyed?",
                             object_handle);
    }
    return skip_call;
}

static void AllocateDescriptorSet(VkDevice device, VkDescriptorPool descriptor_pool, VkDescriptorSet descriptor_set,
                                  VkDebugReportObjectTypeEXT object_type) {
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    log_msg(device_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, object_type,
            reinterpret_cast<uint64_t &>(descriptor_set), __LINE__, OBJTRACK_NONE, LayerName,
            "OBJ[0x%" PRIxLEAST64 "] : CREATE %s object 0x%" PRIxLEAST64, object_track_index++, object_name[object_type],
            reinterpret_cast<uint64_t &>(descriptor_set));

    OBJTRACK_NODE *pNewObjNode = new OBJTRACK_NODE;
    pNewObjNode->object_type = object_type;
    pNewObjNode->status = OBJSTATUS_NONE;
    pNewObjNode->handle = reinterpret_cast<uint64_t &>(descriptor_set);
    pNewObjNode->parent_object = reinterpret_cast<uint64_t &>(descriptor_pool);
    device_data->object_map[VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT][reinterpret_cast<uint64_t &>(descriptor_set)] =
        pNewObjNode;
    device_data->num_objects[object_type]++;
    device_data->num_total_objects++;
}

static bool ValidateDescriptorSet(VkDevice device, VkDescriptorPool descriptor_pool, VkDescriptorSet descriptor_set) {
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    bool skip_call = false;
    uint64_t object_handle = reinterpret_cast<uint64_t &>(descriptor_set);
    auto dsItem = device_data->object_map[VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT].find(object_handle);
    if (dsItem != device_data->object_map[VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT].end()) {
        OBJTRACK_NODE *pNode = dsItem->second;

        if (pNode->parent_object != reinterpret_cast<uint64_t &>(descriptor_pool)) {
            skip_call |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->object_type, object_handle,
                                 __LINE__, OBJTRACK_DESCRIPTOR_POOL_MISMATCH, LayerName,
                                 "FreeDescriptorSets is attempting to free descriptorSet 0x%" PRIxLEAST64
                                 " belonging to Descriptor Pool 0x%" PRIxLEAST64 " from pool 0x%" PRIxLEAST64 ").",
                                 reinterpret_cast<uint64_t &>(descriptor_set), pNode->parent_object,
                                 reinterpret_cast<uint64_t &>(descriptor_pool));
        }
    } else {
        skip_call |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, object_handle,
                             __LINE__, OBJTRACK_NONE, LayerName, "Unable to remove descriptor set obj 0x%" PRIxLEAST64
                                                                 ". Was it created? Has it already been destroyed?",
                             object_handle);
    }
    return skip_call;
}

static void CreateQueue(VkDevice device, VkQueue vkObj, VkDebugReportObjectTypeEXT object_type) {
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    log_msg(device_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, object_type, reinterpret_cast<uint64_t>(vkObj), __LINE__,
            OBJTRACK_NONE, LayerName, "OBJ[0x%" PRIxLEAST64 "] : CREATE %s object 0x%" PRIxLEAST64, object_track_index++,
            object_name[object_type], reinterpret_cast<uint64_t>(vkObj));

    OBJTRACK_NODE *p_obj_node = NULL;
    auto queue_item = device_data->object_map[VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT].find(reinterpret_cast<uint64_t>(vkObj));
    if (queue_item == device_data->object_map[VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT].end()) {
        p_obj_node = new OBJTRACK_NODE;
        device_data->object_map[VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT][reinterpret_cast<uint64_t>(vkObj)] = p_obj_node;
        device_data->num_objects[object_type]++;
        device_data->num_total_objects++;
    } else {
        p_obj_node = queue_item->second;
    }
    p_obj_node->object_type = object_type;
    p_obj_node->status = OBJSTATUS_NONE;
    p_obj_node->handle = reinterpret_cast<uint64_t>(vkObj);
}

static void CreateSwapchainImageObject(VkDevice dispatchable_object, VkImage swapchain_image, VkSwapchainKHR swapchain) {
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(dispatchable_object), layer_data_map);
    log_msg(device_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
            reinterpret_cast<uint64_t &>(swapchain_image), __LINE__, OBJTRACK_NONE, LayerName,
            "OBJ[0x%" PRIxLEAST64 "] : CREATE %s object 0x%" PRIxLEAST64, object_track_index++, "SwapchainImage",
            reinterpret_cast<uint64_t &>(swapchain_image));

    OBJTRACK_NODE *pNewObjNode = new OBJTRACK_NODE;
    pNewObjNode->object_type = VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT;
    pNewObjNode->status = OBJSTATUS_NONE;
    pNewObjNode->handle = reinterpret_cast<uint64_t &>(swapchain_image);
    pNewObjNode->parent_object = reinterpret_cast<uint64_t &>(swapchain);
    device_data->swapchainImageMap[reinterpret_cast<uint64_t &>(swapchain_image)] = pNewObjNode;
}

template<typename T>
uint64_t handle_value(T handle) {
    return reinterpret_cast<uint64_t &>(handle);
}
template<typename T>
uint64_t handle_value(T *handle) {
    return reinterpret_cast<uint64_t>(handle);
}

template <typename T1, typename T2>
static void CreateObject(T1 dispatchable_object, T2 object, VkDebugReportObjectTypeEXT object_type, const VkAllocationCallbacks *pAllocator) {
    layer_data *instance_data = get_my_data_ptr(get_dispatch_key(dispatchable_object), layer_data_map);

    auto object_handle = handle_value(object);
    bool custom_allocator = pAllocator != nullptr;

    log_msg(instance_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, object_type, object_handle,
            __LINE__, OBJTRACK_NONE, LayerName, "OBJ[0x%" PRIxLEAST64 "] : CREATE %s object 0x%" PRIxLEAST64, object_track_index++,
            object_name[object_type], object_handle);

    OBJTRACK_NODE *pNewObjNode = new OBJTRACK_NODE;
    pNewObjNode->object_type = object_type;
    pNewObjNode->status = custom_allocator ? OBJSTATUS_CUSTOM_ALLOCATOR : OBJSTATUS_NONE;
    pNewObjNode->handle = object_handle;
    instance_data->object_map[object_type][object_handle] = pNewObjNode;
    instance_data->num_objects[object_type]++;
    instance_data->num_total_objects++;
}

template <typename T1, typename T2>
static void DestroyObject(T1 dispatchable_object, T2 object, VkDebugReportObjectTypeEXT object_type, const VkAllocationCallbacks *pAllocator) {
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(dispatchable_object), layer_data_map);

    auto object_handle = handle_value(object);
    bool custom_allocator = pAllocator != nullptr;

    auto item = device_data->object_map[object_type].find(object_handle);
    if (item != device_data->object_map[object_type].end()) {

        OBJTRACK_NODE *pNode = item->second;
        assert(device_data->num_total_objects > 0);
        device_data->num_total_objects--;
        assert(device_data->num_objects[pNode->object_type] > 0);
        device_data->num_objects[pNode->object_type]--;

        log_msg(device_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, pNode->object_type, object_handle, __LINE__,
                OBJTRACK_NONE, LayerName,
                "OBJ_STAT Destroy %s obj 0x%" PRIxLEAST64 " (%" PRIu64 " total objs remain & %" PRIu64 " %s objs).",
                object_name[pNode->object_type], reinterpret_cast<uint64_t &>(object), device_data->num_total_objects,
                device_data->num_objects[pNode->object_type], object_name[pNode->object_type]);

        auto allocated_with_custom = (pNode->status & OBJSTATUS_CUSTOM_ALLOCATOR) ? true : false;
        if (custom_allocator ^ allocated_with_custom) {
            log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, object_type, object_handle, __LINE__,
                    OBJTRACK_ALLOCATOR_MISMATCH, LayerName,
                    "Custom allocator %sspecified while destroying %s obj 0x%" PRIxLEAST64 " but %sspecified at creation",
                    (custom_allocator ? "" : "not "), object_name[object_type], object_handle,
                    (allocated_with_custom ? "" : "not "));
        }

        delete pNode;
        device_data->object_map[object_type].erase(item);
    } else {
        log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, object_handle, __LINE__,
                OBJTRACK_UNKNOWN_OBJECT, LayerName,
                "Unable to remove %s obj 0x%" PRIxLEAST64 ". Was it created? Has it already been destroyed?",
                object_name[object_type], object_handle);
    }
}

template <typename T1, typename T2>
static bool ValidateObject(T1 dispatchable_object, T2 object, VkDebugReportObjectTypeEXT object_type, bool null_allowed,
                           int error_code = -1) {
    if (null_allowed && (object == VK_NULL_HANDLE)) {
        return false;
    }
    auto object_handle = handle_value(object);

    layer_data *device_data = get_my_data_ptr(get_dispatch_key(dispatchable_object), layer_data_map);
    if (device_data->object_map[object_type].find(object_handle) == device_data->object_map[object_type].end()) {
        // If object is an image, also look for it in the swapchain image map
        if ((object_type != VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT) ||
            (device_data->swapchainImageMap.find(object_handle) == device_data->swapchainImageMap.end())) {
            const char *error_msg = (error_code == -1) ? "" : validation_error_map[error_code];
            return log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, object_type, object_handle, __LINE__,
                           error_code, LayerName, "Invalid %s Object 0x%" PRIxLEAST64 ". %s", object_name[object_type],
                           object_handle, error_msg);
        }
    }
    return false;
}

static void DeviceReportUndestroyedObjects(VkDevice device, VkDebugReportObjectTypeEXT object_type) {
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    for (auto item = device_data->object_map[object_type].begin(); item != device_data->object_map[object_type].end();) {
        OBJTRACK_NODE *object_info = item->second;
        log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, object_info->object_type, object_info->handle, __LINE__,
                OBJTRACK_OBJECT_LEAK, LayerName,
                "OBJ ERROR : For device 0x%" PRIxLEAST64 ", %s object 0x%" PRIxLEAST64 " has not been destroyed.",
                reinterpret_cast<uint64_t>(device), object_name[object_type], object_info->handle);
        item = device_data->object_map[object_type].erase(item);
    }
}

VKAPI_ATTR void VKAPI_CALL DestroyInstance(VkInstance instance, const VkAllocationCallbacks *pAllocator) {
    std::unique_lock<std::mutex> lock(global_lock);

    dispatch_key key = get_dispatch_key(instance);
    layer_data *instance_data = get_my_data_ptr(key, layer_data_map);

    // Enable the temporary callback(s) here to catch cleanup issues:
    bool callback_setup = false;
    if (instance_data->num_tmp_callbacks > 0) {
        if (!layer_enable_tmp_callbacks(instance_data->report_data, instance_data->num_tmp_callbacks,
                                        instance_data->tmp_dbg_create_infos, instance_data->tmp_callbacks)) {
            callback_setup = true;
        }
    }

    ValidateObject(instance, instance, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT, true, VALIDATION_ERROR_00021);

    DestroyObject(instance, instance, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT, pAllocator);
    // Report any remaining objects in LL

    for (auto iit = instance_data->object_map[VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT].begin();
         iit != instance_data->object_map[VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT].end();) {
        OBJTRACK_NODE *pNode = iit->second;

        VkDevice device = reinterpret_cast<VkDevice>(pNode->handle);

        log_msg(instance_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->object_type, pNode->handle, __LINE__,
                OBJTRACK_OBJECT_LEAK, LayerName, "OBJ ERROR : %s object 0x%" PRIxLEAST64 " has not been destroyed.",
                string_VkDebugReportObjectTypeEXT(pNode->object_type), pNode->handle);
        // Semaphore:
        DeviceReportUndestroyedObjects(device, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT);
        DeviceReportUndestroyedObjects(device, VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT);
        DeviceReportUndestroyedObjects(device, VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT);
        DeviceReportUndestroyedObjects(device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT);
        DeviceReportUndestroyedObjects(device, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT);
        DeviceReportUndestroyedObjects(device, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT);
        DeviceReportUndestroyedObjects(device, VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT);
        DeviceReportUndestroyedObjects(device, VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT);
        DeviceReportUndestroyedObjects(device, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT);
        DeviceReportUndestroyedObjects(device, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT);
        DeviceReportUndestroyedObjects(device, VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT);
        DeviceReportUndestroyedObjects(device, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT);
        DeviceReportUndestroyedObjects(device, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT);
        DeviceReportUndestroyedObjects(device, VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT);
        DeviceReportUndestroyedObjects(device, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT);
        DeviceReportUndestroyedObjects(device, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT);
        DeviceReportUndestroyedObjects(device, VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT);
        DeviceReportUndestroyedObjects(device, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT);
        DeviceReportUndestroyedObjects(device, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT);
        DeviceReportUndestroyedObjects(device, VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT);
        DeviceReportUndestroyedObjects(device, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT);
        DeviceReportUndestroyedObjects(device, VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT);
        // DeviceReportUndestroyedObjects(device, VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_EXT);
    }
    instance_data->object_map[VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT].clear();

    VkLayerInstanceDispatchTable *pInstanceTable = get_dispatch_table(ot_instance_table_map, instance);
    pInstanceTable->DestroyInstance(instance, pAllocator);

    // Disable and cleanup the temporary callback(s):
    if (callback_setup) {
        layer_disable_tmp_callbacks(instance_data->report_data, instance_data->num_tmp_callbacks, instance_data->tmp_callbacks);
    }
    if (instance_data->num_tmp_callbacks > 0) {
        layer_free_tmp_callbacks(instance_data->tmp_dbg_create_infos, instance_data->tmp_callbacks);
        instance_data->num_tmp_callbacks = 0;
    }

    // Clean up logging callback, if any
    while (instance_data->logging_callback.size() > 0) {
        VkDebugReportCallbackEXT callback = instance_data->logging_callback.back();
        layer_destroy_msg_callback(instance_data->report_data, callback, pAllocator);
        instance_data->logging_callback.pop_back();
    }

    layer_debug_report_destroy_instance(instance_data->report_data);
    layer_data_map.erase(key);

    instanceExtMap.erase(pInstanceTable);
    lock.unlock();
    ot_instance_table_map.erase(key);
}

VKAPI_ATTR void VKAPI_CALL DestroyDevice(VkDevice device, const VkAllocationCallbacks *pAllocator) {

    std::unique_lock<std::mutex> lock(global_lock);
    ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, true, VALIDATION_ERROR_00052);
    DestroyObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, pAllocator);

    // Report any remaining objects associated with this VkDevice object in LL
    DeviceReportUndestroyedObjects(device, VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT);
    DeviceReportUndestroyedObjects(device, VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT);
    DeviceReportUndestroyedObjects(device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT);
    DeviceReportUndestroyedObjects(device, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT);
    DeviceReportUndestroyedObjects(device, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT);
    DeviceReportUndestroyedObjects(device, VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT);
    DeviceReportUndestroyedObjects(device, VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT);
    DeviceReportUndestroyedObjects(device, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT);
    DeviceReportUndestroyedObjects(device, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT);
    DeviceReportUndestroyedObjects(device, VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT);
    DeviceReportUndestroyedObjects(device, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT);
    DeviceReportUndestroyedObjects(device, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT);
    DeviceReportUndestroyedObjects(device, VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT);
    DeviceReportUndestroyedObjects(device, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT);
    DeviceReportUndestroyedObjects(device, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT);
    DeviceReportUndestroyedObjects(device, VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT);
    DeviceReportUndestroyedObjects(device, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT);
    // DeviceReportUndestroyedObjects(device, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT);
    DeviceReportUndestroyedObjects(device, VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT);
    DeviceReportUndestroyedObjects(device, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT);
    // DeviceReportUndestroyedObjects(device, VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT);
    DeviceReportUndestroyedObjects(device, VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT);
    // DeviceReportUndestroyedObjects(device, VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_EXT);

    // Clean up Queue's MemRef Linked Lists
    DestroyQueueDataStructures(device);

    lock.unlock();

    dispatch_key key = get_dispatch_key(device);
    VkLayerDispatchTable *pDisp = get_dispatch_table(ot_device_table_map, device);
    pDisp->DestroyDevice(device, pAllocator);
    ot_device_table_map.erase(key);
}

VKAPI_ATTR void VKAPI_CALL GetPhysicalDeviceFeatures(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures *pFeatures) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(physicalDevice, physicalDevice, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, false,
                                    VALIDATION_ERROR_01679);
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_instance_table_map, physicalDevice)->GetPhysicalDeviceFeatures(physicalDevice, pFeatures);
}

VKAPI_ATTR void VKAPI_CALL GetPhysicalDeviceFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format,
                                                             VkFormatProperties *pFormatProperties) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(physicalDevice, physicalDevice, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, false,
                                    VALIDATION_ERROR_01683);
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_instance_table_map, physicalDevice)
        ->GetPhysicalDeviceFormatProperties(physicalDevice, format, pFormatProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL GetPhysicalDeviceImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format,
                                                                      VkImageType type, VkImageTiling tiling,
                                                                      VkImageUsageFlags usage, VkImageCreateFlags flags,
                                                                      VkImageFormatProperties *pImageFormatProperties) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(physicalDevice, physicalDevice, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, false,
                                    VALIDATION_ERROR_01686);
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result =
        get_dispatch_table(ot_instance_table_map, physicalDevice)
            ->GetPhysicalDeviceImageFormatProperties(physicalDevice, format, type, tiling, usage, flags, pImageFormatProperties);
    return result;
}

VKAPI_ATTR void VKAPI_CALL GetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties *pProperties) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(physicalDevice, physicalDevice, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, false,
                                    VALIDATION_ERROR_00026);
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_instance_table_map, physicalDevice)->GetPhysicalDeviceProperties(physicalDevice, pProperties);
}

VKAPI_ATTR void VKAPI_CALL GetPhysicalDeviceMemoryProperties(VkPhysicalDevice physicalDevice,
                                                             VkPhysicalDeviceMemoryProperties *pMemoryProperties) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(physicalDevice, physicalDevice, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, false,
                                    VALIDATION_ERROR_00609);
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_instance_table_map, physicalDevice)->GetPhysicalDeviceMemoryProperties(physicalDevice, pMemoryProperties);
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL GetInstanceProcAddr(VkInstance instance, const char *pName);

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL GetDeviceProcAddr(VkDevice device, const char *pName);

VKAPI_ATTR VkResult VKAPI_CALL EnumerateInstanceExtensionProperties(const char *pLayerName, uint32_t *pPropertyCount,
                                                                    VkExtensionProperties *pProperties);

VKAPI_ATTR VkResult VKAPI_CALL EnumerateInstanceLayerProperties(uint32_t *pPropertyCount, VkLayerProperties *pProperties);

VKAPI_ATTR VkResult VKAPI_CALL EnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t *pPropertyCount,
                                                              VkLayerProperties *pProperties);

VKAPI_ATTR VkResult VKAPI_CALL QueueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo *pSubmits, VkFence fence) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(queue, fence, VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT, true, VALIDATION_ERROR_00130);
        if (pSubmits) {
            for (uint32_t idx0 = 0; idx0 < submitCount; ++idx0) {
                if (pSubmits[idx0].pCommandBuffers) {
                    for (uint32_t idx1 = 0; idx1 < pSubmits[idx0].commandBufferCount; ++idx1) {
                        skip_call |= ValidateObject(queue, pSubmits[idx0].pCommandBuffers[idx1],
                                                    VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false, VALIDATION_ERROR_00149);
                    }
                }
                if (pSubmits[idx0].pSignalSemaphores) {
                    for (uint32_t idx2 = 0; idx2 < pSubmits[idx0].signalSemaphoreCount; ++idx2) {
                        skip_call |= ValidateObject(queue, pSubmits[idx0].pSignalSemaphores[idx2],
                                                    VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT, false, VALIDATION_ERROR_00150);
                    }
                }
                if (pSubmits[idx0].pWaitSemaphores) {
                    for (uint32_t idx3 = 0; idx3 < pSubmits[idx0].waitSemaphoreCount; ++idx3) {
                        skip_call |= ValidateObject(queue, pSubmits[idx0].pWaitSemaphores[idx3],
                                                    VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT, false, VALIDATION_ERROR_00146);
                    }
                }
            }
        }
        if (queue) {
            skip_call |= ValidateObject(queue, queue, VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT, false, VALIDATION_ERROR_00128);
        }
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result = get_dispatch_table(ot_device_table_map, queue)->QueueSubmit(queue, submitCount, pSubmits, fence);
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL QueueWaitIdle(VkQueue queue) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(queue, queue, VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT, false, VALIDATION_ERROR_00317);
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result = get_dispatch_table(ot_device_table_map, queue)->QueueWaitIdle(queue);
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL DeviceWaitIdle(VkDevice device) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00318);
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result = get_dispatch_table(ot_device_table_map, device)->DeviceWaitIdle(device);
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL AllocateMemory(VkDevice device, const VkMemoryAllocateInfo *pAllocateInfo,
                                              const VkAllocationCallbacks *pAllocator, VkDeviceMemory *pMemory) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00612);
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result = get_dispatch_table(ot_device_table_map, device)->AllocateMemory(device, pAllocateInfo, pAllocator, pMemory);
    {
        std::lock_guard<std::mutex> lock(global_lock);
        if (result == VK_SUCCESS) {
            CreateObject(device, *pMemory, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, pAllocator);
        }
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL FlushMappedMemoryRanges(VkDevice device, uint32_t memoryRangeCount,
                                                       const VkMappedMemoryRange *pMemoryRanges) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00635);
        if (pMemoryRanges) {
            for (uint32_t idx0 = 0; idx0 < memoryRangeCount; ++idx0) {
                if (pMemoryRanges[idx0].memory) {
                    skip_call |= ValidateObject(device, pMemoryRanges[idx0].memory, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT,
                                                false, VALIDATION_ERROR_00648);
                }
            }
        }
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result =
        get_dispatch_table(ot_device_table_map, device)->FlushMappedMemoryRanges(device, memoryRangeCount, pMemoryRanges);
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL InvalidateMappedMemoryRanges(VkDevice device, uint32_t memoryRangeCount,
                                                            const VkMappedMemoryRange *pMemoryRanges) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00638);
        if (pMemoryRanges) {
            for (uint32_t idx0 = 0; idx0 < memoryRangeCount; ++idx0) {
                if (pMemoryRanges[idx0].memory) {
                    skip_call |= ValidateObject(device, pMemoryRanges[idx0].memory, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT,
                                                false, VALIDATION_ERROR_00648);
                }
            }
        }
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result =
        get_dispatch_table(ot_device_table_map, device)->InvalidateMappedMemoryRanges(device, memoryRangeCount, pMemoryRanges);
    return result;
}

VKAPI_ATTR void VKAPI_CALL GetDeviceMemoryCommitment(VkDevice device, VkDeviceMemory memory,
                                                     VkDeviceSize *pCommittedMemoryInBytes) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00654);
        skip_call |= ValidateObject(device, memory, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, false, VALIDATION_ERROR_00655);
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, device)->GetDeviceMemoryCommitment(device, memory, pCommittedMemoryInBytes);
}

VKAPI_ATTR VkResult VKAPI_CALL BindBufferMemory(VkDevice device, VkBuffer buffer, VkDeviceMemory memory,
                                                VkDeviceSize memoryOffset) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, false, VALIDATION_ERROR_00799);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00798);
        skip_call |= ValidateObject(device, memory, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, false, VALIDATION_ERROR_00800);
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result = get_dispatch_table(ot_device_table_map, device)->BindBufferMemory(device, buffer, memory, memoryOffset);
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL BindImageMemory(VkDevice device, VkImage image, VkDeviceMemory memory, VkDeviceSize memoryOffset) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00807);
        skip_call |= ValidateObject(device, image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, false, VALIDATION_ERROR_00808);
        skip_call |= ValidateObject(device, memory, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, false, VALIDATION_ERROR_00809);
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result = get_dispatch_table(ot_device_table_map, device)->BindImageMemory(device, image, memory, memoryOffset);
    return result;
}

VKAPI_ATTR void VKAPI_CALL GetBufferMemoryRequirements(VkDevice device, VkBuffer buffer,
                                                       VkMemoryRequirements *pMemoryRequirements) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, false, VALIDATION_ERROR_00784);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00783);
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, device)->GetBufferMemoryRequirements(device, buffer, pMemoryRequirements);
}

VKAPI_ATTR void VKAPI_CALL GetImageMemoryRequirements(VkDevice device, VkImage image, VkMemoryRequirements *pMemoryRequirements) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00787);
        skip_call |= ValidateObject(device, image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, false, VALIDATION_ERROR_00788);
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, device)->GetImageMemoryRequirements(device, image, pMemoryRequirements);
}

VKAPI_ATTR void VKAPI_CALL GetImageSparseMemoryRequirements(VkDevice device, VkImage image, uint32_t *pSparseMemoryRequirementCount,
                                                            VkSparseImageMemoryRequirements *pSparseMemoryRequirements) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_01610);
        skip_call |= ValidateObject(device, image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, false, VALIDATION_ERROR_01611);
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, device)
        ->GetImageSparseMemoryRequirements(device, image, pSparseMemoryRequirementCount, pSparseMemoryRequirements);
}

VKAPI_ATTR void VKAPI_CALL GetPhysicalDeviceSparseImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format,
                                                                        VkImageType type, VkSampleCountFlagBits samples,
                                                                        VkImageUsageFlags usage, VkImageTiling tiling,
                                                                        uint32_t *pPropertyCount,
                                                                        VkSparseImageFormatProperties *pProperties) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(physicalDevice, physicalDevice, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, false,
                                    VALIDATION_ERROR_01601);
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_instance_table_map, physicalDevice)
        ->GetPhysicalDeviceSparseImageFormatProperties(physicalDevice, format, type, samples, usage, tiling, pPropertyCount,
                                                       pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL CreateFence(VkDevice device, const VkFenceCreateInfo *pCreateInfo,
                                           const VkAllocationCallbacks *pAllocator, VkFence *pFence) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00166);
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result = get_dispatch_table(ot_device_table_map, device)->CreateFence(device, pCreateInfo, pAllocator, pFence);
    {
        std::lock_guard<std::mutex> lock(global_lock);
        if (result == VK_SUCCESS) {
            CreateObject(device, *pFence, VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT, pAllocator);
        }
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyFence(VkDevice device, VkFence fence, const VkAllocationCallbacks *pAllocator) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00176);
        skip_call |= ValidateObject(device, fence, VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT, true, VALIDATION_ERROR_00177);
    }
    if (skip_call) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(global_lock);
        DestroyObject(device, fence, VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT, pAllocator);
    }
    get_dispatch_table(ot_device_table_map, device)->DestroyFence(device, fence, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL ResetFences(VkDevice device, uint32_t fenceCount, const VkFence *pFences) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00184);
        if (pFences) {
            for (uint32_t idx0 = 0; idx0 < fenceCount; ++idx0) {
                skip_call |=
                    ValidateObject(device, pFences[idx0], VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT, false, VALIDATION_ERROR_00187);
            }
        }
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result = get_dispatch_table(ot_device_table_map, device)->ResetFences(device, fenceCount, pFences);
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL GetFenceStatus(VkDevice device, VkFence fence) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00180);
        skip_call |= ValidateObject(device, fence, VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT, false, VALIDATION_ERROR_00181);
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result = get_dispatch_table(ot_device_table_map, device)->GetFenceStatus(device, fence);
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL WaitForFences(VkDevice device, uint32_t fenceCount, const VkFence *pFences, VkBool32 waitAll,
                                             uint64_t timeout) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00188);
        if (pFences) {
            for (uint32_t idx0 = 0; idx0 < fenceCount; ++idx0) {
                skip_call |=
                    ValidateObject(device, pFences[idx0], VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT, false, VALIDATION_ERROR_00191);
            }
        }
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result = get_dispatch_table(ot_device_table_map, device)->WaitForFences(device, fenceCount, pFences, waitAll, timeout);
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateSemaphore(VkDevice device, const VkSemaphoreCreateInfo *pCreateInfo,
                                               const VkAllocationCallbacks *pAllocator, VkSemaphore *pSemaphore) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00192);
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result = get_dispatch_table(ot_device_table_map, device)->CreateSemaphore(device, pCreateInfo, pAllocator, pSemaphore);
    {
        std::lock_guard<std::mutex> lock(global_lock);
        if (result == VK_SUCCESS) {
            CreateObject(device, *pSemaphore, VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT, pAllocator);
        }
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroySemaphore(VkDevice device, VkSemaphore semaphore, const VkAllocationCallbacks *pAllocator) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00202);
        skip_call |= ValidateObject(device, semaphore, VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT, true, VALIDATION_ERROR_00203);
    }
    if (skip_call) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(global_lock);
        DestroyObject(device, semaphore, VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT, pAllocator);
    }
    get_dispatch_table(ot_device_table_map, device)->DestroySemaphore(device, semaphore, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL CreateEvent(VkDevice device, const VkEventCreateInfo *pCreateInfo,
                                           const VkAllocationCallbacks *pAllocator, VkEvent *pEvent) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00206);
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result = get_dispatch_table(ot_device_table_map, device)->CreateEvent(device, pCreateInfo, pAllocator, pEvent);
    {
        std::lock_guard<std::mutex> lock(global_lock);
        if (result == VK_SUCCESS) {
            CreateObject(device, *pEvent, VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT, pAllocator);
        }
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyEvent(VkDevice device, VkEvent event, const VkAllocationCallbacks *pAllocator) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00216);
        skip_call |= ValidateObject(device, event, VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT, true, VALIDATION_ERROR_00217);
    }
    if (skip_call) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(global_lock);
        DestroyObject(device, event, VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT, pAllocator);
    }
    get_dispatch_table(ot_device_table_map, device)->DestroyEvent(device, event, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL GetEventStatus(VkDevice device, VkEvent event) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00220);
        skip_call |= ValidateObject(device, event, VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT, false, VALIDATION_ERROR_00221);
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result = get_dispatch_table(ot_device_table_map, device)->GetEventStatus(device, event);
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL SetEvent(VkDevice device, VkEvent event) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00223);
        skip_call |= ValidateObject(device, event, VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT, false, VALIDATION_ERROR_00224);
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result = get_dispatch_table(ot_device_table_map, device)->SetEvent(device, event);
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL ResetEvent(VkDevice device, VkEvent event) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00227);
        skip_call |= ValidateObject(device, event, VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT, false, VALIDATION_ERROR_00228);
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result = get_dispatch_table(ot_device_table_map, device)->ResetEvent(device, event);
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateQueryPool(VkDevice device, const VkQueryPoolCreateInfo *pCreateInfo,
                                               const VkAllocationCallbacks *pAllocator, VkQueryPool *pQueryPool) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_01002);
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result = get_dispatch_table(ot_device_table_map, device)->CreateQueryPool(device, pCreateInfo, pAllocator, pQueryPool);
    {
        std::lock_guard<std::mutex> lock(global_lock);
        if (result == VK_SUCCESS) {
            CreateObject(device, *pQueryPool, VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT, pAllocator);
        }
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyQueryPool(VkDevice device, VkQueryPool queryPool, const VkAllocationCallbacks *pAllocator) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_01015);
        skip_call |= ValidateObject(device, queryPool, VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT, true, VALIDATION_ERROR_01016);
    }
    if (skip_call) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(global_lock);
        DestroyObject(device, queryPool, VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT, pAllocator);
    }
    get_dispatch_table(ot_device_table_map, device)->DestroyQueryPool(device, queryPool, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL GetQueryPoolResults(VkDevice device, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount,
                                                   size_t dataSize, void *pData, VkDeviceSize stride, VkQueryResultFlags flags) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_01054);
        skip_call |= ValidateObject(device, queryPool, VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT, false, VALIDATION_ERROR_01055);
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result = get_dispatch_table(ot_device_table_map, device)
                          ->GetQueryPoolResults(device, queryPool, firstQuery, queryCount, dataSize, pData, stride, flags);
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateBuffer(VkDevice device, const VkBufferCreateInfo *pCreateInfo,
                                            const VkAllocationCallbacks *pAllocator, VkBuffer *pBuffer) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00659);
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result = get_dispatch_table(ot_device_table_map, device)->CreateBuffer(device, pCreateInfo, pAllocator, pBuffer);
    {
        std::lock_guard<std::mutex> lock(global_lock);
        if (result == VK_SUCCESS) {
            CreateObject(device, *pBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, pAllocator);
        }
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyBuffer(VkDevice device, VkBuffer buffer, const VkAllocationCallbacks *pAllocator) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, true, VALIDATION_ERROR_00680);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00679);
    }
    if (skip_call) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(global_lock);
        DestroyObject(device, buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, pAllocator);
    }
    get_dispatch_table(ot_device_table_map, device)->DestroyBuffer(device, buffer, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL CreateBufferView(VkDevice device, const VkBufferViewCreateInfo *pCreateInfo,
                                                const VkAllocationCallbacks *pAllocator, VkBufferView *pView) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00683);
        if (pCreateInfo) {
            skip_call |=
                ValidateObject(device, pCreateInfo->buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, false, VALIDATION_ERROR_00699);
        }
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result = get_dispatch_table(ot_device_table_map, device)->CreateBufferView(device, pCreateInfo, pAllocator, pView);
    {
        std::lock_guard<std::mutex> lock(global_lock);
        if (result == VK_SUCCESS) {
            CreateObject(device, *pView, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT, pAllocator);
        }
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyBufferView(VkDevice device, VkBufferView bufferView, const VkAllocationCallbacks *pAllocator) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, bufferView, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT, true, VALIDATION_ERROR_00705);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00704);
    }
    if (skip_call) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(global_lock);
        DestroyObject(device, bufferView, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT, pAllocator);
    }
    get_dispatch_table(ot_device_table_map, device)->DestroyBufferView(device, bufferView, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL CreateImage(VkDevice device, const VkImageCreateInfo *pCreateInfo,
                                           const VkAllocationCallbacks *pAllocator, VkImage *pImage) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00709);
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result = get_dispatch_table(ot_device_table_map, device)->CreateImage(device, pCreateInfo, pAllocator, pImage);
    {
        std::lock_guard<std::mutex> lock(global_lock);
        if (result == VK_SUCCESS) {
            CreateObject(device, *pImage, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, pAllocator);
        }
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyImage(VkDevice device, VkImage image, const VkAllocationCallbacks *pAllocator) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00746);
        skip_call |= ValidateObject(device, image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, true, VALIDATION_ERROR_00747);
    }
    if (skip_call) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(global_lock);
        DestroyObject(device, image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, pAllocator);
    }
    get_dispatch_table(ot_device_table_map, device)->DestroyImage(device, image, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL GetImageSubresourceLayout(VkDevice device, VkImage image, const VkImageSubresource *pSubresource,
                                                     VkSubresourceLayout *pLayout) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00734);
        skip_call |= ValidateObject(device, image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, false, VALIDATION_ERROR_00735);
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, device)->GetImageSubresourceLayout(device, image, pSubresource, pLayout);
}

VKAPI_ATTR VkResult VKAPI_CALL CreateImageView(VkDevice device, const VkImageViewCreateInfo *pCreateInfo,
                                               const VkAllocationCallbacks *pAllocator, VkImageView *pView) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00750);
        if (pCreateInfo) {
            skip_call |=
                ValidateObject(device, pCreateInfo->image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, false, VALIDATION_ERROR_00763);
        }
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result = get_dispatch_table(ot_device_table_map, device)->CreateImageView(device, pCreateInfo, pAllocator, pView);
    {
        std::lock_guard<std::mutex> lock(global_lock);
        if (result == VK_SUCCESS) {
            CreateObject(device, *pView, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT, pAllocator);
        }
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyImageView(VkDevice device, VkImageView imageView, const VkAllocationCallbacks *pAllocator) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00779);
        skip_call |= ValidateObject(device, imageView, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT, true, VALIDATION_ERROR_00780);
    }
    if (skip_call) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(global_lock);
        DestroyObject(device, imageView, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT, pAllocator);
    }
    get_dispatch_table(ot_device_table_map, device)->DestroyImageView(device, imageView, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL CreateShaderModule(VkDevice device, const VkShaderModuleCreateInfo *pCreateInfo,
                                                  const VkAllocationCallbacks *pAllocator, VkShaderModule *pShaderModule) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00466);
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result =
        get_dispatch_table(ot_device_table_map, device)->CreateShaderModule(device, pCreateInfo, pAllocator, pShaderModule);
    {
        std::lock_guard<std::mutex> lock(global_lock);
        if (result == VK_SUCCESS) {
            CreateObject(device, *pShaderModule, VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, pAllocator);
        }
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyShaderModule(VkDevice device, VkShaderModule shaderModule,
                                               const VkAllocationCallbacks *pAllocator) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00481);
        skip_call |=
            ValidateObject(device, shaderModule, VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, true, VALIDATION_ERROR_00482);
    }
    if (skip_call) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(global_lock);
        DestroyObject(device, shaderModule, VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, pAllocator);
    }
    get_dispatch_table(ot_device_table_map, device)->DestroyShaderModule(device, shaderModule, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL CreatePipelineCache(VkDevice device, const VkPipelineCacheCreateInfo *pCreateInfo,
                                                   const VkAllocationCallbacks *pAllocator, VkPipelineCache *pPipelineCache) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00562);
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result =
        get_dispatch_table(ot_device_table_map, device)->CreatePipelineCache(device, pCreateInfo, pAllocator, pPipelineCache);
    {
        std::lock_guard<std::mutex> lock(global_lock);
        if (result == VK_SUCCESS) {
            CreateObject(device, *pPipelineCache, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT, pAllocator);
        }
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyPipelineCache(VkDevice device, VkPipelineCache pipelineCache,
                                                const VkAllocationCallbacks *pAllocator) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00585);
        skip_call |=
            ValidateObject(device, pipelineCache, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT, true, VALIDATION_ERROR_00586);
    }
    if (skip_call) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(global_lock);
        DestroyObject(device, pipelineCache, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT, pAllocator);
    }
    get_dispatch_table(ot_device_table_map, device)->DestroyPipelineCache(device, pipelineCache, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL GetPipelineCacheData(VkDevice device, VkPipelineCache pipelineCache, size_t *pDataSize,
                                                    void *pData) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00578);
        skip_call |=
            ValidateObject(device, pipelineCache, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT, false, VALIDATION_ERROR_00579);
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result =
        get_dispatch_table(ot_device_table_map, device)->GetPipelineCacheData(device, pipelineCache, pDataSize, pData);
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL MergePipelineCaches(VkDevice device, VkPipelineCache dstCache, uint32_t srcCacheCount,
                                                   const VkPipelineCache *pSrcCaches) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00572);
        skip_call |=
            ValidateObject(device, dstCache, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT, false, VALIDATION_ERROR_00573);
        if (pSrcCaches) {
            for (uint32_t idx0 = 0; idx0 < srcCacheCount; ++idx0) {
                skip_call |= ValidateObject(device, pSrcCaches[idx0], VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT, false,
                                            VALIDATION_ERROR_00577);
            }
        }
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result =
        get_dispatch_table(ot_device_table_map, device)->MergePipelineCaches(device, dstCache, srcCacheCount, pSrcCaches);
    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyPipeline(VkDevice device, VkPipeline pipeline, const VkAllocationCallbacks *pAllocator) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00558);
        skip_call |= ValidateObject(device, pipeline, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT, true, VALIDATION_ERROR_00559);
    }
    if (skip_call) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(global_lock);
        DestroyObject(device, pipeline, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT, pAllocator);
    }
    get_dispatch_table(ot_device_table_map, device)->DestroyPipeline(device, pipeline, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL CreatePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo *pCreateInfo,
                                                    const VkAllocationCallbacks *pAllocator, VkPipelineLayout *pPipelineLayout) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00861);
        if (pCreateInfo) {
            if (pCreateInfo->pSetLayouts) {
                for (uint32_t idx0 = 0; idx0 < pCreateInfo->setLayoutCount; ++idx0) {
                    skip_call |=
                        ValidateObject(device, pCreateInfo->pSetLayouts[idx0],
                                       VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT, false, VALIDATION_ERROR_00875);
                }
            }
        }
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result =
        get_dispatch_table(ot_device_table_map, device)->CreatePipelineLayout(device, pCreateInfo, pAllocator, pPipelineLayout);
    {
        std::lock_guard<std::mutex> lock(global_lock);
        if (result == VK_SUCCESS) {
            CreateObject(device, *pPipelineLayout, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT, pAllocator);
        }
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout,
                                                 const VkAllocationCallbacks *pAllocator) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00885);
        skip_call |=
            ValidateObject(device, pipelineLayout, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT, true, VALIDATION_ERROR_00886);
    }
    if (skip_call) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(global_lock);
        DestroyObject(device, pipelineLayout, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT, pAllocator);
    }
    get_dispatch_table(ot_device_table_map, device)->DestroyPipelineLayout(device, pipelineLayout, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL CreateSampler(VkDevice device, const VkSamplerCreateInfo *pCreateInfo,
                                             const VkAllocationCallbacks *pAllocator, VkSampler *pSampler) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00812);
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result = get_dispatch_table(ot_device_table_map, device)->CreateSampler(device, pCreateInfo, pAllocator, pSampler);
    {
        std::lock_guard<std::mutex> lock(global_lock);
        if (result == VK_SUCCESS) {
            CreateObject(device, *pSampler, VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT, pAllocator);
        }
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroySampler(VkDevice device, VkSampler sampler, const VkAllocationCallbacks *pAllocator) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00840);
        skip_call |= ValidateObject(device, sampler, VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT, true, VALIDATION_ERROR_00841);
    }
    if (skip_call) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(global_lock);
        DestroyObject(device, sampler, VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT, pAllocator);
    }
    get_dispatch_table(ot_device_table_map, device)->DestroySampler(device, sampler, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL CreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo *pCreateInfo,
                                                         const VkAllocationCallbacks *pAllocator,
                                                         VkDescriptorSetLayout *pSetLayout) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00844);
        if (pCreateInfo) {
            if (pCreateInfo->pBindings) {
                for (uint32_t idx0 = 0; idx0 < pCreateInfo->bindingCount; ++idx0) {
                    if ((pCreateInfo->pBindings[idx0].descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER) ||
                        (pCreateInfo->pBindings[idx0].descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)) {
                        if (pCreateInfo->pBindings[idx0].pImmutableSamplers) {
                            for (uint32_t idx1 = 0; idx1 < pCreateInfo->pBindings[idx0].descriptorCount; ++idx1) {
                                skip_call |= ValidateObject(device, pCreateInfo->pBindings[idx0].pImmutableSamplers[idx1],
                                                            VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT, false, VALIDATION_ERROR_00852);
                            }
                        }
                    }
                }
            }
        }
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result =
        get_dispatch_table(ot_device_table_map, device)->CreateDescriptorSetLayout(device, pCreateInfo, pAllocator, pSetLayout);
    {
        std::lock_guard<std::mutex> lock(global_lock);
        if (result == VK_SUCCESS) {
            CreateObject(device, *pSetLayout, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT, pAllocator);
        }
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout,
                                                      const VkAllocationCallbacks *pAllocator) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, descriptorSetLayout, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT, true,
                                    VALIDATION_ERROR_00858);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00857);
    }
    if (skip_call) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(global_lock);
        DestroyObject(device, descriptorSetLayout, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT, pAllocator);
    }
    get_dispatch_table(ot_device_table_map, device)->DestroyDescriptorSetLayout(device, descriptorSetLayout, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL CreateDescriptorPool(VkDevice device, const VkDescriptorPoolCreateInfo *pCreateInfo,
                                                    const VkAllocationCallbacks *pAllocator, VkDescriptorPool *pDescriptorPool) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00889);
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result =
        get_dispatch_table(ot_device_table_map, device)->CreateDescriptorPool(device, pCreateInfo, pAllocator, pDescriptorPool);
    {
        std::lock_guard<std::mutex> lock(global_lock);
        if (result == VK_SUCCESS) {
            CreateObject(device, *pDescriptorPool, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT, pAllocator);
        }
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL ResetDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool,
                                                   VkDescriptorPoolResetFlags flags) {
    bool skip_call = false;
    std::unique_lock<std::mutex> lock(global_lock);
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    skip_call |=
        ValidateObject(device, descriptorPool, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT, false, VALIDATION_ERROR_00930);
    skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00929);
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    // A DescriptorPool's descriptor sets are implicitly deleted when the pool is reset.
    // Remove this pool's descriptor sets from our descriptorSet map.
    auto itr = device_data->object_map[VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT].begin();
    while (itr != device_data->object_map[VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT].end()) {
        OBJTRACK_NODE *pNode = (*itr).second;
        auto del_itr = itr++;
        if (pNode->parent_object == reinterpret_cast<uint64_t &>(descriptorPool)) {
            DestroyObject(device, (VkDescriptorSet)((*del_itr).first),
                                         VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, nullptr);
        }
    }
    lock.unlock();
    VkResult result = get_dispatch_table(ot_device_table_map, device)->ResetDescriptorPool(device, descriptorPool, flags);
    return result;
}

VKAPI_ATTR void VKAPI_CALL UpdateDescriptorSets(VkDevice device, uint32_t descriptorWriteCount,
                                                const VkWriteDescriptorSet *pDescriptorWrites, uint32_t descriptorCopyCount,
                                                const VkCopyDescriptorSet *pDescriptorCopies) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00933);
        if (pDescriptorCopies) {
            for (uint32_t idx0 = 0; idx0 < descriptorCopyCount; ++idx0) {
                if (pDescriptorCopies[idx0].dstSet) {
                    skip_call |= ValidateObject(device, pDescriptorCopies[idx0].dstSet,
                                                VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, false, VALIDATION_ERROR_00972);
                }
                if (pDescriptorCopies[idx0].srcSet) {
                    skip_call |= ValidateObject(device, pDescriptorCopies[idx0].srcSet,
                                                VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, false, VALIDATION_ERROR_00971);
                }
            }
        }
        if (pDescriptorWrites) {
            for (uint32_t idx1 = 0; idx1 < descriptorWriteCount; ++idx1) {
                if (pDescriptorWrites[idx1].dstSet) {
                    skip_call |= ValidateObject(device, pDescriptorWrites[idx1].dstSet,
                                                VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, false, VALIDATION_ERROR_00955);
                }
                if ((pDescriptorWrites[idx1].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER) ||
                    (pDescriptorWrites[idx1].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER)) {
                    for (uint32_t idx2 = 0; idx2 < pDescriptorWrites[idx1].descriptorCount; ++idx2) {
                        skip_call |= ValidateObject(device, pDescriptorWrites[idx1].pTexelBufferView[idx2],
                                                    VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT, false, VALIDATION_ERROR_00940);
                    }
                }
                if ((pDescriptorWrites[idx1].descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) ||
                    (pDescriptorWrites[idx1].descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE) ||
                    (pDescriptorWrites[idx1].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE) ||
                    (pDescriptorWrites[idx1].descriptorType == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT)) {
                    for (uint32_t idx3 = 0; idx3 < pDescriptorWrites[idx1].descriptorCount; ++idx3) {
                        if (pDescriptorWrites[idx1].pImageInfo[idx3].imageView) {
                            skip_call |= ValidateObject(device, pDescriptorWrites[idx1].pImageInfo[idx3].imageView,
                                                        VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT, false, VALIDATION_ERROR_00943);
                        }
                    }
                }
                if ((pDescriptorWrites[idx1].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) ||
                    (pDescriptorWrites[idx1].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) ||
                    (pDescriptorWrites[idx1].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) ||
                    (pDescriptorWrites[idx1].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)) {
                    for (uint32_t idx4 = 0; idx4 < pDescriptorWrites[idx1].descriptorCount; ++idx4) {
                        if (pDescriptorWrites[idx1].pBufferInfo[idx4].buffer) {
                            skip_call |= ValidateObject(device, pDescriptorWrites[idx1].pBufferInfo[idx4].buffer,
                                                        VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, false, VALIDATION_ERROR_00962);
                        }
                    }
                }
            }
        }
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, device)
        ->UpdateDescriptorSets(device, descriptorWriteCount, pDescriptorWrites, descriptorCopyCount, pDescriptorCopies);
}

VKAPI_ATTR VkResult VKAPI_CALL CreateFramebuffer(VkDevice device, const VkFramebufferCreateInfo *pCreateInfo,
                                                 const VkAllocationCallbacks *pAllocator, VkFramebuffer *pFramebuffer) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00400);
        if (pCreateInfo) {
            if (pCreateInfo->pAttachments) {
                for (uint32_t idx0 = 0; idx0 < pCreateInfo->attachmentCount; ++idx0) {
                    skip_call |= ValidateObject(device, pCreateInfo->pAttachments[idx0], VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT,
                                                false, VALIDATION_ERROR_00420);
                }
            }
            if (pCreateInfo->renderPass) {
                skip_call |= ValidateObject(device, pCreateInfo->renderPass, VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT, false,
                                            VALIDATION_ERROR_00419);
            }
        }
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result =
        get_dispatch_table(ot_device_table_map, device)->CreateFramebuffer(device, pCreateInfo, pAllocator, pFramebuffer);
    {
        std::lock_guard<std::mutex> lock(global_lock);
        if (result == VK_SUCCESS) {
            CreateObject(device, *pFramebuffer, VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT, pAllocator);
        }
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyFramebuffer(VkDevice device, VkFramebuffer framebuffer, const VkAllocationCallbacks *pAllocator) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00425);
        skip_call |= ValidateObject(device, framebuffer, VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT, true, VALIDATION_ERROR_00426);
    }
    if (skip_call) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(global_lock);
        DestroyObject(device, framebuffer, VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT, pAllocator);
    }
    get_dispatch_table(ot_device_table_map, device)->DestroyFramebuffer(device, framebuffer, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL CreateRenderPass(VkDevice device, const VkRenderPassCreateInfo *pCreateInfo,
                                                const VkAllocationCallbacks *pAllocator, VkRenderPass *pRenderPass) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00319);
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result =
        get_dispatch_table(ot_device_table_map, device)->CreateRenderPass(device, pCreateInfo, pAllocator, pRenderPass);
    {
        std::lock_guard<std::mutex> lock(global_lock);
        if (result == VK_SUCCESS) {
            CreateObject(device, *pRenderPass, VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT, pAllocator);
        }
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyRenderPass(VkDevice device, VkRenderPass renderPass, const VkAllocationCallbacks *pAllocator) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00396);
        skip_call |= ValidateObject(device, renderPass, VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT, true, VALIDATION_ERROR_00397);
    }
    if (skip_call) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(global_lock);
        DestroyObject(device, renderPass, VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT, pAllocator);
    }
    get_dispatch_table(ot_device_table_map, device)->DestroyRenderPass(device, renderPass, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL GetRenderAreaGranularity(VkDevice device, VkRenderPass renderPass, VkExtent2D *pGranularity) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00449);
        skip_call |= ValidateObject(device, renderPass, VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT, false, VALIDATION_ERROR_00450);
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, device)->GetRenderAreaGranularity(device, renderPass, pGranularity);
}

VKAPI_ATTR VkResult VKAPI_CALL CreateCommandPool(VkDevice device, const VkCommandPoolCreateInfo *pCreateInfo,
                                                 const VkAllocationCallbacks *pAllocator, VkCommandPool *pCommandPool) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00064);
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result =
        get_dispatch_table(ot_device_table_map, device)->CreateCommandPool(device, pCreateInfo, pAllocator, pCommandPool);
    {
        std::lock_guard<std::mutex> lock(global_lock);
        if (result == VK_SUCCESS) {
            CreateObject(device, *pCommandPool, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT, pAllocator);
        }
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL ResetCommandPool(VkDevice device, VkCommandPool commandPool, VkCommandPoolResetFlags flags) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |=
            ValidateObject(device, commandPool, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT, false, VALIDATION_ERROR_00074);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00073);
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result = get_dispatch_table(ot_device_table_map, device)->ResetCommandPool(device, commandPool, flags);
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL BeginCommandBuffer(VkCommandBuffer command_buffer, const VkCommandBufferBeginInfo *begin_info) {
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(command_buffer), layer_data_map);
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(command_buffer, command_buffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false,
                                    VALIDATION_ERROR_00108);
        if (begin_info) {
            OBJTRACK_NODE *pNode =
                device_data->object_map[VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT][reinterpret_cast<const uint64_t>(command_buffer)];
            if ((begin_info->pInheritanceInfo) && (pNode->status & OBJSTATUS_COMMAND_BUFFER_SECONDARY)) {
                skip_call |= ValidateObject(command_buffer, begin_info->pInheritanceInfo->framebuffer,
                                                           VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT, true);
                skip_call |= ValidateObject(command_buffer, begin_info->pInheritanceInfo->renderPass,
                                                           VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT, true);
            }
        }
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result = get_dispatch_table(ot_device_table_map, command_buffer)->BeginCommandBuffer(command_buffer, begin_info);
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL EndCommandBuffer(VkCommandBuffer commandBuffer) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false,
                                    VALIDATION_ERROR_00125);
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result = get_dispatch_table(ot_device_table_map, commandBuffer)->EndCommandBuffer(commandBuffer);
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL ResetCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferResetFlags flags) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false,
                                    VALIDATION_ERROR_00090);
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result = get_dispatch_table(ot_device_table_map, commandBuffer)->ResetCommandBuffer(commandBuffer, flags);
    return result;
}

VKAPI_ATTR void VKAPI_CALL CmdBindPipeline(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint,
                                           VkPipeline pipeline) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false,
                                    VALIDATION_ERROR_00599);
        skip_call |=
            ValidateObject(commandBuffer, pipeline, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT, false, VALIDATION_ERROR_00601);
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, commandBuffer)->CmdBindPipeline(commandBuffer, pipelineBindPoint, pipeline);
}

VKAPI_ATTR void VKAPI_CALL CmdSetViewport(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount,
                                          const VkViewport *pViewports) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false,
                                    VALIDATION_ERROR_01443);
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, commandBuffer)->CmdSetViewport(commandBuffer, firstViewport, viewportCount, pViewports);
}

VKAPI_ATTR void VKAPI_CALL CmdSetScissor(VkCommandBuffer commandBuffer, uint32_t firstScissor, uint32_t scissorCount,
                                         const VkRect2D *pScissors) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false,
                                    VALIDATION_ERROR_01492);
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, commandBuffer)->CmdSetScissor(commandBuffer, firstScissor, scissorCount, pScissors);
}

VKAPI_ATTR void VKAPI_CALL CmdSetLineWidth(VkCommandBuffer commandBuffer, float lineWidth) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false,
                                    VALIDATION_ERROR_01478);
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, commandBuffer)->CmdSetLineWidth(commandBuffer, lineWidth);
}

VKAPI_ATTR void VKAPI_CALL CmdSetDepthBias(VkCommandBuffer commandBuffer, float depthBiasConstantFactor, float depthBiasClamp,
                                           float depthBiasSlopeFactor) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false,
                                    VALIDATION_ERROR_01483);
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, commandBuffer)
        ->CmdSetDepthBias(commandBuffer, depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor);
}

VKAPI_ATTR void VKAPI_CALL CmdSetBlendConstants(VkCommandBuffer commandBuffer, const float blendConstants[4]) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false,
                                    VALIDATION_ERROR_01551);
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, commandBuffer)->CmdSetBlendConstants(commandBuffer, blendConstants);
}

VKAPI_ATTR void VKAPI_CALL CmdSetDepthBounds(VkCommandBuffer commandBuffer, float minDepthBounds, float maxDepthBounds) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false,
                                    VALIDATION_ERROR_01507);
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, commandBuffer)->CmdSetDepthBounds(commandBuffer, minDepthBounds, maxDepthBounds);
}

VKAPI_ATTR void VKAPI_CALL CmdSetStencilCompareMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask,
                                                    uint32_t compareMask) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false,
                                    VALIDATION_ERROR_01515);
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, commandBuffer)->CmdSetStencilCompareMask(commandBuffer, faceMask, compareMask);
}

VKAPI_ATTR void VKAPI_CALL CmdSetStencilWriteMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t writeMask) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false,
                                    VALIDATION_ERROR_01521);
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, commandBuffer)->CmdSetStencilWriteMask(commandBuffer, faceMask, writeMask);
}

VKAPI_ATTR void VKAPI_CALL CmdSetStencilReference(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t reference) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false,
                                    VALIDATION_ERROR_01527);
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, commandBuffer)->CmdSetStencilReference(commandBuffer, faceMask, reference);
}

VKAPI_ATTR void VKAPI_CALL CmdBindDescriptorSets(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint,
                                                 VkPipelineLayout layout, uint32_t firstSet, uint32_t descriptorSetCount,
                                                 const VkDescriptorSet *pDescriptorSets, uint32_t dynamicOffsetCount,
                                                 const uint32_t *pDynamicOffsets) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false,
                                    VALIDATION_ERROR_00979);
        skip_call |=
            ValidateObject(commandBuffer, layout, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT, false, VALIDATION_ERROR_00981);
        if (pDescriptorSets) {
            for (uint32_t idx0 = 0; idx0 < descriptorSetCount; ++idx0) {
                skip_call |= ValidateObject(commandBuffer, pDescriptorSets[idx0],
                                                           VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, false);
            }
        }
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, commandBuffer)
        ->CmdBindDescriptorSets(commandBuffer, pipelineBindPoint, layout, firstSet, descriptorSetCount, pDescriptorSets,
                                dynamicOffsetCount, pDynamicOffsets);
}

VKAPI_ATTR void VKAPI_CALL CmdBindIndexBuffer(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset,
                                              VkIndexType indexType) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(commandBuffer, buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, false, VALIDATION_ERROR_01354);
        skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false,
                                    VALIDATION_ERROR_01353);
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, commandBuffer)->CmdBindIndexBuffer(commandBuffer, buffer, offset, indexType);
}

VKAPI_ATTR void VKAPI_CALL CmdBindVertexBuffers(VkCommandBuffer commandBuffer, uint32_t firstBinding, uint32_t bindingCount,
                                                const VkBuffer *pBuffers, const VkDeviceSize *pOffsets) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false,
                                    VALIDATION_ERROR_01419);
        if (pBuffers) {
            for (uint32_t idx0 = 0; idx0 < bindingCount; ++idx0) {
                skip_call |=
                    ValidateObject(commandBuffer, pBuffers[idx0], VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, false);
            }
        }
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, commandBuffer)
        ->CmdBindVertexBuffers(commandBuffer, firstBinding, bindingCount, pBuffers, pOffsets);
}

VKAPI_ATTR void VKAPI_CALL CmdDraw(VkCommandBuffer commandBuffer, uint32_t vertexCount, uint32_t instanceCount,
                                   uint32_t firstVertex, uint32_t firstInstance) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false,
                                    VALIDATION_ERROR_01362);
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, commandBuffer)
        ->CmdDraw(commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

VKAPI_ATTR void VKAPI_CALL CmdDrawIndexed(VkCommandBuffer commandBuffer, uint32_t indexCount, uint32_t instanceCount,
                                          uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false,
                                    VALIDATION_ERROR_01369);
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, commandBuffer)
        ->CmdDrawIndexed(commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

VKAPI_ATTR void VKAPI_CALL CmdDrawIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount,
                                           uint32_t stride) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(commandBuffer, buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, false, VALIDATION_ERROR_01378);
        skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false,
                                    VALIDATION_ERROR_01377);
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, commandBuffer)->CmdDrawIndirect(commandBuffer, buffer, offset, drawCount, stride);
}

VKAPI_ATTR void VKAPI_CALL CmdDrawIndexedIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset,
                                                  uint32_t drawCount, uint32_t stride) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(commandBuffer, buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, false, VALIDATION_ERROR_01390);
        skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false,
                                    VALIDATION_ERROR_01389);
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, commandBuffer)
        ->CmdDrawIndexedIndirect(commandBuffer, buffer, offset, drawCount, stride);
}

VKAPI_ATTR void VKAPI_CALL CmdDispatch(VkCommandBuffer commandBuffer, uint32_t x, uint32_t y, uint32_t z) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false,
                                    VALIDATION_ERROR_01559);
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, commandBuffer)->CmdDispatch(commandBuffer, x, y, z);
}

VKAPI_ATTR void VKAPI_CALL CmdDispatchIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(commandBuffer, buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, false, VALIDATION_ERROR_01566);
        skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false,
                                    VALIDATION_ERROR_01565);
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, commandBuffer)->CmdDispatchIndirect(commandBuffer, buffer, offset);
}

VKAPI_ATTR void VKAPI_CALL CmdCopyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer,
                                         uint32_t regionCount, const VkBufferCopy *pRegions) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false,
                                    VALIDATION_ERROR_01166);
        skip_call |=
            ValidateObject(commandBuffer, dstBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, false, VALIDATION_ERROR_01168);
        skip_call |=
            ValidateObject(commandBuffer, srcBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, false, VALIDATION_ERROR_01167);
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, commandBuffer)
        ->CmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, regionCount, pRegions);
}

VKAPI_ATTR void VKAPI_CALL CmdCopyImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout,
                                        VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount,
                                        const VkImageCopy *pRegions) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false,
                                    VALIDATION_ERROR_01186);
        skip_call |= ValidateObject(commandBuffer, dstImage, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, false, VALIDATION_ERROR_01189);
        skip_call |= ValidateObject(commandBuffer, srcImage, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, false, VALIDATION_ERROR_01187);
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, commandBuffer)
        ->CmdCopyImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
}

VKAPI_ATTR void VKAPI_CALL CmdBlitImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout,
                                        VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount,
                                        const VkImageBlit *pRegions, VkFilter filter) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false,
                                    VALIDATION_ERROR_01291);
        skip_call |= ValidateObject(commandBuffer, dstImage, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, false, VALIDATION_ERROR_01294);
        skip_call |= ValidateObject(commandBuffer, srcImage, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, false, VALIDATION_ERROR_01292);
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, commandBuffer)
        ->CmdBlitImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions, filter);
}

VKAPI_ATTR void VKAPI_CALL CmdCopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkImage dstImage,
                                                VkImageLayout dstImageLayout, uint32_t regionCount,
                                                const VkBufferImageCopy *pRegions) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false,
                                    VALIDATION_ERROR_01235);
        skip_call |= ValidateObject(commandBuffer, dstImage, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, false, VALIDATION_ERROR_01237);
        skip_call |=
            ValidateObject(commandBuffer, srcBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, false, VALIDATION_ERROR_01236);
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, commandBuffer)
        ->CmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, dstImageLayout, regionCount, pRegions);
}

VKAPI_ATTR void VKAPI_CALL CmdCopyImageToBuffer(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout,
                                                VkBuffer dstBuffer, uint32_t regionCount, const VkBufferImageCopy *pRegions) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false,
                                    VALIDATION_ERROR_01253);
        skip_call |=
            ValidateObject(commandBuffer, dstBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, false, VALIDATION_ERROR_01256);
        skip_call |= ValidateObject(commandBuffer, srcImage, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, false, VALIDATION_ERROR_01254);
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, commandBuffer)
        ->CmdCopyImageToBuffer(commandBuffer, srcImage, srcImageLayout, dstBuffer, regionCount, pRegions);
}

VKAPI_ATTR void VKAPI_CALL CmdUpdateBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset,
                                           VkDeviceSize dataSize, const uint32_t *pData) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false,
                                    VALIDATION_ERROR_01150);
        skip_call |=
            ValidateObject(commandBuffer, dstBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, false, VALIDATION_ERROR_01151);
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, commandBuffer)->CmdUpdateBuffer(commandBuffer, dstBuffer, dstOffset, dataSize, pData);
}

VKAPI_ATTR void VKAPI_CALL CmdFillBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset,
                                         VkDeviceSize size, uint32_t data) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false,
                                    VALIDATION_ERROR_01138);
        skip_call |=
            ValidateObject(commandBuffer, dstBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, false, VALIDATION_ERROR_01139);
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, commandBuffer)->CmdFillBuffer(commandBuffer, dstBuffer, dstOffset, size, data);
}

VKAPI_ATTR void VKAPI_CALL CmdClearColorImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout,
                                              const VkClearColorValue *pColor, uint32_t rangeCount,
                                              const VkImageSubresourceRange *pRanges) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false,
                                    VALIDATION_ERROR_01089);
        skip_call |= ValidateObject(commandBuffer, image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, false, VALIDATION_ERROR_01090);
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, commandBuffer)
        ->CmdClearColorImage(commandBuffer, image, imageLayout, pColor, rangeCount, pRanges);
}

VKAPI_ATTR void VKAPI_CALL CmdClearDepthStencilImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout,
                                                     const VkClearDepthStencilValue *pDepthStencil, uint32_t rangeCount,
                                                     const VkImageSubresourceRange *pRanges) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false,
                                    VALIDATION_ERROR_01104);
        skip_call |= ValidateObject(commandBuffer, image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, false, VALIDATION_ERROR_01105);
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, commandBuffer)
        ->CmdClearDepthStencilImage(commandBuffer, image, imageLayout, pDepthStencil, rangeCount, pRanges);
}

VKAPI_ATTR void VKAPI_CALL CmdClearAttachments(VkCommandBuffer commandBuffer, uint32_t attachmentCount,
                                               const VkClearAttachment *pAttachments, uint32_t rectCount,
                                               const VkClearRect *pRects) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false,
                                    VALIDATION_ERROR_01117);
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, commandBuffer)
        ->CmdClearAttachments(commandBuffer, attachmentCount, pAttachments, rectCount, pRects);
}

VKAPI_ATTR void VKAPI_CALL CmdResolveImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout,
                                           VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount,
                                           const VkImageResolve *pRegions) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false,
                                    VALIDATION_ERROR_01327);
        skip_call |= ValidateObject(commandBuffer, dstImage, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, false, VALIDATION_ERROR_01330);
        skip_call |= ValidateObject(commandBuffer, srcImage, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, false, VALIDATION_ERROR_01328);
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, commandBuffer)
        ->CmdResolveImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
}

VKAPI_ATTR void VKAPI_CALL CmdSetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false,
                                    VALIDATION_ERROR_00232);
        skip_call |= ValidateObject(commandBuffer, event, VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT, false, VALIDATION_ERROR_00233);
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, commandBuffer)->CmdSetEvent(commandBuffer, event, stageMask);
}

VKAPI_ATTR void VKAPI_CALL CmdResetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false,
                                    VALIDATION_ERROR_00243);
        skip_call |= ValidateObject(commandBuffer, event, VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT, false, VALIDATION_ERROR_00244);
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, commandBuffer)->CmdResetEvent(commandBuffer, event, stageMask);
}

VKAPI_ATTR void VKAPI_CALL CmdWaitEvents(VkCommandBuffer commandBuffer, uint32_t eventCount, const VkEvent *pEvents,
                                         VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
                                         uint32_t memoryBarrierCount, const VkMemoryBarrier *pMemoryBarriers,
                                         uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier *pBufferMemoryBarriers,
                                         uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier *pImageMemoryBarriers) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false,
                                    VALIDATION_ERROR_00252);
        if (pBufferMemoryBarriers) {
            for (uint32_t idx0 = 0; idx0 < bufferMemoryBarrierCount; ++idx0) {
                if (pBufferMemoryBarriers[idx0].buffer) {
                    skip_call |= ValidateObject(commandBuffer, pBufferMemoryBarriers[idx0].buffer,
                                                VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, false, VALIDATION_ERROR_00259);
                }
            }
        }
        if (pEvents) {
            for (uint32_t idx1 = 0; idx1 < eventCount; ++idx1) {
                skip_call |= ValidateObject(commandBuffer, pEvents[idx1], VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT, false,
                                            VALIDATION_ERROR_00253);
            }
        }
        if (pImageMemoryBarriers) {
            for (uint32_t idx2 = 0; idx2 < imageMemoryBarrierCount; ++idx2) {
                if (pImageMemoryBarriers[idx2].image) {
                    skip_call |= ValidateObject(commandBuffer, pImageMemoryBarriers[idx2].image,
                                                VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, false, VALIDATION_ERROR_00260);
                }
            }
        }
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, commandBuffer)
        ->CmdWaitEvents(commandBuffer, eventCount, pEvents, srcStageMask, dstStageMask, memoryBarrierCount, pMemoryBarriers,
                        bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
}

VKAPI_ATTR void VKAPI_CALL CmdPipelineBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask,
                                              VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags,
                                              uint32_t memoryBarrierCount, const VkMemoryBarrier *pMemoryBarriers,
                                              uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier *pBufferMemoryBarriers,
                                              uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier *pImageMemoryBarriers) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false,
                                    VALIDATION_ERROR_00270);
        if (pBufferMemoryBarriers) {
            for (uint32_t idx0 = 0; idx0 < bufferMemoryBarrierCount; ++idx0) {
                if (pBufferMemoryBarriers[idx0].buffer) {
                    skip_call |= ValidateObject(commandBuffer, pBufferMemoryBarriers[idx0].buffer,
                                                VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, false, VALIDATION_ERROR_00277);
                }
            }
        }
        if (pImageMemoryBarriers) {
            for (uint32_t idx1 = 0; idx1 < imageMemoryBarrierCount; ++idx1) {
                if (pImageMemoryBarriers[idx1].image) {
                    skip_call |= ValidateObject(commandBuffer, pImageMemoryBarriers[idx1].image,
                                                VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, false, VALIDATION_ERROR_00278);
                }
            }
        }
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, commandBuffer)
        ->CmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, dependencyFlags, memoryBarrierCount, pMemoryBarriers,
                             bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
}

VKAPI_ATTR void VKAPI_CALL CmdBeginQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query,
                                         VkQueryControlFlags flags) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false,
                                    VALIDATION_ERROR_01035);
        skip_call |=
            ValidateObject(commandBuffer, queryPool, VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT, false, VALIDATION_ERROR_01036);
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, commandBuffer)->CmdBeginQuery(commandBuffer, queryPool, query, flags);
}

VKAPI_ATTR void VKAPI_CALL CmdEndQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false,
                                    VALIDATION_ERROR_01043);
        skip_call |=
            ValidateObject(commandBuffer, queryPool, VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT, false, VALIDATION_ERROR_01044);
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, commandBuffer)->CmdEndQuery(commandBuffer, queryPool, query);
}

VKAPI_ATTR void VKAPI_CALL CmdResetQueryPool(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery,
                                             uint32_t queryCount) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false,
                                    VALIDATION_ERROR_01021);
        skip_call |=
            ValidateObject(commandBuffer, queryPool, VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT, false, VALIDATION_ERROR_01022);
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, commandBuffer)->CmdResetQueryPool(commandBuffer, queryPool, firstQuery, queryCount);
}

VKAPI_ATTR void VKAPI_CALL CmdWriteTimestamp(VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage,
                                             VkQueryPool queryPool, uint32_t query) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false,
                                    VALIDATION_ERROR_01078);
        skip_call |=
            ValidateObject(commandBuffer, queryPool, VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT, false, VALIDATION_ERROR_01080);
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, commandBuffer)->CmdWriteTimestamp(commandBuffer, pipelineStage, queryPool, query);
}

VKAPI_ATTR void VKAPI_CALL CmdCopyQueryPoolResults(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery,
                                                   uint32_t queryCount, VkBuffer dstBuffer, VkDeviceSize dstOffset,
                                                   VkDeviceSize stride, VkQueryResultFlags flags) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false,
                                    VALIDATION_ERROR_01068);
        skip_call |=
            ValidateObject(commandBuffer, dstBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, false, VALIDATION_ERROR_01070);
        skip_call |=
            ValidateObject(commandBuffer, queryPool, VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT, false, VALIDATION_ERROR_01069);
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, commandBuffer)
        ->CmdCopyQueryPoolResults(commandBuffer, queryPool, firstQuery, queryCount, dstBuffer, dstOffset, stride, flags);
}

VKAPI_ATTR void VKAPI_CALL CmdPushConstants(VkCommandBuffer commandBuffer, VkPipelineLayout layout, VkShaderStageFlags stageFlags,
                                            uint32_t offset, uint32_t size, const void *pValues) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false,
                                    VALIDATION_ERROR_00993);
        skip_call |=
            ValidateObject(commandBuffer, layout, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT, false, VALIDATION_ERROR_00994);
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, commandBuffer)
        ->CmdPushConstants(commandBuffer, layout, stageFlags, offset, size, pValues);
}

VKAPI_ATTR void VKAPI_CALL CmdBeginRenderPass(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo *pRenderPassBegin,
                                              VkSubpassContents contents) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false,
                                    VALIDATION_ERROR_00435);
        if (pRenderPassBegin) {
            skip_call |= ValidateObject(commandBuffer, pRenderPassBegin->framebuffer,
                                                       VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT, false);
            skip_call |= ValidateObject(commandBuffer, pRenderPassBegin->renderPass,
                                                       VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT, false);
        }
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, commandBuffer)->CmdBeginRenderPass(commandBuffer, pRenderPassBegin, contents);
}

VKAPI_ATTR void VKAPI_CALL CmdNextSubpass(VkCommandBuffer commandBuffer, VkSubpassContents contents) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false,
                                    VALIDATION_ERROR_00454);
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, commandBuffer)->CmdNextSubpass(commandBuffer, contents);
}

VKAPI_ATTR void VKAPI_CALL CmdEndRenderPass(VkCommandBuffer commandBuffer) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false,
                                    VALIDATION_ERROR_00461);
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, commandBuffer)->CmdEndRenderPass(commandBuffer);
}

VKAPI_ATTR void VKAPI_CALL CmdExecuteCommands(VkCommandBuffer commandBuffer, uint32_t commandBufferCount,
                                              const VkCommandBuffer *pCommandBuffers) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false,
                                    VALIDATION_ERROR_00159);
        if (pCommandBuffers) {
            for (uint32_t idx0 = 0; idx0 < commandBufferCount; ++idx0) {
                skip_call |= ValidateObject(commandBuffer, pCommandBuffers[idx0], VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                                            false, VALIDATION_ERROR_00160);
            }
        }
    }
    if (skip_call) {
        return;
    }
    get_dispatch_table(ot_device_table_map, commandBuffer)->CmdExecuteCommands(commandBuffer, commandBufferCount, pCommandBuffers);
}

VKAPI_ATTR void VKAPI_CALL DestroySurfaceKHR(VkInstance instance, VkSurfaceKHR surface, const VkAllocationCallbacks *pAllocator) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(instance, instance, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT, false);
        skip_call |= ValidateObject(instance, surface, VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT, false);
    }
    if (skip_call) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(global_lock);
        DestroyObject(instance, surface, VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT, pAllocator);
    }
    get_dispatch_table(ot_instance_table_map, instance)->DestroySurfaceKHR(instance, surface, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL GetPhysicalDeviceSurfaceSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex,
                                                                  VkSurfaceKHR surface, VkBool32 *pSupported) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |=
            ValidateObject(physicalDevice, physicalDevice, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, false);
        skip_call |= ValidateObject(physicalDevice, surface, VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT, false);
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result = get_dispatch_table(ot_instance_table_map, physicalDevice)
                          ->GetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIndex, surface, pSupported);
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL GetPhysicalDeviceSurfaceCapabilitiesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
                                                                       VkSurfaceCapabilitiesKHR *pSurfaceCapabilities) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |=
            ValidateObject(physicalDevice, physicalDevice, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, false);
        skip_call |= ValidateObject(physicalDevice, surface, VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT, false);
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result = get_dispatch_table(ot_instance_table_map, physicalDevice)
                          ->GetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, pSurfaceCapabilities);
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL GetPhysicalDeviceSurfaceFormatsKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
                                                                  uint32_t *pSurfaceFormatCount,
                                                                  VkSurfaceFormatKHR *pSurfaceFormats) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |=
            ValidateObject(physicalDevice, physicalDevice, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, false);
        skip_call |= ValidateObject(physicalDevice, surface, VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT, false);
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result = get_dispatch_table(ot_instance_table_map, physicalDevice)
                          ->GetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, pSurfaceFormatCount, pSurfaceFormats);
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL GetPhysicalDeviceSurfacePresentModesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
                                                                       uint32_t *pPresentModeCount,
                                                                       VkPresentModeKHR *pPresentModes) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |=
            ValidateObject(physicalDevice, physicalDevice, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, false);
        skip_call |= ValidateObject(physicalDevice, surface, VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT, false);
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result = get_dispatch_table(ot_instance_table_map, physicalDevice)
                          ->GetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, pPresentModeCount, pPresentModes);
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR *pCreateInfo,
                                                  const VkAllocationCallbacks *pAllocator, VkSwapchainKHR *pSwapchain) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
        if (pCreateInfo) {
            skip_call |= ValidateObject(device, pCreateInfo->oldSwapchain,
                                                       VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT, true);
            layer_data *device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
            skip_call |= ValidateObject(device_data->physical_device, pCreateInfo->surface,
                                                       VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT, false);
        }
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result =
        get_dispatch_table(ot_device_table_map, device)->CreateSwapchainKHR(device, pCreateInfo, pAllocator, pSwapchain);
    {
        std::lock_guard<std::mutex> lock(global_lock);
        if (result == VK_SUCCESS) {
            CreateObject(device, *pSwapchain, VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT, pAllocator);
        }
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL AcquireNextImageKHR(VkDevice device, VkSwapchainKHR swapchain, uint64_t timeout,
                                                   VkSemaphore semaphore, VkFence fence, uint32_t *pImageIndex) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
        skip_call |= ValidateObject(device, fence, VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT, true);
        skip_call |= ValidateObject(device, semaphore, VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT, true);
        skip_call |= ValidateObject(device, swapchain, VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT, false);
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result = get_dispatch_table(ot_device_table_map, device)
                          ->AcquireNextImageKHR(device, swapchain, timeout, semaphore, fence, pImageIndex);
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL QueuePresentKHR(VkQueue queue, const VkPresentInfoKHR *pPresentInfo) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        if (pPresentInfo) {
            if (pPresentInfo->pSwapchains) {
                for (uint32_t idx0 = 0; idx0 < pPresentInfo->swapchainCount; ++idx0) {
                    skip_call |= ValidateObject(queue, pPresentInfo->pSwapchains[idx0],
                                                               VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT, false);
                }
            }
            if (pPresentInfo->pWaitSemaphores) {
                for (uint32_t idx1 = 0; idx1 < pPresentInfo->waitSemaphoreCount; ++idx1) {
                    skip_call |= ValidateObject(queue, pPresentInfo->pWaitSemaphores[idx1],
                                                               VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT, false);
                }
            }
        }
        skip_call |= ValidateObject(queue, queue, VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT, false);
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result = get_dispatch_table(ot_device_table_map, queue)->QueuePresentKHR(queue, pPresentInfo);
    return result;
}

#ifdef VK_USE_PLATFORM_WIN32_KHR
VKAPI_ATTR VkResult VKAPI_CALL CreateWin32SurfaceKHR(VkInstance instance, const VkWin32SurfaceCreateInfoKHR *pCreateInfo,
                                                     const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(instance, instance, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT, false);
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result =
        get_dispatch_table(ot_instance_table_map, instance)->CreateWin32SurfaceKHR(instance, pCreateInfo, pAllocator, pSurface);
    {
        std::lock_guard<std::mutex> lock(global_lock);
        if (result == VK_SUCCESS) {
            CreateObject(instance, *pSurface, VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT, pAllocator);
        }
    }
    return result;
}

VKAPI_ATTR VkBool32 VKAPI_CALL GetPhysicalDeviceWin32PresentationSupportKHR(VkPhysicalDevice physicalDevice,
                                                                            uint32_t queueFamilyIndex) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |=
            ValidateObject(physicalDevice, physicalDevice, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, false);
    }
    if (skip_call) {
        return VK_FALSE;
    }
    VkBool32 result = get_dispatch_table(ot_instance_table_map, physicalDevice)
                          ->GetPhysicalDeviceWin32PresentationSupportKHR(physicalDevice, queueFamilyIndex);
    return result;
}
#endif // VK_USE_PLATFORM_WIN32_KHR

#ifdef VK_USE_PLATFORM_XCB_KHR
VKAPI_ATTR VkResult VKAPI_CALL CreateXcbSurfaceKHR(VkInstance instance, const VkXcbSurfaceCreateInfoKHR *pCreateInfo,
                                                   const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(instance, instance, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT, false);
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result =
        get_dispatch_table(ot_instance_table_map, instance)->CreateXcbSurfaceKHR(instance, pCreateInfo, pAllocator, pSurface);
    {
        std::lock_guard<std::mutex> lock(global_lock);
        if (result == VK_SUCCESS) {
            CreateObject(instance, *pSurface, VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT, pAllocator);
        }
    }
    return result;
}

VKAPI_ATTR VkBool32 VKAPI_CALL GetPhysicalDeviceXcbPresentationSupportKHR(VkPhysicalDevice physicalDevice,
                                                                          uint32_t queueFamilyIndex, xcb_connection_t *connection,
                                                                          xcb_visualid_t visual_id) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |=
            ValidateObject(physicalDevice, physicalDevice, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, false);
    }
    if (skip_call) {
        return VK_FALSE;
    }
    VkBool32 result = get_dispatch_table(ot_instance_table_map, physicalDevice)
                          ->GetPhysicalDeviceXcbPresentationSupportKHR(physicalDevice, queueFamilyIndex, connection, visual_id);
    return result;
}
#endif // VK_USE_PLATFORM_XCB_KHR

#ifdef VK_USE_PLATFORM_XLIB_KHR
VKAPI_ATTR VkResult VKAPI_CALL CreateXlibSurfaceKHR(VkInstance instance, const VkXlibSurfaceCreateInfoKHR *pCreateInfo,
                                                    const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(instance, instance, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT, false);
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result =
        get_dispatch_table(ot_instance_table_map, instance)->CreateXlibSurfaceKHR(instance, pCreateInfo, pAllocator, pSurface);
    {
        std::lock_guard<std::mutex> lock(global_lock);
        if (result == VK_SUCCESS) {
            CreateObject(instance, *pSurface, VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT, pAllocator);
        }
    }
    return result;
}

VKAPI_ATTR VkBool32 VKAPI_CALL GetPhysicalDeviceXlibPresentationSupportKHR(VkPhysicalDevice physicalDevice,
                                                                           uint32_t queueFamilyIndex, Display *dpy,
                                                                           VisualID visualID) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |=
            ValidateObject(physicalDevice, physicalDevice, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, false);
    }
    if (skip_call) {
        return VK_FALSE;
    }
    VkBool32 result = get_dispatch_table(ot_instance_table_map, physicalDevice)
                          ->GetPhysicalDeviceXlibPresentationSupportKHR(physicalDevice, queueFamilyIndex, dpy, visualID);
    return result;
}
#endif // VK_USE_PLATFORM_XLIB_KHR

#ifdef VK_USE_PLATFORM_MIR_KHR
VKAPI_ATTR VkResult VKAPI_CALL CreateMirSurfaceKHR(VkInstance instance, const VkMirSurfaceCreateInfoKHR *pCreateInfo,
                                                   const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(instance, instance, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT, false);
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result =
        get_dispatch_table(ot_instance_table_map, instance)->CreateMirSurfaceKHR(instance, pCreateInfo, pAllocator, pSurface);
    {
        std::lock_guard<std::mutex> lock(global_lock);
        if (result == VK_SUCCESS) {
            CreateObject(instance, *pSurface, VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT, pAllocator);
        }
    }
    return result;
}

VKAPI_ATTR VkBool32 VKAPI_CALL GetPhysicalDeviceMirPresentationSupportKHR(VkPhysicalDevice physicalDevice,
                                                                          uint32_t queueFamilyIndex, MirConnection *connection) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |=
            ValidateObject(physicalDevice, physicalDevice, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, false);
    }
    if (skip_call) {
        return VK_FALSE;
    }
    VkBool32 result = get_dispatch_table(ot_instance_table_map, physicalDevice)
                          ->GetPhysicalDeviceMirPresentationSupportKHR(physicalDevice, queueFamilyIndex, connection);
    return result;
}
#endif // VK_USE_PLATFORM_MIR_KHR

#ifdef VK_USE_PLATFORM_WAYLAND_KHR
VKAPI_ATTR VkResult VKAPI_CALL CreateWaylandSurfaceKHR(VkInstance instance, const VkWaylandSurfaceCreateInfoKHR *pCreateInfo,
                                                       const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(instance, instance, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT, false);
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result =
        get_dispatch_table(ot_instance_table_map, instance)->CreateWaylandSurfaceKHR(instance, pCreateInfo, pAllocator, pSurface);
    {
        std::lock_guard<std::mutex> lock(global_lock);
        if (result == VK_SUCCESS) {
            CreateObject(instance, *pSurface, VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT, pAllocator);
        }
    }
    return result;
}

VKAPI_ATTR VkBool32 VKAPI_CALL GetPhysicalDeviceWaylandPresentationSupportKHR(VkPhysicalDevice physicalDevice,
                                                                              uint32_t queueFamilyIndex,
                                                                              struct wl_display *display) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |=
            ValidateObject(physicalDevice, physicalDevice, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, false);
    }
    if (skip_call) {
        return VK_FALSE;
    }
    VkBool32 result = get_dispatch_table(ot_instance_table_map, physicalDevice)
                          ->GetPhysicalDeviceWaylandPresentationSupportKHR(physicalDevice, queueFamilyIndex, display);
    return result;
}
#endif // VK_USE_PLATFORM_WAYLAND_KHR

#ifdef VK_USE_PLATFORM_ANDROID_KHR
VKAPI_ATTR VkResult VKAPI_CALL CreateAndroidSurfaceKHR(VkInstance instance, const VkAndroidSurfaceCreateInfoKHR *pCreateInfo,
                                                       const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(instance, instance, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT, false);
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result =
        get_dispatch_table(ot_instance_table_map, instance)->CreateAndroidSurfaceKHR(instance, pCreateInfo, pAllocator, pSurface);
    {
        std::lock_guard<std::mutex> lock(global_lock);
        if (result == VK_SUCCESS) {
            CreateObject(instance, *pSurface, VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT, pAllocator);
        }
    }
    return result;
}
#endif // VK_USE_PLATFORM_ANDROID_KHR

VKAPI_ATTR VkResult VKAPI_CALL CreateSharedSwapchainsKHR(VkDevice device, uint32_t swapchainCount,
                                                         const VkSwapchainCreateInfoKHR *pCreateInfos,
                                                         const VkAllocationCallbacks *pAllocator, VkSwapchainKHR *pSwapchains) {
    bool skip_call = false;
    uint32_t i = 0;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
        if (NULL != pCreateInfos) {
            for (i = 0; i < swapchainCount; i++) {
                skip_call |= ValidateObject(device, pCreateInfos[i].oldSwapchain,
                                                           VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT, true);
                layer_data *device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
                skip_call |= ValidateObject(device_data->physical_device, pCreateInfos[i].surface,
                                                           VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT, false);
            }
        }
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result =
        get_dispatch_table(ot_device_table_map, device)->CreateSharedSwapchainsKHR(device, swapchainCount, pCreateInfos, pAllocator, pSwapchains);
    {
        std::lock_guard<std::mutex> lock(global_lock);
        if (result == VK_SUCCESS) {
            for (i = 0; i < swapchainCount; i++) {
                CreateObject(device, pSwapchains[i], VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT, pAllocator);
            }
        }
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateDebugReportCallbackEXT(VkInstance instance,
                                                            const VkDebugReportCallbackCreateInfoEXT *pCreateInfo,
                                                            const VkAllocationCallbacks *pAllocator,
                                                            VkDebugReportCallbackEXT *pCallback) {
    VkLayerInstanceDispatchTable *pInstanceTable = get_dispatch_table(ot_instance_table_map, instance);
    VkResult result = pInstanceTable->CreateDebugReportCallbackEXT(instance, pCreateInfo, pAllocator, pCallback);
    if (VK_SUCCESS == result) {
        layer_data *instance_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
        result = layer_create_msg_callback(instance_data->report_data, false, pCreateInfo, pAllocator, pCallback);
        CreateObject(instance, *pCallback, VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_EXT, pAllocator);
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT msgCallback,
                                                         const VkAllocationCallbacks *pAllocator) {
    VkLayerInstanceDispatchTable *pInstanceTable = get_dispatch_table(ot_instance_table_map, instance);
    pInstanceTable->DestroyDebugReportCallbackEXT(instance, msgCallback, pAllocator);
    layer_data *instance_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    layer_destroy_msg_callback(instance_data->report_data, msgCallback, pAllocator);
    DestroyObject(instance, msgCallback, VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_EXT, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL DebugReportMessageEXT(VkInstance instance, VkDebugReportFlagsEXT flags,
                                                 VkDebugReportObjectTypeEXT objType, uint64_t object, size_t location,
                                                 int32_t msgCode, const char *pLayerPrefix, const char *pMsg) {
    VkLayerInstanceDispatchTable *pInstanceTable = get_dispatch_table(ot_instance_table_map, instance);
    pInstanceTable->DebugReportMessageEXT(instance, flags, objType, object, location, msgCode, pLayerPrefix, pMsg);
}

static const VkExtensionProperties instance_extensions[] = {{VK_EXT_DEBUG_REPORT_EXTENSION_NAME, VK_EXT_DEBUG_REPORT_SPEC_VERSION}};

static const VkLayerProperties globalLayerProps = {"VK_LAYER_LUNARG_object_tracker",
                                                   VK_LAYER_API_VERSION, // specVersion
                                                   1,                    // implementationVersion
                                                   "LunarG Validation Layer"};

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
        return util_GetExtensionProperties(1, instance_extensions, pCount, pProperties);

    return VK_ERROR_LAYER_NOT_PRESENT;
}

VKAPI_ATTR VkResult VKAPI_CALL EnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice, const char *pLayerName,
                                                                  uint32_t *pCount, VkExtensionProperties *pProperties) {
    if (pLayerName && !strcmp(pLayerName, globalLayerProps.layerName))
        return util_GetExtensionProperties(0, nullptr, pCount, pProperties);

    assert(physicalDevice);
    VkLayerInstanceDispatchTable *pTable = get_dispatch_table(ot_instance_table_map, physicalDevice);
    return pTable->EnumerateDeviceExtensionProperties(physicalDevice, NULL, pCount, pProperties);
}

static inline PFN_vkVoidFunction InterceptMsgCallbackGetProcAddrCommand(const char *name, VkInstance instance) {
    layer_data *instance_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    return debug_report_get_instance_proc_addr(instance_data->report_data, name);
}

static inline PFN_vkVoidFunction InterceptWsiEnabledCommand(const char *name, VkInstance instance) {
    VkLayerInstanceDispatchTable *pTable = get_dispatch_table(ot_instance_table_map, instance);
    if (instanceExtMap.size() == 0 || !instanceExtMap[pTable].wsi_enabled)
        return nullptr;

    if (!strcmp("vkDestroySurfaceKHR", name))
        return reinterpret_cast<PFN_vkVoidFunction>(DestroySurfaceKHR);
    if (!strcmp("vkGetPhysicalDeviceSurfaceSupportKHR", name))
        return reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceSurfaceSupportKHR);
    if (!strcmp("vkGetPhysicalDeviceSurfaceCapabilitiesKHR", name))
        return reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceSurfaceCapabilitiesKHR);
    if (!strcmp("vkGetPhysicalDeviceSurfaceFormatsKHR", name))
        return reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceSurfaceFormatsKHR);
    if (!strcmp("vkGetPhysicalDeviceSurfacePresentModesKHR", name))
        return reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceSurfacePresentModesKHR);

#ifdef VK_USE_PLATFORM_WIN32_KHR
    if ((instanceExtMap[pTable].win32_enabled == true) && !strcmp("vkCreateWin32SurfaceKHR", name))
        return reinterpret_cast<PFN_vkVoidFunction>(CreateWin32SurfaceKHR);
    if ((instanceExtMap[pTable].win32_enabled == true) && !strcmp("vkGetPhysicalDeviceWin32PresentationSupportKHR", name))
        return reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceWin32PresentationSupportKHR);
#endif // VK_USE_PLATFORM_WIN32_KHR
#ifdef VK_USE_PLATFORM_XCB_KHR
    if ((instanceExtMap[pTable].xcb_enabled == true) && !strcmp("vkCreateXcbSurfaceKHR", name))
        return reinterpret_cast<PFN_vkVoidFunction>(CreateXcbSurfaceKHR);
    if ((instanceExtMap[pTable].xcb_enabled == true) && !strcmp("vkGetPhysicalDeviceXcbPresentationSupportKHR", name))
        return reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceXcbPresentationSupportKHR);
#endif // VK_USE_PLATFORM_XCB_KHR
#ifdef VK_USE_PLATFORM_XLIB_KHR
    if ((instanceExtMap[pTable].xlib_enabled == true) && !strcmp("vkCreateXlibSurfaceKHR", name))
        return reinterpret_cast<PFN_vkVoidFunction>(CreateXlibSurfaceKHR);
    if ((instanceExtMap[pTable].xlib_enabled == true) && !strcmp("vkGetPhysicalDeviceXlibPresentationSupportKHR", name))
        return reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceXlibPresentationSupportKHR);
#endif // VK_USE_PLATFORM_XLIB_KHR
#ifdef VK_USE_PLATFORM_MIR_KHR
    if ((instanceExtMap[pTable].mir_enabled == true) && !strcmp("vkCreateMirSurfaceKHR", name))
        return reinterpret_cast<PFN_vkVoidFunction>(CreateMirSurfaceKHR);
    if ((instanceExtMap[pTable].mir_enabled == true) && !strcmp("vkGetPhysicalDeviceMirPresentationSupportKHR", name))
        return reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceMirPresentationSupportKHR);
#endif // VK_USE_PLATFORM_MIR_KHR
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    if ((instanceExtMap[pTable].wayland_enabled == true) && !strcmp("vkCreateWaylandSurfaceKHR", name))
        return reinterpret_cast<PFN_vkVoidFunction>(CreateWaylandSurfaceKHR);
    if ((instanceExtMap[pTable].wayland_enabled == true) && !strcmp("vkGetPhysicalDeviceWaylandPresentationSupportKHR", name))
        return reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceWaylandPresentationSupportKHR);
#endif // VK_USE_PLATFORM_WAYLAND_KHR
#ifdef VK_USE_PLATFORM_ANDROID_KHR
    if ((instanceExtMap[pTable].android_enabled == true) && !strcmp("vkCreateAndroidSurfaceKHR", name))
        return reinterpret_cast<PFN_vkVoidFunction>(CreateAndroidSurfaceKHR);
#endif // VK_USE_PLATFORM_ANDROID_KHR

    return nullptr;
}

static void CheckDeviceRegisterExtensions(const VkDeviceCreateInfo *pCreateInfo, VkDevice device) {
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    device_data->wsi_enabled = false;
    device_data->wsi_display_swapchain_enabled = false;
    device_data->objtrack_extensions_enabled = false;

    for (uint32_t i = 0; i < pCreateInfo->enabledExtensionCount; i++) {
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
            device_data->wsi_enabled = true;
        }
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_DISPLAY_SWAPCHAIN_EXTENSION_NAME) == 0) {
            device_data->wsi_display_swapchain_enabled = true;
        }
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], "OBJTRACK_EXTENSIONS") == 0) {
            device_data->objtrack_extensions_enabled = true;
        }
    }
}

static void CheckInstanceRegisterExtensions(const VkInstanceCreateInfo *pCreateInfo, VkInstance instance) {
    VkLayerInstanceDispatchTable *pDisp = get_dispatch_table(ot_instance_table_map, instance);
 

    instanceExtMap[pDisp] = {};

    for (uint32_t i = 0; i < pCreateInfo->enabledExtensionCount; i++) {
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_SURFACE_EXTENSION_NAME) == 0) {
            instanceExtMap[pDisp].wsi_enabled = true;
        }
#ifdef VK_USE_PLATFORM_XLIB_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_XLIB_SURFACE_EXTENSION_NAME) == 0) {
            instanceExtMap[pDisp].xlib_enabled = true;
        }
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_XCB_SURFACE_EXTENSION_NAME) == 0) {
            instanceExtMap[pDisp].xcb_enabled = true;
        }
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME) == 0) {
            instanceExtMap[pDisp].wayland_enabled = true;
        }
#endif
#ifdef VK_USE_PLATFORM_MIR_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_MIR_SURFACE_EXTENSION_NAME) == 0) {
            instanceExtMap[pDisp].mir_enabled = true;
        }
#endif
#ifdef VK_USE_PLATFORM_ANDROID_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_ANDROID_SURFACE_EXTENSION_NAME) == 0) {
            instanceExtMap[pDisp].android_enabled = true;
        }
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_WIN32_SURFACE_EXTENSION_NAME) == 0) {
            instanceExtMap[pDisp].win32_enabled = true;
        }
#endif
    }
}

VKAPI_ATTR VkResult VKAPI_CALL CreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo *pCreateInfo,
                                            const VkAllocationCallbacks *pAllocator, VkDevice *pDevice) {
    std::lock_guard<std::mutex> lock(global_lock);
    layer_data *phy_dev_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    VkLayerDeviceCreateInfo *chain_info = get_chain_info(pCreateInfo, VK_LAYER_LINK_INFO);

    assert(chain_info->u.pLayerInfo);
    PFN_vkGetInstanceProcAddr fpGetInstanceProcAddr = chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    PFN_vkGetDeviceProcAddr fpGetDeviceProcAddr = chain_info->u.pLayerInfo->pfnNextGetDeviceProcAddr;
    PFN_vkCreateDevice fpCreateDevice = (PFN_vkCreateDevice)fpGetInstanceProcAddr(phy_dev_data->instance, "vkCreateDevice");
    if (fpCreateDevice == NULL) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    // Advance the link info for the next element on the chain
    chain_info->u.pLayerInfo = chain_info->u.pLayerInfo->pNext;

    VkResult result = fpCreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);
    if (result != VK_SUCCESS) {
        return result;
    }

    layer_data *device_data = get_my_data_ptr(get_dispatch_key(*pDevice), layer_data_map);
    device_data->report_data = layer_debug_report_create_device(phy_dev_data->report_data, *pDevice);

    // Add link back to physDev
    device_data->physical_device = physicalDevice;

    initDeviceTable(*pDevice, fpGetDeviceProcAddr, ot_device_table_map);

    CheckDeviceRegisterExtensions(pCreateInfo, *pDevice);
    CreateObject(*pDevice, *pDevice, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, pAllocator);

    return result;
}

VKAPI_ATTR void VKAPI_CALL GetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice physicalDevice,
                                                                  uint32_t *pQueueFamilyPropertyCount,
                                                                  VkQueueFamilyProperties *pQueueFamilyProperties) {
    get_dispatch_table(ot_instance_table_map, physicalDevice)
        ->GetPhysicalDeviceQueueFamilyProperties(physicalDevice, pQueueFamilyPropertyCount, pQueueFamilyProperties);
    std::lock_guard<std::mutex> lock(global_lock);
    if (pQueueFamilyProperties != NULL) {
        layer_data *instance_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
        for (uint32_t i = 0; i < *pQueueFamilyPropertyCount; i++) {
            instance_data->queue_family_properties.emplace_back(pQueueFamilyProperties[i]);
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
    initInstanceTable(*pInstance, fpGetInstanceProcAddr, ot_instance_table_map);
    VkLayerInstanceDispatchTable *pInstanceTable = get_dispatch_table(ot_instance_table_map, *pInstance);

    // Look for one or more debug report create info structures, and copy the
    // callback(s) for each one found (for use by vkDestroyInstance)
    layer_copy_tmp_callbacks(pCreateInfo->pNext, &instance_data->num_tmp_callbacks, &instance_data->tmp_dbg_create_infos,
                             &instance_data->tmp_callbacks);

    instance_data->report_data = debug_report_create_instance(pInstanceTable, *pInstance, pCreateInfo->enabledExtensionCount,
                                                              pCreateInfo->ppEnabledExtensionNames);

    InitObjectTracker(instance_data, pAllocator);
    CheckInstanceRegisterExtensions(pCreateInfo, *pInstance);

    CreateObject(*pInstance, *pInstance, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT, pAllocator);

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL EnumeratePhysicalDevices(VkInstance instance, uint32_t *pPhysicalDeviceCount,
                                                        VkPhysicalDevice *pPhysicalDevices) {
    bool skip_call = VK_FALSE;
    std::unique_lock<std::mutex> lock(global_lock);
    skip_call |= ValidateObject(instance, instance, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT, false, VALIDATION_ERROR_00023);
    lock.unlock();
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result = get_dispatch_table(ot_instance_table_map, instance)
                          ->EnumeratePhysicalDevices(instance, pPhysicalDeviceCount, pPhysicalDevices);
    lock.lock();
    if (result == VK_SUCCESS) {
        if (pPhysicalDevices) {
            for (uint32_t i = 0; i < *pPhysicalDeviceCount; i++) {
                CreateObject(instance, pPhysicalDevices[i], VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, nullptr);
            }
        }
    }
    lock.unlock();
    return result;
}

VKAPI_ATTR void VKAPI_CALL GetDeviceQueue(VkDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, VkQueue *pQueue) {
    std::unique_lock<std::mutex> lock(global_lock);
    ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00062);
    lock.unlock();

    get_dispatch_table(ot_device_table_map, device)->GetDeviceQueue(device, queueFamilyIndex, queueIndex, pQueue);

    lock.lock();

    CreateQueue(device, *pQueue, VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT);
    AddQueueInfo(device, queueFamilyIndex, *pQueue);
}

VKAPI_ATTR void VKAPI_CALL FreeMemory(VkDevice device, VkDeviceMemory memory, const VkAllocationCallbacks *pAllocator) {
    bool skip = false;
    std::unique_lock<std::mutex> lock(global_lock);
    skip |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00621);
    skip |= ValidateObject(device, memory, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, true, VALIDATION_ERROR_00622);
    lock.unlock();
    if (!skip) {
        get_dispatch_table(ot_device_table_map, device)->FreeMemory(device, memory, pAllocator);

        lock.lock();
        DestroyObject(device, memory, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, pAllocator);
    }
}

VKAPI_ATTR VkResult VKAPI_CALL MapMemory(VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size,
                                         VkMemoryMapFlags flags, void **ppData) {
    bool skip_call = VK_FALSE;
    std::unique_lock<std::mutex> lock(global_lock);
    skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00631);
    lock.unlock();
    if (skip_call == VK_TRUE) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result = get_dispatch_table(ot_device_table_map, device)->MapMemory(device, memory, offset, size, flags, ppData);
    return result;
}

VKAPI_ATTR void VKAPI_CALL UnmapMemory(VkDevice device, VkDeviceMemory memory) {
    bool skip_call = VK_FALSE;
    std::unique_lock<std::mutex> lock(global_lock);
    skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00650);
    lock.unlock();
    if (skip_call == VK_TRUE) {
        return;
    }

    get_dispatch_table(ot_device_table_map, device)->UnmapMemory(device, memory);
}
VKAPI_ATTR VkResult VKAPI_CALL QueueBindSparse(VkQueue queue, uint32_t bindInfoCount, const VkBindSparseInfo *pBindInfo,
                                               VkFence fence) {
    std::unique_lock<std::mutex> lock(global_lock);
    ValidateQueueFlags(queue, "QueueBindSparse");

    for (uint32_t i = 0; i < bindInfoCount; i++) {
        for (uint32_t j = 0; j < pBindInfo[i].bufferBindCount; j++)
            ValidateObject(queue, pBindInfo[i].pBufferBinds[j].buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT,
                                          false);
        for (uint32_t j = 0; j < pBindInfo[i].imageOpaqueBindCount; j++)
            ValidateObject(queue, pBindInfo[i].pImageOpaqueBinds[j].image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                                          false);
        for (uint32_t j = 0; j < pBindInfo[i].imageBindCount; j++)
            ValidateObject(queue, pBindInfo[i].pImageBinds[j].image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, false);
    }
    lock.unlock();

    VkResult result = get_dispatch_table(ot_device_table_map, queue)->QueueBindSparse(queue, bindInfoCount, pBindInfo, fence);
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL AllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo *pAllocateInfo,
                                                      VkCommandBuffer *pCommandBuffers) {
    bool skip_call = VK_FALSE;
    std::unique_lock<std::mutex> lock(global_lock);
    skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00084);
    skip_call |= ValidateObject(device, pAllocateInfo->commandPool, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT, false,
                                VALIDATION_ERROR_00090);
    lock.unlock();

    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }

    VkResult result =
        get_dispatch_table(ot_device_table_map, device)->AllocateCommandBuffers(device, pAllocateInfo, pCommandBuffers);

    lock.lock();
    for (uint32_t i = 0; i < pAllocateInfo->commandBufferCount; i++) {
        AllocateCommandBuffer(device, pAllocateInfo->commandPool, pCommandBuffers[i],
                              VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, pAllocateInfo->level);
    }
    lock.unlock();

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL AllocateDescriptorSets(VkDevice device, const VkDescriptorSetAllocateInfo *pAllocateInfo,
                                                      VkDescriptorSet *pDescriptorSets) {
    bool skip_call = VK_FALSE;
    std::unique_lock<std::mutex> lock(global_lock);
    skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00908);
    skip_call |= ValidateObject(device, pAllocateInfo->descriptorPool,
                                               VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT, false);
    for (uint32_t i = 0; i < pAllocateInfo->descriptorSetCount; i++) {
        skip_call |= ValidateObject(device, pAllocateInfo->pSetLayouts[i],
                                                   VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT, false);
    }
    lock.unlock();
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }

    VkResult result =
        get_dispatch_table(ot_device_table_map, device)->AllocateDescriptorSets(device, pAllocateInfo, pDescriptorSets);

    if (VK_SUCCESS == result) {
        lock.lock();
        for (uint32_t i = 0; i < pAllocateInfo->descriptorSetCount; i++) {
            AllocateDescriptorSet(device, pAllocateInfo->descriptorPool, pDescriptorSets[i],
                                  VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT);
        }
        lock.unlock();
    }

    return result;
}

VKAPI_ATTR void VKAPI_CALL FreeCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t commandBufferCount,
                                              const VkCommandBuffer *pCommandBuffers) {
    bool skip_call = false;
    std::unique_lock<std::mutex> lock(global_lock);
    ValidateObject(device, commandPool, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT, false, VALIDATION_ERROR_00099);
    ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00098);
    for (uint32_t i = 0; i < commandBufferCount; i++) {
        skip_call |= ValidateCommandBuffer(device, commandPool, pCommandBuffers[i]);
    }

    for (uint32_t i = 0; i < commandBufferCount; i++) {
        DestroyObject(device, pCommandBuffers[i], VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, nullptr);
    }

    lock.unlock();
    if (!skip_call) {
        get_dispatch_table(ot_device_table_map, device)
            ->FreeCommandBuffers(device, commandPool, commandBufferCount, pCommandBuffers);
    }
}
VKAPI_ATTR void VKAPI_CALL DestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain, const VkAllocationCallbacks *pAllocator) {
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    // A swapchain's images are implicitly deleted when the swapchain is deleted.
    // Remove this swapchain's images from our map of such images.
    std::unordered_map<uint64_t, OBJTRACK_NODE *>::iterator itr = device_data->swapchainImageMap.begin();
    while (itr != device_data->swapchainImageMap.end()) {
        OBJTRACK_NODE *pNode = (*itr).second;
        if (pNode->parent_object == reinterpret_cast<uint64_t &>(swapchain)) {
            delete pNode;
            auto delete_item = itr++;
            device_data->swapchainImageMap.erase(delete_item);
        } else {
            ++itr;
        }
    }
    DestroyObject(device, swapchain, VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT, pAllocator);
    lock.unlock();

    get_dispatch_table(ot_device_table_map, device)->DestroySwapchainKHR(device, swapchain, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL FreeDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, uint32_t descriptorSetCount,
                                                  const VkDescriptorSet *pDescriptorSets) {
    bool skip_call = false;
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    std::unique_lock<std::mutex> lock(global_lock);
    skip_call |=
        ValidateObject(device, descriptorPool, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT, false, VALIDATION_ERROR_00924);
    skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00923);
    for (uint32_t i = 0; i < descriptorSetCount; i++) {
        skip_call |= ValidateDescriptorSet(device, descriptorPool, pDescriptorSets[i]);
    }

    for (uint32_t i = 0; i < descriptorSetCount; i++) {
        DestroyObject(device, pDescriptorSets[i], VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, nullptr);
    }

    lock.unlock();
    if (!skip_call) {
        result = get_dispatch_table(ot_device_table_map, device)
                     ->FreeDescriptorSets(device, descriptorPool, descriptorSetCount, pDescriptorSets);
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool,
                                                 const VkAllocationCallbacks *pAllocator) {
    bool skip_call = VK_FALSE;
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00904);
    skip_call |=
        ValidateObject(device, descriptorPool, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT, true, VALIDATION_ERROR_00905);
    lock.unlock();
    if (skip_call) {
        return;
    }
    // A DescriptorPool's descriptor sets are implicitly deleted when the pool is deleted.
    // Remove this pool's descriptor sets from our descriptorSet map.
    lock.lock();
    std::unordered_map<uint64_t, OBJTRACK_NODE *>::iterator itr =
        device_data->object_map[VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT].begin();
    while (itr != device_data->object_map[VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT].end()) {
        OBJTRACK_NODE *pNode = (*itr).second;
        auto del_itr = itr++;
        if (pNode->parent_object == reinterpret_cast<uint64_t &>(descriptorPool)) {
            DestroyObject(device, (VkDescriptorSet)((*del_itr).first),
                                         VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, nullptr);
        }
    }
    DestroyObject(device, descriptorPool, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT, pAllocator);
    lock.unlock();
    get_dispatch_table(ot_device_table_map, device)->DestroyDescriptorPool(device, descriptorPool, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL DestroyCommandPool(VkDevice device, VkCommandPool commandPool, const VkAllocationCallbacks *pAllocator) {
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    bool skip_call = false;
    std::unique_lock<std::mutex> lock(global_lock);
    skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00080);
    skip_call |= ValidateObject(device, commandPool, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT, true, VALIDATION_ERROR_00081);
    lock.unlock();
    if (skip_call) {
        return;
    }
    lock.lock();
    // A CommandPool's command buffers are implicitly deleted when the pool is deleted.
    // Remove this pool's cmdBuffers from our cmd buffer map.
    auto itr = device_data->object_map[VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT].begin();
    auto del_itr = itr;
    while (itr != device_data->object_map[VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT].end()) {
        OBJTRACK_NODE *pNode = (*itr).second;
        del_itr = itr++;
        if (pNode->parent_object == reinterpret_cast<uint64_t &>(commandPool)) {
            skip_call |= ValidateCommandBuffer(device, commandPool, reinterpret_cast<VkCommandBuffer>((*del_itr).first));
            DestroyObject(device, reinterpret_cast<VkCommandBuffer>((*del_itr).first),
                                      VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, nullptr);
        }
    }
    DestroyObject(device, commandPool, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT, pAllocator);
    lock.unlock();
    get_dispatch_table(ot_device_table_map, device)->DestroyCommandPool(device, commandPool, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL GetSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain, uint32_t *pSwapchainImageCount,
                                                     VkImage *pSwapchainImages) {
    bool skip_call = VK_FALSE;
    std::unique_lock<std::mutex> lock(global_lock);
    skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    lock.unlock();
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result = get_dispatch_table(ot_device_table_map, device)
                          ->GetSwapchainImagesKHR(device, swapchain, pSwapchainImageCount, pSwapchainImages);
    if (pSwapchainImages != NULL) {
        lock.lock();
        for (uint32_t i = 0; i < *pSwapchainImageCount; i++) {
            CreateSwapchainImageObject(device, pSwapchainImages[i], swapchain);
        }
        lock.unlock();
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount,
                                                       const VkGraphicsPipelineCreateInfo *pCreateInfos,
                                                       const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines) {
    bool skip_call = VK_FALSE;
    std::unique_lock<std::mutex> lock(global_lock);
    skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00519);
    if (pCreateInfos) {
        for (uint32_t idx0 = 0; idx0 < createInfoCount; ++idx0) {
            if (pCreateInfos[idx0].basePipelineHandle) {
                skip_call |= ValidateObject(device, pCreateInfos[idx0].basePipelineHandle,
                                                           VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT, true);
            }
            if (pCreateInfos[idx0].layout) {
                skip_call |= ValidateObject(device, pCreateInfos[idx0].layout,
                                                           VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT, false);
            }
            if (pCreateInfos[idx0].pStages) {
                for (uint32_t idx1 = 0; idx1 < pCreateInfos[idx0].stageCount; ++idx1) {
                    if (pCreateInfos[idx0].pStages[idx1].module) {
                        skip_call |= ValidateObject(device, pCreateInfos[idx0].pStages[idx1].module,
                                                                   VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, false);
                    }
                }
            }
            if (pCreateInfos[idx0].renderPass) {
                skip_call |= ValidateObject(device, pCreateInfos[idx0].renderPass,
                                                           VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT, false);
            }
        }
    }
    if (pipelineCache) {
        skip_call |=
            ValidateObject(device, pipelineCache, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT, true, VALIDATION_ERROR_00520);
    }
    lock.unlock();
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result = get_dispatch_table(ot_device_table_map, device)
                          ->CreateGraphicsPipelines(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
    lock.lock();
    if (result == VK_SUCCESS) {
        for (uint32_t idx2 = 0; idx2 < createInfoCount; ++idx2) {
            CreateObject(device, pPipelines[idx2], VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT, pAllocator);
        }
    }
    lock.unlock();
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateComputePipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount,
                                                      const VkComputePipelineCreateInfo *pCreateInfos,
                                                      const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines) {
    bool skip_call = VK_FALSE;
    std::unique_lock<std::mutex> lock(global_lock);
    skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false, VALIDATION_ERROR_00486);
    if (pCreateInfos) {
        for (uint32_t idx0 = 0; idx0 < createInfoCount; ++idx0) {
            if (pCreateInfos[idx0].basePipelineHandle) {
                skip_call |= ValidateObject(device, pCreateInfos[idx0].basePipelineHandle,
                                                           VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT, true);
            }
            if (pCreateInfos[idx0].layout) {
                skip_call |= ValidateObject(device, pCreateInfos[idx0].layout,
                                                           VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT, false);
            }
            if (pCreateInfos[idx0].stage.module) {
                skip_call |= ValidateObject(device, pCreateInfos[idx0].stage.module,
                                                           VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, false);
            }
        }
    }
    if (pipelineCache) {
        skip_call |=
            ValidateObject(device, pipelineCache, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT, true, VALIDATION_ERROR_00487);
    }
    lock.unlock();
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result = get_dispatch_table(ot_device_table_map, device)
                          ->CreateComputePipelines(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
    lock.lock();
    if (result == VK_SUCCESS) {
        for (uint32_t idx1 = 0; idx1 < createInfoCount; ++idx1) {
            CreateObject(device, pPipelines[idx1], VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT, pAllocator);
        }
    }
    lock.unlock();
    return result;
}

// VK_EXT_debug_marker Extension
VKAPI_ATTR VkResult VKAPI_CALL DebugMarkerSetObjectTagEXT(VkDevice device, VkDebugMarkerObjectTagInfoEXT *pTagInfo) {
    bool skip_call = VK_FALSE;
    std::unique_lock<std::mutex> lock(global_lock);
    skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    lock.unlock();
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result = get_dispatch_table(ot_device_table_map, device)->DebugMarkerSetObjectTagEXT(device, pTagInfo);
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL DebugMarkerSetObjectNameEXT(VkDevice device, VkDebugMarkerObjectNameInfoEXT *pNameInfo) {
    bool skip_call = VK_FALSE;
    std::unique_lock<std::mutex> lock(global_lock);
    skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    lock.unlock();
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result = get_dispatch_table(ot_device_table_map, device)->DebugMarkerSetObjectNameEXT(device, pNameInfo);
    return result;
}

VKAPI_ATTR void VKAPI_CALL CmdDebugMarkerBeginEXT(VkCommandBuffer commandBuffer, VkDebugMarkerMarkerInfoEXT *pMarkerInfo) {
    bool skip_call = VK_FALSE;
    std::unique_lock<std::mutex> lock(global_lock);
    skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
    lock.unlock();
    if (!skip_call) {
        get_dispatch_table(ot_device_table_map, commandBuffer)->CmdDebugMarkerBeginEXT(commandBuffer, pMarkerInfo);
    }
}

VKAPI_ATTR void VKAPI_CALL CmdDebugMarkerEndEXT(VkCommandBuffer commandBuffer) {
    bool skip_call = VK_FALSE;
    std::unique_lock<std::mutex> lock(global_lock);
    skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
    lock.unlock();
    if (!skip_call) {
        get_dispatch_table(ot_device_table_map, commandBuffer)->CmdDebugMarkerEndEXT(commandBuffer);
    }
}

VKAPI_ATTR void VKAPI_CALL CmdDebugMarkerInsertEXT(VkCommandBuffer commandBuffer, VkDebugMarkerMarkerInfoEXT *pMarkerInfo) {
    bool skip_call = VK_FALSE;
    std::unique_lock<std::mutex> lock(global_lock);
    skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
    lock.unlock();
    if (!skip_call) {
        get_dispatch_table(ot_device_table_map, commandBuffer)->CmdDebugMarkerInsertEXT(commandBuffer, pMarkerInfo);
    }
}

// VK_NV_external_memory_capabilities Extension
VKAPI_ATTR VkResult VKAPI_CALL GetPhysicalDeviceExternalImageFormatPropertiesNV(
    VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage,
    VkImageCreateFlags flags, VkExternalMemoryHandleTypeFlagsNV externalHandleType,
    VkExternalImageFormatPropertiesNV *pExternalImageFormatProperties) {

    bool skip_call = false;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip_call |= ValidateObject(physicalDevice, physicalDevice, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, false);
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result = get_dispatch_table(ot_instance_table_map, physicalDevice)
                          ->GetPhysicalDeviceExternalImageFormatPropertiesNV(physicalDevice, format, type, tiling, usage, flags,
                                                                             externalHandleType, pExternalImageFormatProperties);
    return result;
}

#ifdef VK_USE_PLATFORM_WIN32_KHR
// VK_NV_external_memory_win32 Extension
VKAPI_ATTR VkResult VKAPI_CALL GetMemoryWin32HandleNV(VkDevice device, VkDeviceMemory memory,
                                                      VkExternalMemoryHandleTypeFlagsNV handleType, HANDLE *pHandle) {
    bool skip_call = VK_FALSE;
    std::unique_lock<std::mutex> lock(global_lock);
    skip_call |= ValidateObject(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    skip_call |= ValidateObject(device, memory, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, false);
    lock.unlock();
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result = get_dispatch_table(ot_device_table_map, device)->GetMemoryWin32HandleNV(device, memory, handleType, pHandle);
    return result;
}
#endif // VK_USE_PLATFORM_WIN32_KHR

// VK_AMD_draw_indirect_count Extension
VKAPI_ATTR void VKAPI_CALL CmdDrawIndirectCountAMD(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset,
                                                   VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount,
                                                   uint32_t stride) {
    bool skip_call = VK_FALSE;
    std::unique_lock<std::mutex> lock(global_lock);
    skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    skip_call |= ValidateObject(commandBuffer, buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, false);
    lock.unlock();
    if (!skip_call) {
        get_dispatch_table(ot_device_table_map, commandBuffer)
            ->CmdDrawIndirectCountAMD(commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
    }
}

VKAPI_ATTR void VKAPI_CALL CmdDrawIndexedIndirectCountAMD(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset,
                                                          VkBuffer countBuffer, VkDeviceSize countBufferOffset,
                                                          uint32_t maxDrawCount, uint32_t stride) {
    bool skip_call = VK_FALSE;
    std::unique_lock<std::mutex> lock(global_lock);
    skip_call |= ValidateObject(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    skip_call |= ValidateObject(commandBuffer, buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, false);
    lock.unlock();
    if (!skip_call) {
        get_dispatch_table(ot_device_table_map, commandBuffer)
            ->CmdDrawIndexedIndirectCountAMD(commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
    }
}


static inline PFN_vkVoidFunction InterceptCoreDeviceCommand(const char *name) {
    if (!name || name[0] != 'v' || name[1] != 'k')
        return NULL;

    name += 2;
    if (!strcmp(name, "GetDeviceProcAddr"))
        return (PFN_vkVoidFunction)GetDeviceProcAddr;
    if (!strcmp(name, "DestroyDevice"))
        return (PFN_vkVoidFunction)DestroyDevice;
    if (!strcmp(name, "GetDeviceQueue"))
        return (PFN_vkVoidFunction)GetDeviceQueue;
    if (!strcmp(name, "QueueSubmit"))
        return (PFN_vkVoidFunction)QueueSubmit;
    if (!strcmp(name, "QueueWaitIdle"))
        return (PFN_vkVoidFunction)QueueWaitIdle;
    if (!strcmp(name, "DeviceWaitIdle"))
        return (PFN_vkVoidFunction)DeviceWaitIdle;
    if (!strcmp(name, "AllocateMemory"))
        return (PFN_vkVoidFunction)AllocateMemory;
    if (!strcmp(name, "FreeMemory"))
        return (PFN_vkVoidFunction)FreeMemory;
    if (!strcmp(name, "MapMemory"))
        return (PFN_vkVoidFunction)MapMemory;
    if (!strcmp(name, "UnmapMemory"))
        return (PFN_vkVoidFunction)UnmapMemory;
    if (!strcmp(name, "FlushMappedMemoryRanges"))
        return (PFN_vkVoidFunction)FlushMappedMemoryRanges;
    if (!strcmp(name, "InvalidateMappedMemoryRanges"))
        return (PFN_vkVoidFunction)InvalidateMappedMemoryRanges;
    if (!strcmp(name, "GetDeviceMemoryCommitment"))
        return (PFN_vkVoidFunction)GetDeviceMemoryCommitment;
    if (!strcmp(name, "BindBufferMemory"))
        return (PFN_vkVoidFunction)BindBufferMemory;
    if (!strcmp(name, "BindImageMemory"))
        return (PFN_vkVoidFunction)BindImageMemory;
    if (!strcmp(name, "GetBufferMemoryRequirements"))
        return (PFN_vkVoidFunction)GetBufferMemoryRequirements;
    if (!strcmp(name, "GetImageMemoryRequirements"))
        return (PFN_vkVoidFunction)GetImageMemoryRequirements;
    if (!strcmp(name, "GetImageSparseMemoryRequirements"))
        return (PFN_vkVoidFunction)GetImageSparseMemoryRequirements;
    if (!strcmp(name, "QueueBindSparse"))
        return (PFN_vkVoidFunction)QueueBindSparse;
    if (!strcmp(name, "CreateFence"))
        return (PFN_vkVoidFunction)CreateFence;
    if (!strcmp(name, "DestroyFence"))
        return (PFN_vkVoidFunction)DestroyFence;
    if (!strcmp(name, "ResetFences"))
        return (PFN_vkVoidFunction)ResetFences;
    if (!strcmp(name, "GetFenceStatus"))
        return (PFN_vkVoidFunction)GetFenceStatus;
    if (!strcmp(name, "WaitForFences"))
        return (PFN_vkVoidFunction)WaitForFences;
    if (!strcmp(name, "CreateSemaphore"))
        return (PFN_vkVoidFunction)CreateSemaphore;
    if (!strcmp(name, "DestroySemaphore"))
        return (PFN_vkVoidFunction)DestroySemaphore;
    if (!strcmp(name, "CreateEvent"))
        return (PFN_vkVoidFunction)CreateEvent;
    if (!strcmp(name, "DestroyEvent"))
        return (PFN_vkVoidFunction)DestroyEvent;
    if (!strcmp(name, "GetEventStatus"))
        return (PFN_vkVoidFunction)GetEventStatus;
    if (!strcmp(name, "SetEvent"))
        return (PFN_vkVoidFunction)SetEvent;
    if (!strcmp(name, "ResetEvent"))
        return (PFN_vkVoidFunction)ResetEvent;
    if (!strcmp(name, "CreateQueryPool"))
        return (PFN_vkVoidFunction)CreateQueryPool;
    if (!strcmp(name, "DestroyQueryPool"))
        return (PFN_vkVoidFunction)DestroyQueryPool;
    if (!strcmp(name, "GetQueryPoolResults"))
        return (PFN_vkVoidFunction)GetQueryPoolResults;
    if (!strcmp(name, "CreateBuffer"))
        return (PFN_vkVoidFunction)CreateBuffer;
    if (!strcmp(name, "DestroyBuffer"))
        return (PFN_vkVoidFunction)DestroyBuffer;
    if (!strcmp(name, "CreateBufferView"))
        return (PFN_vkVoidFunction)CreateBufferView;
    if (!strcmp(name, "DestroyBufferView"))
        return (PFN_vkVoidFunction)DestroyBufferView;
    if (!strcmp(name, "CreateImage"))
        return (PFN_vkVoidFunction)CreateImage;
    if (!strcmp(name, "DestroyImage"))
        return (PFN_vkVoidFunction)DestroyImage;
    if (!strcmp(name, "GetImageSubresourceLayout"))
        return (PFN_vkVoidFunction)GetImageSubresourceLayout;
    if (!strcmp(name, "CreateImageView"))
        return (PFN_vkVoidFunction)CreateImageView;
    if (!strcmp(name, "DestroyImageView"))
        return (PFN_vkVoidFunction)DestroyImageView;
    if (!strcmp(name, "CreateShaderModule"))
        return (PFN_vkVoidFunction)CreateShaderModule;
    if (!strcmp(name, "DestroyShaderModule"))
        return (PFN_vkVoidFunction)DestroyShaderModule;
    if (!strcmp(name, "CreatePipelineCache"))
        return (PFN_vkVoidFunction)CreatePipelineCache;
    if (!strcmp(name, "DestroyPipelineCache"))
        return (PFN_vkVoidFunction)DestroyPipelineCache;
    if (!strcmp(name, "GetPipelineCacheData"))
        return (PFN_vkVoidFunction)GetPipelineCacheData;
    if (!strcmp(name, "MergePipelineCaches"))
        return (PFN_vkVoidFunction)MergePipelineCaches;
    if (!strcmp(name, "CreateGraphicsPipelines"))
        return (PFN_vkVoidFunction)CreateGraphicsPipelines;
    if (!strcmp(name, "CreateComputePipelines"))
        return (PFN_vkVoidFunction)CreateComputePipelines;
    if (!strcmp(name, "DestroyPipeline"))
        return (PFN_vkVoidFunction)DestroyPipeline;
    if (!strcmp(name, "CreatePipelineLayout"))
        return (PFN_vkVoidFunction)CreatePipelineLayout;
    if (!strcmp(name, "DestroyPipelineLayout"))
        return (PFN_vkVoidFunction)DestroyPipelineLayout;
    if (!strcmp(name, "CreateSampler"))
        return (PFN_vkVoidFunction)CreateSampler;
    if (!strcmp(name, "DestroySampler"))
        return (PFN_vkVoidFunction)DestroySampler;
    if (!strcmp(name, "CreateDescriptorSetLayout"))
        return (PFN_vkVoidFunction)CreateDescriptorSetLayout;
    if (!strcmp(name, "DestroyDescriptorSetLayout"))
        return (PFN_vkVoidFunction)DestroyDescriptorSetLayout;
    if (!strcmp(name, "CreateDescriptorPool"))
        return (PFN_vkVoidFunction)CreateDescriptorPool;
    if (!strcmp(name, "DestroyDescriptorPool"))
        return (PFN_vkVoidFunction)DestroyDescriptorPool;
    if (!strcmp(name, "ResetDescriptorPool"))
        return (PFN_vkVoidFunction)ResetDescriptorPool;
    if (!strcmp(name, "AllocateDescriptorSets"))
        return (PFN_vkVoidFunction)AllocateDescriptorSets;
    if (!strcmp(name, "FreeDescriptorSets"))
        return (PFN_vkVoidFunction)FreeDescriptorSets;
    if (!strcmp(name, "UpdateDescriptorSets"))
        return (PFN_vkVoidFunction)UpdateDescriptorSets;
    if (!strcmp(name, "CreateFramebuffer"))
        return (PFN_vkVoidFunction)CreateFramebuffer;
    if (!strcmp(name, "DestroyFramebuffer"))
        return (PFN_vkVoidFunction)DestroyFramebuffer;
    if (!strcmp(name, "CreateRenderPass"))
        return (PFN_vkVoidFunction)CreateRenderPass;
    if (!strcmp(name, "DestroyRenderPass"))
        return (PFN_vkVoidFunction)DestroyRenderPass;
    if (!strcmp(name, "GetRenderAreaGranularity"))
        return (PFN_vkVoidFunction)GetRenderAreaGranularity;
    if (!strcmp(name, "CreateCommandPool"))
        return (PFN_vkVoidFunction)CreateCommandPool;
    if (!strcmp(name, "DestroyCommandPool"))
        return (PFN_vkVoidFunction)DestroyCommandPool;
    if (!strcmp(name, "ResetCommandPool"))
        return (PFN_vkVoidFunction)ResetCommandPool;
    if (!strcmp(name, "AllocateCommandBuffers"))
        return (PFN_vkVoidFunction)AllocateCommandBuffers;
    if (!strcmp(name, "FreeCommandBuffers"))
        return (PFN_vkVoidFunction)FreeCommandBuffers;
    if (!strcmp(name, "BeginCommandBuffer"))
        return (PFN_vkVoidFunction)BeginCommandBuffer;
    if (!strcmp(name, "EndCommandBuffer"))
        return (PFN_vkVoidFunction)EndCommandBuffer;
    if (!strcmp(name, "ResetCommandBuffer"))
        return (PFN_vkVoidFunction)ResetCommandBuffer;
    if (!strcmp(name, "CmdBindPipeline"))
        return (PFN_vkVoidFunction)CmdBindPipeline;
    if (!strcmp(name, "CmdSetViewport"))
        return (PFN_vkVoidFunction)CmdSetViewport;
    if (!strcmp(name, "CmdSetScissor"))
        return (PFN_vkVoidFunction)CmdSetScissor;
    if (!strcmp(name, "CmdSetLineWidth"))
        return (PFN_vkVoidFunction)CmdSetLineWidth;
    if (!strcmp(name, "CmdSetDepthBias"))
        return (PFN_vkVoidFunction)CmdSetDepthBias;
    if (!strcmp(name, "CmdSetBlendConstants"))
        return (PFN_vkVoidFunction)CmdSetBlendConstants;
    if (!strcmp(name, "CmdSetDepthBounds"))
        return (PFN_vkVoidFunction)CmdSetDepthBounds;
    if (!strcmp(name, "CmdSetStencilCompareMask"))
        return (PFN_vkVoidFunction)CmdSetStencilCompareMask;
    if (!strcmp(name, "CmdSetStencilWriteMask"))
        return (PFN_vkVoidFunction)CmdSetStencilWriteMask;
    if (!strcmp(name, "CmdSetStencilReference"))
        return (PFN_vkVoidFunction)CmdSetStencilReference;
    if (!strcmp(name, "CmdBindDescriptorSets"))
        return (PFN_vkVoidFunction)CmdBindDescriptorSets;
    if (!strcmp(name, "CmdBindIndexBuffer"))
        return (PFN_vkVoidFunction)CmdBindIndexBuffer;
    if (!strcmp(name, "CmdBindVertexBuffers"))
        return (PFN_vkVoidFunction)CmdBindVertexBuffers;
    if (!strcmp(name, "CmdDraw"))
        return (PFN_vkVoidFunction)CmdDraw;
    if (!strcmp(name, "CmdDrawIndexed"))
        return (PFN_vkVoidFunction)CmdDrawIndexed;
    if (!strcmp(name, "CmdDrawIndirect"))
        return (PFN_vkVoidFunction)CmdDrawIndirect;
    if (!strcmp(name, "CmdDrawIndexedIndirect"))
        return (PFN_vkVoidFunction)CmdDrawIndexedIndirect;
    if (!strcmp(name, "CmdDispatch"))
        return (PFN_vkVoidFunction)CmdDispatch;
    if (!strcmp(name, "CmdDispatchIndirect"))
        return (PFN_vkVoidFunction)CmdDispatchIndirect;
    if (!strcmp(name, "CmdCopyBuffer"))
        return (PFN_vkVoidFunction)CmdCopyBuffer;
    if (!strcmp(name, "CmdCopyImage"))
        return (PFN_vkVoidFunction)CmdCopyImage;
    if (!strcmp(name, "CmdBlitImage"))
        return (PFN_vkVoidFunction)CmdBlitImage;
    if (!strcmp(name, "CmdCopyBufferToImage"))
        return (PFN_vkVoidFunction)CmdCopyBufferToImage;
    if (!strcmp(name, "CmdCopyImageToBuffer"))
        return (PFN_vkVoidFunction)CmdCopyImageToBuffer;
    if (!strcmp(name, "CmdUpdateBuffer"))
        return (PFN_vkVoidFunction)CmdUpdateBuffer;
    if (!strcmp(name, "CmdFillBuffer"))
        return (PFN_vkVoidFunction)CmdFillBuffer;
    if (!strcmp(name, "CmdClearColorImage"))
        return (PFN_vkVoidFunction)CmdClearColorImage;
    if (!strcmp(name, "CmdClearDepthStencilImage"))
        return (PFN_vkVoidFunction)CmdClearDepthStencilImage;
    if (!strcmp(name, "CmdClearAttachments"))
        return (PFN_vkVoidFunction)CmdClearAttachments;
    if (!strcmp(name, "CmdResolveImage"))
        return (PFN_vkVoidFunction)CmdResolveImage;
    if (!strcmp(name, "CmdSetEvent"))
        return (PFN_vkVoidFunction)CmdSetEvent;
    if (!strcmp(name, "CmdResetEvent"))
        return (PFN_vkVoidFunction)CmdResetEvent;
    if (!strcmp(name, "CmdWaitEvents"))
        return (PFN_vkVoidFunction)CmdWaitEvents;
    if (!strcmp(name, "CmdPipelineBarrier"))
        return (PFN_vkVoidFunction)CmdPipelineBarrier;
    if (!strcmp(name, "CmdBeginQuery"))
        return (PFN_vkVoidFunction)CmdBeginQuery;
    if (!strcmp(name, "CmdEndQuery"))
        return (PFN_vkVoidFunction)CmdEndQuery;
    if (!strcmp(name, "CmdResetQueryPool"))
        return (PFN_vkVoidFunction)CmdResetQueryPool;
    if (!strcmp(name, "CmdWriteTimestamp"))
        return (PFN_vkVoidFunction)CmdWriteTimestamp;
    if (!strcmp(name, "CmdCopyQueryPoolResults"))
        return (PFN_vkVoidFunction)CmdCopyQueryPoolResults;
    if (!strcmp(name, "CmdPushConstants"))
        return (PFN_vkVoidFunction)CmdPushConstants;
    if (!strcmp(name, "CmdBeginRenderPass"))
        return (PFN_vkVoidFunction)CmdBeginRenderPass;
    if (!strcmp(name, "CmdNextSubpass"))
        return (PFN_vkVoidFunction)CmdNextSubpass;
    if (!strcmp(name, "CmdEndRenderPass"))
        return (PFN_vkVoidFunction)CmdEndRenderPass;
    if (!strcmp(name, "CmdExecuteCommands"))
        return (PFN_vkVoidFunction)CmdExecuteCommands;
    if (!strcmp(name, "DebugMarkerSetObjectTagEXT"))
        return (PFN_vkVoidFunction)DebugMarkerSetObjectTagEXT;
    if (!strcmp(name, "DebugMarkerSetObjectNameEXT"))
        return (PFN_vkVoidFunction)DebugMarkerSetObjectNameEXT;
    if (!strcmp(name, "CmdDebugMarkerBeginEXT"))
        return (PFN_vkVoidFunction)CmdDebugMarkerBeginEXT;
    if (!strcmp(name, "CmdDebugMarkerEndEXT"))
        return (PFN_vkVoidFunction)CmdDebugMarkerEndEXT;
    if (!strcmp(name, "CmdDebugMarkerInsertEXT"))
        return (PFN_vkVoidFunction)CmdDebugMarkerInsertEXT;
#ifdef VK_USE_PLATFORM_WIN32_KHR
    if (!strcmp(name, "GetMemoryWin32HandleNV"))
        return (PFN_vkVoidFunction)GetMemoryWin32HandleNV;
#endif // VK_USE_PLATFORM_WIN32_KHR
    if (!strcmp(name, "CmdDrawIndirectCountAMD"))
        return (PFN_vkVoidFunction)CmdDrawIndirectCountAMD;
    if (!strcmp(name, "CmdDrawIndexedIndirectCountAMD"))
        return (PFN_vkVoidFunction)CmdDrawIndexedIndirectCountAMD;

    return NULL;
}
static inline PFN_vkVoidFunction InterceptCoreInstanceCommand(const char *name) {
    if (!name || name[0] != 'v' || name[1] != 'k')
        return NULL;

    name += 2;
    if (!strcmp(name, "CreateInstance"))
        return (PFN_vkVoidFunction)CreateInstance;
    if (!strcmp(name, "DestroyInstance"))
        return (PFN_vkVoidFunction)DestroyInstance;
    if (!strcmp(name, "EnumeratePhysicalDevices"))
        return (PFN_vkVoidFunction)EnumeratePhysicalDevices;
    if (!strcmp(name, "GetPhysicalDeviceFeatures"))
        return (PFN_vkVoidFunction)GetPhysicalDeviceFeatures;
    if (!strcmp(name, "GetPhysicalDeviceFormatProperties"))
        return (PFN_vkVoidFunction)GetPhysicalDeviceFormatProperties;
    if (!strcmp(name, "GetPhysicalDeviceImageFormatProperties"))
        return (PFN_vkVoidFunction)GetPhysicalDeviceImageFormatProperties;
    if (!strcmp(name, "GetPhysicalDeviceProperties"))
        return (PFN_vkVoidFunction)GetPhysicalDeviceProperties;
    if (!strcmp(name, "GetPhysicalDeviceQueueFamilyProperties"))
        return (PFN_vkVoidFunction)GetPhysicalDeviceQueueFamilyProperties;
    if (!strcmp(name, "GetPhysicalDeviceMemoryProperties"))
        return (PFN_vkVoidFunction)GetPhysicalDeviceMemoryProperties;
    if (!strcmp(name, "GetInstanceProcAddr"))
        return (PFN_vkVoidFunction)GetInstanceProcAddr;
    if (!strcmp(name, "CreateDevice"))
        return (PFN_vkVoidFunction)CreateDevice;
    if (!strcmp(name, "EnumerateInstanceExtensionProperties"))
        return (PFN_vkVoidFunction)EnumerateInstanceExtensionProperties;
    if (!strcmp(name, "EnumerateInstanceLayerProperties"))
        return (PFN_vkVoidFunction)EnumerateInstanceLayerProperties;
    if (!strcmp(name, "EnumerateDeviceLayerProperties"))
        return (PFN_vkVoidFunction)EnumerateDeviceLayerProperties;
    if (!strcmp(name, "GetPhysicalDeviceSparseImageFormatProperties"))
        return (PFN_vkVoidFunction)GetPhysicalDeviceSparseImageFormatProperties;
    if (!strcmp(name, "GetPhysicalDeviceExternalImageFormatPropertiesNV"))
        return (PFN_vkVoidFunction)GetPhysicalDeviceExternalImageFormatPropertiesNV;

    return NULL;
}

static inline PFN_vkVoidFunction InterceptWsiEnabledCommand(const char *name, VkDevice device) {
    if (device) {
        layer_data *device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

        if (device_data->wsi_enabled) {
            if (!strcmp("vkCreateSwapchainKHR", name))
                return reinterpret_cast<PFN_vkVoidFunction>(CreateSwapchainKHR);
            if (!strcmp("vkDestroySwapchainKHR", name))
                return reinterpret_cast<PFN_vkVoidFunction>(DestroySwapchainKHR);
            if (!strcmp("vkGetSwapchainImagesKHR", name))
                return reinterpret_cast<PFN_vkVoidFunction>(GetSwapchainImagesKHR);
            if (!strcmp("vkAcquireNextImageKHR", name))
                return reinterpret_cast<PFN_vkVoidFunction>(AcquireNextImageKHR);
            if (!strcmp("vkQueuePresentKHR", name))
                return reinterpret_cast<PFN_vkVoidFunction>(QueuePresentKHR);
        }

        if (device_data->wsi_display_swapchain_enabled) {
            if (!strcmp("vkCreateSharedSwapchainsKHR", name)) {
                return reinterpret_cast<PFN_vkVoidFunction>(CreateSharedSwapchainsKHR);
            }
        }
    }

    return nullptr;
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL GetDeviceProcAddr(VkDevice device, const char *funcName) {
    PFN_vkVoidFunction addr;
    addr = InterceptCoreDeviceCommand(funcName);
    if (addr) {
        return addr;
    }
    assert(device);

    addr = InterceptWsiEnabledCommand(funcName, device);
    if (addr) {
        return addr;
    }
    if (get_dispatch_table(ot_device_table_map, device)->GetDeviceProcAddr == NULL) {
        return NULL;
    }
    return get_dispatch_table(ot_device_table_map, device)->GetDeviceProcAddr(device, funcName);
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL GetInstanceProcAddr(VkInstance instance, const char *funcName) {
    PFN_vkVoidFunction addr;
    addr = InterceptCoreInstanceCommand(funcName);
    if (!addr) {
        addr = InterceptCoreDeviceCommand(funcName);
    }
    if (!addr) {
        addr = InterceptWsiEnabledCommand(funcName, VkDevice(VK_NULL_HANDLE));
    }
    if (addr) {
        return addr;
    }
    assert(instance);

    addr = InterceptMsgCallbackGetProcAddrCommand(funcName, instance);
    if (addr) {
        return addr;
    }
    addr = InterceptWsiEnabledCommand(funcName, instance);
    if (addr) {
        return addr;
    }
    if (get_dispatch_table(ot_instance_table_map, instance)->GetInstanceProcAddr == NULL) {
        return NULL;
    }
    return get_dispatch_table(ot_instance_table_map, instance)->GetInstanceProcAddr(instance, funcName);
}

} // namespace object_tracker

// vk_layer_logging.h expects these to be defined
VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugReportCallbackEXT(VkInstance instance,
                                                              const VkDebugReportCallbackCreateInfoEXT *pCreateInfo,
                                                              const VkAllocationCallbacks *pAllocator,
                                                              VkDebugReportCallbackEXT *pMsgCallback) {
    return object_tracker::CreateDebugReportCallbackEXT(instance, pCreateInfo, pAllocator, pMsgCallback);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT msgCallback,
                                                           const VkAllocationCallbacks *pAllocator) {
    object_tracker::DestroyDebugReportCallbackEXT(instance, msgCallback, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL vkDebugReportMessageEXT(VkInstance instance, VkDebugReportFlagsEXT flags,
                                                   VkDebugReportObjectTypeEXT objType, uint64_t object, size_t location,
                                                   int32_t msgCode, const char *pLayerPrefix, const char *pMsg) {
    object_tracker::DebugReportMessageEXT(instance, flags, objType, object, location, msgCode, pLayerPrefix, pMsg);
}

// Loader-layer interface v0, just wrappers since there is only a layer
VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceExtensionProperties(const char *pLayerName, uint32_t *pCount,
                                                                                      VkExtensionProperties *pProperties) {
    return object_tracker::EnumerateInstanceExtensionProperties(pLayerName, pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceLayerProperties(uint32_t *pCount,
                                                                                  VkLayerProperties *pProperties) {
    return object_tracker::EnumerateInstanceLayerProperties(pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t *pCount,
                                                                                VkLayerProperties *pProperties) {
    // The layer command handles VK_NULL_HANDLE just fine internally
    assert(physicalDevice == VK_NULL_HANDLE);
    return object_tracker::EnumerateDeviceLayerProperties(VK_NULL_HANDLE, pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice dev, const char *funcName) {
    return object_tracker::GetDeviceProcAddr(dev, funcName);
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char *funcName) {
    return object_tracker::GetInstanceProcAddr(instance, funcName);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice,
                                                                                    const char *pLayerName, uint32_t *pCount,
                                                                                    VkExtensionProperties *pProperties) {
    // The layer command handles VK_NULL_HANDLE just fine internally
    assert(physicalDevice == VK_NULL_HANDLE);
    return object_tracker::EnumerateDeviceExtensionProperties(VK_NULL_HANDLE, pLayerName, pCount, pProperties);
}
