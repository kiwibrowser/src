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
 * Author: Cody Northrop <cnorthrop@google.com>
 * Author: Michael Lentine <mlentine@google.com>
 * Author: Tobin Ehlis <tobine@google.com>
 * Author: Chia-I Wu <olv@google.com>
 * Author: Chris Forbes <chrisf@ijw.co.nz>
 * Author: Mark Lobodzinski <mark@lunarg.com>
 * Author: Ian Elliott <ianelliott@google.com>
 */

// Allow use of STL min and max functions in Windows
#define NOMINMAX

#include <SPIRV/spirv.hpp>
#include <algorithm>
#include <assert.h>
#include <iostream>
#include <list>
#include <map>
#include <mutex>
#include <set>
//#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <tuple>

#include "vk_loader_platform.h"
#include "vk_dispatch_table_helper.h"
#include "vk_struct_string_helper_cpp.h"
#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wwrite-strings"
#endif
#if defined(__GNUC__)
#pragma GCC diagnostic warning "-Wwrite-strings"
#endif
#include "vk_struct_size_helper.h"
#include "core_validation.h"
#include "vk_layer_table.h"
#include "vk_layer_data.h"
#include "vk_layer_extension_utils.h"
#include "vk_layer_utils.h"
#include "spirv-tools/libspirv.h"

#if defined __ANDROID__
#include <android/log.h>
#define LOGCONSOLE(...) ((void)__android_log_print(ANDROID_LOG_INFO, "DS", __VA_ARGS__))
#else
#define LOGCONSOLE(...)                                                                                                            \
    {                                                                                                                              \
        printf(__VA_ARGS__);                                                                                                       \
        printf("\n");                                                                                                              \
    }
#endif

using namespace std;

namespace core_validation {

using std::unordered_map;
using std::unordered_set;

// WSI Image Objects bypass usual Image Object creation methods.  A special Memory
// Object value will be used to identify them internally.
static const VkDeviceMemory MEMTRACKER_SWAP_CHAIN_IMAGE_KEY = (VkDeviceMemory)(-1);
// 2nd special memory handle used to flag object as unbound from memory
static const VkDeviceMemory MEMORY_UNBOUND = VkDeviceMemory(~((uint64_t)(0)) - 1);

struct devExts {
    bool wsi_enabled;
    bool wsi_display_swapchain_enabled;
    unordered_map<VkSwapchainKHR, unique_ptr<SWAPCHAIN_NODE>> swapchainMap;
    unordered_map<VkImage, VkSwapchainKHR> imageToSwapchainMap;
};

// fwd decls
struct shader_module;

struct instance_layer_data {
    VkInstance instance = VK_NULL_HANDLE;
    debug_report_data *report_data = nullptr;
    std::vector<VkDebugReportCallbackEXT> logging_callback;
    VkLayerInstanceDispatchTable dispatch_table;

    CALL_STATE vkEnumeratePhysicalDevicesState = UNCALLED;
    uint32_t physical_devices_count = 0;
    CHECK_DISABLED disabled = {};

    unordered_map<VkPhysicalDevice, PHYSICAL_DEVICE_STATE> physical_device_map;
    unordered_map<VkSurfaceKHR, SURFACE_STATE> surface_map;

    bool surfaceExtensionEnabled = false;
    bool displayExtensionEnabled = false;
#ifdef VK_USE_PLATFORM_ANDROID_KHR
    bool androidSurfaceExtensionEnabled = false;
#endif
#ifdef VK_USE_PLATFORM_MIR_KHR
    bool mirSurfaceExtensionEnabled = false;
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    bool waylandSurfaceExtensionEnabled = false;
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
    bool win32SurfaceExtensionEnabled = false;
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
    bool xcbSurfaceExtensionEnabled = false;
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
    bool xlibSurfaceExtensionEnabled = false;
#endif
};

struct layer_data {
    debug_report_data *report_data = nullptr;
    VkLayerDispatchTable dispatch_table;

    devExts device_extensions = {};
    unordered_set<VkQueue> queues;  // All queues under given device
    // Global set of all cmdBuffers that are inFlight on this device
    unordered_set<VkCommandBuffer> globalInFlightCmdBuffers;
    // Layer specific data
    unordered_map<VkSampler, unique_ptr<SAMPLER_STATE>> samplerMap;
    unordered_map<VkImageView, unique_ptr<IMAGE_VIEW_STATE>> imageViewMap;
    unordered_map<VkImage, unique_ptr<IMAGE_STATE>> imageMap;
    unordered_map<VkBufferView, unique_ptr<BUFFER_VIEW_STATE>> bufferViewMap;
    unordered_map<VkBuffer, unique_ptr<BUFFER_NODE>> bufferMap;
    unordered_map<VkPipeline, PIPELINE_STATE *> pipelineMap;
    unordered_map<VkCommandPool, COMMAND_POOL_NODE> commandPoolMap;
    unordered_map<VkDescriptorPool, DESCRIPTOR_POOL_STATE *> descriptorPoolMap;
    unordered_map<VkDescriptorSet, cvdescriptorset::DescriptorSet *> setMap;
    unordered_map<VkDescriptorSetLayout, cvdescriptorset::DescriptorSetLayout *> descriptorSetLayoutMap;
    unordered_map<VkPipelineLayout, PIPELINE_LAYOUT_NODE> pipelineLayoutMap;
    unordered_map<VkDeviceMemory, unique_ptr<DEVICE_MEM_INFO>> memObjMap;
    unordered_map<VkFence, FENCE_NODE> fenceMap;
    unordered_map<VkQueue, QUEUE_NODE> queueMap;
    unordered_map<VkEvent, EVENT_STATE> eventMap;
    unordered_map<QueryObject, bool> queryToStateMap;
    unordered_map<VkQueryPool, QUERY_POOL_NODE> queryPoolMap;
    unordered_map<VkSemaphore, SEMAPHORE_NODE> semaphoreMap;
    unordered_map<VkCommandBuffer, GLOBAL_CB_NODE *> commandBufferMap;
    unordered_map<VkFramebuffer, unique_ptr<FRAMEBUFFER_STATE>> frameBufferMap;
    unordered_map<VkImage, vector<ImageSubresourcePair>> imageSubresourceMap;
    unordered_map<ImageSubresourcePair, IMAGE_LAYOUT_NODE> imageLayoutMap;
    unordered_map<VkRenderPass, unique_ptr<RENDER_PASS_STATE>> renderPassMap;
    unordered_map<VkShaderModule, unique_ptr<shader_module>> shaderModuleMap;
    VkDevice device = VK_NULL_HANDLE;

    instance_layer_data *instance_data = nullptr;  // from device to enclosing instance

    VkPhysicalDeviceFeatures enabled_features = {};
    // Device specific data
    PHYS_DEV_PROPERTIES_NODE phys_dev_properties = {};
    VkPhysicalDeviceMemoryProperties phys_dev_mem_props = {};
};

// TODO : Do we need to guard access to layer_data_map w/ lock?
static unordered_map<void *, layer_data *> layer_data_map;
static unordered_map<void *, instance_layer_data *> instance_layer_data_map;

static const VkLayerProperties global_layer = {
    "VK_LAYER_LUNARG_core_validation", VK_LAYER_API_VERSION, 1, "LunarG Validation Layer",
};

template <class TCreateInfo> void ValidateLayerOrdering(const TCreateInfo &createInfo) {
    bool foundLayer = false;
    for (uint32_t i = 0; i < createInfo.enabledLayerCount; ++i) {
        if (!strcmp(createInfo.ppEnabledLayerNames[i], global_layer.layerName)) {
            foundLayer = true;
        }
        // This has to be logged to console as we don't have a callback at this point.
        if (!foundLayer && !strcmp(createInfo.ppEnabledLayerNames[0], "VK_LAYER_GOOGLE_unique_objects")) {
            LOGCONSOLE("Cannot activate layer VK_LAYER_GOOGLE_unique_objects prior to activating %s.",
                       global_layer.layerName);
        }
    }
}

// Code imported from shader_checker
static void build_def_index(shader_module *);

// A forward iterator over spirv instructions. Provides easy access to len, opcode, and content words
// without the caller needing to care too much about the physical SPIRV module layout.
struct spirv_inst_iter {
    std::vector<uint32_t>::const_iterator zero;
    std::vector<uint32_t>::const_iterator it;

    uint32_t len() {
        auto result = *it >> 16;
        assert(result > 0);
        return result;
    }

    uint32_t opcode() { return *it & 0x0ffffu; }

    uint32_t const &word(unsigned n) {
        assert(n < len());
        return it[n];
    }

    uint32_t offset() { return (uint32_t)(it - zero); }

    spirv_inst_iter() {}

    spirv_inst_iter(std::vector<uint32_t>::const_iterator zero, std::vector<uint32_t>::const_iterator it) : zero(zero), it(it) {}

    bool operator==(spirv_inst_iter const &other) { return it == other.it; }

    bool operator!=(spirv_inst_iter const &other) { return it != other.it; }

    spirv_inst_iter operator++(int) { /* x++ */
        spirv_inst_iter ii = *this;
        it += len();
        return ii;
    }

    spirv_inst_iter operator++() { /* ++x; */
        it += len();
        return *this;
    }

    /* The iterator and the value are the same thing. */
    spirv_inst_iter &operator*() { return *this; }
    spirv_inst_iter const &operator*() const { return *this; }
};

struct shader_module {
    /* the spirv image itself */
    vector<uint32_t> words;
    /* a mapping of <id> to the first word of its def. this is useful because walking type
     * trees, constant expressions, etc requires jumping all over the instruction stream.
     */
    unordered_map<unsigned, unsigned> def_index;

    shader_module(VkShaderModuleCreateInfo const *pCreateInfo)
        : words((uint32_t *)pCreateInfo->pCode, (uint32_t *)pCreateInfo->pCode + pCreateInfo->codeSize / sizeof(uint32_t)),
          def_index() {

        build_def_index(this);
    }

    /* expose begin() / end() to enable range-based for */
    spirv_inst_iter begin() const { return spirv_inst_iter(words.begin(), words.begin() + 5); } /* first insn */
    spirv_inst_iter end() const { return spirv_inst_iter(words.begin(), words.end()); }         /* just past last insn */
    /* given an offset into the module, produce an iterator there. */
    spirv_inst_iter at(unsigned offset) const { return spirv_inst_iter(words.begin(), words.begin() + offset); }

    /* gets an iterator to the definition of an id */
    spirv_inst_iter get_def(unsigned id) const {
        auto it = def_index.find(id);
        if (it == def_index.end()) {
            return end();
        }
        return at(it->second);
    }
};

// TODO : This can be much smarter, using separate locks for separate global data
static std::mutex global_lock;

// Return IMAGE_VIEW_STATE ptr for specified imageView or else NULL
IMAGE_VIEW_STATE *getImageViewState(const layer_data *dev_data, VkImageView image_view) {
    auto iv_it = dev_data->imageViewMap.find(image_view);
    if (iv_it == dev_data->imageViewMap.end()) {
        return nullptr;
    }
    return iv_it->second.get();
}
// Return sampler node ptr for specified sampler or else NULL
SAMPLER_STATE *getSamplerState(const layer_data *dev_data, VkSampler sampler) {
    auto sampler_it = dev_data->samplerMap.find(sampler);
    if (sampler_it == dev_data->samplerMap.end()) {
        return nullptr;
    }
    return sampler_it->second.get();
}
// Return image node ptr for specified image or else NULL
IMAGE_STATE *getImageState(const layer_data *dev_data, VkImage image) {
    auto img_it = dev_data->imageMap.find(image);
    if (img_it == dev_data->imageMap.end()) {
        return nullptr;
    }
    return img_it->second.get();
}
// Return buffer node ptr for specified buffer or else NULL
BUFFER_NODE *getBufferNode(const layer_data *dev_data, VkBuffer buffer) {
    auto buff_it = dev_data->bufferMap.find(buffer);
    if (buff_it == dev_data->bufferMap.end()) {
        return nullptr;
    }
    return buff_it->second.get();
}
// Return swapchain node for specified swapchain or else NULL
SWAPCHAIN_NODE *getSwapchainNode(const layer_data *dev_data, VkSwapchainKHR swapchain) {
    auto swp_it = dev_data->device_extensions.swapchainMap.find(swapchain);
    if (swp_it == dev_data->device_extensions.swapchainMap.end()) {
        return nullptr;
    }
    return swp_it->second.get();
}
// Return swapchain for specified image or else NULL
VkSwapchainKHR getSwapchainFromImage(const layer_data *dev_data, VkImage image) {
    auto img_it = dev_data->device_extensions.imageToSwapchainMap.find(image);
    if (img_it == dev_data->device_extensions.imageToSwapchainMap.end()) {
        return VK_NULL_HANDLE;
    }
    return img_it->second;
}
// Return buffer node ptr for specified buffer or else NULL
BUFFER_VIEW_STATE *getBufferViewState(const layer_data *my_data, VkBufferView buffer_view) {
    auto bv_it = my_data->bufferViewMap.find(buffer_view);
    if (bv_it == my_data->bufferViewMap.end()) {
        return nullptr;
    }
    return bv_it->second.get();
}

FENCE_NODE *getFenceNode(layer_data *dev_data, VkFence fence) {
    auto it = dev_data->fenceMap.find(fence);
    if (it == dev_data->fenceMap.end()) {
        return nullptr;
    }
    return &it->second;
}

EVENT_STATE *getEventNode(layer_data *dev_data, VkEvent event) {
    auto it = dev_data->eventMap.find(event);
    if (it == dev_data->eventMap.end()) {
        return nullptr;
    }
    return &it->second;
}

QUERY_POOL_NODE *getQueryPoolNode(layer_data *dev_data, VkQueryPool query_pool) {
    auto it = dev_data->queryPoolMap.find(query_pool);
    if (it == dev_data->queryPoolMap.end()) {
        return nullptr;
    }
    return &it->second;
}

QUEUE_NODE *getQueueNode(layer_data *dev_data, VkQueue queue) {
    auto it = dev_data->queueMap.find(queue);
    if (it == dev_data->queueMap.end()) {
        return nullptr;
    }
    return &it->second;
}

SEMAPHORE_NODE *getSemaphoreNode(layer_data *dev_data, VkSemaphore semaphore) {
    auto it = dev_data->semaphoreMap.find(semaphore);
    if (it == dev_data->semaphoreMap.end()) {
        return nullptr;
    }
    return &it->second;
}

COMMAND_POOL_NODE *getCommandPoolNode(layer_data *dev_data, VkCommandPool pool) {
    auto it = dev_data->commandPoolMap.find(pool);
    if (it == dev_data->commandPoolMap.end()) {
        return nullptr;
    }
    return &it->second;
}

PHYSICAL_DEVICE_STATE *getPhysicalDeviceState(instance_layer_data *instance_data, VkPhysicalDevice phys) {
    auto it = instance_data->physical_device_map.find(phys);
    if (it == instance_data->physical_device_map.end()) {
        return nullptr;
    }
    return &it->second;
}

SURFACE_STATE *getSurfaceState(instance_layer_data *instance_data, VkSurfaceKHR surface) {
    auto it = instance_data->surface_map.find(surface);
    if (it == instance_data->surface_map.end()) {
        return nullptr;
    }
    return &it->second;
}

// Return ptr to memory binding for given handle of specified type
static BINDABLE *GetObjectMemBinding(layer_data *my_data, uint64_t handle, VkDebugReportObjectTypeEXT type) {
    switch (type) {
    case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT:
        return getImageState(my_data, VkImage(handle));
    case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT:
        return getBufferNode(my_data, VkBuffer(handle));
    default:
        break;
    }
    return nullptr;
}
// prototype
static GLOBAL_CB_NODE *getCBNode(layer_data const *, const VkCommandBuffer);

// Helper function to validate correct usage bits set for buffers or images
//  Verify that (actual & desired) flags != 0 or,
//   if strict is true, verify that (actual & desired) flags == desired
//  In case of error, report it via dbg callbacks
static bool validate_usage_flags(layer_data *my_data, VkFlags actual, VkFlags desired, VkBool32 strict,
                                     uint64_t obj_handle, VkDebugReportObjectTypeEXT obj_type, char const *ty_str,
                                     char const *func_name, char const *usage_str) {
    bool correct_usage = false;
    bool skip_call = false;
    if (strict)
        correct_usage = ((actual & desired) == desired);
    else
        correct_usage = ((actual & desired) != 0);
    if (!correct_usage) {
        skip_call = log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, obj_type, obj_handle, __LINE__,
                            MEMTRACK_INVALID_USAGE_FLAG, "MEM", "Invalid usage flag for %s 0x%" PRIxLEAST64
                                                                " used by %s. In this case, %s should have %s set during creation.",
                            ty_str, obj_handle, func_name, ty_str, usage_str);
    }
    return skip_call;
}

// Helper function to validate usage flags for buffers
// For given buffer_node send actual vs. desired usage off to helper above where
//  an error will be flagged if usage is not correct
static bool ValidateImageUsageFlags(layer_data *dev_data, IMAGE_STATE const *image_state, VkFlags desired, VkBool32 strict,
                                    char const *func_name, char const *usage_string) {
    return validate_usage_flags(dev_data, image_state->createInfo.usage, desired, strict,
                                reinterpret_cast<const uint64_t &>(image_state->image), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                                "image", func_name, usage_string);
}

// Helper function to validate usage flags for buffers
// For given buffer_node send actual vs. desired usage off to helper above where
//  an error will be flagged if usage is not correct
static bool ValidateBufferUsageFlags(layer_data *dev_data, BUFFER_NODE const *buffer_node, VkFlags desired, VkBool32 strict,
                                     char const *func_name, char const *usage_string) {
    return validate_usage_flags(dev_data, buffer_node->createInfo.usage, desired, strict,
                                reinterpret_cast<const uint64_t &>(buffer_node->buffer), VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT,
                                "buffer", func_name, usage_string);
}

// Return ptr to info in map container containing mem, or NULL if not found
//  Calls to this function should be wrapped in mutex
DEVICE_MEM_INFO *getMemObjInfo(const layer_data *dev_data, const VkDeviceMemory mem) {
    auto mem_it = dev_data->memObjMap.find(mem);
    if (mem_it == dev_data->memObjMap.end()) {
        return NULL;
    }
    return mem_it->second.get();
}

static void add_mem_obj_info(layer_data *my_data, void *object, const VkDeviceMemory mem,
                             const VkMemoryAllocateInfo *pAllocateInfo) {
    assert(object != NULL);

    my_data->memObjMap[mem] = unique_ptr<DEVICE_MEM_INFO>(new DEVICE_MEM_INFO(object, mem, pAllocateInfo));
}

// Helper function to print lowercase string of object type
//  TODO: Unify string helper functions, this should really come out of a string helper if not there already
static const char *object_type_to_string(VkDebugReportObjectTypeEXT type) {
    switch (type) {
    case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT:
        return "image";
    case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT:
        return "buffer";
    case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT:
        return "image view";
    case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT:
        return "buffer view";
    case VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT:
        return "swapchain";
    case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT:
        return "descriptor set";
    case VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT:
        return "framebuffer";
    case VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT:
        return "event";
    case VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT:
        return "query pool";
    case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT:
        return "descriptor pool";
    case VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT:
        return "command pool";
    case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT:
        return "pipeline";
    case VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT:
        return "sampler";
    case VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT:
        return "renderpass";
    case VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT:
        return "device memory";
    case VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT:
        return "semaphore";
    default:
        return "unknown";
    }
}

// For given bound_object_handle, bound to given mem allocation, verify that the range for the bound object is valid
static bool ValidateMemoryIsValid(layer_data *dev_data, VkDeviceMemory mem, uint64_t bound_object_handle,
                                  VkDebugReportObjectTypeEXT type, const char *functionName) {
    DEVICE_MEM_INFO *mem_info = getMemObjInfo(dev_data, mem);
    if (mem_info) {
        if (!mem_info->bound_ranges[bound_object_handle].valid) {
            return log_msg(dev_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT,
                           reinterpret_cast<uint64_t &>(mem), __LINE__, MEMTRACK_INVALID_MEM_REGION, "MEM",
                           "%s: Cannot read invalid region of memory allocation 0x%" PRIx64 " for bound %s object 0x%" PRIx64
                           ", please fill the memory before using.",
                           functionName, reinterpret_cast<uint64_t &>(mem), object_type_to_string(type), bound_object_handle);
        }
    }
    return false;
}
// For given image_state
//  If mem is special swapchain key, then verify that image_state valid member is true
//  Else verify that the image's bound memory range is valid
static bool ValidateImageMemoryIsValid(layer_data *dev_data, IMAGE_STATE *image_state, const char *functionName) {
    if (image_state->binding.mem == MEMTRACKER_SWAP_CHAIN_IMAGE_KEY) {
        if (!image_state->valid) {
            return log_msg(dev_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT,
                           reinterpret_cast<uint64_t &>(image_state->binding.mem), __LINE__, MEMTRACK_INVALID_MEM_REGION, "MEM",
                           "%s: Cannot read invalid swapchain image 0x%" PRIx64 ", please fill the memory before using.",
                           functionName, reinterpret_cast<uint64_t &>(image_state->image));
        }
    } else {
        return ValidateMemoryIsValid(dev_data, image_state->binding.mem, reinterpret_cast<uint64_t &>(image_state->image),
                                     VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, functionName);
    }
    return false;
}
// For given buffer_node, verify that the range it's bound to is valid
static bool ValidateBufferMemoryIsValid(layer_data *dev_data, BUFFER_NODE *buffer_node, const char *functionName) {
    return ValidateMemoryIsValid(dev_data, buffer_node->binding.mem, reinterpret_cast<uint64_t &>(buffer_node->buffer),
                                 VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, functionName);
}
// For the given memory allocation, set the range bound by the given handle object to the valid param value
static void SetMemoryValid(layer_data *dev_data, VkDeviceMemory mem, uint64_t handle, bool valid) {
    DEVICE_MEM_INFO *mem_info = getMemObjInfo(dev_data, mem);
    if (mem_info) {
        mem_info->bound_ranges[handle].valid = valid;
    }
}
// For given image node
//  If mem is special swapchain key, then set entire image_state to valid param value
//  Else set the image's bound memory range to valid param value
static void SetImageMemoryValid(layer_data *dev_data, IMAGE_STATE *image_state, bool valid) {
    if (image_state->binding.mem == MEMTRACKER_SWAP_CHAIN_IMAGE_KEY) {
        image_state->valid = valid;
    } else {
        SetMemoryValid(dev_data, image_state->binding.mem, reinterpret_cast<uint64_t &>(image_state->image), valid);
    }
}
// For given buffer node set the buffer's bound memory range to valid param value
static void SetBufferMemoryValid(layer_data *dev_data, BUFFER_NODE *buffer_node, bool valid) {
    SetMemoryValid(dev_data, buffer_node->binding.mem, reinterpret_cast<uint64_t &>(buffer_node->buffer), valid);
}
// Find CB Info and add mem reference to list container
// Find Mem Obj Info and add CB reference to list container
static bool update_cmd_buf_and_mem_references(layer_data *dev_data, const VkCommandBuffer cb, const VkDeviceMemory mem,
                                              const char *apiName) {
    bool skip_call = false;

    // Skip validation if this image was created through WSI
    if (mem != MEMTRACKER_SWAP_CHAIN_IMAGE_KEY) {

        // First update CB binding in MemObj mini CB list
        DEVICE_MEM_INFO *pMemInfo = getMemObjInfo(dev_data, mem);
        if (pMemInfo) {
            // Now update CBInfo's Mem reference list
            GLOBAL_CB_NODE *cb_node = getCBNode(dev_data, cb);
            pMemInfo->cb_bindings.insert(cb_node);
            // TODO: keep track of all destroyed CBs so we know if this is a stale or simply invalid object
            if (cb_node) {
                cb_node->memObjs.insert(mem);
            }
        }
    }
    return skip_call;
}

// Create binding link between given sampler and command buffer node
void AddCommandBufferBindingSampler(GLOBAL_CB_NODE *cb_node, SAMPLER_STATE *sampler_state) {
    sampler_state->cb_bindings.insert(cb_node);
    cb_node->object_bindings.insert(
        {reinterpret_cast<uint64_t &>(sampler_state->sampler), VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT});
}

// Create binding link between given image node and command buffer node
void AddCommandBufferBindingImage(const layer_data *dev_data, GLOBAL_CB_NODE *cb_node, IMAGE_STATE *image_state) {
    // Skip validation if this image was created through WSI
    if (image_state->binding.mem != MEMTRACKER_SWAP_CHAIN_IMAGE_KEY) {
        // First update CB binding in MemObj mini CB list
        DEVICE_MEM_INFO *pMemInfo = getMemObjInfo(dev_data, image_state->binding.mem);
        if (pMemInfo) {
            pMemInfo->cb_bindings.insert(cb_node);
            // Now update CBInfo's Mem reference list
            cb_node->memObjs.insert(image_state->binding.mem);
        }
        // Now update cb binding for image
        cb_node->object_bindings.insert({reinterpret_cast<uint64_t &>(image_state->image), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT});
        image_state->cb_bindings.insert(cb_node);
    }
}

// Create binding link between given image view node and its image with command buffer node
void AddCommandBufferBindingImageView(const layer_data *dev_data, GLOBAL_CB_NODE *cb_node, IMAGE_VIEW_STATE *view_state) {
    // First add bindings for imageView
    view_state->cb_bindings.insert(cb_node);
    cb_node->object_bindings.insert(
        {reinterpret_cast<uint64_t &>(view_state->image_view), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT});
    auto image_state = getImageState(dev_data, view_state->create_info.image);
    // Add bindings for image within imageView
    if (image_state) {
        AddCommandBufferBindingImage(dev_data, cb_node, image_state);
    }
}

// Create binding link between given buffer node and command buffer node
void AddCommandBufferBindingBuffer(const layer_data *dev_data, GLOBAL_CB_NODE *cb_node, BUFFER_NODE *buff_node) {
    // First update CB binding in MemObj mini CB list
    DEVICE_MEM_INFO *pMemInfo = getMemObjInfo(dev_data, buff_node->binding.mem);
    if (pMemInfo) {
        pMemInfo->cb_bindings.insert(cb_node);
        // Now update CBInfo's Mem reference list
        cb_node->memObjs.insert(buff_node->binding.mem);
    }
    // Now update cb binding for buffer
    cb_node->object_bindings.insert({reinterpret_cast<uint64_t &>(buff_node->buffer), VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT});
    buff_node->cb_bindings.insert(cb_node);
}

// Create binding link between given buffer view node and its buffer with command buffer node
void AddCommandBufferBindingBufferView(const layer_data *dev_data, GLOBAL_CB_NODE *cb_node, BUFFER_VIEW_STATE *view_state) {
    // First add bindings for bufferView
    view_state->cb_bindings.insert(cb_node);
    cb_node->object_bindings.insert(
        {reinterpret_cast<uint64_t &>(view_state->buffer_view), VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT});
    auto buffer_node = getBufferNode(dev_data, view_state->create_info.buffer);
    // Add bindings for buffer within bufferView
    if (buffer_node) {
        AddCommandBufferBindingBuffer(dev_data, cb_node, buffer_node);
    }
}

// For every mem obj bound to particular CB, free bindings related to that CB
static void clear_cmd_buf_and_mem_references(layer_data *dev_data, GLOBAL_CB_NODE *cb_node) {
    if (cb_node) {
        if (cb_node->memObjs.size() > 0) {
            for (auto mem : cb_node->memObjs) {
                DEVICE_MEM_INFO *pInfo = getMemObjInfo(dev_data, mem);
                if (pInfo) {
                    pInfo->cb_bindings.erase(cb_node);
                }
            }
            cb_node->memObjs.clear();
        }
        cb_node->validate_functions.clear();
    }
}
// Overloaded call to above function when GLOBAL_CB_NODE has not already been looked-up
static void clear_cmd_buf_and_mem_references(layer_data *dev_data, const VkCommandBuffer cb) {
    clear_cmd_buf_and_mem_references(dev_data, getCBNode(dev_data, cb));
}

// Clear a single object binding from given memory object, or report error if binding is missing
static bool ClearMemoryObjectBinding(layer_data *dev_data, uint64_t handle, VkDebugReportObjectTypeEXT type, VkDeviceMemory mem) {
    DEVICE_MEM_INFO *mem_info = getMemObjInfo(dev_data, mem);
    // This obj is bound to a memory object. Remove the reference to this object in that memory object's list
    if (mem_info && !mem_info->obj_bindings.erase({handle, type})) {
        return log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, type, handle, __LINE__, MEMTRACK_INVALID_OBJECT,
                    "MEM", "While trying to clear mem binding for %s obj 0x%" PRIxLEAST64
                           ", unable to find that object referenced by mem obj 0x%" PRIxLEAST64,
                    object_type_to_string(type), handle, (uint64_t)mem);
    }
    return false;
}

// ClearMemoryObjectBindings clears the binding of objects to memory
//  For the given object it pulls the memory bindings and makes sure that the bindings
//  no longer refer to the object being cleared. This occurs when objects are destroyed.
static bool ClearMemoryObjectBindings(layer_data *dev_data, uint64_t handle, VkDebugReportObjectTypeEXT type) {
    bool skip = false;
    BINDABLE *mem_binding = GetObjectMemBinding(dev_data, handle, type);
    if (mem_binding) {
        if (!mem_binding->sparse) {
            skip = ClearMemoryObjectBinding(dev_data, handle, type, mem_binding->binding.mem);
        } else { // Sparse, clear all bindings
            for (auto& sparse_mem_binding : mem_binding->sparse_bindings) {
                skip |= ClearMemoryObjectBinding(dev_data, handle, type, sparse_mem_binding.mem);
            }
        }
    }
    return skip;
}

// For given mem object, verify that it is not null or UNBOUND, if it is, report error. Return skip value.
bool VerifyBoundMemoryIsValid(const layer_data *dev_data, VkDeviceMemory mem, uint64_t handle, const char *api_name,
                              const char *type_name) {
    bool result = false;
    if (VK_NULL_HANDLE == mem) {
        result = log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, handle,
                         __LINE__, MEMTRACK_OBJECT_NOT_BOUND, "MEM",
                         "%s: Vk%s object 0x%" PRIxLEAST64 " used with no memory bound. Memory should be bound by calling "
                         "vkBind%sMemory().",
                         api_name, type_name, handle, type_name);
    } else if (MEMORY_UNBOUND == mem) {
        result = log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, handle,
                         __LINE__, MEMTRACK_OBJECT_NOT_BOUND, "MEM",
                         "%s: Vk%s object 0x%" PRIxLEAST64 " used with no memory bound and previously bound memory was freed. "
                         "Memory must not be freed prior to this operation.",
                         api_name, type_name, handle);
    }
    return result;
}

// Check to see if memory was ever bound to this image
bool ValidateMemoryIsBoundToImage(const layer_data *dev_data, const IMAGE_STATE *image_state, const char *api_name) {
    bool result = false;
    if (0 == (static_cast<uint32_t>(image_state->createInfo.flags) & VK_IMAGE_CREATE_SPARSE_BINDING_BIT)) {
        result = VerifyBoundMemoryIsValid(dev_data, image_state->binding.mem, reinterpret_cast<const uint64_t &>(image_state->image),
                                          api_name, "Image");
    }
    return result;
}

// Check to see if memory was bound to this buffer
bool ValidateMemoryIsBoundToBuffer(const layer_data *dev_data, const BUFFER_NODE *buffer_node, const char *api_name) {
    bool result = false;
    if (0 == (static_cast<uint32_t>(buffer_node->createInfo.flags) & VK_BUFFER_CREATE_SPARSE_BINDING_BIT)) {
        result = VerifyBoundMemoryIsValid(dev_data, buffer_node->binding.mem,
                                          reinterpret_cast<const uint64_t &>(buffer_node->buffer), api_name, "Buffer");
    }
    return result;
}

// SetMemBinding is used to establish immutable, non-sparse binding between a single image/buffer object and memory object
// For NULL mem case, output warning
// Make sure given object is in global object map
//  IF a previous binding existed, output validation error
//  Otherwise, add reference from objectInfo to memoryInfo
//  Add reference off of objInfo
static bool SetMemBinding(layer_data *dev_data, VkDeviceMemory mem, uint64_t handle, VkDebugReportObjectTypeEXT type,
                          const char *apiName) {
    bool skip_call = false;
    // It's an error to bind an object to NULL memory
    if (mem == VK_NULL_HANDLE) {
        skip_call = log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, type, handle, __LINE__, MEMTRACK_INVALID_MEM_OBJ,
                            "MEM", "In %s, attempting to Bind Obj(0x%" PRIxLEAST64 ") to NULL", apiName, handle);
    } else {
        BINDABLE *mem_binding = GetObjectMemBinding(dev_data, handle, type);
        assert(mem_binding);
        // TODO : Add check here to make sure object isn't sparse
        //  VALIDATION_ERROR_00792 for buffers
        //  VALIDATION_ERROR_00804 for images
        assert(!mem_binding->sparse);
        DEVICE_MEM_INFO *mem_info = getMemObjInfo(dev_data, mem);
        if (mem_info) {
            DEVICE_MEM_INFO *prev_binding = getMemObjInfo(dev_data, mem_binding->binding.mem);
            if (prev_binding) {
                skip_call |=
                    log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT,
                            reinterpret_cast<uint64_t &>(mem), __LINE__, MEMTRACK_REBIND_OBJECT, "MEM",
                            "In %s, attempting to bind memory (0x%" PRIxLEAST64 ") to object (0x%" PRIxLEAST64
                            ") which has already been bound to mem object 0x%" PRIxLEAST64,
                            apiName, reinterpret_cast<uint64_t &>(mem), handle, reinterpret_cast<uint64_t &>(prev_binding->mem));
            } else if (mem_binding->binding.mem == MEMORY_UNBOUND) {
                skip_call |=
                    log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT,
                            reinterpret_cast<uint64_t &>(mem), __LINE__, MEMTRACK_REBIND_OBJECT, "MEM",
                            "In %s, attempting to bind memory (0x%" PRIxLEAST64 ") to object (0x%" PRIxLEAST64
                            ") which was previous bound to memory that has since been freed. Memory bindings are immutable in "
                            "Vulkan so this attempt to bind to new memory is not allowed.",
                            apiName, reinterpret_cast<uint64_t &>(mem), handle);
            } else {
                mem_info->obj_bindings.insert({handle, type});
                // For image objects, make sure default memory state is correctly set
                // TODO : What's the best/correct way to handle this?
                if (VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT == type) {
                    auto const image_state = getImageState(dev_data, VkImage(handle));
                    if (image_state) {
                        VkImageCreateInfo ici = image_state->createInfo;
                        if (ici.usage & (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)) {
                            // TODO::  More memory state transition stuff.
                        }
                    }
                }
                mem_binding->binding.mem = mem;
            }
        }
    }
    return skip_call;
}

// For NULL mem case, clear any previous binding Else...
// Make sure given object is in its object map
//  IF a previous binding existed, update binding
//  Add reference from objectInfo to memoryInfo
//  Add reference off of object's binding info
// Return VK_TRUE if addition is successful, VK_FALSE otherwise
static bool SetSparseMemBinding(layer_data *dev_data, MEM_BINDING binding, uint64_t handle, VkDebugReportObjectTypeEXT type,
                                const char *apiName) {
    bool skip_call = VK_FALSE;
    // Handle NULL case separately, just clear previous binding & decrement reference
    if (binding.mem == VK_NULL_HANDLE) {
        // TODO : This should cause the range of the resource to be unbound according to spec
    } else {
        BINDABLE *mem_binding = GetObjectMemBinding(dev_data, handle, type);
        assert(mem_binding);
        assert(mem_binding->sparse);
        DEVICE_MEM_INFO *mem_info = getMemObjInfo(dev_data, binding.mem);
        if (mem_info) {
            mem_info->obj_bindings.insert({handle, type});
            // Need to set mem binding for this object
            mem_binding->sparse_bindings.insert(binding);
        }
    }
    return skip_call;
}

// For handle of given object type, return memory binding
static bool get_mem_for_type(layer_data *dev_data, uint64_t handle, VkDebugReportObjectTypeEXT type, VkDeviceMemory *mem) {
    bool skip_call = false;
    *mem = VK_NULL_HANDLE;
    switch (type) {
    case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT:
        *mem = getImageState(dev_data, VkImage(handle))->binding.mem;
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT:
        *mem = getBufferNode(dev_data, VkBuffer(handle))->binding.mem;
        break;
    default:
        assert(0);
    }
    if (!*mem) {
        skip_call = log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, type, handle, __LINE__, MEMTRACK_INVALID_OBJECT,
                            "MEM", "Trying to get mem binding for %s object 0x%" PRIxLEAST64
                                   " but binding is NULL. Has memory been bound to this object?",
                            object_type_to_string(type), handle);
    }
    return skip_call;
}

// Print details of MemObjInfo list
static void print_mem_list(layer_data *dev_data) {
    // Early out if info is not requested
    if (!(dev_data->report_data->active_flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)) {
        return;
    }

    // Just printing each msg individually for now, may want to package these into single large print
    log_msg(dev_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, 0, __LINE__,
            MEMTRACK_NONE, "MEM", "Details of Memory Object list (of size " PRINTF_SIZE_T_SPECIFIER " elements)",
            dev_data->memObjMap.size());
    log_msg(dev_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, 0, __LINE__,
            MEMTRACK_NONE, "MEM", "=============================");

    if (dev_data->memObjMap.size() <= 0)
        return;

    for (auto ii = dev_data->memObjMap.begin(); ii != dev_data->memObjMap.end(); ++ii) {
        auto mem_info = (*ii).second.get();

        log_msg(dev_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, 0,
                __LINE__, MEMTRACK_NONE, "MEM", "    ===MemObjInfo at 0x%p===", (void *)mem_info);
        log_msg(dev_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, 0,
                __LINE__, MEMTRACK_NONE, "MEM", "    Mem object: 0x%" PRIxLEAST64, (uint64_t)(mem_info->mem));
        log_msg(dev_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, 0,
                __LINE__, MEMTRACK_NONE, "MEM", "    Ref Count: " PRINTF_SIZE_T_SPECIFIER,
                mem_info->cb_bindings.size() + mem_info->obj_bindings.size());
        if (0 != mem_info->alloc_info.allocationSize) {
            string pAllocInfoMsg = vk_print_vkmemoryallocateinfo(&mem_info->alloc_info, "MEM(INFO):         ");
            log_msg(dev_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, 0,
                    __LINE__, MEMTRACK_NONE, "MEM", "    Mem Alloc info:\n%s", pAllocInfoMsg.c_str());
        } else {
            log_msg(dev_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, 0,
                    __LINE__, MEMTRACK_NONE, "MEM", "    Mem Alloc info is NULL (alloc done by vkCreateSwapchainKHR())");
        }

        log_msg(dev_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, 0,
                __LINE__, MEMTRACK_NONE, "MEM", "    VK OBJECT Binding list of size " PRINTF_SIZE_T_SPECIFIER " elements:",
                mem_info->obj_bindings.size());
        if (mem_info->obj_bindings.size() > 0) {
            for (auto obj : mem_info->obj_bindings) {
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT,
                        0, __LINE__, MEMTRACK_NONE, "MEM", "       VK OBJECT 0x%" PRIx64, obj.handle);
            }
        }

        log_msg(dev_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, 0,
                __LINE__, MEMTRACK_NONE, "MEM",
                "    VK Command Buffer (CB) binding list of size " PRINTF_SIZE_T_SPECIFIER " elements",
                mem_info->cb_bindings.size());
        if (mem_info->cb_bindings.size() > 0) {
            for (auto cb : mem_info->cb_bindings) {
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT,
                        0, __LINE__, MEMTRACK_NONE, "MEM", "      VK command buffer 0x%p", cb);
            }
        }
    }
}

static void printCBList(layer_data *my_data) {
    GLOBAL_CB_NODE *pCBInfo = NULL;

    // Early out if info is not requested
    if (!(my_data->report_data->active_flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)) {
        return;
    }

    log_msg(my_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, 0, __LINE__,
            MEMTRACK_NONE, "MEM", "Details of command buffer list (of size " PRINTF_SIZE_T_SPECIFIER " elements)",
            my_data->commandBufferMap.size());
    log_msg(my_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, 0, __LINE__,
            MEMTRACK_NONE, "MEM", "==================");

    if (my_data->commandBufferMap.size() <= 0)
        return;

    for (auto &cb_node : my_data->commandBufferMap) {
        pCBInfo = cb_node.second;

        log_msg(my_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, 0,
                __LINE__, MEMTRACK_NONE, "MEM", "    CB Info (0x%p) has command buffer 0x%p", (void *)pCBInfo,
                (void *)pCBInfo->commandBuffer);

        if (pCBInfo->memObjs.size() <= 0)
            continue;
        for (auto obj : pCBInfo->memObjs) {
            log_msg(my_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, 0,
                    __LINE__, MEMTRACK_NONE, "MEM", "      Mem obj 0x%" PRIx64, (uint64_t)obj);
        }
    }
}

// Return a string representation of CMD_TYPE enum
static string cmdTypeToString(CMD_TYPE cmd) {
    switch (cmd) {
    case CMD_BINDPIPELINE:
        return "CMD_BINDPIPELINE";
    case CMD_BINDPIPELINEDELTA:
        return "CMD_BINDPIPELINEDELTA";
    case CMD_SETVIEWPORTSTATE:
        return "CMD_SETVIEWPORTSTATE";
    case CMD_SETLINEWIDTHSTATE:
        return "CMD_SETLINEWIDTHSTATE";
    case CMD_SETDEPTHBIASSTATE:
        return "CMD_SETDEPTHBIASSTATE";
    case CMD_SETBLENDSTATE:
        return "CMD_SETBLENDSTATE";
    case CMD_SETDEPTHBOUNDSSTATE:
        return "CMD_SETDEPTHBOUNDSSTATE";
    case CMD_SETSTENCILREADMASKSTATE:
        return "CMD_SETSTENCILREADMASKSTATE";
    case CMD_SETSTENCILWRITEMASKSTATE:
        return "CMD_SETSTENCILWRITEMASKSTATE";
    case CMD_SETSTENCILREFERENCESTATE:
        return "CMD_SETSTENCILREFERENCESTATE";
    case CMD_BINDDESCRIPTORSETS:
        return "CMD_BINDDESCRIPTORSETS";
    case CMD_BINDINDEXBUFFER:
        return "CMD_BINDINDEXBUFFER";
    case CMD_BINDVERTEXBUFFER:
        return "CMD_BINDVERTEXBUFFER";
    case CMD_DRAW:
        return "CMD_DRAW";
    case CMD_DRAWINDEXED:
        return "CMD_DRAWINDEXED";
    case CMD_DRAWINDIRECT:
        return "CMD_DRAWINDIRECT";
    case CMD_DRAWINDEXEDINDIRECT:
        return "CMD_DRAWINDEXEDINDIRECT";
    case CMD_DISPATCH:
        return "CMD_DISPATCH";
    case CMD_DISPATCHINDIRECT:
        return "CMD_DISPATCHINDIRECT";
    case CMD_COPYBUFFER:
        return "CMD_COPYBUFFER";
    case CMD_COPYIMAGE:
        return "CMD_COPYIMAGE";
    case CMD_BLITIMAGE:
        return "CMD_BLITIMAGE";
    case CMD_COPYBUFFERTOIMAGE:
        return "CMD_COPYBUFFERTOIMAGE";
    case CMD_COPYIMAGETOBUFFER:
        return "CMD_COPYIMAGETOBUFFER";
    case CMD_CLONEIMAGEDATA:
        return "CMD_CLONEIMAGEDATA";
    case CMD_UPDATEBUFFER:
        return "CMD_UPDATEBUFFER";
    case CMD_FILLBUFFER:
        return "CMD_FILLBUFFER";
    case CMD_CLEARCOLORIMAGE:
        return "CMD_CLEARCOLORIMAGE";
    case CMD_CLEARATTACHMENTS:
        return "CMD_CLEARCOLORATTACHMENT";
    case CMD_CLEARDEPTHSTENCILIMAGE:
        return "CMD_CLEARDEPTHSTENCILIMAGE";
    case CMD_RESOLVEIMAGE:
        return "CMD_RESOLVEIMAGE";
    case CMD_SETEVENT:
        return "CMD_SETEVENT";
    case CMD_RESETEVENT:
        return "CMD_RESETEVENT";
    case CMD_WAITEVENTS:
        return "CMD_WAITEVENTS";
    case CMD_PIPELINEBARRIER:
        return "CMD_PIPELINEBARRIER";
    case CMD_BEGINQUERY:
        return "CMD_BEGINQUERY";
    case CMD_ENDQUERY:
        return "CMD_ENDQUERY";
    case CMD_RESETQUERYPOOL:
        return "CMD_RESETQUERYPOOL";
    case CMD_COPYQUERYPOOLRESULTS:
        return "CMD_COPYQUERYPOOLRESULTS";
    case CMD_WRITETIMESTAMP:
        return "CMD_WRITETIMESTAMP";
    case CMD_INITATOMICCOUNTERS:
        return "CMD_INITATOMICCOUNTERS";
    case CMD_LOADATOMICCOUNTERS:
        return "CMD_LOADATOMICCOUNTERS";
    case CMD_SAVEATOMICCOUNTERS:
        return "CMD_SAVEATOMICCOUNTERS";
    case CMD_BEGINRENDERPASS:
        return "CMD_BEGINRENDERPASS";
    case CMD_ENDRENDERPASS:
        return "CMD_ENDRENDERPASS";
    default:
        return "UNKNOWN";
    }
}

// SPIRV utility functions
static void build_def_index(shader_module *module) {
    for (auto insn : *module) {
        switch (insn.opcode()) {
        /* Types */
        case spv::OpTypeVoid:
        case spv::OpTypeBool:
        case spv::OpTypeInt:
        case spv::OpTypeFloat:
        case spv::OpTypeVector:
        case spv::OpTypeMatrix:
        case spv::OpTypeImage:
        case spv::OpTypeSampler:
        case spv::OpTypeSampledImage:
        case spv::OpTypeArray:
        case spv::OpTypeRuntimeArray:
        case spv::OpTypeStruct:
        case spv::OpTypeOpaque:
        case spv::OpTypePointer:
        case spv::OpTypeFunction:
        case spv::OpTypeEvent:
        case spv::OpTypeDeviceEvent:
        case spv::OpTypeReserveId:
        case spv::OpTypeQueue:
        case spv::OpTypePipe:
            module->def_index[insn.word(1)] = insn.offset();
            break;

        /* Fixed constants */
        case spv::OpConstantTrue:
        case spv::OpConstantFalse:
        case spv::OpConstant:
        case spv::OpConstantComposite:
        case spv::OpConstantSampler:
        case spv::OpConstantNull:
            module->def_index[insn.word(2)] = insn.offset();
            break;

        /* Specialization constants */
        case spv::OpSpecConstantTrue:
        case spv::OpSpecConstantFalse:
        case spv::OpSpecConstant:
        case spv::OpSpecConstantComposite:
        case spv::OpSpecConstantOp:
            module->def_index[insn.word(2)] = insn.offset();
            break;

        /* Variables */
        case spv::OpVariable:
            module->def_index[insn.word(2)] = insn.offset();
            break;

        /* Functions */
        case spv::OpFunction:
            module->def_index[insn.word(2)] = insn.offset();
            break;

        default:
            /* We don't care about any other defs for now. */
            break;
        }
    }
}

static spirv_inst_iter find_entrypoint(shader_module *src, char const *name, VkShaderStageFlagBits stageBits) {
    for (auto insn : *src) {
        if (insn.opcode() == spv::OpEntryPoint) {
            auto entrypointName = (char const *)&insn.word(3);
            auto entrypointStageBits = 1u << insn.word(1);

            if (!strcmp(entrypointName, name) && (entrypointStageBits & stageBits)) {
                return insn;
            }
        }
    }

    return src->end();
}

static char const *storage_class_name(unsigned sc) {
    switch (sc) {
    case spv::StorageClassInput:
        return "input";
    case spv::StorageClassOutput:
        return "output";
    case spv::StorageClassUniformConstant:
        return "const uniform";
    case spv::StorageClassUniform:
        return "uniform";
    case spv::StorageClassWorkgroup:
        return "workgroup local";
    case spv::StorageClassCrossWorkgroup:
        return "workgroup global";
    case spv::StorageClassPrivate:
        return "private global";
    case spv::StorageClassFunction:
        return "function";
    case spv::StorageClassGeneric:
        return "generic";
    case spv::StorageClassAtomicCounter:
        return "atomic counter";
    case spv::StorageClassImage:
        return "image";
    case spv::StorageClassPushConstant:
        return "push constant";
    default:
        return "unknown";
    }
}

/* get the value of an integral constant */
unsigned get_constant_value(shader_module const *src, unsigned id) {
    auto value = src->get_def(id);
    assert(value != src->end());

    if (value.opcode() != spv::OpConstant) {
        /* TODO: Either ensure that the specialization transform is already performed on a module we're
            considering here, OR -- specialize on the fly now.
            */
        return 1;
    }

    return value.word(3);
}


static void describe_type_inner(std::ostringstream &ss, shader_module const *src, unsigned type) {
    auto insn = src->get_def(type);
    assert(insn != src->end());

    switch (insn.opcode()) {
    case spv::OpTypeBool:
        ss << "bool";
        break;
    case spv::OpTypeInt:
        ss << (insn.word(3) ? 's' : 'u') << "int" << insn.word(2);
        break;
    case spv::OpTypeFloat:
        ss << "float" << insn.word(2);
        break;
    case spv::OpTypeVector:
        ss << "vec" << insn.word(3) << " of ";
        describe_type_inner(ss, src, insn.word(2));
        break;
    case spv::OpTypeMatrix:
        ss << "mat" << insn.word(3) << " of ";
        describe_type_inner(ss, src, insn.word(2));
        break;
    case spv::OpTypeArray:
        ss << "arr[" << get_constant_value(src, insn.word(3)) << "] of ";
        describe_type_inner(ss, src, insn.word(2));
        break;
    case spv::OpTypePointer:
        ss << "ptr to " << storage_class_name(insn.word(2)) << " ";
        describe_type_inner(ss, src, insn.word(3));
        break;
    case spv::OpTypeStruct: {
        ss << "struct of (";
        for (unsigned i = 2; i < insn.len(); i++) {
            describe_type_inner(ss, src, insn.word(i));
            if (i == insn.len() - 1) {
                ss << ")";
            } else {
                ss << ", ";
            }
        }
        break;
    }
    case spv::OpTypeSampler:
        ss << "sampler";
        break;
    case spv::OpTypeSampledImage:
        ss << "sampler+";
        describe_type_inner(ss, src, insn.word(2));
        break;
    case spv::OpTypeImage:
        ss << "image(dim=" << insn.word(3) << ", sampled=" << insn.word(7) << ")";
        break;
    default:
        ss << "oddtype";
        break;
    }
}


static std::string describe_type(shader_module const *src, unsigned type) {
    std::ostringstream ss;
    describe_type_inner(ss, src, type);
    return ss.str();
}


static bool is_narrow_numeric_type(spirv_inst_iter type)
{
    if (type.opcode() != spv::OpTypeInt && type.opcode() != spv::OpTypeFloat)
        return false;
    return type.word(2) < 64;
}


static bool types_match(shader_module const *a, shader_module const *b, unsigned a_type, unsigned b_type, bool a_arrayed, bool b_arrayed, bool relaxed) {
    /* walk two type trees together, and complain about differences */
    auto a_insn = a->get_def(a_type);
    auto b_insn = b->get_def(b_type);
    assert(a_insn != a->end());
    assert(b_insn != b->end());

    if (a_arrayed && a_insn.opcode() == spv::OpTypeArray) {
        return types_match(a, b, a_insn.word(2), b_type, false, b_arrayed, relaxed);
    }

    if (b_arrayed && b_insn.opcode() == spv::OpTypeArray) {
        /* we probably just found the extra level of arrayness in b_type: compare the type inside it to a_type */
        return types_match(a, b, a_type, b_insn.word(2), a_arrayed, false, relaxed);
    }

    if (a_insn.opcode() == spv::OpTypeVector && relaxed && is_narrow_numeric_type(b_insn)) {
        return types_match(a, b, a_insn.word(2), b_type, a_arrayed, b_arrayed, false);
    }

    if (a_insn.opcode() != b_insn.opcode()) {
        return false;
    }

    if (a_insn.opcode() == spv::OpTypePointer) {
        /* match on pointee type. storage class is expected to differ */
        return types_match(a, b, a_insn.word(3), b_insn.word(3), a_arrayed, b_arrayed, relaxed);
    }

    if (a_arrayed || b_arrayed) {
        /* if we havent resolved array-of-verts by here, we're not going to. */
        return false;
    }

    switch (a_insn.opcode()) {
    case spv::OpTypeBool:
        return true;
    case spv::OpTypeInt:
        /* match on width, signedness */
        return a_insn.word(2) == b_insn.word(2) && a_insn.word(3) == b_insn.word(3);
    case spv::OpTypeFloat:
        /* match on width */
        return a_insn.word(2) == b_insn.word(2);
    case spv::OpTypeVector:
        /* match on element type, count. */
        if (!types_match(a, b, a_insn.word(2), b_insn.word(2), a_arrayed, b_arrayed, false))
            return false;
        if (relaxed && is_narrow_numeric_type(a->get_def(a_insn.word(2)))) {
            return a_insn.word(3) >= b_insn.word(3);
        }
        else {
            return a_insn.word(3) == b_insn.word(3);
        }
    case spv::OpTypeMatrix:
        /* match on element type, count. */
        return types_match(a, b, a_insn.word(2), b_insn.word(2), a_arrayed, b_arrayed, false) && a_insn.word(3) == b_insn.word(3);
    case spv::OpTypeArray:
        /* match on element type, count. these all have the same layout. we don't get here if
         * b_arrayed. This differs from vector & matrix types in that the array size is the id of a constant instruction,
         * not a literal within OpTypeArray */
        return types_match(a, b, a_insn.word(2), b_insn.word(2), a_arrayed, b_arrayed, false) &&
               get_constant_value(a, a_insn.word(3)) == get_constant_value(b, b_insn.word(3));
    case spv::OpTypeStruct:
        /* match on all element types */
        {
            if (a_insn.len() != b_insn.len()) {
                return false; /* structs cannot match if member counts differ */
            }

            for (unsigned i = 2; i < a_insn.len(); i++) {
                if (!types_match(a, b, a_insn.word(i), b_insn.word(i), a_arrayed, b_arrayed, false)) {
                    return false;
                }
            }

            return true;
        }
    default:
        /* remaining types are CLisms, or may not appear in the interfaces we
         * are interested in. Just claim no match.
         */
        return false;
    }
}

static int value_or_default(std::unordered_map<unsigned, unsigned> const &map, unsigned id, int def) {
    auto it = map.find(id);
    if (it == map.end())
        return def;
    else
        return it->second;
}

static unsigned get_locations_consumed_by_type(shader_module const *src, unsigned type, bool strip_array_level) {
    auto insn = src->get_def(type);
    assert(insn != src->end());

    switch (insn.opcode()) {
    case spv::OpTypePointer:
        /* see through the ptr -- this is only ever at the toplevel for graphics shaders;
         * we're never actually passing pointers around. */
        return get_locations_consumed_by_type(src, insn.word(3), strip_array_level);
    case spv::OpTypeArray:
        if (strip_array_level) {
            return get_locations_consumed_by_type(src, insn.word(2), false);
        } else {
            return get_constant_value(src, insn.word(3)) * get_locations_consumed_by_type(src, insn.word(2), false);
        }
    case spv::OpTypeMatrix:
        /* num locations is the dimension * element size */
        return insn.word(3) * get_locations_consumed_by_type(src, insn.word(2), false);
    case spv::OpTypeVector: {
        auto scalar_type = src->get_def(insn.word(2));
        auto bit_width = (scalar_type.opcode() == spv::OpTypeInt || scalar_type.opcode() == spv::OpTypeFloat) ?
            scalar_type.word(2) : 32;

        /* locations are 128-bit wide; 3- and 4-component vectors of 64 bit
         * types require two. */
        return (bit_width * insn.word(3) + 127) / 128;
    }
    default:
        /* everything else is just 1. */
        return 1;

        /* TODO: extend to handle 64bit scalar types, whose vectors may need
         * multiple locations. */
    }
}

static unsigned get_locations_consumed_by_format(VkFormat format) {
    switch (format) {
    case VK_FORMAT_R64G64B64A64_SFLOAT:
    case VK_FORMAT_R64G64B64A64_SINT:
    case VK_FORMAT_R64G64B64A64_UINT:
    case VK_FORMAT_R64G64B64_SFLOAT:
    case VK_FORMAT_R64G64B64_SINT:
    case VK_FORMAT_R64G64B64_UINT:
        return 2;
    default:
        return 1;
    }
}

typedef std::pair<unsigned, unsigned> location_t;
typedef std::pair<unsigned, unsigned> descriptor_slot_t;

struct interface_var {
    uint32_t id;
    uint32_t type_id;
    uint32_t offset;
    bool is_patch;
    bool is_block_member;
    /* TODO: collect the name, too? Isn't required to be present. */
};

struct shader_stage_attributes {
    char const *const name;
    bool arrayed_input;
    bool arrayed_output;
};

static shader_stage_attributes shader_stage_attribs[] = {
    {"vertex shader", false, false},
    {"tessellation control shader", true, true},
    {"tessellation evaluation shader", true, false},
    {"geometry shader", true, false},
    {"fragment shader", false, false},
};

static spirv_inst_iter get_struct_type(shader_module const *src, spirv_inst_iter def, bool is_array_of_verts) {
    while (true) {

        if (def.opcode() == spv::OpTypePointer) {
            def = src->get_def(def.word(3));
        } else if (def.opcode() == spv::OpTypeArray && is_array_of_verts) {
            def = src->get_def(def.word(2));
            is_array_of_verts = false;
        } else if (def.opcode() == spv::OpTypeStruct) {
            return def;
        } else {
            return src->end();
        }
    }
}

static void collect_interface_block_members(shader_module const *src,
                                            std::map<location_t, interface_var> *out,
                                            std::unordered_map<unsigned, unsigned> const &blocks, bool is_array_of_verts,
                                            uint32_t id, uint32_t type_id, bool is_patch) {
    /* Walk down the type_id presented, trying to determine whether it's actually an interface block. */
    auto type = get_struct_type(src, src->get_def(type_id), is_array_of_verts && !is_patch);
    if (type == src->end() || blocks.find(type.word(1)) == blocks.end()) {
        /* this isn't an interface block. */
        return;
    }

    std::unordered_map<unsigned, unsigned> member_components;

    /* Walk all the OpMemberDecorate for type's result id -- first pass, collect components. */
    for (auto insn : *src) {
        if (insn.opcode() == spv::OpMemberDecorate && insn.word(1) == type.word(1)) {
            unsigned member_index = insn.word(2);

            if (insn.word(3) == spv::DecorationComponent) {
                unsigned component = insn.word(4);
                member_components[member_index] = component;
            }
        }
    }

    /* Second pass -- produce the output, from Location decorations */
    for (auto insn : *src) {
        if (insn.opcode() == spv::OpMemberDecorate && insn.word(1) == type.word(1)) {
            unsigned member_index = insn.word(2);
            unsigned member_type_id = type.word(2 + member_index);

            if (insn.word(3) == spv::DecorationLocation) {
                unsigned location = insn.word(4);
                unsigned num_locations = get_locations_consumed_by_type(src, member_type_id, false);
                auto component_it = member_components.find(member_index);
                unsigned component = component_it == member_components.end() ? 0 : component_it->second;

                for (unsigned int offset = 0; offset < num_locations; offset++) {
                    interface_var v;
                    v.id = id;
                    /* TODO: member index in interface_var too? */
                    v.type_id = member_type_id;
                    v.offset = offset;
                    v.is_patch = is_patch;
                    v.is_block_member = true;
                    (*out)[std::make_pair(location + offset, component)] = v;
                }
            }
        }
    }
}

static std::map<location_t, interface_var> collect_interface_by_location(
        shader_module const *src, spirv_inst_iter entrypoint,
        spv::StorageClass sinterface, bool is_array_of_verts) {

    std::unordered_map<unsigned, unsigned> var_locations;
    std::unordered_map<unsigned, unsigned> var_builtins;
    std::unordered_map<unsigned, unsigned> var_components;
    std::unordered_map<unsigned, unsigned> blocks;
    std::unordered_map<unsigned, unsigned> var_patch;

    for (auto insn : *src) {

        /* We consider two interface models: SSO rendezvous-by-location, and
         * builtins. Complain about anything that fits neither model.
         */
        if (insn.opcode() == spv::OpDecorate) {
            if (insn.word(2) == spv::DecorationLocation) {
                var_locations[insn.word(1)] = insn.word(3);
            }

            if (insn.word(2) == spv::DecorationBuiltIn) {
                var_builtins[insn.word(1)] = insn.word(3);
            }

            if (insn.word(2) == spv::DecorationComponent) {
                var_components[insn.word(1)] = insn.word(3);
            }

            if (insn.word(2) == spv::DecorationBlock) {
                blocks[insn.word(1)] = 1;
            }

            if (insn.word(2) == spv::DecorationPatch) {
                var_patch[insn.word(1)] = 1;
            }
        }
    }

    /* TODO: handle grouped decorations */
    /* TODO: handle index=1 dual source outputs from FS -- two vars will
     * have the same location, and we DON'T want to clobber. */

    /* find the end of the entrypoint's name string. additional zero bytes follow the actual null
       terminator, to fill out the rest of the word - so we only need to look at the last byte in
       the word to determine which word contains the terminator. */
    uint32_t word = 3;
    while (entrypoint.word(word) & 0xff000000u) {
        ++word;
    }
    ++word;

    std::map<location_t, interface_var> out;

    for (; word < entrypoint.len(); word++) {
        auto insn = src->get_def(entrypoint.word(word));
        assert(insn != src->end());
        assert(insn.opcode() == spv::OpVariable);

        if (insn.word(3) == static_cast<uint32_t>(sinterface)) {
            unsigned id = insn.word(2);
            unsigned type = insn.word(1);

            int location = value_or_default(var_locations, id, -1);
            int builtin = value_or_default(var_builtins, id, -1);
            unsigned component = value_or_default(var_components, id, 0); /* unspecified is OK, is 0 */
            bool is_patch = var_patch.find(id) != var_patch.end();

            /* All variables and interface block members in the Input or Output storage classes
             * must be decorated with either a builtin or an explicit location.
             *
             * TODO: integrate the interface block support here. For now, don't complain --
             * a valid SPIRV module will only hit this path for the interface block case, as the
             * individual members of the type are decorated, rather than variable declarations.
             */

            if (location != -1) {
                /* A user-defined interface variable, with a location. Where a variable
                 * occupied multiple locations, emit one result for each. */
                unsigned num_locations = get_locations_consumed_by_type(src, type, is_array_of_verts && !is_patch);
                for (unsigned int offset = 0; offset < num_locations; offset++) {
                    interface_var v;
                    v.id = id;
                    v.type_id = type;
                    v.offset = offset;
                    v.is_patch = is_patch;
                    v.is_block_member = false;
                    out[std::make_pair(location + offset, component)] = v;
                }
            } else if (builtin == -1) {
                /* An interface block instance */
                collect_interface_block_members(src, &out, blocks, is_array_of_verts, id, type, is_patch);
            }
        }
    }

    return out;
}

static std::vector<std::pair<uint32_t, interface_var>> collect_interface_by_input_attachment_index(
        debug_report_data *report_data, shader_module const *src,
        std::unordered_set<uint32_t> const &accessible_ids) {

    std::vector<std::pair<uint32_t, interface_var>> out;

    for (auto insn : *src) {
        if (insn.opcode() == spv::OpDecorate) {
            if (insn.word(2) == spv::DecorationInputAttachmentIndex) {
                auto attachment_index = insn.word(3);
                auto id = insn.word(1);

                if (accessible_ids.count(id)) {
                    auto def = src->get_def(id);
                    assert(def != src->end());

                    if (def.opcode() == spv::OpVariable && insn.word(3) == spv::StorageClassUniformConstant) {
                        auto num_locations = get_locations_consumed_by_type(src, def.word(1), false);
                        for (unsigned int offset = 0; offset < num_locations; offset++) {
                            interface_var v;
                            v.id = id;
                            v.type_id = def.word(1);
                            v.offset = offset;
                            v.is_patch = false;
                            v.is_block_member = false;
                            out.emplace_back(attachment_index + offset, v);
                        }
                    }
                }
            }
        }
    }

    return out;
}

static std::vector<std::pair<descriptor_slot_t, interface_var>> collect_interface_by_descriptor_slot(
        debug_report_data *report_data, shader_module const *src,
        std::unordered_set<uint32_t> const &accessible_ids) {

    std::unordered_map<unsigned, unsigned> var_sets;
    std::unordered_map<unsigned, unsigned> var_bindings;

    for (auto insn : *src) {
        /* All variables in the Uniform or UniformConstant storage classes are required to be decorated with both
         * DecorationDescriptorSet and DecorationBinding.
         */
        if (insn.opcode() == spv::OpDecorate) {
            if (insn.word(2) == spv::DecorationDescriptorSet) {
                var_sets[insn.word(1)] = insn.word(3);
            }

            if (insn.word(2) == spv::DecorationBinding) {
                var_bindings[insn.word(1)] = insn.word(3);
            }
        }
    }

    std::vector<std::pair<descriptor_slot_t, interface_var>> out;

    for (auto id : accessible_ids) {
        auto insn = src->get_def(id);
        assert(insn != src->end());

        if (insn.opcode() == spv::OpVariable &&
            (insn.word(3) == spv::StorageClassUniform || insn.word(3) == spv::StorageClassUniformConstant)) {
            unsigned set = value_or_default(var_sets, insn.word(2), 0);
            unsigned binding = value_or_default(var_bindings, insn.word(2), 0);

            interface_var v;
            v.id = insn.word(2);
            v.type_id = insn.word(1);
            v.offset = 0;
            v.is_patch = false;
            v.is_block_member = false;
            out.emplace_back(std::make_pair(set, binding), v);
        }
    }

    return out;
}

static bool validate_interface_between_stages(debug_report_data *report_data, shader_module const *producer,
                                              spirv_inst_iter producer_entrypoint, shader_stage_attributes const *producer_stage,
                                              shader_module const *consumer, spirv_inst_iter consumer_entrypoint,
                                              shader_stage_attributes const *consumer_stage) {
    bool pass = true;

    auto outputs = collect_interface_by_location(producer, producer_entrypoint, spv::StorageClassOutput, producer_stage->arrayed_output);
    auto inputs = collect_interface_by_location(consumer, consumer_entrypoint, spv::StorageClassInput, consumer_stage->arrayed_input);

    auto a_it = outputs.begin();
    auto b_it = inputs.begin();

    /* maps sorted by key (location); walk them together to find mismatches */
    while ((outputs.size() > 0 && a_it != outputs.end()) || (inputs.size() && b_it != inputs.end())) {
        bool a_at_end = outputs.size() == 0 || a_it == outputs.end();
        bool b_at_end = inputs.size() == 0 || b_it == inputs.end();
        auto a_first = a_at_end ? std::make_pair(0u, 0u) : a_it->first;
        auto b_first = b_at_end ? std::make_pair(0u, 0u) : b_it->first;

        if (b_at_end || ((!a_at_end) && (a_first < b_first))) {
            if (log_msg(report_data, VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0,
                        __LINE__, SHADER_CHECKER_OUTPUT_NOT_CONSUMED, "SC",
                        "%s writes to output location %u.%u which is not consumed by %s", producer_stage->name, a_first.first,
                        a_first.second, consumer_stage->name)) {
                pass = false;
            }
            a_it++;
        } else if (a_at_end || a_first > b_first) {
            if (log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0,
                        __LINE__, SHADER_CHECKER_INPUT_NOT_PRODUCED, "SC",
                        "%s consumes input location %u.%u which is not written by %s", consumer_stage->name, b_first.first, b_first.second,
                        producer_stage->name)) {
                pass = false;
            }
            b_it++;
        } else {
            // subtleties of arrayed interfaces:
            // - if is_patch, then the member is not arrayed, even though the interface may be.
            // - if is_block_member, then the extra array level of an arrayed interface is not
            //   expressed in the member type -- it's expressed in the block type.
            if (!types_match(producer, consumer, a_it->second.type_id, b_it->second.type_id,
                             producer_stage->arrayed_output && !a_it->second.is_patch && !a_it->second.is_block_member,
                             consumer_stage->arrayed_input && !b_it->second.is_patch && !b_it->second.is_block_member,
                             true)) {
                if (log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0,
                            __LINE__, SHADER_CHECKER_INTERFACE_TYPE_MISMATCH, "SC", "Type mismatch on location %u.%u: '%s' vs '%s'",
                            a_first.first, a_first.second,
                            describe_type(producer, a_it->second.type_id).c_str(),
                            describe_type(consumer, b_it->second.type_id).c_str())) {
                    pass = false;
                }
            }
            if (a_it->second.is_patch != b_it->second.is_patch) {
                if (log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, /*dev*/ 0,
                            __LINE__, SHADER_CHECKER_INTERFACE_TYPE_MISMATCH, "SC",
                            "Decoration mismatch on location %u.%u: is per-%s in %s stage but "
                            "per-%s in %s stage", a_first.first, a_first.second,
                            a_it->second.is_patch ? "patch" : "vertex", producer_stage->name,
                            b_it->second.is_patch ? "patch" : "vertex", consumer_stage->name)) {
                    pass = false;
                }
            }
            a_it++;
            b_it++;
        }
    }

    return pass;
}

enum FORMAT_TYPE {
    FORMAT_TYPE_UNDEFINED,
    FORMAT_TYPE_FLOAT, /* UNORM, SNORM, FLOAT, USCALED, SSCALED, SRGB -- anything we consider float in the shader */
    FORMAT_TYPE_SINT,
    FORMAT_TYPE_UINT,
};

static unsigned get_format_type(VkFormat fmt) {
    switch (fmt) {
    case VK_FORMAT_UNDEFINED:
        return FORMAT_TYPE_UNDEFINED;
    case VK_FORMAT_R8_SINT:
    case VK_FORMAT_R8G8_SINT:
    case VK_FORMAT_R8G8B8_SINT:
    case VK_FORMAT_R8G8B8A8_SINT:
    case VK_FORMAT_R16_SINT:
    case VK_FORMAT_R16G16_SINT:
    case VK_FORMAT_R16G16B16_SINT:
    case VK_FORMAT_R16G16B16A16_SINT:
    case VK_FORMAT_R32_SINT:
    case VK_FORMAT_R32G32_SINT:
    case VK_FORMAT_R32G32B32_SINT:
    case VK_FORMAT_R32G32B32A32_SINT:
    case VK_FORMAT_R64_SINT:
    case VK_FORMAT_R64G64_SINT:
    case VK_FORMAT_R64G64B64_SINT:
    case VK_FORMAT_R64G64B64A64_SINT:
    case VK_FORMAT_B8G8R8_SINT:
    case VK_FORMAT_B8G8R8A8_SINT:
    case VK_FORMAT_A8B8G8R8_SINT_PACK32:
    case VK_FORMAT_A2B10G10R10_SINT_PACK32:
    case VK_FORMAT_A2R10G10B10_SINT_PACK32:
        return FORMAT_TYPE_SINT;
    case VK_FORMAT_R8_UINT:
    case VK_FORMAT_R8G8_UINT:
    case VK_FORMAT_R8G8B8_UINT:
    case VK_FORMAT_R8G8B8A8_UINT:
    case VK_FORMAT_R16_UINT:
    case VK_FORMAT_R16G16_UINT:
    case VK_FORMAT_R16G16B16_UINT:
    case VK_FORMAT_R16G16B16A16_UINT:
    case VK_FORMAT_R32_UINT:
    case VK_FORMAT_R32G32_UINT:
    case VK_FORMAT_R32G32B32_UINT:
    case VK_FORMAT_R32G32B32A32_UINT:
    case VK_FORMAT_R64_UINT:
    case VK_FORMAT_R64G64_UINT:
    case VK_FORMAT_R64G64B64_UINT:
    case VK_FORMAT_R64G64B64A64_UINT:
    case VK_FORMAT_B8G8R8_UINT:
    case VK_FORMAT_B8G8R8A8_UINT:
    case VK_FORMAT_A8B8G8R8_UINT_PACK32:
    case VK_FORMAT_A2B10G10R10_UINT_PACK32:
    case VK_FORMAT_A2R10G10B10_UINT_PACK32:
        return FORMAT_TYPE_UINT;
    default:
        return FORMAT_TYPE_FLOAT;
    }
}

/* characterizes a SPIR-V type appearing in an interface to a FF stage,
 * for comparison to a VkFormat's characterization above. */
static unsigned get_fundamental_type(shader_module const *src, unsigned type) {
    auto insn = src->get_def(type);
    assert(insn != src->end());

    switch (insn.opcode()) {
    case spv::OpTypeInt:
        return insn.word(3) ? FORMAT_TYPE_SINT : FORMAT_TYPE_UINT;
    case spv::OpTypeFloat:
        return FORMAT_TYPE_FLOAT;
    case spv::OpTypeVector:
        return get_fundamental_type(src, insn.word(2));
    case spv::OpTypeMatrix:
        return get_fundamental_type(src, insn.word(2));
    case spv::OpTypeArray:
        return get_fundamental_type(src, insn.word(2));
    case spv::OpTypePointer:
        return get_fundamental_type(src, insn.word(3));
    case spv::OpTypeImage:
        return get_fundamental_type(src, insn.word(2));

    default:
        return FORMAT_TYPE_UNDEFINED;
    }
}

static uint32_t get_shader_stage_id(VkShaderStageFlagBits stage) {
    uint32_t bit_pos = u_ffs(stage);
    return bit_pos - 1;
}

static bool validate_vi_consistency(debug_report_data *report_data, VkPipelineVertexInputStateCreateInfo const *vi) {
    /* walk the binding descriptions, which describe the step rate and stride of each vertex buffer.
     * each binding should be specified only once.
     */
    std::unordered_map<uint32_t, VkVertexInputBindingDescription const *> bindings;
    bool pass = true;

    for (unsigned i = 0; i < vi->vertexBindingDescriptionCount; i++) {
        auto desc = &vi->pVertexBindingDescriptions[i];
        auto &binding = bindings[desc->binding];
        if (binding) {
            if (log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0,
                        __LINE__, SHADER_CHECKER_INCONSISTENT_VI, "SC",
                        "Duplicate vertex input binding descriptions for binding %d", desc->binding)) {
                pass = false;
            }
        } else {
            binding = desc;
        }
    }

    return pass;
}

static bool validate_vi_against_vs_inputs(debug_report_data *report_data, VkPipelineVertexInputStateCreateInfo const *vi,
                                          shader_module const *vs, spirv_inst_iter entrypoint) {
    bool pass = true;

    auto inputs = collect_interface_by_location(vs, entrypoint, spv::StorageClassInput, false);

    /* Build index by location */
    std::map<uint32_t, VkVertexInputAttributeDescription const *> attribs;
    if (vi) {
        for (unsigned i = 0; i < vi->vertexAttributeDescriptionCount; i++) {
            auto num_locations = get_locations_consumed_by_format(vi->pVertexAttributeDescriptions[i].format);
            for (auto j = 0u; j < num_locations; j++) {
                attribs[vi->pVertexAttributeDescriptions[i].location + j] = &vi->pVertexAttributeDescriptions[i];
            }
        }
    }

    auto it_a = attribs.begin();
    auto it_b = inputs.begin();
    bool used = false;

    while ((attribs.size() > 0 && it_a != attribs.end()) || (inputs.size() > 0 && it_b != inputs.end())) {
        bool a_at_end = attribs.size() == 0 || it_a == attribs.end();
        bool b_at_end = inputs.size() == 0 || it_b == inputs.end();
        auto a_first = a_at_end ? 0 : it_a->first;
        auto b_first = b_at_end ? 0 : it_b->first.first;
        if (!a_at_end && (b_at_end || a_first < b_first)) {
            if (!used && log_msg(report_data, VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0,
                        __LINE__, SHADER_CHECKER_OUTPUT_NOT_CONSUMED, "SC",
                        "Vertex attribute at location %d not consumed by vertex shader", a_first)) {
                pass = false;
            }
            used = false;
            it_a++;
        } else if (!b_at_end && (a_at_end || b_first < a_first)) {
            if (log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, /*dev*/ 0,
                        __LINE__, SHADER_CHECKER_INPUT_NOT_PRODUCED, "SC", "Vertex shader consumes input at location %d but not provided",
                        b_first)) {
                pass = false;
            }
            it_b++;
        } else {
            unsigned attrib_type = get_format_type(it_a->second->format);
            unsigned input_type = get_fundamental_type(vs, it_b->second.type_id);

            /* type checking */
            if (attrib_type != FORMAT_TYPE_UNDEFINED && input_type != FORMAT_TYPE_UNDEFINED && attrib_type != input_type) {
                if (log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0,
                            __LINE__, SHADER_CHECKER_INTERFACE_TYPE_MISMATCH, "SC",
                            "Attribute type of `%s` at location %d does not match vertex shader input type of `%s`",
                            string_VkFormat(it_a->second->format), a_first,
                            describe_type(vs, it_b->second.type_id).c_str())) {
                    pass = false;
                }
            }

            /* OK! */
            used = true;
            it_b++;
        }
    }

    return pass;
}

static bool validate_fs_outputs_against_render_pass(debug_report_data *report_data, shader_module const *fs,
                                                    spirv_inst_iter entrypoint, VkRenderPassCreateInfo const *rpci,
                                                    uint32_t subpass_index) {
    std::map<uint32_t, VkFormat> color_attachments;
    auto subpass = rpci->pSubpasses[subpass_index];
    for (auto i = 0u; i < subpass.colorAttachmentCount; ++i) {
        uint32_t attachment = subpass.pColorAttachments[i].attachment;
        if (attachment == VK_ATTACHMENT_UNUSED)
            continue;
        if (rpci->pAttachments[attachment].format != VK_FORMAT_UNDEFINED) {
            color_attachments[i] = rpci->pAttachments[attachment].format;
        }
    }

    bool pass = true;

    /* TODO: dual source blend index (spv::DecIndex, zero if not provided) */

    auto outputs = collect_interface_by_location(fs, entrypoint, spv::StorageClassOutput, false);

    auto it_a = outputs.begin();
    auto it_b = color_attachments.begin();

    /* Walk attachment list and outputs together */

    while ((outputs.size() > 0 && it_a != outputs.end()) || (color_attachments.size() > 0 && it_b != color_attachments.end())) {
        bool a_at_end = outputs.size() == 0 || it_a == outputs.end();
        bool b_at_end = color_attachments.size() == 0 || it_b == color_attachments.end();

        if (!a_at_end && (b_at_end || it_a->first.first < it_b->first)) {
            if (log_msg(report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0,
                        __LINE__, SHADER_CHECKER_OUTPUT_NOT_CONSUMED, "SC",
                        "fragment shader writes to output location %d with no matching attachment", it_a->first.first)) {
                pass = false;
            }
            it_a++;
        } else if (!b_at_end && (a_at_end || it_a->first.first > it_b->first)) {
            if (log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0,
                        __LINE__, SHADER_CHECKER_INPUT_NOT_PRODUCED, "SC", "Attachment %d not written by fragment shader",
                        it_b->first)) {
                pass = false;
            }
            it_b++;
        } else {
            unsigned output_type = get_fundamental_type(fs, it_a->second.type_id);
            unsigned att_type = get_format_type(it_b->second);

            /* type checking */
            if (att_type != FORMAT_TYPE_UNDEFINED && output_type != FORMAT_TYPE_UNDEFINED && att_type != output_type) {
                if (log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0,
                            __LINE__, SHADER_CHECKER_INTERFACE_TYPE_MISMATCH, "SC",
                            "Attachment %d of type `%s` does not match fragment shader output type of `%s`", it_b->first,
                            string_VkFormat(it_b->second),
                            describe_type(fs, it_a->second.type_id).c_str())) {
                    pass = false;
                }
            }

            /* OK! */
            it_a++;
            it_b++;
        }
    }

    return pass;
}

/* For some analyses, we need to know about all ids referenced by the static call tree of a particular
 * entrypoint. This is important for identifying the set of shader resources actually used by an entrypoint,
 * for example.
 * Note: we only explore parts of the image which might actually contain ids we care about for the above analyses.
 *  - NOT the shader input/output interfaces.
 *
 * TODO: The set of interesting opcodes here was determined by eyeballing the SPIRV spec. It might be worth
 * converting parts of this to be generated from the machine-readable spec instead.
 */
static std::unordered_set<uint32_t> mark_accessible_ids(shader_module const *src, spirv_inst_iter entrypoint) {
    std::unordered_set<uint32_t> ids;
    std::unordered_set<uint32_t> worklist;
    worklist.insert(entrypoint.word(2));

    while (!worklist.empty()) {
        auto id_iter = worklist.begin();
        auto id = *id_iter;
        worklist.erase(id_iter);

        auto insn = src->get_def(id);
        if (insn == src->end()) {
            /* id is something we didn't collect in build_def_index. that's OK -- we'll stumble
             * across all kinds of things here that we may not care about. */
            continue;
        }

        /* try to add to the output set */
        if (!ids.insert(id).second) {
            continue; /* if we already saw this id, we don't want to walk it again. */
        }

        switch (insn.opcode()) {
        case spv::OpFunction:
            /* scan whole body of the function, enlisting anything interesting */
            while (++insn, insn.opcode() != spv::OpFunctionEnd) {
                switch (insn.opcode()) {
                case spv::OpLoad:
                case spv::OpAtomicLoad:
                case spv::OpAtomicExchange:
                case spv::OpAtomicCompareExchange:
                case spv::OpAtomicCompareExchangeWeak:
                case spv::OpAtomicIIncrement:
                case spv::OpAtomicIDecrement:
                case spv::OpAtomicIAdd:
                case spv::OpAtomicISub:
                case spv::OpAtomicSMin:
                case spv::OpAtomicUMin:
                case spv::OpAtomicSMax:
                case spv::OpAtomicUMax:
                case spv::OpAtomicAnd:
                case spv::OpAtomicOr:
                case spv::OpAtomicXor:
                    worklist.insert(insn.word(3)); /* ptr */
                    break;
                case spv::OpStore:
                case spv::OpAtomicStore:
                    worklist.insert(insn.word(1)); /* ptr */
                    break;
                case spv::OpAccessChain:
                case spv::OpInBoundsAccessChain:
                    worklist.insert(insn.word(3)); /* base ptr */
                    break;
                case spv::OpSampledImage:
                case spv::OpImageSampleImplicitLod:
                case spv::OpImageSampleExplicitLod:
                case spv::OpImageSampleDrefImplicitLod:
                case spv::OpImageSampleDrefExplicitLod:
                case spv::OpImageSampleProjImplicitLod:
                case spv::OpImageSampleProjExplicitLod:
                case spv::OpImageSampleProjDrefImplicitLod:
                case spv::OpImageSampleProjDrefExplicitLod:
                case spv::OpImageFetch:
                case spv::OpImageGather:
                case spv::OpImageDrefGather:
                case spv::OpImageRead:
                case spv::OpImage:
                case spv::OpImageQueryFormat:
                case spv::OpImageQueryOrder:
                case spv::OpImageQuerySizeLod:
                case spv::OpImageQuerySize:
                case spv::OpImageQueryLod:
                case spv::OpImageQueryLevels:
                case spv::OpImageQuerySamples:
                case spv::OpImageSparseSampleImplicitLod:
                case spv::OpImageSparseSampleExplicitLod:
                case spv::OpImageSparseSampleDrefImplicitLod:
                case spv::OpImageSparseSampleDrefExplicitLod:
                case spv::OpImageSparseSampleProjImplicitLod:
                case spv::OpImageSparseSampleProjExplicitLod:
                case spv::OpImageSparseSampleProjDrefImplicitLod:
                case spv::OpImageSparseSampleProjDrefExplicitLod:
                case spv::OpImageSparseFetch:
                case spv::OpImageSparseGather:
                case spv::OpImageSparseDrefGather:
                case spv::OpImageTexelPointer:
                    worklist.insert(insn.word(3)); /* image or sampled image */
                    break;
                case spv::OpImageWrite:
                    worklist.insert(insn.word(1)); /* image -- different operand order to above */
                    break;
                case spv::OpFunctionCall:
                    for (uint32_t i = 3; i < insn.len(); i++) {
                        worklist.insert(insn.word(i)); /* fn itself, and all args */
                    }
                    break;

                case spv::OpExtInst:
                    for (uint32_t i = 5; i < insn.len(); i++) {
                        worklist.insert(insn.word(i)); /* operands to ext inst */
                    }
                    break;
                }
            }
            break;
        }
    }

    return ids;
}

static bool validate_push_constant_block_against_pipeline(debug_report_data *report_data,
                                                          std::vector<VkPushConstantRange> const *push_constant_ranges,
                                                          shader_module const *src, spirv_inst_iter type,
                                                          VkShaderStageFlagBits stage) {
    bool pass = true;

    /* strip off ptrs etc */
    type = get_struct_type(src, type, false);
    assert(type != src->end());

    /* validate directly off the offsets. this isn't quite correct for arrays
     * and matrices, but is a good first step. TODO: arrays, matrices, weird
     * sizes */
    for (auto insn : *src) {
        if (insn.opcode() == spv::OpMemberDecorate && insn.word(1) == type.word(1)) {

            if (insn.word(3) == spv::DecorationOffset) {
                unsigned offset = insn.word(4);
                auto size = 4; /* bytes; TODO: calculate this based on the type */

                bool found_range = false;
                for (auto const &range : *push_constant_ranges) {
                    if (range.offset <= offset && range.offset + range.size >= offset + size) {
                        found_range = true;

                        if ((range.stageFlags & stage) == 0) {
                            if (log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0,
                                        __LINE__, SHADER_CHECKER_PUSH_CONSTANT_NOT_ACCESSIBLE_FROM_STAGE, "SC",
                                        "Push constant range covering variable starting at "
                                        "offset %u not accessible from stage %s",
                                        offset, string_VkShaderStageFlagBits(stage))) {
                                pass = false;
                            }
                        }

                        break;
                    }
                }

                if (!found_range) {
                    if (log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0,
                                __LINE__, SHADER_CHECKER_PUSH_CONSTANT_OUT_OF_RANGE, "SC",
                                "Push constant range covering variable starting at "
                                "offset %u not declared in layout",
                                offset)) {
                        pass = false;
                    }
                }
            }
        }
    }

    return pass;
}

static bool validate_push_constant_usage(debug_report_data *report_data,
                                         std::vector<VkPushConstantRange> const *push_constant_ranges, shader_module const *src,
                                         std::unordered_set<uint32_t> accessible_ids, VkShaderStageFlagBits stage) {
    bool pass = true;

    for (auto id : accessible_ids) {
        auto def_insn = src->get_def(id);
        if (def_insn.opcode() == spv::OpVariable && def_insn.word(3) == spv::StorageClassPushConstant) {
            pass &= validate_push_constant_block_against_pipeline(report_data, push_constant_ranges, src,
                                                                  src->get_def(def_insn.word(1)), stage);
        }
    }

    return pass;
}

// For given pipelineLayout verify that the set_layout_node at slot.first
//  has the requested binding at slot.second and return ptr to that binding
static VkDescriptorSetLayoutBinding const * get_descriptor_binding(PIPELINE_LAYOUT_NODE const *pipelineLayout, descriptor_slot_t slot) {

    if (!pipelineLayout)
        return nullptr;

    if (slot.first >= pipelineLayout->set_layouts.size())
        return nullptr;

    return pipelineLayout->set_layouts[slot.first]->GetDescriptorSetLayoutBindingPtrFromBinding(slot.second);
}

// Block of code at start here for managing/tracking Pipeline state that this layer cares about

static uint64_t g_drawCount[NUM_DRAW_TYPES] = {0, 0, 0, 0};

// TODO : Should be tracking lastBound per commandBuffer and when draws occur, report based on that cmd buffer lastBound
//   Then need to synchronize the accesses based on cmd buffer so that if I'm reading state on one cmd buffer, updates
//   to that same cmd buffer by separate thread are not changing state from underneath us
// Track the last cmd buffer touched by this thread

static bool hasDrawCmd(GLOBAL_CB_NODE *pCB) {
    for (uint32_t i = 0; i < NUM_DRAW_TYPES; i++) {
        if (pCB->drawCount[i])
            return true;
    }
    return false;
}

// Check object status for selected flag state
static bool validate_status(layer_data *my_data, GLOBAL_CB_NODE *pNode, CBStatusFlags status_mask, VkFlags msg_flags,
                            DRAW_STATE_ERROR error_code, const char *fail_msg) {
    if (!(pNode->status & status_mask)) {
        return log_msg(my_data->report_data, msg_flags, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                       reinterpret_cast<const uint64_t &>(pNode->commandBuffer), __LINE__, error_code, "DS",
                       "command buffer object 0x%" PRIxLEAST64 ": %s", reinterpret_cast<const uint64_t &>(pNode->commandBuffer),
                       fail_msg);
    }
    return false;
}

// Retrieve pipeline node ptr for given pipeline object
static PIPELINE_STATE *getPipelineState(layer_data const *my_data, VkPipeline pipeline) {
    auto it = my_data->pipelineMap.find(pipeline);
    if (it == my_data->pipelineMap.end()) {
        return nullptr;
    }
    return it->second;
}

static RENDER_PASS_STATE *getRenderPassState(layer_data const *my_data, VkRenderPass renderpass) {
    auto it = my_data->renderPassMap.find(renderpass);
    if (it == my_data->renderPassMap.end()) {
        return nullptr;
    }
    return it->second.get();
}

static FRAMEBUFFER_STATE *getFramebufferState(const layer_data *my_data, VkFramebuffer framebuffer) {
    auto it = my_data->frameBufferMap.find(framebuffer);
    if (it == my_data->frameBufferMap.end()) {
        return nullptr;
    }
    return it->second.get();
}

cvdescriptorset::DescriptorSetLayout const *getDescriptorSetLayout(layer_data const *my_data, VkDescriptorSetLayout dsLayout) {
    auto it = my_data->descriptorSetLayoutMap.find(dsLayout);
    if (it == my_data->descriptorSetLayoutMap.end()) {
        return nullptr;
    }
    return it->second;
}

static PIPELINE_LAYOUT_NODE const *getPipelineLayout(layer_data const *my_data, VkPipelineLayout pipeLayout) {
    auto it = my_data->pipelineLayoutMap.find(pipeLayout);
    if (it == my_data->pipelineLayoutMap.end()) {
        return nullptr;
    }
    return &it->second;
}

// Return true if for a given PSO, the given state enum is dynamic, else return false
static bool isDynamic(const PIPELINE_STATE *pPipeline, const VkDynamicState state) {
    if (pPipeline && pPipeline->graphicsPipelineCI.pDynamicState) {
        for (uint32_t i = 0; i < pPipeline->graphicsPipelineCI.pDynamicState->dynamicStateCount; i++) {
            if (state == pPipeline->graphicsPipelineCI.pDynamicState->pDynamicStates[i])
                return true;
        }
    }
    return false;
}

// Validate state stored as flags at time of draw call
static bool validate_draw_state_flags(layer_data *dev_data, GLOBAL_CB_NODE *pCB, const PIPELINE_STATE *pPipe, bool indexedDraw) {
    bool result = false;
    if (pPipe->graphicsPipelineCI.pInputAssemblyState &&
        ((pPipe->graphicsPipelineCI.pInputAssemblyState->topology == VK_PRIMITIVE_TOPOLOGY_LINE_LIST) ||
         (pPipe->graphicsPipelineCI.pInputAssemblyState->topology == VK_PRIMITIVE_TOPOLOGY_LINE_STRIP))) {
        result |= validate_status(dev_data, pCB, CBSTATUS_LINE_WIDTH_SET, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                  DRAWSTATE_LINE_WIDTH_NOT_BOUND, "Dynamic line width state not set for this command buffer");
    }
    if (pPipe->graphicsPipelineCI.pRasterizationState &&
        (pPipe->graphicsPipelineCI.pRasterizationState->depthBiasEnable == VK_TRUE)) {
        result |= validate_status(dev_data, pCB, CBSTATUS_DEPTH_BIAS_SET, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                  DRAWSTATE_DEPTH_BIAS_NOT_BOUND, "Dynamic depth bias state not set for this command buffer");
    }
    if (pPipe->blendConstantsEnabled) {
        result |= validate_status(dev_data, pCB, CBSTATUS_BLEND_CONSTANTS_SET, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                  DRAWSTATE_BLEND_NOT_BOUND, "Dynamic blend constants state not set for this command buffer");
    }
    if (pPipe->graphicsPipelineCI.pDepthStencilState &&
        (pPipe->graphicsPipelineCI.pDepthStencilState->depthBoundsTestEnable == VK_TRUE)) {
        result |= validate_status(dev_data, pCB, CBSTATUS_DEPTH_BOUNDS_SET, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                  DRAWSTATE_DEPTH_BOUNDS_NOT_BOUND, "Dynamic depth bounds state not set for this command buffer");
    }
    if (pPipe->graphicsPipelineCI.pDepthStencilState &&
        (pPipe->graphicsPipelineCI.pDepthStencilState->stencilTestEnable == VK_TRUE)) {
        result |= validate_status(dev_data, pCB, CBSTATUS_STENCIL_READ_MASK_SET, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                  DRAWSTATE_STENCIL_NOT_BOUND, "Dynamic stencil read mask state not set for this command buffer");
        result |= validate_status(dev_data, pCB, CBSTATUS_STENCIL_WRITE_MASK_SET, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                  DRAWSTATE_STENCIL_NOT_BOUND, "Dynamic stencil write mask state not set for this command buffer");
        result |= validate_status(dev_data, pCB, CBSTATUS_STENCIL_REFERENCE_SET, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                  DRAWSTATE_STENCIL_NOT_BOUND, "Dynamic stencil reference state not set for this command buffer");
    }
    if (indexedDraw) {
        result |= validate_status(dev_data, pCB, CBSTATUS_INDEX_BUFFER_BOUND, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                  DRAWSTATE_INDEX_BUFFER_NOT_BOUND,
                                  "Index buffer object not bound to this command buffer when Indexed Draw attempted");
    }
    return result;
}

// Verify attachment reference compatibility according to spec
//  If one array is larger, treat missing elements of shorter array as VK_ATTACHMENT_UNUSED & other array much match this
//  If both AttachmentReference arrays have requested index, check their corresponding AttachmentDescriptions
//   to make sure that format and samples counts match.
//  If not, they are not compatible.
static bool attachment_references_compatible(const uint32_t index, const VkAttachmentReference *pPrimary,
                                             const uint32_t primaryCount, const VkAttachmentDescription *pPrimaryAttachments,
                                             const VkAttachmentReference *pSecondary, const uint32_t secondaryCount,
                                             const VkAttachmentDescription *pSecondaryAttachments) {
    // Check potential NULL cases first to avoid nullptr issues later
    if (pPrimary == nullptr) {
        if (pSecondary == nullptr) {
            return true;
        }
        return false;
    } else if (pSecondary == nullptr) {
        return false;
    }
    if (index >= primaryCount) { // Check secondary as if primary is VK_ATTACHMENT_UNUSED
        if (VK_ATTACHMENT_UNUSED == pSecondary[index].attachment)
            return true;
    } else if (index >= secondaryCount) { // Check primary as if secondary is VK_ATTACHMENT_UNUSED
        if (VK_ATTACHMENT_UNUSED == pPrimary[index].attachment)
            return true;
    } else { // Format and sample count must match
        if ((pPrimary[index].attachment == VK_ATTACHMENT_UNUSED) && (pSecondary[index].attachment == VK_ATTACHMENT_UNUSED)) {
            return true;
        } else if ((pPrimary[index].attachment == VK_ATTACHMENT_UNUSED) || (pSecondary[index].attachment == VK_ATTACHMENT_UNUSED)) {
            return false;
        }
        if ((pPrimaryAttachments[pPrimary[index].attachment].format ==
             pSecondaryAttachments[pSecondary[index].attachment].format) &&
            (pPrimaryAttachments[pPrimary[index].attachment].samples ==
             pSecondaryAttachments[pSecondary[index].attachment].samples))
            return true;
    }
    // Format and sample counts didn't match
    return false;
}
// TODO : Scrub verify_renderpass_compatibility() and validateRenderPassCompatibility() and unify them and/or share code
// For given primary RenderPass object and secondry RenderPassCreateInfo, verify that they're compatible
static bool verify_renderpass_compatibility(const layer_data *my_data, const VkRenderPassCreateInfo *primaryRPCI,
                                            const VkRenderPassCreateInfo *secondaryRPCI, string &errorMsg) {
    if (primaryRPCI->subpassCount != secondaryRPCI->subpassCount) {
        stringstream errorStr;
        errorStr << "RenderPass for primary cmdBuffer has " << primaryRPCI->subpassCount
                 << " subpasses but renderPass for secondary cmdBuffer has " << secondaryRPCI->subpassCount << " subpasses.";
        errorMsg = errorStr.str();
        return false;
    }
    uint32_t spIndex = 0;
    for (spIndex = 0; spIndex < primaryRPCI->subpassCount; ++spIndex) {
        // For each subpass, verify that corresponding color, input, resolve & depth/stencil attachment references are compatible
        uint32_t primaryColorCount = primaryRPCI->pSubpasses[spIndex].colorAttachmentCount;
        uint32_t secondaryColorCount = secondaryRPCI->pSubpasses[spIndex].colorAttachmentCount;
        uint32_t colorMax = std::max(primaryColorCount, secondaryColorCount);
        for (uint32_t cIdx = 0; cIdx < colorMax; ++cIdx) {
            if (!attachment_references_compatible(cIdx, primaryRPCI->pSubpasses[spIndex].pColorAttachments, primaryColorCount,
                                                  primaryRPCI->pAttachments, secondaryRPCI->pSubpasses[spIndex].pColorAttachments,
                                                  secondaryColorCount, secondaryRPCI->pAttachments)) {
                stringstream errorStr;
                errorStr << "color attachments at index " << cIdx << " of subpass index " << spIndex << " are not compatible.";
                errorMsg = errorStr.str();
                return false;
            } else if (!attachment_references_compatible(cIdx, primaryRPCI->pSubpasses[spIndex].pResolveAttachments,
                                                         primaryColorCount, primaryRPCI->pAttachments,
                                                         secondaryRPCI->pSubpasses[spIndex].pResolveAttachments,
                                                         secondaryColorCount, secondaryRPCI->pAttachments)) {
                stringstream errorStr;
                errorStr << "resolve attachments at index " << cIdx << " of subpass index " << spIndex << " are not compatible.";
                errorMsg = errorStr.str();
                return false;
            }
        }

        if (!attachment_references_compatible(0, primaryRPCI->pSubpasses[spIndex].pDepthStencilAttachment,
                                              1, primaryRPCI->pAttachments,
                                              secondaryRPCI->pSubpasses[spIndex].pDepthStencilAttachment,
                                              1, secondaryRPCI->pAttachments)) {
            stringstream errorStr;
            errorStr << "depth/stencil attachments of subpass index " << spIndex << " are not compatible.";
            errorMsg = errorStr.str();
            return false;
        }

        uint32_t primaryInputCount = primaryRPCI->pSubpasses[spIndex].inputAttachmentCount;
        uint32_t secondaryInputCount = secondaryRPCI->pSubpasses[spIndex].inputAttachmentCount;
        uint32_t inputMax = std::max(primaryInputCount, secondaryInputCount);
        for (uint32_t i = 0; i < inputMax; ++i) {
            if (!attachment_references_compatible(i, primaryRPCI->pSubpasses[spIndex].pInputAttachments, primaryColorCount,
                                                  primaryRPCI->pAttachments, secondaryRPCI->pSubpasses[spIndex].pInputAttachments,
                                                  secondaryColorCount, secondaryRPCI->pAttachments)) {
                stringstream errorStr;
                errorStr << "input attachments at index " << i << " of subpass index " << spIndex << " are not compatible.";
                errorMsg = errorStr.str();
                return false;
            }
        }
    }
    return true;
}

// For given cvdescriptorset::DescriptorSet, verify that its Set is compatible w/ the setLayout corresponding to
// pipelineLayout[layoutIndex]
static bool verify_set_layout_compatibility(layer_data *my_data, const cvdescriptorset::DescriptorSet *pSet,
                                            PIPELINE_LAYOUT_NODE const *pipeline_layout, const uint32_t layoutIndex,
                                            string &errorMsg) {
    auto num_sets = pipeline_layout->set_layouts.size();
    if (layoutIndex >= num_sets) {
        stringstream errorStr;
        errorStr << "VkPipelineLayout (" << pipeline_layout->layout << ") only contains " << num_sets
                 << " setLayouts corresponding to sets 0-" << num_sets - 1 << ", but you're attempting to bind set to index "
                 << layoutIndex;
        errorMsg = errorStr.str();
        return false;
    }
    auto layout_node = pipeline_layout->set_layouts[layoutIndex];
    return pSet->IsCompatible(layout_node, &errorMsg);
}

// Validate that data for each specialization entry is fully contained within the buffer.
static bool validate_specialization_offsets(debug_report_data *report_data, VkPipelineShaderStageCreateInfo const *info) {
    bool pass = true;

    VkSpecializationInfo const *spec = info->pSpecializationInfo;

    if (spec) {
        for (auto i = 0u; i < spec->mapEntryCount; i++) {
            if (spec->pMapEntries[i].offset + spec->pMapEntries[i].size > spec->dataSize) {
                if (log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                            /*dev*/ 0, __LINE__, SHADER_CHECKER_BAD_SPECIALIZATION, "SC",
                            "Specialization entry %u (for constant id %u) references memory outside provided "
                            "specialization data (bytes %u.." PRINTF_SIZE_T_SPECIFIER "; " PRINTF_SIZE_T_SPECIFIER
                            " bytes provided)",
                            i, spec->pMapEntries[i].constantID, spec->pMapEntries[i].offset,
                            spec->pMapEntries[i].offset + spec->pMapEntries[i].size - 1, spec->dataSize)) {

                    pass = false;
                }
            }
        }
    }

    return pass;
}

static bool descriptor_type_match(shader_module const *module, uint32_t type_id,
                                  VkDescriptorType descriptor_type, unsigned &descriptor_count) {
    auto type = module->get_def(type_id);

    descriptor_count = 1;

    /* Strip off any array or ptrs. Where we remove array levels, adjust the
     * descriptor count for each dimension. */
    while (type.opcode() == spv::OpTypeArray || type.opcode() == spv::OpTypePointer) {
        if (type.opcode() == spv::OpTypeArray) {
            descriptor_count *= get_constant_value(module, type.word(3));
            type = module->get_def(type.word(2));
        }
        else {
            type = module->get_def(type.word(3));
        }
    }

    switch (type.opcode()) {
    case spv::OpTypeStruct: {
        for (auto insn : *module) {
            if (insn.opcode() == spv::OpDecorate && insn.word(1) == type.word(1)) {
                if (insn.word(2) == spv::DecorationBlock) {
                    return descriptor_type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
                           descriptor_type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
                } else if (insn.word(2) == spv::DecorationBufferBlock) {
                    return descriptor_type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ||
                           descriptor_type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
                }
            }
        }

        /* Invalid */
        return false;
    }

    case spv::OpTypeSampler:
        return descriptor_type == VK_DESCRIPTOR_TYPE_SAMPLER ||
            descriptor_type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

    case spv::OpTypeSampledImage:
        if (descriptor_type == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER) {
            /* Slight relaxation for some GLSL historical madness: samplerBuffer
             * doesn't really have a sampler, and a texel buffer descriptor
             * doesn't really provide one. Allow this slight mismatch.
             */
            auto image_type = module->get_def(type.word(2));
            auto dim = image_type.word(3);
            auto sampled = image_type.word(7);
            return dim == spv::DimBuffer && sampled == 1;
        }
        return descriptor_type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

    case spv::OpTypeImage: {
        /* Many descriptor types backing image types-- depends on dimension
         * and whether the image will be used with a sampler. SPIRV for
         * Vulkan requires that sampled be 1 or 2 -- leaving the decision to
         * runtime is unacceptable.
         */
        auto dim = type.word(3);
        auto sampled = type.word(7);

        if (dim == spv::DimSubpassData) {
            return descriptor_type == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        } else if (dim == spv::DimBuffer) {
            if (sampled == 1) {
                return descriptor_type == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
            } else {
                return descriptor_type == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
            }
        } else if (sampled == 1) {
            return descriptor_type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ||
                descriptor_type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        } else {
            return descriptor_type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        }
    }

    /* We shouldn't really see any other junk types -- but if we do, they're
     * a mismatch.
     */
    default:
        return false; /* Mismatch */
    }
}

static bool require_feature(debug_report_data *report_data, VkBool32 feature, char const *feature_name) {
    if (!feature) {
        if (log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0,
                    __LINE__, SHADER_CHECKER_FEATURE_NOT_ENABLED, "SC",
                    "Shader requires VkPhysicalDeviceFeatures::%s but is not "
                    "enabled on the device",
                    feature_name)) {
            return false;
        }
    }

    return true;
}

static bool validate_shader_capabilities(debug_report_data *report_data, shader_module const *src,
                                         VkPhysicalDeviceFeatures const *enabledFeatures) {
    bool pass = true;


    for (auto insn : *src) {
        if (insn.opcode() == spv::OpCapability) {
            switch (insn.word(1)) {
            case spv::CapabilityMatrix:
            case spv::CapabilityShader:
            case spv::CapabilityInputAttachment:
            case spv::CapabilitySampled1D:
            case spv::CapabilityImage1D:
            case spv::CapabilitySampledBuffer:
            case spv::CapabilityImageBuffer:
            case spv::CapabilityImageQuery:
            case spv::CapabilityDerivativeControl:
                // Always supported by a Vulkan 1.0 implementation -- no feature bits.
                break;

            case spv::CapabilityGeometry:
                pass &= require_feature(report_data, enabledFeatures->geometryShader, "geometryShader");
                break;

            case spv::CapabilityTessellation:
                pass &= require_feature(report_data, enabledFeatures->tessellationShader, "tessellationShader");
                break;

            case spv::CapabilityFloat64:
                pass &= require_feature(report_data, enabledFeatures->shaderFloat64, "shaderFloat64");
                break;

            case spv::CapabilityInt64:
                pass &= require_feature(report_data, enabledFeatures->shaderInt64, "shaderInt64");
                break;

            case spv::CapabilityTessellationPointSize:
            case spv::CapabilityGeometryPointSize:
                pass &= require_feature(report_data, enabledFeatures->shaderTessellationAndGeometryPointSize,
                                        "shaderTessellationAndGeometryPointSize");
                break;

            case spv::CapabilityImageGatherExtended:
                pass &= require_feature(report_data, enabledFeatures->shaderImageGatherExtended, "shaderImageGatherExtended");
                break;

            case spv::CapabilityStorageImageMultisample:
                pass &= require_feature(report_data, enabledFeatures->shaderStorageImageMultisample, "shaderStorageImageMultisample");
                break;

            case spv::CapabilityUniformBufferArrayDynamicIndexing:
                pass &= require_feature(report_data, enabledFeatures->shaderUniformBufferArrayDynamicIndexing,
                                        "shaderUniformBufferArrayDynamicIndexing");
                break;

            case spv::CapabilitySampledImageArrayDynamicIndexing:
                pass &= require_feature(report_data, enabledFeatures->shaderSampledImageArrayDynamicIndexing,
                                        "shaderSampledImageArrayDynamicIndexing");
                break;

            case spv::CapabilityStorageBufferArrayDynamicIndexing:
                pass &= require_feature(report_data, enabledFeatures->shaderStorageBufferArrayDynamicIndexing,
                                        "shaderStorageBufferArrayDynamicIndexing");
                break;

            case spv::CapabilityStorageImageArrayDynamicIndexing:
                pass &= require_feature(report_data, enabledFeatures->shaderStorageImageArrayDynamicIndexing,
                                        "shaderStorageImageArrayDynamicIndexing");
                break;

            case spv::CapabilityClipDistance:
                pass &= require_feature(report_data, enabledFeatures->shaderClipDistance, "shaderClipDistance");
                break;

            case spv::CapabilityCullDistance:
                pass &= require_feature(report_data, enabledFeatures->shaderCullDistance, "shaderCullDistance");
                break;

            case spv::CapabilityImageCubeArray:
                pass &= require_feature(report_data, enabledFeatures->imageCubeArray, "imageCubeArray");
                break;

            case spv::CapabilitySampleRateShading:
                pass &= require_feature(report_data, enabledFeatures->sampleRateShading, "sampleRateShading");
                break;

            case spv::CapabilitySparseResidency:
                pass &= require_feature(report_data, enabledFeatures->shaderResourceResidency, "shaderResourceResidency");
                break;

            case spv::CapabilityMinLod:
                pass &= require_feature(report_data, enabledFeatures->shaderResourceMinLod, "shaderResourceMinLod");
                break;

            case spv::CapabilitySampledCubeArray:
                pass &= require_feature(report_data, enabledFeatures->imageCubeArray, "imageCubeArray");
                break;

            case spv::CapabilityImageMSArray:
                pass &= require_feature(report_data, enabledFeatures->shaderStorageImageMultisample, "shaderStorageImageMultisample");
                break;

            case spv::CapabilityStorageImageExtendedFormats:
                pass &= require_feature(report_data, enabledFeatures->shaderStorageImageExtendedFormats,
                                        "shaderStorageImageExtendedFormats");
                break;

            case spv::CapabilityInterpolationFunction:
                pass &= require_feature(report_data, enabledFeatures->sampleRateShading, "sampleRateShading");
                break;

            case spv::CapabilityStorageImageReadWithoutFormat:
                pass &= require_feature(report_data, enabledFeatures->shaderStorageImageReadWithoutFormat,
                                        "shaderStorageImageReadWithoutFormat");
                break;

            case spv::CapabilityStorageImageWriteWithoutFormat:
                pass &= require_feature(report_data, enabledFeatures->shaderStorageImageWriteWithoutFormat,
                                        "shaderStorageImageWriteWithoutFormat");
                break;

            case spv::CapabilityMultiViewport:
                pass &= require_feature(report_data, enabledFeatures->multiViewport, "multiViewport");
                break;

            default:
                if (log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0,
                            __LINE__, SHADER_CHECKER_BAD_CAPABILITY, "SC",
                            "Shader declares capability %u, not supported in Vulkan.",
                            insn.word(1)))
                    pass = false;
                break;
            }
        }
    }

    return pass;
}


static uint32_t descriptor_type_to_reqs(shader_module const *module, uint32_t type_id) {
    auto type = module->get_def(type_id);

    while (true) {
        switch (type.opcode()) {
        case spv::OpTypeArray:
        case spv::OpTypeSampledImage:
            type = module->get_def(type.word(2));
            break;
        case spv::OpTypePointer:
            type = module->get_def(type.word(3));
            break;
        case spv::OpTypeImage: {
            auto dim = type.word(3);
            auto arrayed = type.word(5);
            auto msaa = type.word(6);

            switch (dim) {
            case spv::Dim1D:
                return arrayed ? DESCRIPTOR_REQ_VIEW_TYPE_1D_ARRAY : DESCRIPTOR_REQ_VIEW_TYPE_1D;
            case spv::Dim2D:
                return (msaa ? DESCRIPTOR_REQ_MULTI_SAMPLE : DESCRIPTOR_REQ_SINGLE_SAMPLE) |
                    (arrayed ? DESCRIPTOR_REQ_VIEW_TYPE_2D_ARRAY : DESCRIPTOR_REQ_VIEW_TYPE_2D);
            case spv::Dim3D:
                return DESCRIPTOR_REQ_VIEW_TYPE_3D;
            case spv::DimCube:
                return arrayed ? DESCRIPTOR_REQ_VIEW_TYPE_CUBE_ARRAY : DESCRIPTOR_REQ_VIEW_TYPE_CUBE;
            case spv::DimSubpassData:
                return msaa ? DESCRIPTOR_REQ_MULTI_SAMPLE : DESCRIPTOR_REQ_SINGLE_SAMPLE;
            default:  // buffer, etc.
                return 0;
            }
        }
        default:
            return 0;
        }
    }
}

static bool
validate_pipeline_shader_stage(debug_report_data *report_data, VkPipelineShaderStageCreateInfo const *pStage,
                               PIPELINE_STATE *pipeline, shader_module **out_module, spirv_inst_iter *out_entrypoint,
                               VkPhysicalDeviceFeatures const *enabledFeatures,
                               std::unordered_map<VkShaderModule, std::unique_ptr<shader_module>> const &shaderModuleMap) {
    bool pass = true;
    auto module_it = shaderModuleMap.find(pStage->module);
    auto module = *out_module = module_it->second.get();

    /* find the entrypoint */
    auto entrypoint = *out_entrypoint = find_entrypoint(module, pStage->pName, pStage->stage);
    if (entrypoint == module->end()) {
        if (log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0,
                    __LINE__, SHADER_CHECKER_MISSING_ENTRYPOINT, "SC",
                    "No entrypoint found named `%s` for stage %s", pStage->pName,
                    string_VkShaderStageFlagBits(pStage->stage))) {
            return false;   // no point continuing beyond here, any analysis is just going to be garbage.
        }
    }

    /* validate shader capabilities against enabled device features */
    pass &= validate_shader_capabilities(report_data, module, enabledFeatures);

    /* mark accessible ids */
    auto accessible_ids = mark_accessible_ids(module, entrypoint);

    /* validate descriptor set layout against what the entrypoint actually uses */
    auto descriptor_uses = collect_interface_by_descriptor_slot(report_data, module, accessible_ids);

    auto pipelineLayout = pipeline->pipeline_layout;

    pass &= validate_specialization_offsets(report_data, pStage);
    pass &= validate_push_constant_usage(report_data, &pipelineLayout.push_constant_ranges, module, accessible_ids, pStage->stage);

    /* validate descriptor use */
    for (auto use : descriptor_uses) {
        // While validating shaders capture which slots are used by the pipeline
        auto & reqs = pipeline->active_slots[use.first.first][use.first.second];
        reqs = descriptor_req(reqs | descriptor_type_to_reqs(module, use.second.type_id));

        /* verify given pipelineLayout has requested setLayout with requested binding */
        const auto &binding = get_descriptor_binding(&pipelineLayout, use.first);
        unsigned required_descriptor_count;

        if (!binding) {
            if (log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0,
                        __LINE__, SHADER_CHECKER_MISSING_DESCRIPTOR, "SC",
                        "Shader uses descriptor slot %u.%u (used as type `%s`) but not declared in pipeline layout",
                        use.first.first, use.first.second, describe_type(module, use.second.type_id).c_str())) {
                pass = false;
            }
        } else if (~binding->stageFlags & pStage->stage) {
            if (log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                        /*dev*/ 0, __LINE__, SHADER_CHECKER_DESCRIPTOR_NOT_ACCESSIBLE_FROM_STAGE, "SC",
                        "Shader uses descriptor slot %u.%u (used "
                        "as type `%s`) but descriptor not "
                        "accessible from stage %s",
                        use.first.first, use.first.second, describe_type(module, use.second.type_id).c_str(),
                        string_VkShaderStageFlagBits(pStage->stage))) {
                pass = false;
            }
        } else if (!descriptor_type_match(module, use.second.type_id, binding->descriptorType,
                                          /*out*/ required_descriptor_count)) {
            if (log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0, __LINE__,
                        SHADER_CHECKER_DESCRIPTOR_TYPE_MISMATCH, "SC", "Type mismatch on descriptor slot "
                                                                       "%u.%u (used as type `%s`) but "
                                                                       "descriptor of type %s",
                        use.first.first, use.first.second, describe_type(module, use.second.type_id).c_str(),
                        string_VkDescriptorType(binding->descriptorType))) {
                pass = false;
            }
        } else if (binding->descriptorCount < required_descriptor_count) {
            if (log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0, __LINE__,
                        SHADER_CHECKER_DESCRIPTOR_TYPE_MISMATCH, "SC",
                        "Shader expects at least %u descriptors for binding %u.%u (used as type `%s`) but only %u provided",
                        required_descriptor_count, use.first.first, use.first.second,
                        describe_type(module, use.second.type_id).c_str(), binding->descriptorCount)) {
                pass = false;
            }
        }
    }

    /* validate use of input attachments against subpass structure */
    if (pStage->stage == VK_SHADER_STAGE_FRAGMENT_BIT) {
        auto input_attachment_uses = collect_interface_by_input_attachment_index(report_data, module, accessible_ids);

        auto rpci = pipeline->render_pass_ci.ptr();
        auto subpass = pipeline->graphicsPipelineCI.subpass;

        for (auto use : input_attachment_uses) {
            auto input_attachments = rpci->pSubpasses[subpass].pInputAttachments;
            auto index = (input_attachments && use.first < rpci->pSubpasses[subpass].inputAttachmentCount) ?
                    input_attachments[use.first].attachment : VK_ATTACHMENT_UNUSED;

            if (index == VK_ATTACHMENT_UNUSED) {
                if (log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0, __LINE__,
                            SHADER_CHECKER_MISSING_INPUT_ATTACHMENT, "SC",
                            "Shader consumes input attachment index %d but not provided in subpass",
                            use.first)) {
                    pass = false;
                }
            }
            else if (get_format_type(rpci->pAttachments[index].format) !=
                    get_fundamental_type(module, use.second.type_id)) {
                if (log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0, __LINE__,
                            SHADER_CHECKER_INPUT_ATTACHMENT_TYPE_MISMATCH, "SC",
                            "Subpass input attachment %u format of %s does not match type used in shader `%s`",
                            use.first, string_VkFormat(rpci->pAttachments[index].format),
                            describe_type(module, use.second.type_id).c_str())) {
                    pass = false;
                }
            }
        }
    }

    return pass;
}


// Validate that the shaders used by the given pipeline and store the active_slots
//  that are actually used by the pipeline into pPipeline->active_slots
static bool
validate_and_capture_pipeline_shader_state(debug_report_data *report_data, PIPELINE_STATE *pPipeline,
                                           VkPhysicalDeviceFeatures const *enabledFeatures,
                                           std::unordered_map<VkShaderModule, unique_ptr<shader_module>> const &shaderModuleMap) {
    auto pCreateInfo = pPipeline->graphicsPipelineCI.ptr();
    int vertex_stage = get_shader_stage_id(VK_SHADER_STAGE_VERTEX_BIT);
    int fragment_stage = get_shader_stage_id(VK_SHADER_STAGE_FRAGMENT_BIT);

    shader_module *shaders[5];
    memset(shaders, 0, sizeof(shaders));
    spirv_inst_iter entrypoints[5];
    memset(entrypoints, 0, sizeof(entrypoints));
    VkPipelineVertexInputStateCreateInfo const *vi = 0;
    bool pass = true;

    for (uint32_t i = 0; i < pCreateInfo->stageCount; i++) {
        auto pStage = &pCreateInfo->pStages[i];
        auto stage_id = get_shader_stage_id(pStage->stage);
        pass &= validate_pipeline_shader_stage(report_data, pStage, pPipeline,
                                               &shaders[stage_id], &entrypoints[stage_id],
                                               enabledFeatures, shaderModuleMap);
    }

    // if the shader stages are no good individually, cross-stage validation is pointless.
    if (!pass)
        return false;

    vi = pCreateInfo->pVertexInputState;

    if (vi) {
        pass &= validate_vi_consistency(report_data, vi);
    }

    if (shaders[vertex_stage]) {
        pass &= validate_vi_against_vs_inputs(report_data, vi, shaders[vertex_stage], entrypoints[vertex_stage]);
    }

    int producer = get_shader_stage_id(VK_SHADER_STAGE_VERTEX_BIT);
    int consumer = get_shader_stage_id(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);

    while (!shaders[producer] && producer != fragment_stage) {
        producer++;
        consumer++;
    }

    for (; producer != fragment_stage && consumer <= fragment_stage; consumer++) {
        assert(shaders[producer]);
        if (shaders[consumer]) {
            pass &= validate_interface_between_stages(report_data,
                                                      shaders[producer], entrypoints[producer], &shader_stage_attribs[producer],
                                                      shaders[consumer], entrypoints[consumer], &shader_stage_attribs[consumer]);

            producer = consumer;
        }
    }

    if (shaders[fragment_stage]) {
        pass &= validate_fs_outputs_against_render_pass(report_data, shaders[fragment_stage], entrypoints[fragment_stage],
                                                        pPipeline->render_pass_ci.ptr(), pCreateInfo->subpass);
    }

    return pass;
}

static bool validate_compute_pipeline(debug_report_data *report_data, PIPELINE_STATE *pPipeline,
                                      VkPhysicalDeviceFeatures const *enabledFeatures,
                                      std::unordered_map<VkShaderModule, unique_ptr<shader_module>> const &shaderModuleMap) {
    auto pCreateInfo = pPipeline->computePipelineCI.ptr();

    shader_module *module;
    spirv_inst_iter entrypoint;

    return validate_pipeline_shader_stage(report_data, &pCreateInfo->stage, pPipeline,
                                          &module, &entrypoint, enabledFeatures, shaderModuleMap);
}
// Return Set node ptr for specified set or else NULL
cvdescriptorset::DescriptorSet *getSetNode(const layer_data *my_data, VkDescriptorSet set) {
    auto set_it = my_data->setMap.find(set);
    if (set_it == my_data->setMap.end()) {
        return NULL;
    }
    return set_it->second;
}
// For the given command buffer, verify and update the state for activeSetBindingsPairs
//  This includes:
//  1. Verifying that any dynamic descriptor in that set has a valid dynamic offset bound.
//     To be valid, the dynamic offset combined with the offset and range from its
//     descriptor update must not overflow the size of its buffer being updated
//  2. Grow updateImages for given pCB to include any bound STORAGE_IMAGE descriptor images
//  3. Grow updateBuffers for pCB to include buffers from STORAGE*_BUFFER descriptor buffers
static bool validate_and_update_drawtime_descriptor_state(
    layer_data *dev_data, GLOBAL_CB_NODE *pCB,
    const vector<std::tuple<cvdescriptorset::DescriptorSet *, std::map<uint32_t, descriptor_req>, std::vector<uint32_t> const *>>
        &activeSetBindingsPairs,
    const char *function) {
    bool result = false;
    for (auto set_bindings_pair : activeSetBindingsPairs) {
        cvdescriptorset::DescriptorSet *set_node = std::get<0>(set_bindings_pair);
        std::string err_str;
        if (!set_node->ValidateDrawState(std::get<1>(set_bindings_pair), *std::get<2>(set_bindings_pair),
                                         &err_str)) {
            // Report error here
            auto set = set_node->GetSet();
            result |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT,
                              reinterpret_cast<const uint64_t &>(set), __LINE__, DRAWSTATE_DESCRIPTOR_SET_NOT_UPDATED, "DS",
                              "Descriptor set 0x%" PRIxLEAST64 " encountered the following validation error at %s() time: %s",
                              reinterpret_cast<const uint64_t &>(set), function, err_str.c_str());
        }
        set_node->GetStorageUpdates(std::get<1>(set_bindings_pair), &pCB->updateBuffers, &pCB->updateImages);
    }
    return result;
}

// For given pipeline, return number of MSAA samples, or one if MSAA disabled
static VkSampleCountFlagBits getNumSamples(PIPELINE_STATE const *pipe) {
    if (pipe->graphicsPipelineCI.pMultisampleState != NULL &&
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO == pipe->graphicsPipelineCI.pMultisampleState->sType) {
        return pipe->graphicsPipelineCI.pMultisampleState->rasterizationSamples;
    }
    return VK_SAMPLE_COUNT_1_BIT;
}

static void list_bits(std::ostream& s, uint32_t bits) {
    for (int i = 0; i < 32 && bits; i++) {
        if (bits & (1 << i)) {
            s << i;
            bits &= ~(1 << i);
            if (bits) {
                s << ",";
            }
        }
    }
}

// Validate draw-time state related to the PSO
static bool validatePipelineDrawtimeState(layer_data const *my_data, LAST_BOUND_STATE const &state, const GLOBAL_CB_NODE *pCB,
                                          PIPELINE_STATE const *pPipeline) {
    bool skip_call = false;

    // Verify vertex binding
    if (pPipeline->vertexBindingDescriptions.size() > 0) {
        for (size_t i = 0; i < pPipeline->vertexBindingDescriptions.size(); i++) {
            auto vertex_binding = pPipeline->vertexBindingDescriptions[i].binding;
            if ((pCB->currentDrawData.buffers.size() < (vertex_binding + 1)) ||
                (pCB->currentDrawData.buffers[vertex_binding] == VK_NULL_HANDLE)) {
                skip_call |= log_msg(
                    my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                    DRAWSTATE_VTX_INDEX_OUT_OF_BOUNDS, "DS",
                    "The Pipeline State Object (0x%" PRIxLEAST64 ") expects that this Command Buffer's vertex binding Index %u "
                    "should be set via vkCmdBindVertexBuffers. This is because VkVertexInputBindingDescription struct "
                    "at index " PRINTF_SIZE_T_SPECIFIER " of pVertexBindingDescriptions has a binding value of %u.",
                    (uint64_t)state.pipeline_state->pipeline, vertex_binding, i, vertex_binding);
            }
        }
    } else {
        if (!pCB->currentDrawData.buffers.empty()) {
            skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT, (VkDebugReportObjectTypeEXT)0,
                                 0, __LINE__, DRAWSTATE_VTX_INDEX_OUT_OF_BOUNDS, "DS",
                                 "Vertex buffers are bound to command buffer (0x%" PRIxLEAST64
                                 ") but no vertex buffers are attached to this Pipeline State Object (0x%" PRIxLEAST64 ").",
                                 (uint64_t)pCB->commandBuffer, (uint64_t)state.pipeline_state->pipeline);
        }
    }
    // If Viewport or scissors are dynamic, verify that dynamic count matches PSO count.
    // Skip check if rasterization is disabled or there is no viewport.
    if ((!pPipeline->graphicsPipelineCI.pRasterizationState ||
         (pPipeline->graphicsPipelineCI.pRasterizationState->rasterizerDiscardEnable == VK_FALSE)) &&
        pPipeline->graphicsPipelineCI.pViewportState) {
        bool dynViewport = isDynamic(pPipeline, VK_DYNAMIC_STATE_VIEWPORT);
        bool dynScissor = isDynamic(pPipeline, VK_DYNAMIC_STATE_SCISSOR);

        if (dynViewport) {
            auto requiredViewportsMask = (1 << pPipeline->graphicsPipelineCI.pViewportState->viewportCount) - 1;
            auto missingViewportMask = ~pCB->viewportMask & requiredViewportsMask;
            if (missingViewportMask) {
                std::stringstream ss;
                ss << "Dynamic viewport(s) ";
                list_bits(ss, missingViewportMask);
                ss << " are used by pipeline state object, but were not provided via calls to vkCmdSetViewport().";
                skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0,
                                     __LINE__, DRAWSTATE_VIEWPORT_SCISSOR_MISMATCH, "DS",
                                     "%s", ss.str().c_str());
            }
        }

        if (dynScissor) {
            auto requiredScissorMask = (1 << pPipeline->graphicsPipelineCI.pViewportState->scissorCount) - 1;
            auto missingScissorMask = ~pCB->scissorMask & requiredScissorMask;
            if (missingScissorMask) {
                std::stringstream ss;
                ss << "Dynamic scissor(s) ";
                list_bits(ss, missingScissorMask);
                ss << " are used by pipeline state object, but were not provided via calls to vkCmdSetScissor().";
                skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0,
                                     __LINE__, DRAWSTATE_VIEWPORT_SCISSOR_MISMATCH, "DS",
                                     "%s", ss.str().c_str());
            }
        }
    }

    // Verify that any MSAA request in PSO matches sample# in bound FB
    // Skip the check if rasterization is disabled.
    if (!pPipeline->graphicsPipelineCI.pRasterizationState ||
        (pPipeline->graphicsPipelineCI.pRasterizationState->rasterizerDiscardEnable == VK_FALSE)) {
        VkSampleCountFlagBits pso_num_samples = getNumSamples(pPipeline);
        if (pCB->activeRenderPass) {
            auto const render_pass_info = pCB->activeRenderPass->createInfo.ptr();
            const VkSubpassDescription *subpass_desc = &render_pass_info->pSubpasses[pCB->activeSubpass];
            uint32_t i;

            const safe_VkPipelineColorBlendStateCreateInfo *color_blend_state = pPipeline->graphicsPipelineCI.pColorBlendState;
            if ((color_blend_state != NULL) && (pCB->activeSubpass == pPipeline->graphicsPipelineCI.subpass) &&
                (color_blend_state->attachmentCount != subpass_desc->colorAttachmentCount)) {
                skip_call |=
                        log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT,
                                reinterpret_cast<const uint64_t &>(pPipeline->pipeline), __LINE__, DRAWSTATE_INVALID_RENDERPASS, "DS",
                                "Render pass subpass %u mismatch with blending state defined and blend state attachment "
                                "count %u while subpass color attachment count %u in Pipeline (0x%" PRIxLEAST64 ")!  These "
                                "must be the same at draw-time.",
                                pCB->activeSubpass, color_blend_state->attachmentCount, subpass_desc->colorAttachmentCount,
                                reinterpret_cast<const uint64_t &>(pPipeline->pipeline));
            }

            unsigned subpass_num_samples = 0;

            for (i = 0; i < subpass_desc->colorAttachmentCount; i++) {
                auto attachment = subpass_desc->pColorAttachments[i].attachment;
                if (attachment != VK_ATTACHMENT_UNUSED)
                    subpass_num_samples |= (unsigned)render_pass_info->pAttachments[attachment].samples;
            }

            if (subpass_desc->pDepthStencilAttachment &&
                subpass_desc->pDepthStencilAttachment->attachment != VK_ATTACHMENT_UNUSED) {
                auto attachment = subpass_desc->pDepthStencilAttachment->attachment;
                subpass_num_samples |= (unsigned)render_pass_info->pAttachments[attachment].samples;
            }

            if (subpass_num_samples && static_cast<unsigned>(pso_num_samples) != subpass_num_samples) {
                skip_call |=
                        log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT,
                                reinterpret_cast<const uint64_t &>(pPipeline->pipeline), __LINE__, DRAWSTATE_NUM_SAMPLES_MISMATCH, "DS",
                                "Num samples mismatch! At draw-time in Pipeline (0x%" PRIxLEAST64
                                ") with %u samples while current RenderPass (0x%" PRIxLEAST64 ") w/ %u samples!",
                                reinterpret_cast<const uint64_t &>(pPipeline->pipeline), pso_num_samples,
                                reinterpret_cast<const uint64_t &>(pCB->activeRenderPass->renderPass), subpass_num_samples);
            }
        } else {
            skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT,
                                 reinterpret_cast<const uint64_t &>(pPipeline->pipeline), __LINE__, DRAWSTATE_NUM_SAMPLES_MISMATCH, "DS",
                                 "No active render pass found at draw-time in Pipeline (0x%" PRIxLEAST64 ")!",
                                 reinterpret_cast<const uint64_t &>(pPipeline->pipeline));
        }
    }
    // Verify that PSO creation renderPass is compatible with active renderPass
    if (pCB->activeRenderPass) {
        std::string err_string;
        if ((pCB->activeRenderPass->renderPass != pPipeline->graphicsPipelineCI.renderPass) &&
            !verify_renderpass_compatibility(my_data, pCB->activeRenderPass->createInfo.ptr(), pPipeline->render_pass_ci.ptr(),
                                             err_string)) {
            // renderPass that PSO was created with must be compatible with active renderPass that PSO is being used with
            skip_call |=
                log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT,
                        reinterpret_cast<const uint64_t &>(pPipeline->pipeline), __LINE__, DRAWSTATE_RENDERPASS_INCOMPATIBLE, "DS",
                        "At Draw time the active render pass (0x%" PRIxLEAST64 ") is incompatible w/ gfx pipeline "
                        "(0x%" PRIxLEAST64 ") that was created w/ render pass (0x%" PRIxLEAST64 ") due to: %s",
                        reinterpret_cast<uint64_t &>(pCB->activeRenderPass->renderPass),
                        reinterpret_cast<uint64_t const &>(pPipeline->pipeline),
                        reinterpret_cast<const uint64_t &>(pPipeline->graphicsPipelineCI.renderPass), err_string.c_str());
        }

        if (pPipeline->graphicsPipelineCI.subpass != pCB->activeSubpass) {
            skip_call |=
                log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT,
                        reinterpret_cast<uint64_t const &>(pPipeline->pipeline), __LINE__, DRAWSTATE_RENDERPASS_INCOMPATIBLE, "DS",
                        "Pipeline was built for subpass %u but used in subpass %u", pPipeline->graphicsPipelineCI.subpass,
                        pCB->activeSubpass);
        }
    }
    // TODO : Add more checks here

    return skip_call;
}

// Validate overall state at the time of a draw call
static bool validate_and_update_draw_state(layer_data *my_data, GLOBAL_CB_NODE *cb_node, const bool indexedDraw,
                                           const VkPipelineBindPoint bindPoint, const char *function) {
    bool result = false;
    auto const &state = cb_node->lastBound[bindPoint];
    PIPELINE_STATE *pPipe = state.pipeline_state;
    if (nullptr == pPipe) {
        result |= log_msg(
            my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0, __LINE__,
            DRAWSTATE_INVALID_PIPELINE, "DS",
            "At Draw/Dispatch time no valid VkPipeline is bound! This is illegal. Please bind one with vkCmdBindPipeline().");
        // Early return as any further checks below will be busted w/o a pipeline
        if (result)
            return true;
    }
    // First check flag states
    if (VK_PIPELINE_BIND_POINT_GRAPHICS == bindPoint)
        result = validate_draw_state_flags(my_data, cb_node, pPipe, indexedDraw);

    // Now complete other state checks
    if (VK_NULL_HANDLE != state.pipeline_layout.layout) {
        string errorString;
        auto pipeline_layout = pPipe->pipeline_layout;

        // Need a vector (vs. std::set) of active Sets for dynamicOffset validation in case same set bound w/ different offsets
        vector<std::tuple<cvdescriptorset::DescriptorSet *, std::map<uint32_t, descriptor_req>, std::vector<uint32_t> const *>>
            activeSetBindingsPairs;
        for (auto & setBindingPair : pPipe->active_slots) {
            uint32_t setIndex = setBindingPair.first;
            // If valid set is not bound throw an error
            if ((state.boundDescriptorSets.size() <= setIndex) || (!state.boundDescriptorSets[setIndex])) {
                result |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                  DRAWSTATE_DESCRIPTOR_SET_NOT_BOUND, "DS",
                                  "VkPipeline 0x%" PRIxLEAST64 " uses set #%u but that set is not bound.", (uint64_t)pPipe->pipeline,
                                  setIndex);
            } else if (!verify_set_layout_compatibility(my_data, state.boundDescriptorSets[setIndex], &pipeline_layout, setIndex,
                                                        errorString)) {
                // Set is bound but not compatible w/ overlapping pipeline_layout from PSO
                VkDescriptorSet setHandle = state.boundDescriptorSets[setIndex]->GetSet();
                result |=
                    log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT,
                            (uint64_t)setHandle, __LINE__, DRAWSTATE_PIPELINE_LAYOUTS_INCOMPATIBLE, "DS",
                            "VkDescriptorSet (0x%" PRIxLEAST64
                            ") bound as set #%u is not compatible with overlapping VkPipelineLayout 0x%" PRIxLEAST64 " due to: %s",
                            reinterpret_cast<uint64_t &>(setHandle), setIndex, reinterpret_cast<uint64_t &>(pipeline_layout.layout),
                            errorString.c_str());
            } else { // Valid set is bound and layout compatible, validate that it's updated
                // Pull the set node
                cvdescriptorset::DescriptorSet *pSet = state.boundDescriptorSets[setIndex];
                // Gather active bindings
                std::unordered_set<uint32_t> bindings;
                for (auto binding : setBindingPair.second) {
                    bindings.insert(binding.first);
                }
                // Bind this set and its active descriptor resources to the command buffer
                pSet->BindCommandBuffer(cb_node, bindings);
                // Save vector of all active sets to verify dynamicOffsets below
                activeSetBindingsPairs.push_back(std::make_tuple(pSet, setBindingPair.second, &state.dynamicOffsets[setIndex]));
                // Make sure set has been updated if it has no immutable samplers
                //  If it has immutable samplers, we'll flag error later as needed depending on binding
                if (!pSet->IsUpdated()) {
                    for (auto binding : bindings) {
                        if (!pSet->GetImmutableSamplerPtrFromBinding(binding)) {
                            result |= log_msg(
                                my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT,
                                (uint64_t)pSet->GetSet(), __LINE__, DRAWSTATE_DESCRIPTOR_SET_NOT_UPDATED, "DS",
                                "Descriptor Set 0x%" PRIxLEAST64 " bound but was never updated. It is now being used to draw so "
                                "this will result in undefined behavior.",
                                (uint64_t)pSet->GetSet());
                        }
                    }
                }
            }
        }
        // For given active slots, verify any dynamic descriptors and record updated images & buffers
        result |= validate_and_update_drawtime_descriptor_state(my_data, cb_node, activeSetBindingsPairs, function);
    }

    // Check general pipeline state that needs to be validated at drawtime
    if (VK_PIPELINE_BIND_POINT_GRAPHICS == bindPoint)
        result |= validatePipelineDrawtimeState(my_data, state, cb_node, pPipe);

    return result;
}

// Validate HW line width capabilities prior to setting requested line width.
static bool verifyLineWidth(layer_data *my_data, DRAW_STATE_ERROR dsError, const uint64_t &target, float lineWidth) {
    bool skip_call = false;

    // First check to see if the physical device supports wide lines.
    if ((VK_FALSE == my_data->enabled_features.wideLines) && (1.0f != lineWidth)) {
        skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, target, __LINE__,
                             dsError, "DS", "Attempt to set lineWidth to %f but physical device wideLines feature "
                                            "not supported/enabled so lineWidth must be 1.0f!",
                             lineWidth);
    } else {
        // Otherwise, make sure the width falls in the valid range.
        if ((my_data->phys_dev_properties.properties.limits.lineWidthRange[0] > lineWidth) ||
            (my_data->phys_dev_properties.properties.limits.lineWidthRange[1] < lineWidth)) {
            skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, target,
                                 __LINE__, dsError, "DS", "Attempt to set lineWidth to %f but physical device limits line width "
                                                          "to between [%f, %f]!",
                                 lineWidth, my_data->phys_dev_properties.properties.limits.lineWidthRange[0],
                                 my_data->phys_dev_properties.properties.limits.lineWidthRange[1]);
        }
    }

    return skip_call;
}

// Verify that create state for a pipeline is valid
static bool verifyPipelineCreateState(layer_data *my_data, const VkDevice device, std::vector<PIPELINE_STATE *> pPipelines,
                                      int pipelineIndex) {
    bool skip_call = false;

    PIPELINE_STATE *pPipeline = pPipelines[pipelineIndex];

    // If create derivative bit is set, check that we've specified a base
    // pipeline correctly, and that the base pipeline was created to allow
    // derivatives.
    if (pPipeline->graphicsPipelineCI.flags & VK_PIPELINE_CREATE_DERIVATIVE_BIT) {
        PIPELINE_STATE *pBasePipeline = nullptr;
        if (!((pPipeline->graphicsPipelineCI.basePipelineHandle != VK_NULL_HANDLE) ^
              (pPipeline->graphicsPipelineCI.basePipelineIndex != -1))) {
            skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                 DRAWSTATE_INVALID_PIPELINE_CREATE_STATE, "DS",
                                 "Invalid Pipeline CreateInfo: exactly one of base pipeline index and handle must be specified");
        } else if (pPipeline->graphicsPipelineCI.basePipelineIndex != -1) {
            if (pPipeline->graphicsPipelineCI.basePipelineIndex >= pipelineIndex) {
                skip_call |=
                    log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                            DRAWSTATE_INVALID_PIPELINE_CREATE_STATE, "DS",
                            "Invalid Pipeline CreateInfo: base pipeline must occur earlier in array than derivative pipeline.");
            } else {
                pBasePipeline = pPipelines[pPipeline->graphicsPipelineCI.basePipelineIndex];
            }
        } else if (pPipeline->graphicsPipelineCI.basePipelineHandle != VK_NULL_HANDLE) {
            pBasePipeline = getPipelineState(my_data, pPipeline->graphicsPipelineCI.basePipelineHandle);
        }

        if (pBasePipeline && !(pBasePipeline->graphicsPipelineCI.flags & VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT)) {
            skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                 DRAWSTATE_INVALID_PIPELINE_CREATE_STATE, "DS",
                                 "Invalid Pipeline CreateInfo: base pipeline does not allow derivatives.");
        }
    }

    if (pPipeline->graphicsPipelineCI.pColorBlendState != NULL) {
        if (!my_data->enabled_features.independentBlend) {
            if (pPipeline->attachments.size() > 1) {
                VkPipelineColorBlendAttachmentState *pAttachments = &pPipeline->attachments[0];
                for (size_t i = 1; i < pPipeline->attachments.size(); i++) {
                    // Quoting the spec: "If [the independent blend] feature is not enabled, the VkPipelineColorBlendAttachmentState
                    // settings for all color attachments must be identical." VkPipelineColorBlendAttachmentState contains
                    // only attachment state, so memcmp is best suited for the comparison
                    if (memcmp(static_cast<const void *>(pAttachments), static_cast<const void *>(&pAttachments[i]),
                               sizeof(pAttachments[0]))) {
                        skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0,
                                             __LINE__, DRAWSTATE_INDEPENDENT_BLEND, "DS",
                                             "Invalid Pipeline CreateInfo: If independent blend feature not "
                                             "enabled, all elements of pAttachments must be identical");
                        break;
                    }
                }
            }
        }
        if (!my_data->enabled_features.logicOp &&
            (pPipeline->graphicsPipelineCI.pColorBlendState->logicOpEnable != VK_FALSE)) {
            skip_call |=
                log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                        DRAWSTATE_DISABLED_LOGIC_OP, "DS",
                        "Invalid Pipeline CreateInfo: If logic operations feature not enabled, logicOpEnable must be VK_FALSE");
        }
    }

    // Ensure the subpass index is valid. If not, then validate_and_capture_pipeline_shader_state
    // produces nonsense errors that confuse users. Other layers should already
    // emit errors for renderpass being invalid.
    auto renderPass = getRenderPassState(my_data, pPipeline->graphicsPipelineCI.renderPass);
    if (renderPass && pPipeline->graphicsPipelineCI.subpass >= renderPass->createInfo.subpassCount) {
        skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                             DRAWSTATE_INVALID_PIPELINE_CREATE_STATE, "DS", "Invalid Pipeline CreateInfo State: Subpass index %u "
                                                                            "is out of range for this renderpass (0..%u)",
                             pPipeline->graphicsPipelineCI.subpass, renderPass->createInfo.subpassCount - 1);
    }

    if (!validate_and_capture_pipeline_shader_state(my_data->report_data, pPipeline, &my_data->enabled_features,
                                                    my_data->shaderModuleMap)) {
        skip_call = true;
    }
    // Each shader's stage must be unique
    if (pPipeline->duplicate_shaders) {
        for (uint32_t stage = VK_SHADER_STAGE_VERTEX_BIT; stage & VK_SHADER_STAGE_ALL_GRAPHICS; stage <<= 1) {
            if (pPipeline->duplicate_shaders & stage) {
                skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0,
                                     __LINE__, DRAWSTATE_INVALID_PIPELINE_CREATE_STATE, "DS",
                                     "Invalid Pipeline CreateInfo State: Multiple shaders provided for stage %s",
                                     string_VkShaderStageFlagBits(VkShaderStageFlagBits(stage)));
            }
        }
    }
    // VS is required
    if (!(pPipeline->active_shaders & VK_SHADER_STAGE_VERTEX_BIT)) {
        skip_call |=
            log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                    DRAWSTATE_INVALID_PIPELINE_CREATE_STATE, "DS", "Invalid Pipeline CreateInfo State: Vertex Shader required");
    }
    // Either both or neither TC/TE shaders should be defined
    if (((pPipeline->active_shaders & VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT) == 0) !=
        ((pPipeline->active_shaders & VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT) == 0)) {
        skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                             DRAWSTATE_INVALID_PIPELINE_CREATE_STATE, "DS",
                             "Invalid Pipeline CreateInfo State: TE and TC shaders must be included or excluded as a pair");
    }
    // Compute shaders should be specified independent of Gfx shaders
    if ((pPipeline->active_shaders & VK_SHADER_STAGE_COMPUTE_BIT) &&
        (pPipeline->active_shaders &
         (VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT |
          VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT))) {
        skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                             DRAWSTATE_INVALID_PIPELINE_CREATE_STATE, "DS",
                             "Invalid Pipeline CreateInfo State: Do not specify Compute Shader for Gfx Pipeline");
    }
    // VK_PRIMITIVE_TOPOLOGY_PATCH_LIST primitive topology is only valid for tessellation pipelines.
    // Mismatching primitive topology and tessellation fails graphics pipeline creation.
    if (pPipeline->active_shaders & (VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT) &&
        (!pPipeline->graphicsPipelineCI.pInputAssemblyState ||
         pPipeline->graphicsPipelineCI.pInputAssemblyState->topology != VK_PRIMITIVE_TOPOLOGY_PATCH_LIST)) {
        skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                             DRAWSTATE_INVALID_PIPELINE_CREATE_STATE, "DS", "Invalid Pipeline CreateInfo State: "
                                                                            "VK_PRIMITIVE_TOPOLOGY_PATCH_LIST must be set as IA "
                                                                            "topology for tessellation pipelines");
    }
    if (pPipeline->graphicsPipelineCI.pInputAssemblyState &&
        pPipeline->graphicsPipelineCI.pInputAssemblyState->topology == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST) {
        if (~pPipeline->active_shaders & VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT) {
            skip_call |=
                log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                        DRAWSTATE_INVALID_PIPELINE_CREATE_STATE, "DS", "Invalid Pipeline CreateInfo State: "
                                                                       "VK_PRIMITIVE_TOPOLOGY_PATCH_LIST primitive "
                                                                       "topology is only valid for tessellation pipelines");
        }
        if (!pPipeline->graphicsPipelineCI.pTessellationState) {
            skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                 DRAWSTATE_INVALID_PIPELINE_CREATE_STATE, "DS",
                                 "Invalid Pipeline CreateInfo State: "
                                 "pTessellationState is NULL when VK_PRIMITIVE_TOPOLOGY_PATCH_LIST primitive "
                                 "topology used. pTessellationState must not be NULL in this case.");
        } else if (!pPipeline->graphicsPipelineCI.pTessellationState->patchControlPoints ||
                   (pPipeline->graphicsPipelineCI.pTessellationState->patchControlPoints > 32)) {
            skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                 DRAWSTATE_INVALID_PIPELINE_CREATE_STATE, "DS", "Invalid Pipeline CreateInfo State: "
                                                                                "VK_PRIMITIVE_TOPOLOGY_PATCH_LIST primitive "
                                                                                "topology used with patchControlPoints value %u."
                                                                                " patchControlPoints should be >0 and <=32.",
                                 pPipeline->graphicsPipelineCI.pTessellationState->patchControlPoints);
        }
    }
    // If a rasterization state is provided, make sure that the line width conforms to the HW.
    if (pPipeline->graphicsPipelineCI.pRasterizationState) {
        if (!isDynamic(pPipeline, VK_DYNAMIC_STATE_LINE_WIDTH)) {
            skip_call |= verifyLineWidth(my_data, DRAWSTATE_INVALID_PIPELINE_CREATE_STATE,
                                         reinterpret_cast<uint64_t const &>(pPipeline->pipeline),
                                         pPipeline->graphicsPipelineCI.pRasterizationState->lineWidth);
        }
    }
    // Viewport state must be included if rasterization is enabled.
    // If the viewport state is included, the viewport and scissor counts should always match.
    // NOTE : Even if these are flagged as dynamic, counts need to be set correctly for shader compiler
    if (!pPipeline->graphicsPipelineCI.pRasterizationState ||
        (pPipeline->graphicsPipelineCI.pRasterizationState->rasterizerDiscardEnable == VK_FALSE)) {
        if (!pPipeline->graphicsPipelineCI.pViewportState) {
            skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                 DRAWSTATE_VIEWPORT_SCISSOR_MISMATCH, "DS", "Gfx Pipeline pViewportState is null. Even if viewport "
                                                                            "and scissors are dynamic PSO must include "
                                                                            "viewportCount and scissorCount in pViewportState.");
        } else if (pPipeline->graphicsPipelineCI.pViewportState->scissorCount !=
                   pPipeline->graphicsPipelineCI.pViewportState->viewportCount) {
            skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                 DRAWSTATE_VIEWPORT_SCISSOR_MISMATCH, "DS",
                                 "Gfx Pipeline viewport count (%u) must match scissor count (%u).",
                                 pPipeline->graphicsPipelineCI.pViewportState->viewportCount,
                                 pPipeline->graphicsPipelineCI.pViewportState->scissorCount);
        } else {
            // If viewport or scissor are not dynamic, then verify that data is appropriate for count
            bool dynViewport = isDynamic(pPipeline, VK_DYNAMIC_STATE_VIEWPORT);
            bool dynScissor = isDynamic(pPipeline, VK_DYNAMIC_STATE_SCISSOR);
            if (!dynViewport) {
                if (pPipeline->graphicsPipelineCI.pViewportState->viewportCount &&
                    !pPipeline->graphicsPipelineCI.pViewportState->pViewports) {
                    skip_call |=
                        log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                DRAWSTATE_VIEWPORT_SCISSOR_MISMATCH, "DS",
                                "Gfx Pipeline viewportCount is %u, but pViewports is NULL. For non-zero viewportCount, you "
                                "must either include pViewports data, or include viewport in pDynamicState and set it with "
                                "vkCmdSetViewport().",
                                pPipeline->graphicsPipelineCI.pViewportState->viewportCount);
                }
            }
            if (!dynScissor) {
                if (pPipeline->graphicsPipelineCI.pViewportState->scissorCount &&
                    !pPipeline->graphicsPipelineCI.pViewportState->pScissors) {
                    skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0,
                                         __LINE__, DRAWSTATE_VIEWPORT_SCISSOR_MISMATCH, "DS",
                                         "Gfx Pipeline scissorCount is %u, but pScissors is NULL. For non-zero scissorCount, you "
                                         "must either include pScissors data, or include scissor in pDynamicState and set it with "
                                         "vkCmdSetScissor().",
                                         pPipeline->graphicsPipelineCI.pViewportState->scissorCount);
                }
            }
        }

        // If rasterization is not disabled, and subpass uses a depth/stencil
        // attachment, pDepthStencilState must be a pointer to a valid structure
        auto subpass_desc = renderPass ? &renderPass->createInfo.pSubpasses[pPipeline->graphicsPipelineCI.subpass] : nullptr;
        if (subpass_desc && subpass_desc->pDepthStencilAttachment &&
            subpass_desc->pDepthStencilAttachment->attachment != VK_ATTACHMENT_UNUSED) {
            if (!pPipeline->graphicsPipelineCI.pDepthStencilState) {
                skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                     __LINE__, DRAWSTATE_INVALID_PIPELINE_CREATE_STATE, "DS",
                                     "Invalid Pipeline CreateInfo State: "
                                     "pDepthStencilState is NULL when rasterization is enabled and subpass uses a "
                                     "depth/stencil attachment");
            }
        }
    }
    return skip_call;
}

// Free the Pipeline nodes
static void deletePipelines(layer_data *my_data) {
    if (my_data->pipelineMap.size() <= 0)
        return;
    for (auto &pipe_map_pair : my_data->pipelineMap) {
        delete pipe_map_pair.second;
    }
    my_data->pipelineMap.clear();
}

// Block of code at start here specifically for managing/tracking DSs

// Return Pool node ptr for specified pool or else NULL
DESCRIPTOR_POOL_STATE *getDescriptorPoolState(const layer_data *dev_data, const VkDescriptorPool pool) {
    auto pool_it = dev_data->descriptorPoolMap.find(pool);
    if (pool_it == dev_data->descriptorPoolMap.end()) {
        return NULL;
    }
    return pool_it->second;
}

// Return false if update struct is of valid type, otherwise flag error and return code from callback
static bool validUpdateStruct(layer_data *my_data, const VkDevice device, const GENERIC_HEADER *pUpdateStruct) {
    switch (pUpdateStruct->sType) {
    case VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET:
    case VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET:
        return false;
    default:
        return log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                       DRAWSTATE_INVALID_UPDATE_STRUCT, "DS",
                       "Unexpected UPDATE struct of type %s (value %u) in vkUpdateDescriptors() struct tree",
                       string_VkStructureType(pUpdateStruct->sType), pUpdateStruct->sType);
    }
}

// Set count for given update struct in the last parameter
static uint32_t getUpdateCount(layer_data *my_data, const VkDevice device, const GENERIC_HEADER *pUpdateStruct) {
    switch (pUpdateStruct->sType) {
    case VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET:
        return ((VkWriteDescriptorSet *)pUpdateStruct)->descriptorCount;
    case VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET:
        // TODO : Need to understand this case better and make sure code is correct
        return ((VkCopyDescriptorSet *)pUpdateStruct)->descriptorCount;
    default:
        return 0;
    }
}

// For given layout and update, return the first overall index of the layout that is updated
static uint32_t getUpdateStartIndex(layer_data *my_data, const VkDevice device, const uint32_t binding_start_index,
                                    const uint32_t arrayIndex, const GENERIC_HEADER *pUpdateStruct) {
    return binding_start_index + arrayIndex;
}
// For given layout and update, return the last overall index of the layout that is updated
static uint32_t getUpdateEndIndex(layer_data *my_data, const VkDevice device, const uint32_t binding_start_index,
                                  const uint32_t arrayIndex, const GENERIC_HEADER *pUpdateStruct) {
    uint32_t count = getUpdateCount(my_data, device, pUpdateStruct);
    return binding_start_index + arrayIndex + count - 1;
}
// Verify that the descriptor type in the update struct matches what's expected by the layout
static bool validateUpdateConsistency(layer_data *my_data, const VkDevice device, const VkDescriptorType layout_type,
                                      const GENERIC_HEADER *pUpdateStruct, uint32_t startIndex, uint32_t endIndex) {
    // First get actual type of update
    bool skip_call = false;
    VkDescriptorType actualType = VK_DESCRIPTOR_TYPE_MAX_ENUM;
    switch (pUpdateStruct->sType) {
    case VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET:
        actualType = ((VkWriteDescriptorSet *)pUpdateStruct)->descriptorType;
        break;
    case VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET:
        /* no need to validate */
        return false;
        break;
    default:
        skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                             DRAWSTATE_INVALID_UPDATE_STRUCT, "DS",
                             "Unexpected UPDATE struct of type %s (value %u) in vkUpdateDescriptors() struct tree",
                             string_VkStructureType(pUpdateStruct->sType), pUpdateStruct->sType);
    }
    if (!skip_call) {
        if (layout_type != actualType) {
            skip_call |= log_msg(
                my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                DRAWSTATE_DESCRIPTOR_TYPE_MISMATCH, "DS",
                "Write descriptor update has descriptor type %s that does not match overlapping binding descriptor type of %s!",
                string_VkDescriptorType(actualType), string_VkDescriptorType(layout_type));
        }
    }
    return skip_call;
}
//TODO: Consolidate functions
bool FindLayout(const GLOBAL_CB_NODE *pCB, ImageSubresourcePair imgpair, IMAGE_CMD_BUF_LAYOUT_NODE &node, const VkImageAspectFlags aspectMask) {
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(pCB->commandBuffer), layer_data_map);
    if (!(imgpair.subresource.aspectMask & aspectMask)) {
        return false;
    }
    VkImageAspectFlags oldAspectMask = imgpair.subresource.aspectMask;
    imgpair.subresource.aspectMask = aspectMask;
    auto imgsubIt = pCB->imageLayoutMap.find(imgpair);
    if (imgsubIt == pCB->imageLayoutMap.end()) {
        return false;
    }
    if (node.layout != VK_IMAGE_LAYOUT_MAX_ENUM && node.layout != imgsubIt->second.layout) {
        log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                reinterpret_cast<uint64_t&>(imgpair.image), __LINE__, DRAWSTATE_INVALID_LAYOUT, "DS",
                "Cannot query for VkImage 0x%" PRIx64 " layout when combined aspect mask %d has multiple layout types: %s and %s",
                reinterpret_cast<uint64_t&>(imgpair.image), oldAspectMask, string_VkImageLayout(node.layout), string_VkImageLayout(imgsubIt->second.layout));
    }
    if (node.initialLayout != VK_IMAGE_LAYOUT_MAX_ENUM && node.initialLayout != imgsubIt->second.initialLayout) {
        log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                reinterpret_cast<uint64_t&>(imgpair.image), __LINE__, DRAWSTATE_INVALID_LAYOUT, "DS",
                "Cannot query for VkImage 0x%" PRIx64 " layout when combined aspect mask %d has multiple initial layout types: %s and %s",
                reinterpret_cast<uint64_t&>(imgpair.image), oldAspectMask, string_VkImageLayout(node.initialLayout), string_VkImageLayout(imgsubIt->second.initialLayout));
    }
    node = imgsubIt->second;
    return true;
}

bool FindLayout(const layer_data *my_data, ImageSubresourcePair imgpair, VkImageLayout &layout, const VkImageAspectFlags aspectMask) {
    if (!(imgpair.subresource.aspectMask & aspectMask)) {
        return false;
    }
    VkImageAspectFlags oldAspectMask = imgpair.subresource.aspectMask;
    imgpair.subresource.aspectMask = aspectMask;
    auto imgsubIt = my_data->imageLayoutMap.find(imgpair);
    if (imgsubIt == my_data->imageLayoutMap.end()) {
        return false;
    }
    if (layout != VK_IMAGE_LAYOUT_MAX_ENUM && layout != imgsubIt->second.layout) {
        log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                reinterpret_cast<uint64_t&>(imgpair.image), __LINE__, DRAWSTATE_INVALID_LAYOUT, "DS",
                "Cannot query for VkImage 0x%" PRIx64 " layout when combined aspect mask %d has multiple layout types: %s and %s",
                reinterpret_cast<uint64_t&>(imgpair.image), oldAspectMask, string_VkImageLayout(layout), string_VkImageLayout(imgsubIt->second.layout));
    }
    layout = imgsubIt->second.layout;
    return true;
}

// find layout(s) on the cmd buf level
bool FindLayout(const GLOBAL_CB_NODE *pCB, VkImage image, VkImageSubresource range, IMAGE_CMD_BUF_LAYOUT_NODE &node) {
    ImageSubresourcePair imgpair = {image, true, range};
    node = IMAGE_CMD_BUF_LAYOUT_NODE(VK_IMAGE_LAYOUT_MAX_ENUM, VK_IMAGE_LAYOUT_MAX_ENUM);
    FindLayout(pCB, imgpair, node, VK_IMAGE_ASPECT_COLOR_BIT);
    FindLayout(pCB, imgpair, node, VK_IMAGE_ASPECT_DEPTH_BIT);
    FindLayout(pCB, imgpair, node, VK_IMAGE_ASPECT_STENCIL_BIT);
    FindLayout(pCB, imgpair, node, VK_IMAGE_ASPECT_METADATA_BIT);
    if (node.layout == VK_IMAGE_LAYOUT_MAX_ENUM) {
        imgpair = {image, false, VkImageSubresource()};
        auto imgsubIt = pCB->imageLayoutMap.find(imgpair);
        if (imgsubIt == pCB->imageLayoutMap.end())
            return false;
        node = imgsubIt->second;
    }
    return true;
}

// find layout(s) on the global level
bool FindLayout(const layer_data *my_data, ImageSubresourcePair imgpair, VkImageLayout &layout) {
    layout = VK_IMAGE_LAYOUT_MAX_ENUM;
    FindLayout(my_data, imgpair, layout, VK_IMAGE_ASPECT_COLOR_BIT);
    FindLayout(my_data, imgpair, layout, VK_IMAGE_ASPECT_DEPTH_BIT);
    FindLayout(my_data, imgpair, layout, VK_IMAGE_ASPECT_STENCIL_BIT);
    FindLayout(my_data, imgpair, layout, VK_IMAGE_ASPECT_METADATA_BIT);
    if (layout == VK_IMAGE_LAYOUT_MAX_ENUM) {
        imgpair = {imgpair.image, false, VkImageSubresource()};
        auto imgsubIt = my_data->imageLayoutMap.find(imgpair);
        if (imgsubIt == my_data->imageLayoutMap.end())
            return false;
        layout = imgsubIt->second.layout;
    }
    return true;
}

bool FindLayout(const layer_data *my_data, VkImage image, VkImageSubresource range, VkImageLayout &layout) {
    ImageSubresourcePair imgpair = {image, true, range};
    return FindLayout(my_data, imgpair, layout);
}

bool FindLayouts(const layer_data *my_data, VkImage image, std::vector<VkImageLayout> &layouts) {
    auto sub_data = my_data->imageSubresourceMap.find(image);
    if (sub_data == my_data->imageSubresourceMap.end())
        return false;
    auto image_state = getImageState(my_data, image);
    if (!image_state)
        return false;
    bool ignoreGlobal = false;
    // TODO: Make this robust for >1 aspect mask. Now it will just say ignore
    // potential errors in this case.
    if (sub_data->second.size() >= (image_state->createInfo.arrayLayers * image_state->createInfo.mipLevels + 1)) {
        ignoreGlobal = true;
    }
    for (auto imgsubpair : sub_data->second) {
        if (ignoreGlobal && !imgsubpair.hasSubresource)
            continue;
        auto img_data = my_data->imageLayoutMap.find(imgsubpair);
        if (img_data != my_data->imageLayoutMap.end()) {
            layouts.push_back(img_data->second.layout);
        }
    }
    return true;
}

// Set the layout on the global level
void SetLayout(layer_data *my_data, ImageSubresourcePair imgpair, const VkImageLayout &layout) {
    VkImage &image = imgpair.image;
    // TODO (mlentine): Maybe set format if new? Not used atm.
    my_data->imageLayoutMap[imgpair].layout = layout;
    // TODO (mlentine): Maybe make vector a set?
    auto subresource = std::find(my_data->imageSubresourceMap[image].begin(), my_data->imageSubresourceMap[image].end(), imgpair);
    if (subresource == my_data->imageSubresourceMap[image].end()) {
        my_data->imageSubresourceMap[image].push_back(imgpair);
    }
}

// Set the layout on the cmdbuf level
void SetLayout(GLOBAL_CB_NODE *pCB, ImageSubresourcePair imgpair, const IMAGE_CMD_BUF_LAYOUT_NODE &node) {
    pCB->imageLayoutMap[imgpair] = node;
    // TODO (mlentine): Maybe make vector a set?
    auto subresource =
        std::find(pCB->imageSubresourceMap[imgpair.image].begin(), pCB->imageSubresourceMap[imgpair.image].end(), imgpair);
    if (subresource == pCB->imageSubresourceMap[imgpair.image].end()) {
        pCB->imageSubresourceMap[imgpair.image].push_back(imgpair);
    }
}

void SetLayout(GLOBAL_CB_NODE *pCB, ImageSubresourcePair imgpair, const VkImageLayout &layout) {
    // TODO (mlentine): Maybe make vector a set?
    if (std::find(pCB->imageSubresourceMap[imgpair.image].begin(), pCB->imageSubresourceMap[imgpair.image].end(), imgpair) !=
        pCB->imageSubresourceMap[imgpair.image].end()) {
        pCB->imageLayoutMap[imgpair].layout = layout;
    } else {
        // TODO (mlentine): Could be expensive and might need to be removed.
        assert(imgpair.hasSubresource);
        IMAGE_CMD_BUF_LAYOUT_NODE node;
        if (!FindLayout(pCB, imgpair.image, imgpair.subresource, node)) {
            node.initialLayout = layout;
        }
        SetLayout(pCB, imgpair, {node.initialLayout, layout});
    }
}

template <class OBJECT, class LAYOUT>
void SetLayout(OBJECT *pObject, ImageSubresourcePair imgpair, const LAYOUT &layout, VkImageAspectFlags aspectMask) {
    if (imgpair.subresource.aspectMask & aspectMask) {
        imgpair.subresource.aspectMask = aspectMask;
        SetLayout(pObject, imgpair, layout);
    }
}

template <class OBJECT, class LAYOUT>
void SetLayout(OBJECT *pObject, VkImage image, VkImageSubresource range, const LAYOUT &layout) {
    ImageSubresourcePair imgpair = {image, true, range};
    SetLayout(pObject, imgpair, layout, VK_IMAGE_ASPECT_COLOR_BIT);
    SetLayout(pObject, imgpair, layout, VK_IMAGE_ASPECT_DEPTH_BIT);
    SetLayout(pObject, imgpair, layout, VK_IMAGE_ASPECT_STENCIL_BIT);
    SetLayout(pObject, imgpair, layout, VK_IMAGE_ASPECT_METADATA_BIT);
}

template <class OBJECT, class LAYOUT> void SetLayout(OBJECT *pObject, VkImage image, const LAYOUT &layout) {
    ImageSubresourcePair imgpair = {image, false, VkImageSubresource()};
    SetLayout(pObject, image, imgpair, layout);
}

void SetLayout(const layer_data *dev_data, GLOBAL_CB_NODE *pCB, VkImageView imageView, const VkImageLayout &layout) {
    auto view_state = getImageViewState(dev_data, imageView);
    assert(view_state);
    auto image = view_state->create_info.image;
    const VkImageSubresourceRange &subRange = view_state->create_info.subresourceRange;
    // TODO: Do not iterate over every possibility - consolidate where possible
    for (uint32_t j = 0; j < subRange.levelCount; j++) {
        uint32_t level = subRange.baseMipLevel + j;
        for (uint32_t k = 0; k < subRange.layerCount; k++) {
            uint32_t layer = subRange.baseArrayLayer + k;
            VkImageSubresource sub = {subRange.aspectMask, level, layer};
            // TODO: If ImageView was created with depth or stencil, transition both layouts as
            // the aspectMask is ignored and both are used. Verify that the extra implicit layout
            // is OK for descriptor set layout validation
            if (subRange.aspectMask & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)) {
                if (vk_format_is_depth_and_stencil(view_state->create_info.format)) {
                    sub.aspectMask |= (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
                }
            }
            SetLayout(pCB, image, sub, layout);
        }
    }
}

// Validate that given set is valid and that it's not being used by an in-flight CmdBuffer
// func_str is the name of the calling function
// Return false if no errors occur
// Return true if validation error occurs and callback returns true (to skip upcoming API call down the chain)
static bool validateIdleDescriptorSet(const layer_data *dev_data, VkDescriptorSet set, std::string func_str) {
    if (dev_data->instance_data->disabled.idle_descriptor_set)
        return false;
    bool skip_call = false;
    auto set_node = dev_data->setMap.find(set);
    if (set_node == dev_data->setMap.end()) {
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT,
                             (uint64_t)(set), __LINE__, DRAWSTATE_DOUBLE_DESTROY, "DS",
                             "Cannot call %s() on descriptor set 0x%" PRIxLEAST64 " that has not been allocated.", func_str.c_str(),
                             (uint64_t)(set));
    } else {
        // TODO : This covers various error cases so should pass error enum into this function and use passed in enum here
        if (set_node->second->in_use.load()) {
            skip_call |=
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT,
                        (uint64_t)(set), __LINE__, VALIDATION_ERROR_00919, "DS",
                        "Cannot call %s() on descriptor set 0x%" PRIxLEAST64 " that is in use by a command buffer. %s",
                        func_str.c_str(), (uint64_t)(set), validation_error_map[VALIDATION_ERROR_00919]);
        }
    }
    return skip_call;
}

// Remove set from setMap and delete the set
static void freeDescriptorSet(layer_data *dev_data, cvdescriptorset::DescriptorSet *descriptor_set) {
    dev_data->setMap.erase(descriptor_set->GetSet());
    delete descriptor_set;
}
// Free all DS Pools including their Sets & related sub-structs
// NOTE : Calls to this function should be wrapped in mutex
static void deletePools(layer_data *my_data) {
    if (my_data->descriptorPoolMap.size() <= 0)
        return;
    for (auto ii = my_data->descriptorPoolMap.begin(); ii != my_data->descriptorPoolMap.end(); ++ii) {
        // Remove this pools' sets from setMap and delete them
        for (auto ds : (*ii).second->sets) {
            freeDescriptorSet(my_data, ds);
        }
        (*ii).second->sets.clear();
    }
    my_data->descriptorPoolMap.clear();
}

static void clearDescriptorPool(layer_data *my_data, const VkDevice device, const VkDescriptorPool pool,
                                VkDescriptorPoolResetFlags flags) {
    DESCRIPTOR_POOL_STATE *pPool = getDescriptorPoolState(my_data, pool);
    // TODO: validate flags
    // For every set off of this pool, clear it, remove from setMap, and free cvdescriptorset::DescriptorSet
    for (auto ds : pPool->sets) {
        freeDescriptorSet(my_data, ds);
    }
    pPool->sets.clear();
    // Reset available count for each type and available sets for this pool
    for (uint32_t i = 0; i < pPool->availableDescriptorTypeCount.size(); ++i) {
        pPool->availableDescriptorTypeCount[i] = pPool->maxDescriptorTypeCount[i];
    }
    pPool->availableSets = pPool->maxSets;
}

// For given CB object, fetch associated CB Node from map
static GLOBAL_CB_NODE *getCBNode(layer_data const *my_data, const VkCommandBuffer cb) {
    auto it = my_data->commandBufferMap.find(cb);
    if (it == my_data->commandBufferMap.end()) {
        log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                reinterpret_cast<const uint64_t &>(cb), __LINE__, DRAWSTATE_INVALID_COMMAND_BUFFER, "DS",
                "Attempt to use CommandBuffer 0x%" PRIxLEAST64 " that doesn't exist!", (uint64_t)(cb));
        return NULL;
    }
    return it->second;
}
// Free all CB Nodes
// NOTE : Calls to this function should be wrapped in mutex
static void deleteCommandBuffers(layer_data *my_data) {
    if (my_data->commandBufferMap.empty()) {
        return;
    }
    for (auto ii = my_data->commandBufferMap.begin(); ii != my_data->commandBufferMap.end(); ++ii) {
        delete (*ii).second;
    }
    my_data->commandBufferMap.clear();
}

static bool report_error_no_cb_begin(const layer_data *dev_data, const VkCommandBuffer cb, const char *caller_name) {
    return log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                   (uint64_t)cb, __LINE__, DRAWSTATE_NO_BEGIN_COMMAND_BUFFER, "DS",
                   "You must call vkBeginCommandBuffer() before this call to %s", caller_name);
}

bool validateCmdsInCmdBuffer(const layer_data *dev_data, const GLOBAL_CB_NODE *pCB, const CMD_TYPE cmd_type) {
    if (!pCB->activeRenderPass)
        return false;
    bool skip_call = false;
    if (pCB->activeSubpassContents == VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS &&
        (cmd_type != CMD_EXECUTECOMMANDS && cmd_type != CMD_NEXTSUBPASS && cmd_type != CMD_ENDRENDERPASS)) {
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                             DRAWSTATE_INVALID_COMMAND_BUFFER, "DS",
                             "Commands cannot be called in a subpass using secondary command buffers.");
    } else if (pCB->activeSubpassContents == VK_SUBPASS_CONTENTS_INLINE && cmd_type == CMD_EXECUTECOMMANDS) {
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                             DRAWSTATE_INVALID_COMMAND_BUFFER, "DS",
                             "vkCmdExecuteCommands() cannot be called in a subpass using inline commands.");
    }
    return skip_call;
}

static bool checkGraphicsBit(const layer_data *my_data, VkQueueFlags flags, const char *name) {
    if (!(flags & VK_QUEUE_GRAPHICS_BIT))
        return log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                       DRAWSTATE_INVALID_COMMAND_BUFFER, "DS",
                       "Cannot call %s on a command buffer allocated from a pool without graphics capabilities.", name);
    return false;
}

static bool checkComputeBit(const layer_data *my_data, VkQueueFlags flags, const char *name) {
    if (!(flags & VK_QUEUE_COMPUTE_BIT))
        return log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                       DRAWSTATE_INVALID_COMMAND_BUFFER, "DS",
                       "Cannot call %s on a command buffer allocated from a pool without compute capabilities.", name);
    return false;
}

static bool checkGraphicsOrComputeBit(const layer_data *my_data, VkQueueFlags flags, const char *name) {
    if (!((flags & VK_QUEUE_GRAPHICS_BIT) || (flags & VK_QUEUE_COMPUTE_BIT)))
        return log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                       DRAWSTATE_INVALID_COMMAND_BUFFER, "DS",
                       "Cannot call %s on a command buffer allocated from a pool without graphics capabilities.", name);
    return false;
}

// Add specified CMD to the CmdBuffer in given pCB, flagging errors if CB is not
//  in the recording state or if there's an issue with the Cmd ordering
static bool addCmd(layer_data *my_data, GLOBAL_CB_NODE *pCB, const CMD_TYPE cmd, const char *caller_name) {
    bool skip_call = false;
    auto pPool = getCommandPoolNode(my_data, pCB->createInfo.commandPool);
    if (pPool) {
        VkQueueFlags flags = my_data->phys_dev_properties.queue_family_properties[pPool->queueFamilyIndex].queueFlags;
        switch (cmd) {
        case CMD_BINDPIPELINE:
        case CMD_BINDPIPELINEDELTA:
        case CMD_BINDDESCRIPTORSETS:
        case CMD_FILLBUFFER:
        case CMD_CLEARCOLORIMAGE:
        case CMD_SETEVENT:
        case CMD_RESETEVENT:
        case CMD_WAITEVENTS:
        case CMD_BEGINQUERY:
        case CMD_ENDQUERY:
        case CMD_RESETQUERYPOOL:
        case CMD_COPYQUERYPOOLRESULTS:
        case CMD_WRITETIMESTAMP:
            skip_call |= checkGraphicsOrComputeBit(my_data, flags, cmdTypeToString(cmd).c_str());
            break;
        case CMD_SETVIEWPORTSTATE:
        case CMD_SETSCISSORSTATE:
        case CMD_SETLINEWIDTHSTATE:
        case CMD_SETDEPTHBIASSTATE:
        case CMD_SETBLENDSTATE:
        case CMD_SETDEPTHBOUNDSSTATE:
        case CMD_SETSTENCILREADMASKSTATE:
        case CMD_SETSTENCILWRITEMASKSTATE:
        case CMD_SETSTENCILREFERENCESTATE:
        case CMD_BINDINDEXBUFFER:
        case CMD_BINDVERTEXBUFFER:
        case CMD_DRAW:
        case CMD_DRAWINDEXED:
        case CMD_DRAWINDIRECT:
        case CMD_DRAWINDEXEDINDIRECT:
        case CMD_BLITIMAGE:
        case CMD_CLEARATTACHMENTS:
        case CMD_CLEARDEPTHSTENCILIMAGE:
        case CMD_RESOLVEIMAGE:
        case CMD_BEGINRENDERPASS:
        case CMD_NEXTSUBPASS:
        case CMD_ENDRENDERPASS:
            skip_call |= checkGraphicsBit(my_data, flags, cmdTypeToString(cmd).c_str());
            break;
        case CMD_DISPATCH:
        case CMD_DISPATCHINDIRECT:
            skip_call |= checkComputeBit(my_data, flags, cmdTypeToString(cmd).c_str());
            break;
        case CMD_COPYBUFFER:
        case CMD_COPYIMAGE:
        case CMD_COPYBUFFERTOIMAGE:
        case CMD_COPYIMAGETOBUFFER:
        case CMD_CLONEIMAGEDATA:
        case CMD_UPDATEBUFFER:
        case CMD_PIPELINEBARRIER:
        case CMD_EXECUTECOMMANDS:
        case CMD_END:
            break;
        default:
            break;
        }
    }
    if (pCB->state != CB_RECORDING) {
        skip_call |= report_error_no_cb_begin(my_data, pCB->commandBuffer, caller_name);
    } else {
        skip_call |= validateCmdsInCmdBuffer(my_data, pCB, cmd);
        CMD_NODE cmdNode = {};
        // init cmd node and append to end of cmd LL
        cmdNode.cmdNumber = ++pCB->numCmds;
        cmdNode.type = cmd;
        pCB->cmds.push_back(cmdNode);
    }
    return skip_call;
}
// For given object struct return a ptr of BASE_NODE type for its wrapping struct
BASE_NODE *GetStateStructPtrFromObject(layer_data *dev_data, VK_OBJECT object_struct) {
    BASE_NODE *base_ptr = nullptr;
    switch (object_struct.type) {
    case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT: {
        base_ptr = getSetNode(dev_data, reinterpret_cast<VkDescriptorSet &>(object_struct.handle));
        break;
    }
    case VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT: {
        base_ptr = getSamplerState(dev_data, reinterpret_cast<VkSampler &>(object_struct.handle));
        break;
    }
    case VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT: {
        base_ptr = getQueryPoolNode(dev_data, reinterpret_cast<VkQueryPool &>(object_struct.handle));
        break;
    }
    case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT: {
        base_ptr = getPipelineState(dev_data, reinterpret_cast<VkPipeline &>(object_struct.handle));
        break;
    }
    case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT: {
        base_ptr = getBufferNode(dev_data, reinterpret_cast<VkBuffer &>(object_struct.handle));
        break;
    }
    case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT: {
        base_ptr = getBufferViewState(dev_data, reinterpret_cast<VkBufferView &>(object_struct.handle));
        break;
    }
    case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT: {
        base_ptr = getImageState(dev_data, reinterpret_cast<VkImage &>(object_struct.handle));
        break;
    }
    case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT: {
        base_ptr = getImageViewState(dev_data, reinterpret_cast<VkImageView &>(object_struct.handle));
        break;
    }
    case VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT: {
        base_ptr = getEventNode(dev_data, reinterpret_cast<VkEvent &>(object_struct.handle));
        break;
    }
    case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT: {
        base_ptr = getDescriptorPoolState(dev_data, reinterpret_cast<VkDescriptorPool &>(object_struct.handle));
        break;
    }
    case VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT: {
        base_ptr = getCommandPoolNode(dev_data, reinterpret_cast<VkCommandPool &>(object_struct.handle));
        break;
    }
    case VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT: {
        base_ptr = getFramebufferState(dev_data, reinterpret_cast<VkFramebuffer &>(object_struct.handle));
        break;
    }
    case VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT: {
        base_ptr = getRenderPassState(dev_data, reinterpret_cast<VkRenderPass &>(object_struct.handle));
        break;
    }
    case VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT: {
        base_ptr = getMemObjInfo(dev_data, reinterpret_cast<VkDeviceMemory &>(object_struct.handle));
        break;
    }
    default:
        // TODO : Any other objects to be handled here?
        assert(0);
        break;
    }
    return base_ptr;
}

// Tie the VK_OBJECT to the cmd buffer which includes:
//  Add object_binding to cmd buffer
//  Add cb_binding to object
static void addCommandBufferBinding(std::unordered_set<GLOBAL_CB_NODE *> *cb_bindings, VK_OBJECT obj, GLOBAL_CB_NODE *cb_node) {
    cb_bindings->insert(cb_node);
    cb_node->object_bindings.insert(obj);
}
// For a given object, if cb_node is in that objects cb_bindings, remove cb_node
static void removeCommandBufferBinding(layer_data *dev_data, VK_OBJECT const *object, GLOBAL_CB_NODE *cb_node) {
    BASE_NODE *base_obj = GetStateStructPtrFromObject(dev_data, *object);
    if (base_obj)
        base_obj->cb_bindings.erase(cb_node);
}
// Reset the command buffer state
//  Maintain the createInfo and set state to CB_NEW, but clear all other state
static void resetCB(layer_data *dev_data, const VkCommandBuffer cb) {
    GLOBAL_CB_NODE *pCB = dev_data->commandBufferMap[cb];
    if (pCB) {
        pCB->in_use.store(0);
        pCB->cmds.clear();
        // Reset CB state (note that createInfo is not cleared)
        pCB->commandBuffer = cb;
        memset(&pCB->beginInfo, 0, sizeof(VkCommandBufferBeginInfo));
        memset(&pCB->inheritanceInfo, 0, sizeof(VkCommandBufferInheritanceInfo));
        pCB->numCmds = 0;
        memset(pCB->drawCount, 0, NUM_DRAW_TYPES * sizeof(uint64_t));
        pCB->state = CB_NEW;
        pCB->submitCount = 0;
        pCB->status = 0;
        pCB->viewportMask = 0;
        pCB->scissorMask = 0;

        for (uint32_t i = 0; i < VK_PIPELINE_BIND_POINT_RANGE_SIZE; ++i) {
            pCB->lastBound[i].reset();
        }

        memset(&pCB->activeRenderPassBeginInfo, 0, sizeof(pCB->activeRenderPassBeginInfo));
        pCB->activeRenderPass = nullptr;
        pCB->activeSubpassContents = VK_SUBPASS_CONTENTS_INLINE;
        pCB->activeSubpass = 0;
        pCB->broken_bindings.clear();
        pCB->waitedEvents.clear();
        pCB->events.clear();
        pCB->writeEventsBeforeWait.clear();
        pCB->waitedEventsBeforeQueryReset.clear();
        pCB->queryToStateMap.clear();
        pCB->activeQueries.clear();
        pCB->startedQueries.clear();
        pCB->imageSubresourceMap.clear();
        pCB->imageLayoutMap.clear();
        pCB->eventToStageMap.clear();
        pCB->drawData.clear();
        pCB->currentDrawData.buffers.clear();
        pCB->primaryCommandBuffer = VK_NULL_HANDLE;
        // Make sure any secondaryCommandBuffers are removed from globalInFlight
        for (auto secondary_cb : pCB->secondaryCommandBuffers) {
            dev_data->globalInFlightCmdBuffers.erase(secondary_cb);
        }
        pCB->secondaryCommandBuffers.clear();
        pCB->updateImages.clear();
        pCB->updateBuffers.clear();
        clear_cmd_buf_and_mem_references(dev_data, pCB);
        pCB->eventUpdates.clear();
        pCB->queryUpdates.clear();

        // Remove object bindings
        for (auto obj : pCB->object_bindings) {
            removeCommandBufferBinding(dev_data, &obj, pCB);
        }
        pCB->object_bindings.clear();
        // Remove this cmdBuffer's reference from each FrameBuffer's CB ref list
        for (auto framebuffer : pCB->framebuffers) {
            auto fb_state = getFramebufferState(dev_data, framebuffer);
            if (fb_state)
                fb_state->cb_bindings.erase(pCB);
        }
        pCB->framebuffers.clear();
        pCB->activeFramebuffer = VK_NULL_HANDLE;
    }
}

// Set PSO-related status bits for CB, including dynamic state set via PSO
static void set_cb_pso_status(GLOBAL_CB_NODE *pCB, const PIPELINE_STATE *pPipe) {
    // Account for any dynamic state not set via this PSO
    if (!pPipe->graphicsPipelineCI.pDynamicState ||
        !pPipe->graphicsPipelineCI.pDynamicState->dynamicStateCount) { // All state is static
        pCB->status |= CBSTATUS_ALL_STATE_SET;
    } else {
        // First consider all state on
        // Then unset any state that's noted as dynamic in PSO
        // Finally OR that into CB statemask
        CBStatusFlags psoDynStateMask = CBSTATUS_ALL_STATE_SET;
        for (uint32_t i = 0; i < pPipe->graphicsPipelineCI.pDynamicState->dynamicStateCount; i++) {
            switch (pPipe->graphicsPipelineCI.pDynamicState->pDynamicStates[i]) {
            case VK_DYNAMIC_STATE_LINE_WIDTH:
                psoDynStateMask &= ~CBSTATUS_LINE_WIDTH_SET;
                break;
            case VK_DYNAMIC_STATE_DEPTH_BIAS:
                psoDynStateMask &= ~CBSTATUS_DEPTH_BIAS_SET;
                break;
            case VK_DYNAMIC_STATE_BLEND_CONSTANTS:
                psoDynStateMask &= ~CBSTATUS_BLEND_CONSTANTS_SET;
                break;
            case VK_DYNAMIC_STATE_DEPTH_BOUNDS:
                psoDynStateMask &= ~CBSTATUS_DEPTH_BOUNDS_SET;
                break;
            case VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK:
                psoDynStateMask &= ~CBSTATUS_STENCIL_READ_MASK_SET;
                break;
            case VK_DYNAMIC_STATE_STENCIL_WRITE_MASK:
                psoDynStateMask &= ~CBSTATUS_STENCIL_WRITE_MASK_SET;
                break;
            case VK_DYNAMIC_STATE_STENCIL_REFERENCE:
                psoDynStateMask &= ~CBSTATUS_STENCIL_REFERENCE_SET;
                break;
            default:
                // TODO : Flag error here
                break;
            }
        }
        pCB->status |= psoDynStateMask;
    }
}

// Print the last bound Gfx Pipeline
static bool printPipeline(layer_data *my_data, const VkCommandBuffer cb) {
    bool skip_call = false;
    GLOBAL_CB_NODE *pCB = getCBNode(my_data, cb);
    if (pCB) {
        PIPELINE_STATE *pPipeTrav = pCB->lastBound[VK_PIPELINE_BIND_POINT_GRAPHICS].pipeline_state;
        if (!pPipeTrav) {
            // nothing to print
        } else {
            skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0,
                                 __LINE__, DRAWSTATE_NONE, "DS", "%s",
                                 vk_print_vkgraphicspipelinecreateinfo(
                                     reinterpret_cast<const VkGraphicsPipelineCreateInfo *>(&pPipeTrav->graphicsPipelineCI), "{DS}")
                                     .c_str());
        }
    }
    return skip_call;
}

static void printCB(layer_data *my_data, const VkCommandBuffer cb) {
    GLOBAL_CB_NODE *pCB = getCBNode(my_data, cb);
    if (pCB && pCB->cmds.size() > 0) {
        log_msg(my_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                DRAWSTATE_NONE, "DS", "Cmds in command buffer 0x%p", (void *)cb);
        vector<CMD_NODE> cmds = pCB->cmds;
        for (auto ii = cmds.begin(); ii != cmds.end(); ++ii) {
            // TODO : Need to pass cmdbuffer as srcObj here
            log_msg(my_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, 0,
                    __LINE__, DRAWSTATE_NONE, "DS", "  CMD 0x%" PRIx64 ": %s", (*ii).cmdNumber, cmdTypeToString((*ii).type).c_str());
        }
    } else {
        // Nothing to print
    }
}

static bool synchAndPrintDSConfig(layer_data *my_data, const VkCommandBuffer cb) {
    bool skip_call = false;
    if (!(my_data->report_data->active_flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)) {
        return skip_call;
    }
    skip_call |= printPipeline(my_data, cb);
    return skip_call;
}

// Flags validation error if the associated call is made inside a render pass. The apiName
// routine should ONLY be called outside a render pass.
static bool insideRenderPass(const layer_data *my_data, GLOBAL_CB_NODE *pCB, const char *apiName) {
    bool inside = false;
    if (pCB->activeRenderPass) {
        inside = log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                         (uint64_t)pCB->commandBuffer, __LINE__, DRAWSTATE_INVALID_RENDERPASS_CMD, "DS",
                         "%s: It is invalid to issue this call inside an active render pass (0x%" PRIxLEAST64 ")", apiName,
                         (uint64_t)pCB->activeRenderPass->renderPass);
    }
    return inside;
}

// Flags validation error if the associated call is made outside a render pass. The apiName
// routine should ONLY be called inside a render pass.
static bool outsideRenderPass(const layer_data *my_data, GLOBAL_CB_NODE *pCB, const char *apiName) {
    bool outside = false;
    if (((pCB->createInfo.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) && (!pCB->activeRenderPass)) ||
        ((pCB->createInfo.level == VK_COMMAND_BUFFER_LEVEL_SECONDARY) && (!pCB->activeRenderPass) &&
         !(pCB->beginInfo.flags & VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT))) {
        outside = log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                          (uint64_t)pCB->commandBuffer, __LINE__, DRAWSTATE_NO_ACTIVE_RENDERPASS, "DS",
                          "%s: This call must be issued inside an active render pass.", apiName);
    }
    return outside;
}

static void init_core_validation(instance_layer_data *instance_data, const VkAllocationCallbacks *pAllocator) {

    layer_debug_actions(instance_data->report_data, instance_data->logging_callback, pAllocator, "lunarg_core_validation");

}

static void checkInstanceRegisterExtensions(const VkInstanceCreateInfo *pCreateInfo, instance_layer_data *instance_data) {
    for (uint32_t i = 0; i < pCreateInfo->enabledExtensionCount; i++) {
        if (!strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_SURFACE_EXTENSION_NAME))
            instance_data->surfaceExtensionEnabled = true;
        if (!strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_DISPLAY_EXTENSION_NAME))
            instance_data->displayExtensionEnabled = true;
#ifdef VK_USE_PLATFORM_ANDROID_KHR
        if (!strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_ANDROID_SURFACE_EXTENSION_NAME))
            instance_data->androidSurfaceExtensionEnabled = true;
#endif
#ifdef VK_USE_PLATFORM_MIR_KHR
        if (!strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_MIR_SURFACE_EXTENSION_NAME))
            instance_data->mirSurfaceExtensionEnabled = true;
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
        if (!strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME))
            instance_data->waylandSurfaceExtensionEnabled = true;
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
        if (!strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_WIN32_SURFACE_EXTENSION_NAME))
            instance_data->win32SurfaceExtensionEnabled = true;
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
        if (!strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_XCB_SURFACE_EXTENSION_NAME))
            instance_data->xcbSurfaceExtensionEnabled = true;
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
        if (!strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_XLIB_SURFACE_EXTENSION_NAME))
            instance_data->xlibSurfaceExtensionEnabled = true;
#endif
    }
}

VKAPI_ATTR VkResult VKAPI_CALL
CreateInstance(const VkInstanceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkInstance *pInstance) {
    VkLayerInstanceCreateInfo *chain_info = get_chain_info(pCreateInfo, VK_LAYER_LINK_INFO);

    assert(chain_info->u.pLayerInfo);
    PFN_vkGetInstanceProcAddr fpGetInstanceProcAddr = chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    PFN_vkCreateInstance fpCreateInstance = (PFN_vkCreateInstance)fpGetInstanceProcAddr(NULL, "vkCreateInstance");
    if (fpCreateInstance == NULL)
        return VK_ERROR_INITIALIZATION_FAILED;

    // Advance the link info for the next element on the chain
    chain_info->u.pLayerInfo = chain_info->u.pLayerInfo->pNext;

    VkResult result = fpCreateInstance(pCreateInfo, pAllocator, pInstance);
    if (result != VK_SUCCESS)
        return result;

    instance_layer_data *instance_data = get_my_data_ptr(get_dispatch_key(*pInstance), instance_layer_data_map);
    instance_data->instance = *pInstance;
    layer_init_instance_dispatch_table(*pInstance, &instance_data->dispatch_table, fpGetInstanceProcAddr);

    instance_data->report_data = debug_report_create_instance(
        &instance_data->dispatch_table, *pInstance, pCreateInfo->enabledExtensionCount, pCreateInfo->ppEnabledExtensionNames);
    checkInstanceRegisterExtensions(pCreateInfo, instance_data);
    init_core_validation(instance_data, pAllocator);

    ValidateLayerOrdering(*pCreateInfo);

    return result;
}

/* hook DestroyInstance to remove tableInstanceMap entry */
VKAPI_ATTR void VKAPI_CALL DestroyInstance(VkInstance instance, const VkAllocationCallbacks *pAllocator) {
    // TODOSC : Shouldn't need any customization here
    dispatch_key key = get_dispatch_key(instance);
    // TBD: Need any locking this early, in case this function is called at the
    // same time by more than one thread?
    instance_layer_data *instance_data = get_my_data_ptr(key, instance_layer_data_map);
    instance_data->dispatch_table.DestroyInstance(instance, pAllocator);

    std::lock_guard<std::mutex> lock(global_lock);
    // Clean up logging callback, if any
    while (instance_data->logging_callback.size() > 0) {
        VkDebugReportCallbackEXT callback = instance_data->logging_callback.back();
        layer_destroy_msg_callback(instance_data->report_data, callback, pAllocator);
        instance_data->logging_callback.pop_back();
    }

    layer_debug_report_destroy_instance(instance_data->report_data);
    layer_data_map.erase(key);
}

static void checkDeviceRegisterExtensions(const VkDeviceCreateInfo *pCreateInfo, VkDevice device) {
    uint32_t i;
    // TBD: Need any locking, in case this function is called at the same time
    // by more than one thread?
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    dev_data->device_extensions.wsi_enabled = false;
    dev_data->device_extensions.wsi_display_swapchain_enabled = false;

    for (i = 0; i < pCreateInfo->enabledExtensionCount; i++) {
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0)
            dev_data->device_extensions.wsi_enabled = true;
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_DISPLAY_SWAPCHAIN_EXTENSION_NAME) == 0)
            dev_data->device_extensions.wsi_display_swapchain_enabled = true;
    }
}

// Verify that queue family has been properly requested
bool ValidateRequestedQueueFamilyProperties(instance_layer_data *instance_data, VkPhysicalDevice gpu, const VkDeviceCreateInfo *create_info) {
    bool skip_call = false;
    auto physical_device_state = getPhysicalDeviceState(instance_data, gpu);
    // First check is app has actually requested queueFamilyProperties
    if (!physical_device_state) {
        skip_call |= log_msg(instance_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT,
                             0, __LINE__, DEVLIMITS_MUST_QUERY_COUNT, "DL",
                             "Invalid call to vkCreateDevice() w/o first calling vkEnumeratePhysicalDevices().");
    } else if (QUERY_DETAILS != physical_device_state->vkGetPhysicalDeviceQueueFamilyPropertiesState) {
        // TODO: This is not called out as an invalid use in the spec so make more informative recommendation.
        skip_call |= log_msg(instance_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT,
                             VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0, __LINE__, DEVLIMITS_INVALID_QUEUE_CREATE_REQUEST,
                             "DL", "Call to vkCreateDevice() w/o first calling vkGetPhysicalDeviceQueueFamilyProperties().");
    } else {
        // Check that the requested queue properties are valid
        for (uint32_t i = 0; i < create_info->queueCreateInfoCount; i++) {
            uint32_t requestedIndex = create_info->pQueueCreateInfos[i].queueFamilyIndex;
            if (requestedIndex >= physical_device_state->queue_family_properties.size()) {
                skip_call |= log_msg(
                    instance_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0,
                    __LINE__, DEVLIMITS_INVALID_QUEUE_CREATE_REQUEST, "DL",
                    "Invalid queue create request in vkCreateDevice(). Invalid queueFamilyIndex %u requested.", requestedIndex);
            } else if (create_info->pQueueCreateInfos[i].queueCount >
                       physical_device_state->queue_family_properties[requestedIndex].queueCount) {
                skip_call |=
                    log_msg(instance_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT,
                            0, __LINE__, DEVLIMITS_INVALID_QUEUE_CREATE_REQUEST, "DL",
                            "Invalid queue create request in vkCreateDevice(). QueueFamilyIndex %u only has %u queues, but "
                            "requested queueCount is %u.",
                            requestedIndex, physical_device_state->queue_family_properties[requestedIndex].queueCount,
                            create_info->pQueueCreateInfos[i].queueCount);
            }
        }
    }
    return skip_call;
}

// Verify that features have been queried and that they are available
static bool ValidateRequestedFeatures(instance_layer_data *dev_data, VkPhysicalDevice phys, const VkPhysicalDeviceFeatures *requested_features) {
    bool skip_call = false;

    auto phys_device_state = getPhysicalDeviceState(dev_data, phys);
    const VkBool32 *actual = reinterpret_cast<VkBool32 *>(&phys_device_state->features);
    const VkBool32 *requested = reinterpret_cast<const VkBool32 *>(requested_features);
    // TODO : This is a nice, compact way to loop through struct, but a bad way to report issues
    //  Need to provide the struct member name with the issue. To do that seems like we'll
    //  have to loop through each struct member which should be done w/ codegen to keep in synch.
    uint32_t errors = 0;
    uint32_t total_bools = sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32);
    for (uint32_t i = 0; i < total_bools; i++) {
        if (requested[i] > actual[i]) {
            // TODO: Add index to struct member name helper to be able to include a feature name
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0, __LINE__, DEVLIMITS_INVALID_FEATURE_REQUESTED,
                "DL", "While calling vkCreateDevice(), requesting feature #%u in VkPhysicalDeviceFeatures struct, "
                "which is not available on this device.",
                i);
            errors++;
        }
    }
    if (errors && (UNCALLED == phys_device_state->vkGetPhysicalDeviceFeaturesState)) {
        // If user didn't request features, notify them that they should
        // TODO: Verify this against the spec. I believe this is an invalid use of the API and should return an error
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                             VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0, __LINE__, DEVLIMITS_INVALID_FEATURE_REQUESTED,
                             "DL", "You requested features that are unavailable on this device. You should first query feature "
                                   "availability by calling vkGetPhysicalDeviceFeatures().");
    }
    return skip_call;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateDevice(VkPhysicalDevice gpu, const VkDeviceCreateInfo *pCreateInfo,
                                            const VkAllocationCallbacks *pAllocator, VkDevice *pDevice) {
    instance_layer_data *my_instance_data = get_my_data_ptr(get_dispatch_key(gpu), instance_layer_data_map);
    bool skip_call = false;

    // Check that any requested features are available
    if (pCreateInfo->pEnabledFeatures) {
        skip_call |= ValidateRequestedFeatures(my_instance_data, gpu, pCreateInfo->pEnabledFeatures);
    }
    skip_call |= ValidateRequestedQueueFamilyProperties(my_instance_data, gpu, pCreateInfo);

    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }

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

    std::unique_lock<std::mutex> lock(global_lock);
    layer_data *my_device_data = get_my_data_ptr(get_dispatch_key(*pDevice), layer_data_map);

    my_device_data->instance_data = my_instance_data;
    // Setup device dispatch table
    layer_init_device_dispatch_table(*pDevice, &my_device_data->dispatch_table, fpGetDeviceProcAddr);
    my_device_data->device = *pDevice;

    my_device_data->report_data = layer_debug_report_create_device(my_instance_data->report_data, *pDevice);
    checkDeviceRegisterExtensions(pCreateInfo, *pDevice);
    // Get physical device limits for this device
    my_instance_data->dispatch_table.GetPhysicalDeviceProperties(gpu, &(my_device_data->phys_dev_properties.properties));
    uint32_t count;
    my_instance_data->dispatch_table.GetPhysicalDeviceQueueFamilyProperties(gpu, &count, nullptr);
    my_device_data->phys_dev_properties.queue_family_properties.resize(count);
    my_instance_data->dispatch_table.GetPhysicalDeviceQueueFamilyProperties(
        gpu, &count, &my_device_data->phys_dev_properties.queue_family_properties[0]);
    // TODO: device limits should make sure these are compatible
    if (pCreateInfo->pEnabledFeatures) {
        my_device_data->enabled_features = *pCreateInfo->pEnabledFeatures;
    } else {
        memset(&my_device_data->enabled_features, 0, sizeof(VkPhysicalDeviceFeatures));
    }
    // Store physical device mem limits into device layer_data struct
    my_instance_data->dispatch_table.GetPhysicalDeviceMemoryProperties(gpu, &my_device_data->phys_dev_mem_props);
    lock.unlock();

    ValidateLayerOrdering(*pCreateInfo);

    return result;
}

// prototype
VKAPI_ATTR void VKAPI_CALL DestroyDevice(VkDevice device, const VkAllocationCallbacks *pAllocator) {
    // TODOSC : Shouldn't need any customization here
    bool skip = false;
    dispatch_key key = get_dispatch_key(device);
    layer_data *dev_data = get_my_data_ptr(key, layer_data_map);
    // Free all the memory
    std::unique_lock<std::mutex> lock(global_lock);
    deletePipelines(dev_data);
    dev_data->renderPassMap.clear();
    deleteCommandBuffers(dev_data);
    // This will also delete all sets in the pool & remove them from setMap
    deletePools(dev_data);
    // All sets should be removed
    assert(dev_data->setMap.empty());
    for (auto del_layout : dev_data->descriptorSetLayoutMap) {
        delete del_layout.second;
    }
    dev_data->descriptorSetLayoutMap.clear();
    dev_data->imageViewMap.clear();
    dev_data->imageMap.clear();
    dev_data->imageSubresourceMap.clear();
    dev_data->imageLayoutMap.clear();
    dev_data->bufferViewMap.clear();
    dev_data->bufferMap.clear();
    // Queues persist until device is destroyed
    dev_data->queueMap.clear();
    log_msg(dev_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
            (uint64_t)device, __LINE__, MEMTRACK_NONE, "MEM", "Printing List details prior to vkDestroyDevice()");
    log_msg(dev_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
            (uint64_t)device, __LINE__, MEMTRACK_NONE, "MEM", "================================================");
    print_mem_list(dev_data);
    printCBList(dev_data);
    // Report any memory leaks
    DEVICE_MEM_INFO *pInfo = NULL;
    if (!dev_data->memObjMap.empty()) {
        for (auto ii = dev_data->memObjMap.begin(); ii != dev_data->memObjMap.end(); ++ii) {
            pInfo = (*ii).second.get();
            if (pInfo->alloc_info.allocationSize != 0) {
                // Valid Usage: All child objects created on device must have been destroyed prior to destroying device
                skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT,
                                (uint64_t)pInfo->mem, __LINE__, MEMTRACK_MEMORY_LEAK, "MEM",
                                "Mem Object 0x%" PRIx64 " has not been freed. You should clean up this memory by calling "
                                "vkFreeMemory(0x%" PRIx64 ") prior to vkDestroyDevice().",
                                (uint64_t)(pInfo->mem), (uint64_t)(pInfo->mem));
            }
        }
    }
    layer_debug_report_destroy_device(device);
    lock.unlock();

#if DISPATCH_MAP_DEBUG
    fprintf(stderr, "Device: 0x%p, key: 0x%p\n", device, key);
#endif
    if (!skip) {
        dev_data->dispatch_table.DestroyDevice(device, pAllocator);
        layer_data_map.erase(key);
    }
}

static const VkExtensionProperties instance_extensions[] = {{VK_EXT_DEBUG_REPORT_EXTENSION_NAME, VK_EXT_DEBUG_REPORT_SPEC_VERSION}};

// This validates that the initial layout specified in the command buffer for
// the IMAGE is the same
// as the global IMAGE layout
static bool ValidateCmdBufImageLayouts(layer_data *dev_data, GLOBAL_CB_NODE *pCB) {
    bool skip_call = false;
    for (auto cb_image_data : pCB->imageLayoutMap) {
        VkImageLayout imageLayout;
        if (!FindLayout(dev_data, cb_image_data.first, imageLayout)) {
            skip_call |=
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, 0,
                        __LINE__, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS", "Cannot submit cmd buffer using deleted image 0x%" PRIx64 ".",
                        reinterpret_cast<const uint64_t &>(cb_image_data.first));
        } else {
            if (cb_image_data.second.initialLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
                // TODO: Set memory invalid which is in mem_tracker currently
            } else if (imageLayout != cb_image_data.second.initialLayout) {
                if (cb_image_data.first.hasSubresource) {
                    skip_call |= log_msg(
                        dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        reinterpret_cast<uint64_t &>(pCB->commandBuffer), __LINE__, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                        "Cannot submit cmd buffer using image (0x%" PRIx64 ") [sub-resource: aspectMask 0x%X array layer %u, mip level %u], "
                        "with layout %s when first use is %s.",
                        reinterpret_cast<const uint64_t &>(cb_image_data.first.image), cb_image_data.first.subresource.aspectMask,
                                cb_image_data.first.subresource.arrayLayer,
                                cb_image_data.first.subresource.mipLevel, string_VkImageLayout(imageLayout),
                        string_VkImageLayout(cb_image_data.second.initialLayout));
                } else {
                    skip_call |= log_msg(
                        dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        reinterpret_cast<uint64_t &>(pCB->commandBuffer), __LINE__, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                        "Cannot submit cmd buffer using image (0x%" PRIx64 ") with layout %s when "
                        "first use is %s.",
                        reinterpret_cast<const uint64_t &>(cb_image_data.first.image), string_VkImageLayout(imageLayout),
                        string_VkImageLayout(cb_image_data.second.initialLayout));
                }
            }
            SetLayout(dev_data, cb_image_data.first, cb_image_data.second.layout);
        }
    }
    return skip_call;
}

// Loop through bound objects and increment their in_use counts
//  For any unknown objects, flag an error
static bool ValidateAndIncrementBoundObjects(layer_data *dev_data, GLOBAL_CB_NODE const *cb_node) {
    bool skip = false;
    DRAW_STATE_ERROR error_code = DRAWSTATE_NONE;
    BASE_NODE *base_obj = nullptr;
    for (auto obj : cb_node->object_bindings) {
        switch (obj.type) {
        case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT: {
            base_obj = getSetNode(dev_data, reinterpret_cast<VkDescriptorSet &>(obj.handle));
            error_code = DRAWSTATE_INVALID_DESCRIPTOR_SET;
            break;
        }
        case VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT: {
            base_obj = getSamplerState(dev_data, reinterpret_cast<VkSampler &>(obj.handle));
            error_code = DRAWSTATE_INVALID_SAMPLER;
            break;
        }
        case VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT: {
            base_obj = getQueryPoolNode(dev_data, reinterpret_cast<VkQueryPool &>(obj.handle));
            error_code = DRAWSTATE_INVALID_QUERY_POOL;
            break;
        }
        case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT: {
            base_obj = getPipelineState(dev_data, reinterpret_cast<VkPipeline &>(obj.handle));
            error_code = DRAWSTATE_INVALID_PIPELINE;
            break;
        }
        case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT: {
            base_obj = getBufferNode(dev_data, reinterpret_cast<VkBuffer &>(obj.handle));
            error_code = DRAWSTATE_INVALID_BUFFER;
            break;
        }
        case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT: {
            base_obj = getBufferViewState(dev_data, reinterpret_cast<VkBufferView &>(obj.handle));
            error_code = DRAWSTATE_INVALID_BUFFER_VIEW;
            break;
        }
        case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT: {
            base_obj = getImageState(dev_data, reinterpret_cast<VkImage &>(obj.handle));
            error_code = DRAWSTATE_INVALID_IMAGE;
            break;
        }
        case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT: {
            base_obj = getImageViewState(dev_data, reinterpret_cast<VkImageView &>(obj.handle));
            error_code = DRAWSTATE_INVALID_IMAGE_VIEW;
            break;
        }
        case VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT: {
            base_obj = getEventNode(dev_data, reinterpret_cast<VkEvent &>(obj.handle));
            error_code = DRAWSTATE_INVALID_EVENT;
            break;
        }
        case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT: {
            base_obj = getDescriptorPoolState(dev_data, reinterpret_cast<VkDescriptorPool &>(obj.handle));
            error_code = DRAWSTATE_INVALID_DESCRIPTOR_POOL;
            break;
        }
        case VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT: {
            base_obj = getCommandPoolNode(dev_data, reinterpret_cast<VkCommandPool &>(obj.handle));
            error_code = DRAWSTATE_INVALID_COMMAND_POOL;
            break;
        }
        case VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT: {
            base_obj = getFramebufferState(dev_data, reinterpret_cast<VkFramebuffer &>(obj.handle));
            error_code = DRAWSTATE_INVALID_FRAMEBUFFER;
            break;
        }
        case VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT: {
            base_obj = getRenderPassState(dev_data, reinterpret_cast<VkRenderPass &>(obj.handle));
            error_code = DRAWSTATE_INVALID_RENDERPASS;
            break;
        }
        case VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT: {
            base_obj = getMemObjInfo(dev_data, reinterpret_cast<VkDeviceMemory &>(obj.handle));
            error_code = DRAWSTATE_INVALID_DEVICE_MEMORY;
            break;
        }
        default:
            // TODO : Merge handling of other objects types into this code
            break;
        }
        if (!base_obj) {
            skip |=
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, obj.type, obj.handle, __LINE__, error_code, "DS",
                        "Cannot submit cmd buffer using deleted %s 0x%" PRIx64 ".", object_type_to_string(obj.type), obj.handle);
        } else {
            base_obj->in_use.fetch_add(1);
        }
    }
    return skip;
}

// Track which resources are in-flight by atomically incrementing their "in_use" count
static bool validateAndIncrementResources(layer_data *dev_data, GLOBAL_CB_NODE *cb_node) {
    bool skip_call = false;

    cb_node->in_use.fetch_add(1);
    dev_data->globalInFlightCmdBuffers.insert(cb_node->commandBuffer);

    // First Increment for all "generic" objects bound to cmd buffer, followed by special-case objects below
    skip_call |= ValidateAndIncrementBoundObjects(dev_data, cb_node);
    // TODO : We should be able to remove the NULL look-up checks from the code below as long as
    //  all the corresponding cases are verified to cause CB_INVALID state and the CB_INVALID state
    //  should then be flagged prior to calling this function
    for (auto drawDataElement : cb_node->drawData) {
        for (auto buffer : drawDataElement.buffers) {
            auto buffer_node = getBufferNode(dev_data, buffer);
            if (!buffer_node) {
                skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT,
                                     (uint64_t)(buffer), __LINE__, DRAWSTATE_INVALID_BUFFER, "DS",
                                     "Cannot submit cmd buffer using deleted buffer 0x%" PRIx64 ".", (uint64_t)(buffer));
            } else {
                buffer_node->in_use.fetch_add(1);
            }
        }
    }
    for (auto event : cb_node->writeEventsBeforeWait) {
        auto event_state = getEventNode(dev_data, event);
        if (event_state)
            event_state->write_in_use++;
    }
    return skip_call;
}

// Note: This function assumes that the global lock is held by the calling
// thread.
// TODO: untangle this.
static bool cleanInFlightCmdBuffer(layer_data *my_data, VkCommandBuffer cmdBuffer) {
    bool skip_call = false;
    GLOBAL_CB_NODE *pCB = getCBNode(my_data, cmdBuffer);
    if (pCB) {
        for (auto queryEventsPair : pCB->waitedEventsBeforeQueryReset) {
            for (auto event : queryEventsPair.second) {
                if (my_data->eventMap[event].needsSignaled) {
                    skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT, 0, 0, DRAWSTATE_INVALID_QUERY, "DS",
                                         "Cannot get query results on queryPool 0x%" PRIx64
                                         " with index %d which was guarded by unsignaled event 0x%" PRIx64 ".",
                                         (uint64_t)(queryEventsPair.first.pool), queryEventsPair.first.index, (uint64_t)(event));
                }
            }
        }
    }
    return skip_call;
}

// TODO: nuke this completely.
// Decrement cmd_buffer in_use and if it goes to 0 remove cmd_buffer from globalInFlightCmdBuffers
static inline void removeInFlightCmdBuffer(layer_data *dev_data, VkCommandBuffer cmd_buffer) {
    // Pull it off of global list initially, but if we find it in any other queue list, add it back in
    GLOBAL_CB_NODE *pCB = getCBNode(dev_data, cmd_buffer);
    pCB->in_use.fetch_sub(1);
    if (!pCB->in_use.load()) {
        dev_data->globalInFlightCmdBuffers.erase(cmd_buffer);
    }
}

// Decrement in-use count for objects bound to command buffer
static void DecrementBoundResources(layer_data *dev_data, GLOBAL_CB_NODE const *cb_node) {
    BASE_NODE *base_obj = nullptr;
    for (auto obj : cb_node->object_bindings) {
        base_obj = GetStateStructPtrFromObject(dev_data, obj);
        if (base_obj) {
            base_obj->in_use.fetch_sub(1);
        }
    }
}

static bool RetireWorkOnQueue(layer_data *dev_data, QUEUE_NODE *pQueue, uint64_t seq)
{
    bool skip_call = false; // TODO: extract everything that might fail to precheck
    std::unordered_map<VkQueue, uint64_t> otherQueueSeqs;

    // Roll this queue forward, one submission at a time.
    while (pQueue->seq < seq) {
        auto & submission = pQueue->submissions.front();

        for (auto & wait : submission.waitSemaphores) {
            auto pSemaphore = getSemaphoreNode(dev_data, wait.semaphore);
            pSemaphore->in_use.fetch_sub(1);
            auto & lastSeq = otherQueueSeqs[wait.queue];
            lastSeq = std::max(lastSeq, wait.seq);
        }

        for (auto & semaphore : submission.signalSemaphores) {
            auto pSemaphore = getSemaphoreNode(dev_data, semaphore);
            pSemaphore->in_use.fetch_sub(1);
        }

        for (auto cb : submission.cbs) {
            auto cb_node = getCBNode(dev_data, cb);
            // First perform decrement on general case bound objects
            DecrementBoundResources(dev_data, cb_node);
            for (auto drawDataElement : cb_node->drawData) {
                for (auto buffer : drawDataElement.buffers) {
                    auto buffer_node = getBufferNode(dev_data, buffer);
                    if (buffer_node) {
                        buffer_node->in_use.fetch_sub(1);
                    }
                }
            }
            for (auto event : cb_node->writeEventsBeforeWait) {
                auto eventNode = dev_data->eventMap.find(event);
                if (eventNode != dev_data->eventMap.end()) {
                    eventNode->second.write_in_use--;
                }
            }
            for (auto queryStatePair : cb_node->queryToStateMap) {
                dev_data->queryToStateMap[queryStatePair.first] = queryStatePair.second;
            }
            for (auto eventStagePair : cb_node->eventToStageMap) {
                dev_data->eventMap[eventStagePair.first].stageMask = eventStagePair.second;
            }

            skip_call |= cleanInFlightCmdBuffer(dev_data, cb);
            removeInFlightCmdBuffer(dev_data, cb);
        }

        auto pFence = getFenceNode(dev_data, submission.fence);
        if (pFence) {
            pFence->state = FENCE_RETIRED;
        }

        pQueue->submissions.pop_front();
        pQueue->seq++;
    }

    // Roll other queues forward to the highest seq we saw a wait for
    for (auto qs : otherQueueSeqs) {
        skip_call |= RetireWorkOnQueue(dev_data, getQueueNode(dev_data, qs.first), qs.second);
    }

    return skip_call;
}


// Submit a fence to a queue, delimiting previous fences and previous untracked
// work by it.
static void
SubmitFence(QUEUE_NODE *pQueue, FENCE_NODE *pFence, uint64_t submitCount)
{
    pFence->state = FENCE_INFLIGHT;
    pFence->signaler.first = pQueue->queue;
    pFence->signaler.second = pQueue->seq + pQueue->submissions.size() + submitCount;
}

static bool validateCommandBufferSimultaneousUse(layer_data *dev_data, GLOBAL_CB_NODE *pCB) {
    bool skip_call = false;
    if (dev_data->globalInFlightCmdBuffers.count(pCB->commandBuffer) &&
        !(pCB->beginInfo.flags & VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT)) {
        skip_call |=
            log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, 0,
                    __LINE__, DRAWSTATE_INVALID_CB_SIMULTANEOUS_USE, "DS",
                    "Command Buffer 0x%" PRIx64 " is already in use and is not marked for simultaneous use.",
                    reinterpret_cast<uint64_t>(pCB->commandBuffer));
    }
    return skip_call;
}

static bool validateCommandBufferState(layer_data *dev_data, GLOBAL_CB_NODE *pCB, const char *call_source) {
    bool skip = false;
    if (dev_data->instance_data->disabled.command_buffer_state)
        return skip;
    // Validate ONE_TIME_SUBMIT_BIT CB is not being submitted more than once
    if ((pCB->beginInfo.flags & VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) && (pCB->submitCount > 1)) {
        skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, 0,
                        __LINE__, DRAWSTATE_COMMAND_BUFFER_SINGLE_SUBMIT_VIOLATION, "DS",
                        "Commandbuffer 0x%" PRIxLEAST64 " was begun w/ VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT "
                        "set, but has been submitted 0x%" PRIxLEAST64 " times.",
                        (uint64_t)(pCB->commandBuffer), pCB->submitCount);
    }
    // Validate that cmd buffers have been updated
    if (CB_RECORDED != pCB->state) {
        if (CB_INVALID == pCB->state) {
            // Inform app of reason CB invalid
            for (auto obj : pCB->broken_bindings) {
                const char *type_str = object_type_to_string(obj.type);
                // Descriptor sets are a special case that can be either destroyed or updated to invalidated a CB
                const char *cause_str =
                    (obj.type == VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT) ? "destroyed or updated" : "destroyed";

                skip |=
                    log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            reinterpret_cast<uint64_t &>(pCB->commandBuffer), __LINE__, DRAWSTATE_INVALID_COMMAND_BUFFER, "DS",
                            "You are submitting command buffer 0x%" PRIxLEAST64 " that is invalid because bound %s 0x%" PRIxLEAST64
                            " was %s.",
                            reinterpret_cast<uint64_t &>(pCB->commandBuffer), type_str, obj.handle, cause_str);
            }
        } else { // Flag error for using CB w/o vkEndCommandBuffer() called
            skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            (uint64_t)(pCB->commandBuffer), __LINE__, DRAWSTATE_NO_END_COMMAND_BUFFER, "DS",
                            "You must call vkEndCommandBuffer() on command buffer 0x%" PRIxLEAST64 " before this call to %s!",
                            reinterpret_cast<uint64_t &>(pCB->commandBuffer), call_source);
        }
    }
    return skip;
}

// Validate that queueFamilyIndices of primary command buffers match this queue
// Secondary command buffers were previously validated in vkCmdExecuteCommands().
static bool validateQueueFamilyIndices(layer_data *dev_data, GLOBAL_CB_NODE *pCB, VkQueue queue) {
    bool skip_call = false;
    auto pPool = getCommandPoolNode(dev_data, pCB->createInfo.commandPool);
    auto queue_node = getQueueNode(dev_data, queue);

    if (pPool && queue_node && (pPool->queueFamilyIndex != queue_node->queueFamilyIndex)) {
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
            reinterpret_cast<uint64_t>(pCB->commandBuffer), __LINE__, DRAWSTATE_INVALID_QUEUE_FAMILY, "DS",
            "vkQueueSubmit: Primary command buffer 0x%" PRIxLEAST64
            " created in queue family %d is being submitted on queue 0x%" PRIxLEAST64 " from queue family %d.",
            reinterpret_cast<uint64_t>(pCB->commandBuffer), pPool->queueFamilyIndex,
            reinterpret_cast<uint64_t>(queue), queue_node->queueFamilyIndex);
    }

    return skip_call;
}

static bool validatePrimaryCommandBufferState(layer_data *dev_data, GLOBAL_CB_NODE *pCB) {
    // Track in-use for resources off of primary and any secondary CBs
    bool skip_call = false;

    // If USAGE_SIMULTANEOUS_USE_BIT not set then CB cannot already be executing
    // on device
    skip_call |= validateCommandBufferSimultaneousUse(dev_data, pCB);

    skip_call |= validateAndIncrementResources(dev_data, pCB);

    if (!pCB->secondaryCommandBuffers.empty()) {
        for (auto secondaryCmdBuffer : pCB->secondaryCommandBuffers) {
            GLOBAL_CB_NODE *pSubCB = getCBNode(dev_data, secondaryCmdBuffer);
            skip_call |= validateAndIncrementResources(dev_data, pSubCB);
            if ((pSubCB->primaryCommandBuffer != pCB->commandBuffer) &&
                !(pSubCB->beginInfo.flags & VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT)) {
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, 0,
                        __LINE__, DRAWSTATE_COMMAND_BUFFER_SINGLE_SUBMIT_VIOLATION, "DS",
                        "Commandbuffer 0x%" PRIxLEAST64 " was submitted with secondary buffer 0x%" PRIxLEAST64
                        " but that buffer has subsequently been bound to "
                        "primary cmd buffer 0x%" PRIxLEAST64
                        " and it does not have VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT set.",
                        reinterpret_cast<uint64_t>(pCB->commandBuffer), reinterpret_cast<uint64_t>(secondaryCmdBuffer),
                        reinterpret_cast<uint64_t>(pSubCB->primaryCommandBuffer));
            }
        }
    }

    skip_call |= validateCommandBufferState(dev_data, pCB, "vkQueueSubmit()");

    return skip_call;
}

static bool
ValidateFenceForSubmit(layer_data *dev_data, FENCE_NODE *pFence)
{
    bool skip_call = false;

    if (pFence) {
        if (pFence->state == FENCE_INFLIGHT) {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT,
                                 (uint64_t)(pFence->fence), __LINE__, DRAWSTATE_INVALID_FENCE, "DS",
                                 "Fence 0x%" PRIx64 " is already in use by another submission.", (uint64_t)(pFence->fence));
        }

        else if (pFence->state == FENCE_RETIRED) {
            skip_call |=
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT,
                        reinterpret_cast<uint64_t &>(pFence->fence), __LINE__, MEMTRACK_INVALID_FENCE_STATE, "MEM",
                        "Fence 0x%" PRIxLEAST64 " submitted in SIGNALED state.  Fences must be reset before being submitted",
                        reinterpret_cast<uint64_t &>(pFence->fence));
        }
    }

    return skip_call;
}


VKAPI_ATTR VkResult VKAPI_CALL
QueueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo *pSubmits, VkFence fence) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(queue), layer_data_map);
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    std::unique_lock<std::mutex> lock(global_lock);

    auto pQueue = getQueueNode(dev_data, queue);
    auto pFence = getFenceNode(dev_data, fence);
    skip_call |= ValidateFenceForSubmit(dev_data, pFence);

    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }

    // TODO : Review these old print functions and clean up as appropriate
    print_mem_list(dev_data);
    printCBList(dev_data);

    // Mark the fence in-use.
    if (pFence) {
        SubmitFence(pQueue, pFence, std::max(1u, submitCount));
    }

    // Now verify each individual submit
    for (uint32_t submit_idx = 0; submit_idx < submitCount; submit_idx++) {
        const VkSubmitInfo *submit = &pSubmits[submit_idx];
        vector<SEMAPHORE_WAIT> semaphore_waits;
        vector<VkSemaphore> semaphore_signals;
        for (uint32_t i = 0; i < submit->waitSemaphoreCount; ++i) {
            VkSemaphore semaphore = submit->pWaitSemaphores[i];
            auto pSemaphore = getSemaphoreNode(dev_data, semaphore);
            if (pSemaphore) {
                if (pSemaphore->signaled) {
                    if (pSemaphore->signaler.first != VK_NULL_HANDLE) {
                        semaphore_waits.push_back({semaphore, pSemaphore->signaler.first, pSemaphore->signaler.second});
                        pSemaphore->in_use.fetch_add(1);
                    }
                    pSemaphore->signaler.first = VK_NULL_HANDLE;
                    pSemaphore->signaled = false;
                } else {
                    skip_call |=
                        log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT,
                                reinterpret_cast<const uint64_t &>(semaphore), __LINE__, DRAWSTATE_QUEUE_FORWARD_PROGRESS, "DS",
                                "Queue 0x%" PRIx64 " is waiting on semaphore 0x%" PRIx64 " that has no way to be signaled.",
                                reinterpret_cast<uint64_t &>(queue), reinterpret_cast<const uint64_t &>(semaphore));
                }
            }
        }
        for (uint32_t i = 0; i < submit->signalSemaphoreCount; ++i) {
            VkSemaphore semaphore = submit->pSignalSemaphores[i];
            auto pSemaphore = getSemaphoreNode(dev_data, semaphore);
            if (pSemaphore) {
                if (pSemaphore->signaled) {
                    skip_call |=
                        log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT,
                                reinterpret_cast<const uint64_t &>(semaphore), __LINE__, DRAWSTATE_QUEUE_FORWARD_PROGRESS, "DS",
                                "Queue 0x%" PRIx64 " is signaling semaphore 0x%" PRIx64
                                " that has already been signaled but not waited on by queue 0x%" PRIx64 ".",
                                reinterpret_cast<uint64_t &>(queue), reinterpret_cast<const uint64_t &>(semaphore),
                                reinterpret_cast<uint64_t &>(pSemaphore->signaler.first));
                } else {
                    pSemaphore->signaler.first = queue;
                    pSemaphore->signaler.second = pQueue->seq + pQueue->submissions.size() + 1;
                    pSemaphore->signaled = true;
                    pSemaphore->in_use.fetch_add(1);
                    semaphore_signals.push_back(semaphore);
                }
            }
        }

        std::vector<VkCommandBuffer> cbs;

        for (uint32_t i = 0; i < submit->commandBufferCount; i++) {
            auto cb_node = getCBNode(dev_data, submit->pCommandBuffers[i]);
            skip_call |= ValidateCmdBufImageLayouts(dev_data, cb_node);
            if (cb_node) {
                cbs.push_back(submit->pCommandBuffers[i]);
                for (auto secondaryCmdBuffer : cb_node->secondaryCommandBuffers) {
                    cbs.push_back(secondaryCmdBuffer);
                }

                cb_node->submitCount++; // increment submit count
                skip_call |= validatePrimaryCommandBufferState(dev_data, cb_node);
                skip_call |= validateQueueFamilyIndices(dev_data, cb_node, queue);
                // Potential early exit here as bad object state may crash in delayed function calls
                if (skip_call)
                    return result;
                // Call submit-time functions to validate/update state
                for (auto &function : cb_node->validate_functions) {
                    skip_call |= function();
                }
                for (auto &function : cb_node->eventUpdates) {
                    skip_call |= function(queue);
                }
                for (auto &function : cb_node->queryUpdates) {
                    skip_call |= function(queue);
                }
            }
        }

        pQueue->submissions.emplace_back(cbs, semaphore_waits, semaphore_signals,
                                         submit_idx == submitCount - 1 ? fence : VK_NULL_HANDLE);
    }

    if (pFence && !submitCount) {
        // If no submissions, but just dropping a fence on the end of the queue,
        // record an empty submission with just the fence, so we can determine
        // its completion.
        pQueue->submissions.emplace_back(std::vector<VkCommandBuffer>(),
                                         std::vector<SEMAPHORE_WAIT>(),
                                         std::vector<VkSemaphore>(),
                                         fence);
    }

    lock.unlock();
    if (!skip_call)
        result = dev_data->dispatch_table.QueueSubmit(queue, submitCount, pSubmits, fence);

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL AllocateMemory(VkDevice device, const VkMemoryAllocateInfo *pAllocateInfo,
                                              const VkAllocationCallbacks *pAllocator, VkDeviceMemory *pMemory) {
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = my_data->dispatch_table.AllocateMemory(device, pAllocateInfo, pAllocator, pMemory);
    // TODO : Track allocations and overall size here
    std::lock_guard<std::mutex> lock(global_lock);
    add_mem_obj_info(my_data, device, *pMemory, pAllocateInfo);
    print_mem_list(my_data);
    return result;
}

// For given obj node, if it is use, flag a validation error and return callback result, else return false
bool ValidateObjectNotInUse(const layer_data *dev_data, BASE_NODE *obj_node, VK_OBJECT obj_struct,
                            UNIQUE_VALIDATION_ERROR_CODE error_code) {
    if (dev_data->instance_data->disabled.object_in_use)
        return false;
    bool skip = false;
    if (obj_node->in_use.load()) {
        skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, obj_struct.type, obj_struct.handle, __LINE__,
                        error_code, "DS", "Cannot delete %s 0x%" PRIx64 " that is currently in use by a command buffer. %s",
                        object_type_to_string(obj_struct.type), obj_struct.handle, validation_error_map[error_code]);
    }
    return skip;
}

static bool PreCallValidateFreeMemory(layer_data *dev_data, VkDeviceMemory mem, DEVICE_MEM_INFO **mem_info, VK_OBJECT *obj_struct) {
    *mem_info = getMemObjInfo(dev_data, mem);
    *obj_struct = {reinterpret_cast<uint64_t &>(mem), VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT};
    if (dev_data->instance_data->disabled.free_memory)
        return false;
    bool skip = false;
    if (*mem_info) {
        skip |= ValidateObjectNotInUse(dev_data, *mem_info, *obj_struct, VALIDATION_ERROR_00620);
    }
    return skip;
}

static void PostCallRecordFreeMemory(layer_data *dev_data, VkDeviceMemory mem, DEVICE_MEM_INFO *mem_info, VK_OBJECT obj_struct) {
    // Clear mem binding for any bound objects
    for (auto obj : mem_info->obj_bindings) {
        log_msg(dev_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, obj.type, obj.handle, __LINE__, MEMTRACK_FREED_MEM_REF,
                "MEM", "VK Object 0x%" PRIxLEAST64 " still has a reference to mem obj 0x%" PRIxLEAST64, obj.handle,
                (uint64_t)mem_info->mem);
        switch (obj.type) {
        case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT: {
            auto image_state = getImageState(dev_data, reinterpret_cast<VkImage &>(obj.handle));
            assert(image_state); // Any destroyed images should already be removed from bindings
            image_state->binding.mem = MEMORY_UNBOUND;
            break;
        }
        case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT: {
            auto buff_node = getBufferNode(dev_data, reinterpret_cast<VkBuffer &>(obj.handle));
            assert(buff_node); // Any destroyed buffers should already be removed from bindings
            buff_node->binding.mem = MEMORY_UNBOUND;
            break;
        }
        default:
            // Should only have buffer or image objects bound to memory
            assert(0);
        }
    }
    // Any bound cmd buffers are now invalid
    invalidateCommandBuffers(mem_info->cb_bindings, obj_struct);
    dev_data->memObjMap.erase(mem);
}

VKAPI_ATTR void VKAPI_CALL FreeMemory(VkDevice device, VkDeviceMemory mem, const VkAllocationCallbacks *pAllocator) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    DEVICE_MEM_INFO *mem_info = nullptr;
    VK_OBJECT obj_struct;
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip = PreCallValidateFreeMemory(dev_data, mem, &mem_info, &obj_struct);
    if (!skip) {
        lock.unlock();
        dev_data->dispatch_table.FreeMemory(device, mem, pAllocator);
        lock.lock();
        PostCallRecordFreeMemory(dev_data, mem, mem_info, obj_struct);
    }
}

// Validate that given Map memory range is valid. This means that the memory should not already be mapped,
//  and that the size of the map range should be:
//  1. Not zero
//  2. Within the size of the memory allocation
static bool ValidateMapMemRange(layer_data *my_data, VkDeviceMemory mem, VkDeviceSize offset, VkDeviceSize size) {
    bool skip_call = false;

    if (size == 0) {
        skip_call = log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT,
                            (uint64_t)mem, __LINE__, MEMTRACK_INVALID_MAP, "MEM",
                            "VkMapMemory: Attempting to map memory range of size zero");
    }

    auto mem_element = my_data->memObjMap.find(mem);
    if (mem_element != my_data->memObjMap.end()) {
        auto mem_info = mem_element->second.get();
        // It is an application error to call VkMapMemory on an object that is already mapped
        if (mem_info->mem_range.size != 0) {
            skip_call = log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT,
                                (uint64_t)mem, __LINE__, MEMTRACK_INVALID_MAP, "MEM",
                                "VkMapMemory: Attempting to map memory on an already-mapped object 0x%" PRIxLEAST64, (uint64_t)mem);
        }

        // Validate that offset + size is within object's allocationSize
        if (size == VK_WHOLE_SIZE) {
            if (offset >= mem_info->alloc_info.allocationSize) {
                skip_call = log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                    VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, (uint64_t)mem, __LINE__, MEMTRACK_INVALID_MAP,
                                    "MEM", "Mapping Memory from 0x%" PRIx64 " to 0x%" PRIx64
                                           " with size of VK_WHOLE_SIZE oversteps total array size 0x%" PRIx64,
                                    offset, mem_info->alloc_info.allocationSize, mem_info->alloc_info.allocationSize);
            }
        } else {
            if ((offset + size) > mem_info->alloc_info.allocationSize) {
                skip_call =
                    log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT,
                            (uint64_t)mem, __LINE__, MEMTRACK_INVALID_MAP, "MEM",
                            "Mapping Memory from 0x%" PRIx64 " to 0x%" PRIx64 " oversteps total array size 0x%" PRIx64, offset,
                            size + offset, mem_info->alloc_info.allocationSize);
            }
        }
    }
    return skip_call;
}

static void storeMemRanges(layer_data *my_data, VkDeviceMemory mem, VkDeviceSize offset, VkDeviceSize size) {
    auto mem_info = getMemObjInfo(my_data, mem);
    if (mem_info) {
        mem_info->mem_range.offset = offset;
        mem_info->mem_range.size = size;
    }
}

static bool deleteMemRanges(layer_data *my_data, VkDeviceMemory mem) {
    bool skip_call = false;
    auto mem_info = getMemObjInfo(my_data, mem);
    if (mem_info) {
        if (!mem_info->mem_range.size) {
            // Valid Usage: memory must currently be mapped
            skip_call = log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT,
                                (uint64_t)mem, __LINE__, MEMTRACK_INVALID_MAP, "MEM",
                                "Unmapping Memory without memory being mapped: mem obj 0x%" PRIxLEAST64, (uint64_t)mem);
        }
        mem_info->mem_range.size = 0;
        if (mem_info->shadow_copy) {
            free(mem_info->shadow_copy_base);
            mem_info->shadow_copy_base = 0;
            mem_info->shadow_copy = 0;
        }
    }
    return skip_call;
}

// Guard value for pad data
static char NoncoherentMemoryFillValue = 0xb;

static void initializeAndTrackMemory(layer_data *dev_data, VkDeviceMemory mem, VkDeviceSize offset, VkDeviceSize size,
                                     void **ppData) {
    auto mem_info = getMemObjInfo(dev_data, mem);
    if (mem_info) {
        mem_info->p_driver_data = *ppData;
        uint32_t index = mem_info->alloc_info.memoryTypeIndex;
        if (dev_data->phys_dev_mem_props.memoryTypes[index].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {
            mem_info->shadow_copy = 0;
        } else {
            if (size == VK_WHOLE_SIZE) {
                size = mem_info->alloc_info.allocationSize - offset;
            }
            mem_info->shadow_pad_size = dev_data->phys_dev_properties.properties.limits.minMemoryMapAlignment;
            assert(vk_safe_modulo(mem_info->shadow_pad_size,
                                  dev_data->phys_dev_properties.properties.limits.minMemoryMapAlignment) == 0);
            // Ensure start of mapped region reflects hardware alignment constraints
            uint64_t map_alignment = dev_data->phys_dev_properties.properties.limits.minMemoryMapAlignment;

            // From spec: (ppData - offset) must be aligned to at least limits::minMemoryMapAlignment.
            uint64_t start_offset = offset % map_alignment;
            // Data passed to driver will be wrapped by a guardband of data to detect over- or under-writes.
            mem_info->shadow_copy_base = malloc(static_cast<size_t>(2 * mem_info->shadow_pad_size + size + map_alignment + start_offset));

            mem_info->shadow_copy =
                reinterpret_cast<char *>((reinterpret_cast<uintptr_t>(mem_info->shadow_copy_base) + map_alignment) &
                                         ~(map_alignment - 1)) + start_offset;
            assert(vk_safe_modulo(reinterpret_cast<uintptr_t>(mem_info->shadow_copy) + mem_info->shadow_pad_size - start_offset,
                                  map_alignment) == 0);

            memset(mem_info->shadow_copy, NoncoherentMemoryFillValue, static_cast<size_t>(2 * mem_info->shadow_pad_size + size));
            *ppData = static_cast<char *>(mem_info->shadow_copy) + mem_info->shadow_pad_size;
        }
    }
}

// Verify that state for fence being waited on is appropriate. That is,
//  a fence being waited on should not already be signaled and
//  it should have been submitted on a queue or during acquire next image
static inline bool verifyWaitFenceState(layer_data *dev_data, VkFence fence, const char *apiCall) {
    bool skip_call = false;

    auto pFence = getFenceNode(dev_data, fence);
    if (pFence) {
        if (pFence->state == FENCE_UNSIGNALED) {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT,
                                 reinterpret_cast<uint64_t &>(fence), __LINE__, MEMTRACK_INVALID_FENCE_STATE, "MEM",
                                 "%s called for fence 0x%" PRIxLEAST64 " which has not been submitted on a Queue or during "
                                 "acquire next image.",
                                 apiCall, reinterpret_cast<uint64_t &>(fence));
        }
    }
    return skip_call;
}

static bool RetireFence(layer_data *dev_data, VkFence fence) {
    auto pFence = getFenceNode(dev_data, fence);
    if (pFence->signaler.first != VK_NULL_HANDLE) {
        /* Fence signaller is a queue -- use this as proof that prior operations
         * on that queue have completed.
         */
        return RetireWorkOnQueue(dev_data,
                                 getQueueNode(dev_data, pFence->signaler.first),
                                 pFence->signaler.second);
    }
    else {
        /* Fence signaller is the WSI. We're not tracking what the WSI op
         * actually /was/ in CV yet, but we need to mark the fence as retired.
         */
        pFence->state = FENCE_RETIRED;
        return false;
    }
}

VKAPI_ATTR VkResult VKAPI_CALL
WaitForFences(VkDevice device, uint32_t fenceCount, const VkFence *pFences, VkBool32 waitAll, uint64_t timeout) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    bool skip_call = false;
    // Verify fence status of submitted fences
    std::unique_lock<std::mutex> lock(global_lock);
    for (uint32_t i = 0; i < fenceCount; i++) {
        skip_call |= verifyWaitFenceState(dev_data, pFences[i], "vkWaitForFences");
    }
    lock.unlock();
    if (skip_call)
        return VK_ERROR_VALIDATION_FAILED_EXT;

    VkResult result = dev_data->dispatch_table.WaitForFences(device, fenceCount, pFences, waitAll, timeout);

    if (result == VK_SUCCESS) {
        lock.lock();
        // When we know that all fences are complete we can clean/remove their CBs
        if (waitAll || fenceCount == 1) {
            for (uint32_t i = 0; i < fenceCount; i++) {
                skip_call |= RetireFence(dev_data, pFences[i]);
            }
        }
        // NOTE : Alternate case not handled here is when some fences have completed. In
        //  this case for app to guarantee which fences completed it will have to call
        //  vkGetFenceStatus() at which point we'll clean/remove their CBs if complete.
        lock.unlock();
    }
    if (skip_call)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL GetFenceStatus(VkDevice device, VkFence fence) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    bool skip_call = false;
    std::unique_lock<std::mutex> lock(global_lock);
    skip_call = verifyWaitFenceState(dev_data, fence, "vkGetFenceStatus");
    lock.unlock();

    if (skip_call)
        return VK_ERROR_VALIDATION_FAILED_EXT;

    VkResult result = dev_data->dispatch_table.GetFenceStatus(device, fence);
    lock.lock();
    if (result == VK_SUCCESS) {
        skip_call |= RetireFence(dev_data, fence);
    }
    lock.unlock();
    if (skip_call)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    return result;
}

VKAPI_ATTR void VKAPI_CALL GetDeviceQueue(VkDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex,
                                                            VkQueue *pQueue) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    dev_data->dispatch_table.GetDeviceQueue(device, queueFamilyIndex, queueIndex, pQueue);
    std::lock_guard<std::mutex> lock(global_lock);

    // Add queue to tracking set only if it is new
    auto result = dev_data->queues.emplace(*pQueue);
    if (result.second == true) {
        QUEUE_NODE *pQNode = &dev_data->queueMap[*pQueue];
        pQNode->queue = *pQueue;
        pQNode->queueFamilyIndex = queueFamilyIndex;
        pQNode->seq = 0;
    }
}

VKAPI_ATTR VkResult VKAPI_CALL QueueWaitIdle(VkQueue queue) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(queue), layer_data_map);
    bool skip_call = false;
    std::unique_lock<std::mutex> lock(global_lock);
    auto pQueue = getQueueNode(dev_data, queue);
    skip_call |= RetireWorkOnQueue(dev_data, pQueue, pQueue->seq + pQueue->submissions.size());
    lock.unlock();
    if (skip_call)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = dev_data->dispatch_table.QueueWaitIdle(queue);
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL DeviceWaitIdle(VkDevice device) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    for (auto & queue : dev_data->queueMap) {
        skip_call |= RetireWorkOnQueue(dev_data, &queue.second, queue.second.seq + queue.second.submissions.size());
    }
    lock.unlock();
    if (skip_call)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = dev_data->dispatch_table.DeviceWaitIdle(device);
    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyFence(VkDevice device, VkFence fence, const VkAllocationCallbacks *pAllocator) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    bool skip_call = false;
    std::unique_lock<std::mutex> lock(global_lock);
    auto fence_pair = dev_data->fenceMap.find(fence);
    if (fence_pair != dev_data->fenceMap.end()) {
        if (fence_pair->second.state == FENCE_INFLIGHT) {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT,
                                 (uint64_t)(fence), __LINE__, DRAWSTATE_INVALID_FENCE, "DS", "Fence 0x%" PRIx64 " is in use.",
                                 (uint64_t)(fence));
        }
        dev_data->fenceMap.erase(fence_pair);
    }
    lock.unlock();

    if (!skip_call)
        dev_data->dispatch_table.DestroyFence(device, fence, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL
DestroySemaphore(VkDevice device, VkSemaphore semaphore, const VkAllocationCallbacks *pAllocator) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    bool skip = false;
    std::unique_lock<std::mutex> lock(global_lock);
    auto sema_node = getSemaphoreNode(dev_data, semaphore);
    if (sema_node) {
        skip |= ValidateObjectNotInUse(dev_data, sema_node,
                                       {reinterpret_cast<uint64_t &>(semaphore), VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT},
                                       VALIDATION_ERROR_00199);
    }
    if (!skip) {
        dev_data->semaphoreMap.erase(semaphore);
        lock.unlock();
        dev_data->dispatch_table.DestroySemaphore(device, semaphore, pAllocator);
    }
}

static bool PreCallValidateDestroyEvent(layer_data *dev_data, VkEvent event, EVENT_STATE **event_state, VK_OBJECT *obj_struct) {
    *event_state = getEventNode(dev_data, event);
    *obj_struct = {reinterpret_cast<uint64_t &>(event), VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT};
    if (dev_data->instance_data->disabled.destroy_event)
        return false;
    bool skip = false;
    if (*event_state) {
        skip |= ValidateObjectNotInUse(dev_data, *event_state, *obj_struct, VALIDATION_ERROR_00213);
    }
    return skip;
}

static void PostCallRecordDestroyEvent(layer_data *dev_data, VkEvent event, EVENT_STATE *event_state, VK_OBJECT obj_struct) {
    invalidateCommandBuffers(event_state->cb_bindings, obj_struct);
    dev_data->eventMap.erase(event);
}

VKAPI_ATTR void VKAPI_CALL DestroyEvent(VkDevice device, VkEvent event, const VkAllocationCallbacks *pAllocator) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    EVENT_STATE *event_state = nullptr;
    VK_OBJECT obj_struct;
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip = PreCallValidateDestroyEvent(dev_data, event, &event_state, &obj_struct);
    if (!skip) {
        lock.unlock();
        dev_data->dispatch_table.DestroyEvent(device, event, pAllocator);
        lock.lock();
        PostCallRecordDestroyEvent(dev_data, event, event_state, obj_struct);
    }
}

VKAPI_ATTR void VKAPI_CALL
DestroyQueryPool(VkDevice device, VkQueryPool queryPool, const VkAllocationCallbacks *pAllocator) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    bool skip = false;
    std::unique_lock<std::mutex> lock(global_lock);
    auto qp_node = getQueryPoolNode(dev_data, queryPool);
    if (qp_node) {
        VK_OBJECT obj_struct = {reinterpret_cast<uint64_t &>(queryPool), VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT};
        skip |= ValidateObjectNotInUse(dev_data, qp_node, obj_struct, VALIDATION_ERROR_01012);
        // Any bound cmd buffers are now invalid
        invalidateCommandBuffers(qp_node->cb_bindings, obj_struct);
    }
    if (!skip) {
        dev_data->queryPoolMap.erase(queryPool);
        lock.unlock();
        dev_data->dispatch_table.DestroyQueryPool(device, queryPool, pAllocator);
    }
}

VKAPI_ATTR VkResult VKAPI_CALL GetQueryPoolResults(VkDevice device, VkQueryPool queryPool, uint32_t firstQuery,
                                                   uint32_t queryCount, size_t dataSize, void *pData, VkDeviceSize stride,
                                                   VkQueryResultFlags flags) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    unordered_map<QueryObject, vector<VkCommandBuffer>> queriesInFlight;
    std::unique_lock<std::mutex> lock(global_lock);
    for (auto cmdBuffer : dev_data->globalInFlightCmdBuffers) {
        auto pCB = getCBNode(dev_data, cmdBuffer);
        for (auto queryStatePair : pCB->queryToStateMap) {
            queriesInFlight[queryStatePair.first].push_back(cmdBuffer);
        }
    }
    bool skip_call = false;
    for (uint32_t i = 0; i < queryCount; ++i) {
        QueryObject query = {queryPool, firstQuery + i};
        auto queryElement = queriesInFlight.find(query);
        auto queryToStateElement = dev_data->queryToStateMap.find(query);
        if (queryToStateElement != dev_data->queryToStateMap.end()) {
            // Available and in flight
            if (queryElement != queriesInFlight.end() && queryToStateElement != dev_data->queryToStateMap.end() &&
                queryToStateElement->second) {
                for (auto cmdBuffer : queryElement->second) {
                    auto pCB = getCBNode(dev_data, cmdBuffer);
                    auto queryEventElement = pCB->waitedEventsBeforeQueryReset.find(query);
                    if (queryEventElement == pCB->waitedEventsBeforeQueryReset.end()) {
                        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                             VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT, 0, __LINE__, DRAWSTATE_INVALID_QUERY, "DS",
                                             "Cannot get query results on queryPool 0x%" PRIx64 " with index %d which is in flight.",
                                             (uint64_t)(queryPool), firstQuery + i);
                    } else {
                        for (auto event : queryEventElement->second) {
                            dev_data->eventMap[event].needsSignaled = true;
                        }
                    }
                }
                // Unavailable and in flight
            } else if (queryElement != queriesInFlight.end() && queryToStateElement != dev_data->queryToStateMap.end() &&
                       !queryToStateElement->second) {
                // TODO : Can there be the same query in use by multiple command buffers in flight?
                bool make_available = false;
                for (auto cmdBuffer : queryElement->second) {
                    auto pCB = getCBNode(dev_data, cmdBuffer);
                    make_available |= pCB->queryToStateMap[query];
                }
                if (!(((flags & VK_QUERY_RESULT_PARTIAL_BIT) || (flags & VK_QUERY_RESULT_WAIT_BIT)) && make_available)) {
                    skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT, 0, __LINE__, DRAWSTATE_INVALID_QUERY, "DS",
                                         "Cannot get query results on queryPool 0x%" PRIx64 " with index %d which is unavailable.",
                                         (uint64_t)(queryPool), firstQuery + i);
                }
                // Unavailable
            } else if (queryToStateElement != dev_data->queryToStateMap.end() && !queryToStateElement->second) {
                skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                     VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT, 0, __LINE__, DRAWSTATE_INVALID_QUERY, "DS",
                                     "Cannot get query results on queryPool 0x%" PRIx64 " with index %d which is unavailable.",
                                     (uint64_t)(queryPool), firstQuery + i);
                // Unitialized
            } else if (queryToStateElement == dev_data->queryToStateMap.end()) {
                skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                     VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT, 0, __LINE__, DRAWSTATE_INVALID_QUERY, "DS",
                                     "Cannot get query results on queryPool 0x%" PRIx64
                                     " with index %d as data has not been collected for this index.",
                                     (uint64_t)(queryPool), firstQuery + i);
            }
        }
    }
    lock.unlock();
    if (skip_call)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    return dev_data->dispatch_table.GetQueryPoolResults(device, queryPool, firstQuery, queryCount, dataSize, pData, stride, flags);
}

static bool validateIdleBuffer(const layer_data *my_data, VkBuffer buffer) {
    bool skip_call = false;
    auto buffer_node = getBufferNode(my_data, buffer);
    if (!buffer_node) {
        skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT,
                             (uint64_t)(buffer), __LINE__, DRAWSTATE_DOUBLE_DESTROY, "DS",
                             "Cannot free buffer 0x%" PRIxLEAST64 " that has not been allocated.", (uint64_t)(buffer));
    } else {
        if (buffer_node->in_use.load()) {
            skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT,
                                 (uint64_t)(buffer), __LINE__, DRAWSTATE_OBJECT_INUSE, "DS",
                                 "Cannot free buffer 0x%" PRIxLEAST64 " that is in use by a command buffer.", (uint64_t)(buffer));
        }
    }
    return skip_call;
}

// Return true if given ranges intersect, else false
// Prereq : For both ranges, range->end - range->start > 0. This case should have already resulted
//  in an error so not checking that here
// pad_ranges bool indicates a linear and non-linear comparison which requires padding
// In the case where padding is required, if an alias is encountered then a validation error is reported and skip_call
//  may be set by the callback function so caller should merge in skip_call value if padding case is possible.
static bool rangesIntersect(layer_data const *dev_data, MEMORY_RANGE const *range1, MEMORY_RANGE const *range2, bool *skip_call) {
    *skip_call = false;
    auto r1_start = range1->start;
    auto r1_end = range1->end;
    auto r2_start = range2->start;
    auto r2_end = range2->end;
    VkDeviceSize pad_align = 1;
    if (range1->linear != range2->linear) {
        pad_align = dev_data->phys_dev_properties.properties.limits.bufferImageGranularity;
    }
    if ((r1_end & ~(pad_align - 1)) < (r2_start & ~(pad_align - 1)))
        return false;
    if ((r1_start & ~(pad_align - 1)) > (r2_end & ~(pad_align - 1)))
        return false;

    if (range1->linear != range2->linear) {
        // In linear vs. non-linear case, it's an error to alias
        const char *r1_linear_str = range1->linear ? "Linear" : "Non-linear";
        const char *r1_type_str = range1->image ? "image" : "buffer";
        const char *r2_linear_str = range2->linear ? "linear" : "non-linear";
        const char *r2_type_str = range2->image ? "image" : "buffer";
        auto obj_type = range1->image ? VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT : VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT;
        *skip_call |=
            log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, obj_type, range1->handle, 0, MEMTRACK_INVALID_ALIASING,
                    "MEM", "%s %s 0x%" PRIx64 " is aliased with %s %s 0x%" PRIx64
                           " which is in violation of the Buffer-Image Granularity section of the Vulkan specification.",
                    r1_linear_str, r1_type_str, range1->handle, r2_linear_str, r2_type_str, range2->handle);
    }
    // Ranges intersect
    return true;
}
// Simplified rangesIntersect that calls above function to check range1 for intersection with offset & end addresses
static bool rangesIntersect(layer_data const *dev_data, MEMORY_RANGE const *range1, VkDeviceSize offset, VkDeviceSize end) {
    // Create a local MEMORY_RANGE struct to wrap offset/size
    MEMORY_RANGE range_wrap;
    // Synch linear with range1 to avoid padding and potential validation error case
    range_wrap.linear = range1->linear;
    range_wrap.start = offset;
    range_wrap.end = end;
    bool tmp_bool;
    return rangesIntersect(dev_data, range1, &range_wrap, &tmp_bool);
}
// For given mem_info, set all ranges valid that intersect [offset-end] range
// TODO : For ranges where there is no alias, we may want to create new buffer ranges that are valid
static void SetMemRangesValid(layer_data const *dev_data, DEVICE_MEM_INFO *mem_info, VkDeviceSize offset, VkDeviceSize end) {
    bool tmp_bool = false;
    MEMORY_RANGE map_range;
    map_range.linear = true;
    map_range.start = offset;
    map_range.end = end;
    for (auto &handle_range_pair : mem_info->bound_ranges) {
        if (rangesIntersect(dev_data, &handle_range_pair.second, &map_range, &tmp_bool)) {
            // TODO : WARN here if tmp_bool true?
            handle_range_pair.second.valid = true;
        }
    }
}
// Object with given handle is being bound to memory w/ given mem_info struct.
//  Track the newly bound memory range with given memoryOffset
//  Also scan any previous ranges, track aliased ranges with new range, and flag an error if a linear
//  and non-linear range incorrectly overlap.
// Return true if an error is flagged and the user callback returns "true", otherwise false
// is_image indicates an image object, otherwise handle is for a buffer
// is_linear indicates a buffer or linear image
static bool InsertMemoryRange(layer_data const *dev_data, uint64_t handle, DEVICE_MEM_INFO *mem_info, VkDeviceSize memoryOffset,
                              VkMemoryRequirements memRequirements, bool is_image, bool is_linear) {
    bool skip_call = false;
    MEMORY_RANGE range;

    range.image = is_image;
    range.handle = handle;
    range.linear = is_linear;
    range.valid = mem_info->global_valid;
    range.memory = mem_info->mem;
    range.start = memoryOffset;
    range.size = memRequirements.size;
    range.end = memoryOffset + memRequirements.size - 1;
    range.aliases.clear();
    // Update Memory aliasing
    // Save aliase ranges so we can copy into final map entry below. Can't do it in loop b/c we don't yet have final ptr. If we
    // inserted into map before loop to get the final ptr, then we may enter loop when not needed & we check range against itself
    std::unordered_set<MEMORY_RANGE *> tmp_alias_ranges;
    for (auto &obj_range_pair : mem_info->bound_ranges) {
        auto check_range = &obj_range_pair.second;
        bool intersection_error = false;
        if (rangesIntersect(dev_data, &range, check_range, &intersection_error)) {
            skip_call |= intersection_error;
            range.aliases.insert(check_range);
            tmp_alias_ranges.insert(check_range);
        }
    }
    mem_info->bound_ranges[handle] = std::move(range);
    for (auto tmp_range : tmp_alias_ranges) {
        tmp_range->aliases.insert(&mem_info->bound_ranges[handle]);
    }
    if (is_image)
        mem_info->bound_images.insert(handle);
    else
        mem_info->bound_buffers.insert(handle);

    return skip_call;
}

static bool InsertImageMemoryRange(layer_data const *dev_data, VkImage image, DEVICE_MEM_INFO *mem_info, VkDeviceSize mem_offset,
                                   VkMemoryRequirements mem_reqs, bool is_linear) {
    return InsertMemoryRange(dev_data, reinterpret_cast<uint64_t &>(image), mem_info, mem_offset, mem_reqs, true, is_linear);
}

static bool InsertBufferMemoryRange(layer_data const *dev_data, VkBuffer buffer, DEVICE_MEM_INFO *mem_info, VkDeviceSize mem_offset,
                                    VkMemoryRequirements mem_reqs) {
    return InsertMemoryRange(dev_data, reinterpret_cast<uint64_t &>(buffer), mem_info, mem_offset, mem_reqs, false, true);
}

// Remove MEMORY_RANGE struct for give handle from bound_ranges of mem_info
//  is_image indicates if handle is for image or buffer
//  This function will also remove the handle-to-index mapping from the appropriate
//  map and clean up any aliases for range being removed.
static void RemoveMemoryRange(uint64_t handle, DEVICE_MEM_INFO *mem_info, bool is_image) {
    auto erase_range = &mem_info->bound_ranges[handle];
    for (auto alias_range : erase_range->aliases) {
        alias_range->aliases.erase(erase_range);
    }
    erase_range->aliases.clear();
    mem_info->bound_ranges.erase(handle);
    if (is_image) {
        mem_info->bound_images.erase(handle);
    } else {
        mem_info->bound_buffers.erase(handle);
    }
}

static void RemoveBufferMemoryRange(uint64_t handle, DEVICE_MEM_INFO *mem_info) { RemoveMemoryRange(handle, mem_info, false); }

static void RemoveImageMemoryRange(uint64_t handle, DEVICE_MEM_INFO *mem_info) { RemoveMemoryRange(handle, mem_info, true); }

VKAPI_ATTR void VKAPI_CALL DestroyBuffer(VkDevice device, VkBuffer buffer,
                                         const VkAllocationCallbacks *pAllocator) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    if (!validateIdleBuffer(dev_data, buffer)) {
        // Clean up memory binding and range information for buffer
        auto buff_node = getBufferNode(dev_data, buffer);
        if (buff_node) {
            // Any bound cmd buffers are now invalid
            invalidateCommandBuffers(buff_node->cb_bindings,
                                     {reinterpret_cast<uint64_t &>(buff_node->buffer), VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT});
            auto mem_info = getMemObjInfo(dev_data, buff_node->binding.mem);
            if (mem_info) {
                RemoveBufferMemoryRange(reinterpret_cast<uint64_t &>(buffer), mem_info);
            }
            ClearMemoryObjectBindings(dev_data, reinterpret_cast<uint64_t &>(buffer), VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT);
            dev_data->bufferMap.erase(buff_node->buffer);
        }
        lock.unlock();
        dev_data->dispatch_table.DestroyBuffer(device, buffer, pAllocator);
    }
}

static bool PreCallValidateDestroyBufferView(layer_data *dev_data, VkBufferView buffer_view, BUFFER_VIEW_STATE **buffer_view_state,
                                             VK_OBJECT *obj_struct) {
    *buffer_view_state = getBufferViewState(dev_data, buffer_view);
    *obj_struct = {reinterpret_cast<uint64_t &>(buffer_view), VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT};
    if (dev_data->instance_data->disabled.destroy_buffer_view)
        return false;
    bool skip = false;
    if (*buffer_view_state) {
        skip |= ValidateObjectNotInUse(dev_data, *buffer_view_state, *obj_struct, VALIDATION_ERROR_00701);
    }
    return skip;
}

static void PostCallRecordDestroyBufferView(layer_data *dev_data, VkBufferView buffer_view, BUFFER_VIEW_STATE *buffer_view_state,
                                            VK_OBJECT obj_struct) {
    // Any bound cmd buffers are now invalid
    invalidateCommandBuffers(buffer_view_state->cb_bindings, obj_struct);
    dev_data->bufferViewMap.erase(buffer_view);
}

VKAPI_ATTR void VKAPI_CALL
DestroyBufferView(VkDevice device, VkBufferView bufferView, const VkAllocationCallbacks *pAllocator) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    // Common data objects used pre & post call
    BUFFER_VIEW_STATE *buffer_view_state = nullptr;
    VK_OBJECT obj_struct;
    std::unique_lock<std::mutex> lock(global_lock);
    // Validate state before calling down chain, update common data if we'll be calling down chain
    bool skip = PreCallValidateDestroyBufferView(dev_data, bufferView, &buffer_view_state, &obj_struct);
    if (!skip) {
        lock.unlock();
        dev_data->dispatch_table.DestroyBufferView(device, bufferView, pAllocator);
        lock.lock();
        PostCallRecordDestroyBufferView(dev_data, bufferView, buffer_view_state, obj_struct);
    }
}

static bool PreCallValidateDestroyImage(layer_data *dev_data, VkImage image, IMAGE_STATE **image_state, VK_OBJECT *obj_struct) {
    *image_state = getImageState(dev_data, image);
    *obj_struct = {reinterpret_cast<uint64_t &>(image), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT};
    if (dev_data->instance_data->disabled.destroy_image)
        return false;
    bool skip = false;
    if (*image_state) {
        skip |= ValidateObjectNotInUse(dev_data, *image_state, *obj_struct, VALIDATION_ERROR_00743);
    }
    return skip;
}

static void PostCallRecordDestroyImage(layer_data *dev_data, VkImage image, IMAGE_STATE *image_state, VK_OBJECT obj_struct) {
    invalidateCommandBuffers(image_state->cb_bindings, obj_struct);
    // Clean up memory mapping, bindings and range references for image
    auto mem_info = getMemObjInfo(dev_data, image_state->binding.mem);
    if (mem_info) {
        RemoveImageMemoryRange(obj_struct.handle, mem_info);
    }
    ClearMemoryObjectBindings(dev_data, obj_struct.handle, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT);
    // Remove image from imageMap
    dev_data->imageMap.erase(image);

    const auto &sub_entry = dev_data->imageSubresourceMap.find(image);
    if (sub_entry != dev_data->imageSubresourceMap.end()) {
        for (const auto &pair : sub_entry->second) {
            dev_data->imageLayoutMap.erase(pair);
        }
        dev_data->imageSubresourceMap.erase(sub_entry);
    }
}

VKAPI_ATTR void VKAPI_CALL DestroyImage(VkDevice device, VkImage image, const VkAllocationCallbacks *pAllocator) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    IMAGE_STATE *image_state = nullptr;
    VK_OBJECT obj_struct;
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip = PreCallValidateDestroyImage(dev_data, image, &image_state, &obj_struct);
    if (!skip) {
        lock.unlock();
        dev_data->dispatch_table.DestroyImage(device, image, pAllocator);
        lock.lock();
        PostCallRecordDestroyImage(dev_data, image, image_state, obj_struct);
    }
}

static bool ValidateMemoryTypes(const layer_data *dev_data, const DEVICE_MEM_INFO *mem_info, const uint32_t memory_type_bits,
                                  const char *funcName) {
    bool skip_call = false;
    if (((1 << mem_info->alloc_info.memoryTypeIndex) & memory_type_bits) == 0) {
        skip_call = log_msg(
            dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT,
            reinterpret_cast<const uint64_t &>(mem_info->mem), __LINE__, MEMTRACK_INVALID_MEM_TYPE, "MT",
            "%s(): MemoryRequirements->memoryTypeBits (0x%X) for this object type are not compatible with the memory "
            "type (0x%X) of this memory object 0x%" PRIx64 ".",
            funcName, memory_type_bits, mem_info->alloc_info.memoryTypeIndex, reinterpret_cast<const uint64_t &>(mem_info->mem));
    }
    return skip_call;
}

VKAPI_ATTR VkResult VKAPI_CALL
BindBufferMemory(VkDevice device, VkBuffer buffer, VkDeviceMemory mem, VkDeviceSize memoryOffset) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    std::unique_lock<std::mutex> lock(global_lock);
    // Track objects tied to memory
    uint64_t buffer_handle = reinterpret_cast<uint64_t &>(buffer);
    bool skip_call = SetMemBinding(dev_data, mem, buffer_handle, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, "vkBindBufferMemory");
    auto buffer_node = getBufferNode(dev_data, buffer);
    if (buffer_node) {
        VkMemoryRequirements memRequirements;
        dev_data->dispatch_table.GetBufferMemoryRequirements(device, buffer, &memRequirements);
        buffer_node->binding.mem = mem;
        buffer_node->binding.offset = memoryOffset;
        buffer_node->binding.size = memRequirements.size;

        // Track and validate bound memory range information
        auto mem_info = getMemObjInfo(dev_data, mem);
        if (mem_info) {
            skip_call |= InsertBufferMemoryRange(dev_data, buffer, mem_info, memoryOffset, memRequirements);
            skip_call |= ValidateMemoryTypes(dev_data, mem_info, memRequirements.memoryTypeBits, "BindBufferMemory");
        }

        // Validate memory requirements alignment
        if (vk_safe_modulo(memoryOffset, memRequirements.alignment) != 0) {
            skip_call |=
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0,
                        __LINE__, DRAWSTATE_INVALID_BUFFER_MEMORY_OFFSET, "DS",
                        "vkBindBufferMemory(): memoryOffset is 0x%" PRIxLEAST64 " but must be an integer multiple of the "
                        "VkMemoryRequirements::alignment value 0x%" PRIxLEAST64
                        ", returned from a call to vkGetBufferMemoryRequirements with buffer",
                        memoryOffset, memRequirements.alignment);
        }

        // Validate device limits alignments
        static const VkBufferUsageFlagBits usage_list[3] = {
            static_cast<VkBufferUsageFlagBits>(VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT};
        static const char *memory_type[3] = {"texel",
                                             "uniform",
                                             "storage"};
        static const char *offset_name[3] = {
            "minTexelBufferOffsetAlignment",
            "minUniformBufferOffsetAlignment",
            "minStorageBufferOffsetAlignment"
        };

        // Keep this one fresh!
        const VkDeviceSize offset_requirement[3] = {
            dev_data->phys_dev_properties.properties.limits.minTexelBufferOffsetAlignment,
            dev_data->phys_dev_properties.properties.limits.minUniformBufferOffsetAlignment,
            dev_data->phys_dev_properties.properties.limits.minStorageBufferOffsetAlignment
        };
        VkBufferUsageFlags usage = dev_data->bufferMap[buffer].get()->createInfo.usage;

        for (int i = 0; i < 3; i++) {
            if (usage & usage_list[i]) {
                if (vk_safe_modulo(memoryOffset, offset_requirement[i]) != 0) {
                    skip_call |=
                        log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT,
                                0, __LINE__, DRAWSTATE_INVALID_TEXEL_BUFFER_OFFSET, "DS",
                                "vkBindBufferMemory(): %s memoryOffset is 0x%" PRIxLEAST64 " but must be a multiple of "
                                "device limit %s 0x%" PRIxLEAST64,
                                memory_type[i], memoryOffset, offset_name[i], offset_requirement[i]);
                }
            }
        }
    }
    print_mem_list(dev_data);
    lock.unlock();
    if (!skip_call) {
        result = dev_data->dispatch_table.BindBufferMemory(device, buffer, mem, memoryOffset);
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL
GetBufferMemoryRequirements(VkDevice device, VkBuffer buffer, VkMemoryRequirements *pMemoryRequirements) {
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    // TODO : What to track here?
    //   Could potentially save returned mem requirements and validate values passed into BindBufferMemory
    my_data->dispatch_table.GetBufferMemoryRequirements(device, buffer, pMemoryRequirements);
}

VKAPI_ATTR void VKAPI_CALL
GetImageMemoryRequirements(VkDevice device, VkImage image, VkMemoryRequirements *pMemoryRequirements) {
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    // TODO : What to track here?
    //   Could potentially save returned mem requirements and validate values passed into BindImageMemory
    my_data->dispatch_table.GetImageMemoryRequirements(device, image, pMemoryRequirements);
}

static bool PreCallValidateDestroyImageView(layer_data *dev_data, VkImageView image_view, IMAGE_VIEW_STATE **image_view_state,
                                            VK_OBJECT *obj_struct) {
    *image_view_state = getImageViewState(dev_data, image_view);
    *obj_struct = {reinterpret_cast<uint64_t &>(image_view), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT};
    if (dev_data->instance_data->disabled.destroy_image_view)
        return false;
    bool skip = false;
    if (*image_view_state) {
        skip |= ValidateObjectNotInUse(dev_data, *image_view_state, *obj_struct, VALIDATION_ERROR_00776);
    }
    return skip;
}

static void PostCallRecordDestroyImageView(layer_data *dev_data, VkImageView image_view, IMAGE_VIEW_STATE *image_view_state,
                                           VK_OBJECT obj_struct) {
    // Any bound cmd buffers are now invalid
    invalidateCommandBuffers(image_view_state->cb_bindings, obj_struct);
    dev_data->imageViewMap.erase(image_view);
}

VKAPI_ATTR void VKAPI_CALL
DestroyImageView(VkDevice device, VkImageView imageView, const VkAllocationCallbacks *pAllocator) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    // Common data objects used pre & post call
    IMAGE_VIEW_STATE *image_view_state = nullptr;
    VK_OBJECT obj_struct;
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip = PreCallValidateDestroyImageView(dev_data, imageView, &image_view_state, &obj_struct);
    if (!skip) {
        lock.unlock();
        dev_data->dispatch_table.DestroyImageView(device, imageView, pAllocator);
        lock.lock();
        PostCallRecordDestroyImageView(dev_data, imageView, image_view_state, obj_struct);
    }
}

VKAPI_ATTR void VKAPI_CALL
DestroyShaderModule(VkDevice device, VkShaderModule shaderModule, const VkAllocationCallbacks *pAllocator) {
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    std::unique_lock<std::mutex> lock(global_lock);
    my_data->shaderModuleMap.erase(shaderModule);
    lock.unlock();

    my_data->dispatch_table.DestroyShaderModule(device, shaderModule, pAllocator);
}

static bool PreCallValidateDestroyPipeline(layer_data *dev_data, VkPipeline pipeline, PIPELINE_STATE **pipeline_state,
                                           VK_OBJECT *obj_struct) {
    *pipeline_state = getPipelineState(dev_data, pipeline);
    *obj_struct = {reinterpret_cast<uint64_t &>(pipeline), VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT};
    if (dev_data->instance_data->disabled.destroy_pipeline)
        return false;
    bool skip = false;
    if (*pipeline_state) {
        skip |= ValidateObjectNotInUse(dev_data, *pipeline_state, *obj_struct, VALIDATION_ERROR_00555);
    }
    return skip;
}

static void PostCallRecordDestroyPipeline(layer_data *dev_data, VkPipeline pipeline, PIPELINE_STATE *pipeline_state,
                                          VK_OBJECT obj_struct) {
    // Any bound cmd buffers are now invalid
    invalidateCommandBuffers(pipeline_state->cb_bindings, obj_struct);
    dev_data->pipelineMap.erase(pipeline);
}

VKAPI_ATTR void VKAPI_CALL
DestroyPipeline(VkDevice device, VkPipeline pipeline, const VkAllocationCallbacks *pAllocator) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    PIPELINE_STATE *pipeline_state = nullptr;
    VK_OBJECT obj_struct;
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip = PreCallValidateDestroyPipeline(dev_data, pipeline, &pipeline_state, &obj_struct);
    if (!skip) {
        lock.unlock();
        dev_data->dispatch_table.DestroyPipeline(device, pipeline, pAllocator);
        lock.lock();
        PostCallRecordDestroyPipeline(dev_data, pipeline, pipeline_state, obj_struct);
    }
}

VKAPI_ATTR void VKAPI_CALL
DestroyPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout, const VkAllocationCallbacks *pAllocator) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    dev_data->pipelineLayoutMap.erase(pipelineLayout);
    lock.unlock();

    dev_data->dispatch_table.DestroyPipelineLayout(device, pipelineLayout, pAllocator);
}

static bool PreCallValidateDestroySampler(layer_data *dev_data, VkSampler sampler, SAMPLER_STATE **sampler_state,
                                          VK_OBJECT *obj_struct) {
    *sampler_state = getSamplerState(dev_data, sampler);
    *obj_struct = {reinterpret_cast<uint64_t &>(sampler), VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT};
    if (dev_data->instance_data->disabled.destroy_sampler)
        return false;
    bool skip = false;
    if (*sampler_state) {
        skip |= ValidateObjectNotInUse(dev_data, *sampler_state, *obj_struct, VALIDATION_ERROR_00837);
    }
    return skip;
}

static void PostCallRecordDestroySampler(layer_data *dev_data, VkSampler sampler, SAMPLER_STATE *sampler_state,
                                         VK_OBJECT obj_struct) {
    // Any bound cmd buffers are now invalid
    if (sampler_state)
        invalidateCommandBuffers(sampler_state->cb_bindings, obj_struct);
    dev_data->samplerMap.erase(sampler);
}

VKAPI_ATTR void VKAPI_CALL
DestroySampler(VkDevice device, VkSampler sampler, const VkAllocationCallbacks *pAllocator) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    SAMPLER_STATE *sampler_state = nullptr;
    VK_OBJECT obj_struct;
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip = PreCallValidateDestroySampler(dev_data, sampler, &sampler_state, &obj_struct);
    if (!skip) {
        lock.unlock();
        dev_data->dispatch_table.DestroySampler(device, sampler, pAllocator);
        lock.lock();
        PostCallRecordDestroySampler(dev_data, sampler, sampler_state, obj_struct);
    }
}

VKAPI_ATTR void VKAPI_CALL
DestroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, const VkAllocationCallbacks *pAllocator) {
    // TODO : Clean up any internal data structures using this obj.
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)
        ->dispatch_table.DestroyDescriptorSetLayout(device, descriptorSetLayout, pAllocator);
}

static bool PreCallValidateDestroyDescriptorPool(layer_data *dev_data, VkDescriptorPool pool,
                                                 DESCRIPTOR_POOL_STATE **desc_pool_state, VK_OBJECT *obj_struct) {
    *desc_pool_state = getDescriptorPoolState(dev_data, pool);
    *obj_struct = {reinterpret_cast<uint64_t &>(pool), VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT};
    if (dev_data->instance_data->disabled.destroy_descriptor_pool)
        return false;
    bool skip = false;
    if (*desc_pool_state) {
        skip |= ValidateObjectNotInUse(dev_data, *desc_pool_state, *obj_struct, VALIDATION_ERROR_00901);
    }
    return skip;
}

static void PostCallRecordDestroyDescriptorPool(layer_data *dev_data, VkDescriptorPool descriptorPool,
                                                DESCRIPTOR_POOL_STATE *desc_pool_state, VK_OBJECT obj_struct) {
    // Any bound cmd buffers are now invalid
    invalidateCommandBuffers(desc_pool_state->cb_bindings, obj_struct);
    // Free sets that were in this pool
    for (auto ds : desc_pool_state->sets) {
        freeDescriptorSet(dev_data, ds);
    }
    dev_data->descriptorPoolMap.erase(descriptorPool);
}

VKAPI_ATTR void VKAPI_CALL
DestroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, const VkAllocationCallbacks *pAllocator) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    DESCRIPTOR_POOL_STATE *desc_pool_state = nullptr;
    VK_OBJECT obj_struct;
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip = PreCallValidateDestroyDescriptorPool(dev_data, descriptorPool, &desc_pool_state, &obj_struct);
    if (!skip) {
        lock.unlock();
        dev_data->dispatch_table.DestroyDescriptorPool(device, descriptorPool, pAllocator);
        lock.lock();
        PostCallRecordDestroyDescriptorPool(dev_data, descriptorPool, desc_pool_state, obj_struct);
    }
}
// Verify cmdBuffer in given cb_node is not in global in-flight set, and return skip_call result
//  If this is a secondary command buffer, then make sure its primary is also in-flight
//  If primary is not in-flight, then remove secondary from global in-flight set
// This function is only valid at a point when cmdBuffer is being reset or freed
static bool checkCommandBufferInFlight(layer_data *dev_data, const GLOBAL_CB_NODE *cb_node, const char *action,
                                       UNIQUE_VALIDATION_ERROR_CODE error_code) {
    bool skip_call = false;
    if (dev_data->globalInFlightCmdBuffers.count(cb_node->commandBuffer)) {
        // Primary CB or secondary where primary is also in-flight is an error
        if ((cb_node->createInfo.level != VK_COMMAND_BUFFER_LEVEL_SECONDARY) ||
            (dev_data->globalInFlightCmdBuffers.count(cb_node->primaryCommandBuffer))) {
            skip_call |=
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        reinterpret_cast<const uint64_t &>(cb_node->commandBuffer), __LINE__, error_code, "DS",
                        "Attempt to %s command buffer (0x%" PRIxLEAST64 ") which is in use. %s", action,
                        reinterpret_cast<const uint64_t &>(cb_node->commandBuffer), validation_error_map[error_code]);
        }
    }
    return skip_call;
}

// Iterate over all cmdBuffers in given commandPool and verify that each is not in use
static bool checkCommandBuffersInFlight(layer_data *dev_data, COMMAND_POOL_NODE *pPool, const char *action,
                                        UNIQUE_VALIDATION_ERROR_CODE error_code) {
    bool skip_call = false;
    for (auto cmd_buffer : pPool->commandBuffers) {
        if (dev_data->globalInFlightCmdBuffers.count(cmd_buffer)) {
            skip_call |= checkCommandBufferInFlight(dev_data, getCBNode(dev_data, cmd_buffer), action, error_code);
        }
    }
    return skip_call;
}

static void clearCommandBuffersInFlight(layer_data *dev_data, COMMAND_POOL_NODE *pPool) {
    for (auto cmd_buffer : pPool->commandBuffers) {
        dev_data->globalInFlightCmdBuffers.erase(cmd_buffer);
    }
}

VKAPI_ATTR void VKAPI_CALL
FreeCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t commandBufferCount, const VkCommandBuffer *pCommandBuffers) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    bool skip_call = false;
    std::unique_lock<std::mutex> lock(global_lock);

    for (uint32_t i = 0; i < commandBufferCount; i++) {
        auto cb_node = getCBNode(dev_data, pCommandBuffers[i]);
        // Delete CB information structure, and remove from commandBufferMap
        if (cb_node) {
            skip_call |= checkCommandBufferInFlight(dev_data, cb_node, "free", VALIDATION_ERROR_00096);
        }
    }

    if (skip_call)
        return;

    auto pPool = getCommandPoolNode(dev_data, commandPool);
    for (uint32_t i = 0; i < commandBufferCount; i++) {
        auto cb_node = getCBNode(dev_data, pCommandBuffers[i]);
        // Delete CB information structure, and remove from commandBufferMap
        if (cb_node) {
            dev_data->globalInFlightCmdBuffers.erase(cb_node->commandBuffer);
            // reset prior to delete for data clean-up
            resetCB(dev_data, cb_node->commandBuffer);
            dev_data->commandBufferMap.erase(cb_node->commandBuffer);
            delete cb_node;
        }

        // Remove commandBuffer reference from commandPoolMap
        pPool->commandBuffers.remove(pCommandBuffers[i]);
    }
    printCBList(dev_data);
    lock.unlock();

    dev_data->dispatch_table.FreeCommandBuffers(device, commandPool, commandBufferCount, pCommandBuffers);
}

VKAPI_ATTR VkResult VKAPI_CALL CreateCommandPool(VkDevice device, const VkCommandPoolCreateInfo *pCreateInfo,
                                                 const VkAllocationCallbacks *pAllocator,
                                                 VkCommandPool *pCommandPool) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    VkResult result = dev_data->dispatch_table.CreateCommandPool(device, pCreateInfo, pAllocator, pCommandPool);

    if (VK_SUCCESS == result) {
        std::lock_guard<std::mutex> lock(global_lock);
        dev_data->commandPoolMap[*pCommandPool].createFlags = pCreateInfo->flags;
        dev_data->commandPoolMap[*pCommandPool].queueFamilyIndex = pCreateInfo->queueFamilyIndex;
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateQueryPool(VkDevice device, const VkQueryPoolCreateInfo *pCreateInfo,
                                               const VkAllocationCallbacks *pAllocator, VkQueryPool *pQueryPool) {

    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->dispatch_table.CreateQueryPool(device, pCreateInfo, pAllocator, pQueryPool);
    if (result == VK_SUCCESS) {
        std::lock_guard<std::mutex> lock(global_lock);
        QUERY_POOL_NODE *qp_node = &dev_data->queryPoolMap[*pQueryPool];
        qp_node->createInfo = *pCreateInfo;
    }
    return result;
}

static bool PreCallValidateDestroyCommandPool(layer_data *dev_data, VkCommandPool pool, COMMAND_POOL_NODE **cp_state) {
    *cp_state = getCommandPoolNode(dev_data, pool);
    if (dev_data->instance_data->disabled.destroy_command_pool)
        return false;
    bool skip = false;
    if (*cp_state) {
        // Verify that command buffers in pool are complete (not in-flight)
        skip |= checkCommandBuffersInFlight(dev_data, *cp_state, "destroy command pool with", VALIDATION_ERROR_00077);
    }
    return skip;
}

static void PostCallRecordDestroyCommandPool(layer_data *dev_data, VkCommandPool pool, COMMAND_POOL_NODE *cp_state) {
    // Must remove cmdpool from cmdpoolmap, after removing all cmdbuffers in its list from the commandBufferMap
    clearCommandBuffersInFlight(dev_data, cp_state);
    for (auto cb : cp_state->commandBuffers) {
        clear_cmd_buf_and_mem_references(dev_data, cb);
        auto cb_node = getCBNode(dev_data, cb);
        // Remove references to this cb_node prior to delete
        // TODO : Need better solution here, resetCB?
        for (auto obj : cb_node->object_bindings) {
            removeCommandBufferBinding(dev_data, &obj, cb_node);
        }
        for (auto framebuffer : cb_node->framebuffers) {
            auto fb_state = getFramebufferState(dev_data, framebuffer);
            if (fb_state)
                fb_state->cb_bindings.erase(cb_node);
        }
        dev_data->commandBufferMap.erase(cb); // Remove this command buffer
        delete cb_node;                       // delete CB info structure
    }
    dev_data->commandPoolMap.erase(pool);
}

// Destroy commandPool along with all of the commandBuffers allocated from that pool
VKAPI_ATTR void VKAPI_CALL DestroyCommandPool(VkDevice device, VkCommandPool commandPool, const VkAllocationCallbacks *pAllocator) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    COMMAND_POOL_NODE *cp_state = nullptr;
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip = PreCallValidateDestroyCommandPool(dev_data, commandPool, &cp_state);
    if (!skip) {
        lock.unlock();
        dev_data->dispatch_table.DestroyCommandPool(device, commandPool, pAllocator);
        lock.lock();
        PostCallRecordDestroyCommandPool(dev_data, commandPool, cp_state);
    }
}

VKAPI_ATTR VkResult VKAPI_CALL
ResetCommandPool(VkDevice device, VkCommandPool commandPool, VkCommandPoolResetFlags flags) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    bool skip_call = false;

    std::unique_lock<std::mutex> lock(global_lock);
    auto pPool = getCommandPoolNode(dev_data, commandPool);
    skip_call |= checkCommandBuffersInFlight(dev_data, pPool, "reset command pool with", VALIDATION_ERROR_00072);
    lock.unlock();

    if (skip_call)
        return VK_ERROR_VALIDATION_FAILED_EXT;

    VkResult result = dev_data->dispatch_table.ResetCommandPool(device, commandPool, flags);

    // Reset all of the CBs allocated from this pool
    if (VK_SUCCESS == result) {
        lock.lock();
        clearCommandBuffersInFlight(dev_data, pPool);
        for (auto cmdBuffer : pPool->commandBuffers) {
            resetCB(dev_data, cmdBuffer);
        }
        lock.unlock();
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL ResetFences(VkDevice device, uint32_t fenceCount, const VkFence *pFences) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    bool skip_call = false;
    std::unique_lock<std::mutex> lock(global_lock);
    for (uint32_t i = 0; i < fenceCount; ++i) {
        auto pFence = getFenceNode(dev_data, pFences[i]);
        if (pFence && pFence->state == FENCE_INFLIGHT) {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT,
                                 reinterpret_cast<const uint64_t &>(pFences[i]), __LINE__, DRAWSTATE_INVALID_FENCE, "DS",
                                 "Fence 0x%" PRIx64 " is in use.", reinterpret_cast<const uint64_t &>(pFences[i]));
        }
    }
    lock.unlock();

    if (skip_call)
        return VK_ERROR_VALIDATION_FAILED_EXT;

    VkResult result = dev_data->dispatch_table.ResetFences(device, fenceCount, pFences);

    if (result == VK_SUCCESS) {
        lock.lock();
        for (uint32_t i = 0; i < fenceCount; ++i) {
            auto pFence = getFenceNode(dev_data, pFences[i]);
            if (pFence) {
                pFence->state = FENCE_UNSIGNALED;
            }
        }
        lock.unlock();
    }

    return result;
}

// For given cb_nodes, invalidate them and track object causing invalidation
void invalidateCommandBuffers(std::unordered_set<GLOBAL_CB_NODE *> cb_nodes, VK_OBJECT obj) {
    for (auto cb_node : cb_nodes) {
        cb_node->state = CB_INVALID;
        cb_node->broken_bindings.push_back(obj);
    }
}

static bool PreCallValidateDestroyFramebuffer(layer_data *dev_data, VkFramebuffer framebuffer,
                                              FRAMEBUFFER_STATE **framebuffer_state, VK_OBJECT *obj_struct) {
    *framebuffer_state = getFramebufferState(dev_data, framebuffer);
    *obj_struct = {reinterpret_cast<uint64_t &>(framebuffer), VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT};
    if (dev_data->instance_data->disabled.destroy_framebuffer)
        return false;
    bool skip = false;
    if (*framebuffer_state) {
        skip |= ValidateObjectNotInUse(dev_data, *framebuffer_state, *obj_struct, VALIDATION_ERROR_00422);
    }
    return skip;
}

static void PostCallRecordDestroyFramebuffer(layer_data *dev_data, VkFramebuffer framebuffer, FRAMEBUFFER_STATE *framebuffer_state,
                                             VK_OBJECT obj_struct) {
    invalidateCommandBuffers(framebuffer_state->cb_bindings, obj_struct);
    dev_data->frameBufferMap.erase(framebuffer);
}

VKAPI_ATTR void VKAPI_CALL
DestroyFramebuffer(VkDevice device, VkFramebuffer framebuffer, const VkAllocationCallbacks *pAllocator) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    FRAMEBUFFER_STATE *framebuffer_state = nullptr;
    VK_OBJECT obj_struct;
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip = PreCallValidateDestroyFramebuffer(dev_data, framebuffer, &framebuffer_state, &obj_struct);
    if (!skip) {
        lock.unlock();
        dev_data->dispatch_table.DestroyFramebuffer(device, framebuffer, pAllocator);
        lock.lock();
        PostCallRecordDestroyFramebuffer(dev_data, framebuffer, framebuffer_state, obj_struct);
    }
}

static bool PreCallValidateDestroyRenderPass(layer_data *dev_data, VkRenderPass render_pass, RENDER_PASS_STATE **rp_state,
                                             VK_OBJECT *obj_struct) {
    *rp_state = getRenderPassState(dev_data, render_pass);
    *obj_struct = {reinterpret_cast<uint64_t &>(render_pass), VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT};
    if (dev_data->instance_data->disabled.destroy_renderpass)
        return false;
    bool skip = false;
    if (*rp_state) {
        skip |= ValidateObjectNotInUse(dev_data, *rp_state, *obj_struct, VALIDATION_ERROR_00393);
    }
    return skip;
}

static void PostCallRecordDestroyRenderPass(layer_data *dev_data, VkRenderPass render_pass, RENDER_PASS_STATE *rp_state,
                                            VK_OBJECT obj_struct) {
    invalidateCommandBuffers(rp_state->cb_bindings, obj_struct);
    dev_data->renderPassMap.erase(render_pass);
}

VKAPI_ATTR void VKAPI_CALL
DestroyRenderPass(VkDevice device, VkRenderPass renderPass, const VkAllocationCallbacks *pAllocator) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    RENDER_PASS_STATE *rp_state = nullptr;
    VK_OBJECT obj_struct;
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip = PreCallValidateDestroyRenderPass(dev_data, renderPass, &rp_state, &obj_struct);
    if (!skip) {
        lock.unlock();
        dev_data->dispatch_table.DestroyRenderPass(device, renderPass, pAllocator);
        lock.lock();
        PostCallRecordDestroyRenderPass(dev_data, renderPass, rp_state, obj_struct);
    }
}

VKAPI_ATTR VkResult VKAPI_CALL CreateBuffer(VkDevice device, const VkBufferCreateInfo *pCreateInfo,
                                            const VkAllocationCallbacks *pAllocator, VkBuffer *pBuffer) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    // TODO: Add check for VALIDATION_ERROR_00658
    // TODO: Add check for VALIDATION_ERROR_00666
    // TODO: Add check for VALIDATION_ERROR_00667
    // TODO: Add check for VALIDATION_ERROR_00668
    // TODO: Add check for VALIDATION_ERROR_00669
    VkResult result = dev_data->dispatch_table.CreateBuffer(device, pCreateInfo, pAllocator, pBuffer);

    if (VK_SUCCESS == result) {
        std::lock_guard<std::mutex> lock(global_lock);
        // TODO : This doesn't create deep copy of pQueueFamilyIndices so need to fix that if/when we want that data to be valid
        dev_data->bufferMap.insert(std::make_pair(*pBuffer, unique_ptr<BUFFER_NODE>(new BUFFER_NODE(*pBuffer, pCreateInfo))));
    }
    return result;
}

static bool PreCallValidateCreateBufferView(layer_data *dev_data, const VkBufferViewCreateInfo *pCreateInfo) {
    bool skip_call = false;
    BUFFER_NODE *buf_node = getBufferNode(dev_data, pCreateInfo->buffer);
    // If this isn't a sparse buffer, it needs to have memory backing it at CreateBufferView time
    if (buf_node) {
        skip_call |= ValidateMemoryIsBoundToBuffer(dev_data, buf_node, "vkCreateBufferView()");
        // In order to create a valid buffer view, the buffer must have been created with at least one of the
        // following flags:  UNIFORM_TEXEL_BUFFER_BIT or STORAGE_TEXEL_BUFFER_BIT
        skip_call |= ValidateBufferUsageFlags(dev_data, buf_node,
                                              VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT,
                                              false, "vkCreateBufferView()", "VK_BUFFER_USAGE_[STORAGE|UNIFORM]_TEXEL_BUFFER_BIT");
    }
    return skip_call;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateBufferView(VkDevice device, const VkBufferViewCreateInfo *pCreateInfo,
                                                const VkAllocationCallbacks *pAllocator, VkBufferView *pView) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip_call = PreCallValidateCreateBufferView(dev_data, pCreateInfo);
    lock.unlock();
    if (skip_call)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = dev_data->dispatch_table.CreateBufferView(device, pCreateInfo, pAllocator, pView);
    if (VK_SUCCESS == result) {
        lock.lock();
        dev_data->bufferViewMap[*pView] = unique_ptr<BUFFER_VIEW_STATE>(new BUFFER_VIEW_STATE(*pView, pCreateInfo));
        lock.unlock();
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateImage(VkDevice device, const VkImageCreateInfo *pCreateInfo,
                                           const VkAllocationCallbacks *pAllocator, VkImage *pImage) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    VkResult result = dev_data->dispatch_table.CreateImage(device, pCreateInfo, pAllocator, pImage);

    if (VK_SUCCESS == result) {
        std::lock_guard<std::mutex> lock(global_lock);
        IMAGE_LAYOUT_NODE image_state;
        image_state.layout = pCreateInfo->initialLayout;
        image_state.format = pCreateInfo->format;
        dev_data->imageMap.insert(std::make_pair(*pImage, unique_ptr<IMAGE_STATE>(new IMAGE_STATE(*pImage, pCreateInfo))));
        ImageSubresourcePair subpair = {*pImage, false, VkImageSubresource()};
        dev_data->imageSubresourceMap[*pImage].push_back(subpair);
        dev_data->imageLayoutMap[subpair] = image_state;
    }
    return result;
}

static void ResolveRemainingLevelsLayers(layer_data *dev_data, VkImageSubresourceRange *range, VkImage image) {
    /* expects global_lock to be held by caller */

    auto image_state = getImageState(dev_data, image);
    if (image_state) {
        /* If the caller used the special values VK_REMAINING_MIP_LEVELS and
         * VK_REMAINING_ARRAY_LAYERS, resolve them now in our internal state to
         * the actual values.
         */
        if (range->levelCount == VK_REMAINING_MIP_LEVELS) {
            range->levelCount = image_state->createInfo.mipLevels - range->baseMipLevel;
        }

        if (range->layerCount == VK_REMAINING_ARRAY_LAYERS) {
            range->layerCount = image_state->createInfo.arrayLayers - range->baseArrayLayer;
        }
    }
}

// Return the correct layer/level counts if the caller used the special
// values VK_REMAINING_MIP_LEVELS or VK_REMAINING_ARRAY_LAYERS.
static void ResolveRemainingLevelsLayers(layer_data *dev_data, uint32_t *levels, uint32_t *layers, VkImageSubresourceRange range,
                                         VkImage image) {
    /* expects global_lock to be held by caller */

    *levels = range.levelCount;
    *layers = range.layerCount;
    auto image_state = getImageState(dev_data, image);
    if (image_state) {
        if (range.levelCount == VK_REMAINING_MIP_LEVELS) {
            *levels = image_state->createInfo.mipLevels - range.baseMipLevel;
        }
        if (range.layerCount == VK_REMAINING_ARRAY_LAYERS) {
            *layers = image_state->createInfo.arrayLayers - range.baseArrayLayer;
        }
    }
}

static bool PreCallValidateCreateImageView(layer_data *dev_data, const VkImageViewCreateInfo *pCreateInfo) {
    bool skip_call = false;
    IMAGE_STATE *image_state = getImageState(dev_data, pCreateInfo->image);
    if (image_state) {
        skip_call |= ValidateImageUsageFlags(
            dev_data, image_state, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT |
                                       VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            false, "vkCreateImageView()",
            "VK_IMAGE_USAGE_[SAMPLED|STORAGE|COLOR_ATTACHMENT|DEPTH_STENCIL_ATTACHMENT|INPUT_ATTACHMENT]_BIT");
        // If this isn't a sparse image, it needs to have memory backing it at CreateImageView time
        skip_call |= ValidateMemoryIsBoundToImage(dev_data, image_state, "vkCreateImageView()");
    }
    return skip_call;
}

static inline void PostCallRecordCreateImageView(layer_data *dev_data, const VkImageViewCreateInfo *pCreateInfo, VkImageView view) {
    dev_data->imageViewMap[view] = unique_ptr<IMAGE_VIEW_STATE>(new IMAGE_VIEW_STATE(view, pCreateInfo));
    ResolveRemainingLevelsLayers(dev_data, &dev_data->imageViewMap[view].get()->create_info.subresourceRange, pCreateInfo->image);
}

VKAPI_ATTR VkResult VKAPI_CALL CreateImageView(VkDevice device, const VkImageViewCreateInfo *pCreateInfo,
                                               const VkAllocationCallbacks *pAllocator, VkImageView *pView) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip_call = PreCallValidateCreateImageView(dev_data, pCreateInfo);
    lock.unlock();
    if (skip_call)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = dev_data->dispatch_table.CreateImageView(device, pCreateInfo, pAllocator, pView);
    if (VK_SUCCESS == result) {
        lock.lock();
        PostCallRecordCreateImageView(dev_data, pCreateInfo, *pView);
        lock.unlock();
    }

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL
CreateFence(VkDevice device, const VkFenceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkFence *pFence) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->dispatch_table.CreateFence(device, pCreateInfo, pAllocator, pFence);
    if (VK_SUCCESS == result) {
        std::lock_guard<std::mutex> lock(global_lock);
        auto &fence_node = dev_data->fenceMap[*pFence];
        fence_node.fence = *pFence;
        fence_node.createInfo = *pCreateInfo;
        fence_node.state = (pCreateInfo->flags & VK_FENCE_CREATE_SIGNALED_BIT) ? FENCE_RETIRED : FENCE_UNSIGNALED;
    }
    return result;
}

// TODO handle pipeline caches
VKAPI_ATTR VkResult VKAPI_CALL CreatePipelineCache(VkDevice device, const VkPipelineCacheCreateInfo *pCreateInfo,
                                                   const VkAllocationCallbacks *pAllocator, VkPipelineCache *pPipelineCache) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->dispatch_table.CreatePipelineCache(device, pCreateInfo, pAllocator, pPipelineCache);
    return result;
}

VKAPI_ATTR void VKAPI_CALL
DestroyPipelineCache(VkDevice device, VkPipelineCache pipelineCache, const VkAllocationCallbacks *pAllocator) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    dev_data->dispatch_table.DestroyPipelineCache(device, pipelineCache, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL
GetPipelineCacheData(VkDevice device, VkPipelineCache pipelineCache, size_t *pDataSize, void *pData) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->dispatch_table.GetPipelineCacheData(device, pipelineCache, pDataSize, pData);
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL
MergePipelineCaches(VkDevice device, VkPipelineCache dstCache, uint32_t srcCacheCount, const VkPipelineCache *pSrcCaches) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->dispatch_table.MergePipelineCaches(device, dstCache, srcCacheCount, pSrcCaches);
    return result;
}

// utility function to set collective state for pipeline
void set_pipeline_state(PIPELINE_STATE *pPipe) {
    // If any attachment used by this pipeline has blendEnable, set top-level blendEnable
    if (pPipe->graphicsPipelineCI.pColorBlendState) {
        for (size_t i = 0; i < pPipe->attachments.size(); ++i) {
            if (VK_TRUE == pPipe->attachments[i].blendEnable) {
                if (((pPipe->attachments[i].dstAlphaBlendFactor >= VK_BLEND_FACTOR_CONSTANT_COLOR) &&
                     (pPipe->attachments[i].dstAlphaBlendFactor <= VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA)) ||
                    ((pPipe->attachments[i].dstColorBlendFactor >= VK_BLEND_FACTOR_CONSTANT_COLOR) &&
                     (pPipe->attachments[i].dstColorBlendFactor <= VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA)) ||
                    ((pPipe->attachments[i].srcAlphaBlendFactor >= VK_BLEND_FACTOR_CONSTANT_COLOR) &&
                     (pPipe->attachments[i].srcAlphaBlendFactor <= VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA)) ||
                    ((pPipe->attachments[i].srcColorBlendFactor >= VK_BLEND_FACTOR_CONSTANT_COLOR) &&
                     (pPipe->attachments[i].srcColorBlendFactor <= VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA))) {
                    pPipe->blendConstantsEnabled = true;
                }
            }
        }
    }
}

VKAPI_ATTR VkResult VKAPI_CALL
CreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t count,
                        const VkGraphicsPipelineCreateInfo *pCreateInfos, const VkAllocationCallbacks *pAllocator,
                        VkPipeline *pPipelines) {
    VkResult result = VK_SUCCESS;
    // TODO What to do with pipelineCache?
    // The order of operations here is a little convoluted but gets the job done
    //  1. Pipeline create state is first shadowed into PIPELINE_STATE struct
    //  2. Create state is then validated (which uses flags setup during shadowing)
    //  3. If everything looks good, we'll then create the pipeline and add NODE to pipelineMap
    bool skip_call = false;
    // TODO : Improve this data struct w/ unique_ptrs so cleanup below is automatic
    vector<PIPELINE_STATE *> pPipeState(count);
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    uint32_t i = 0;
    std::unique_lock<std::mutex> lock(global_lock);

    for (i = 0; i < count; i++) {
        pPipeState[i] = new PIPELINE_STATE;
        pPipeState[i]->initGraphicsPipeline(&pCreateInfos[i]);
        pPipeState[i]->render_pass_ci.initialize(getRenderPassState(dev_data, pCreateInfos[i].renderPass)->createInfo.ptr());
        pPipeState[i]->pipeline_layout = *getPipelineLayout(dev_data, pCreateInfos[i].layout);

        skip_call |= verifyPipelineCreateState(dev_data, device, pPipeState, i);
    }

    if (!skip_call) {
        lock.unlock();
        result =
            dev_data->dispatch_table.CreateGraphicsPipelines(device, pipelineCache, count, pCreateInfos, pAllocator, pPipelines);
        lock.lock();
        for (i = 0; i < count; i++) {
            pPipeState[i]->pipeline = pPipelines[i];
            dev_data->pipelineMap[pPipeState[i]->pipeline] = pPipeState[i];
        }
        lock.unlock();
    } else {
        for (i = 0; i < count; i++) {
            delete pPipeState[i];
        }
        lock.unlock();
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL
CreateComputePipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t count,
                       const VkComputePipelineCreateInfo *pCreateInfos, const VkAllocationCallbacks *pAllocator,
                       VkPipeline *pPipelines) {
    VkResult result = VK_SUCCESS;
    bool skip_call = false;

    // TODO : Improve this data struct w/ unique_ptrs so cleanup below is automatic
    vector<PIPELINE_STATE *> pPipeState(count);
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    uint32_t i = 0;
    std::unique_lock<std::mutex> lock(global_lock);
    for (i = 0; i < count; i++) {
        // TODO: Verify compute stage bits

        // Create and initialize internal tracking data structure
        pPipeState[i] = new PIPELINE_STATE;
        pPipeState[i]->initComputePipeline(&pCreateInfos[i]);
        pPipeState[i]->pipeline_layout = *getPipelineLayout(dev_data, pCreateInfos[i].layout);
        // memcpy(&pPipeState[i]->computePipelineCI, (const void *)&pCreateInfos[i], sizeof(VkComputePipelineCreateInfo));

        // TODO: Add Compute Pipeline Verification
        skip_call |= !validate_compute_pipeline(dev_data->report_data, pPipeState[i], &dev_data->enabled_features,
                                                dev_data->shaderModuleMap);
        // skip_call |= verifyPipelineCreateState(dev_data, device, pPipeState[i]);
    }

    if (!skip_call) {
        lock.unlock();
        result =
            dev_data->dispatch_table.CreateComputePipelines(device, pipelineCache, count, pCreateInfos, pAllocator, pPipelines);
        lock.lock();
        for (i = 0; i < count; i++) {
            pPipeState[i]->pipeline = pPipelines[i];
            dev_data->pipelineMap[pPipeState[i]->pipeline] = pPipeState[i];
        }
        lock.unlock();
    } else {
        for (i = 0; i < count; i++) {
            // Clean up any locally allocated data structures
            delete pPipeState[i];
        }
        lock.unlock();
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateSampler(VkDevice device, const VkSamplerCreateInfo *pCreateInfo,
                                             const VkAllocationCallbacks *pAllocator, VkSampler *pSampler) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->dispatch_table.CreateSampler(device, pCreateInfo, pAllocator, pSampler);
    if (VK_SUCCESS == result) {
        std::lock_guard<std::mutex> lock(global_lock);
        dev_data->samplerMap[*pSampler] = unique_ptr<SAMPLER_STATE>(new SAMPLER_STATE(pSampler, pCreateInfo));
    }
    return result;
}

static bool PreCallValidateCreateDescriptorSetLayout(layer_data *dev_data, const VkDescriptorSetLayoutCreateInfo *create_info) {
    if (dev_data->instance_data->disabled.create_descriptor_set_layout)
        return false;
    return cvdescriptorset::DescriptorSetLayout::ValidateCreateInfo(dev_data->report_data, create_info);
}

static void PostCallRecordCreateDescriptorSetLayout(layer_data *dev_data, const VkDescriptorSetLayoutCreateInfo *create_info,
                                                    VkDescriptorSetLayout set_layout) {
    // TODO: Convert this to unique_ptr to avoid leaks
    dev_data->descriptorSetLayoutMap[set_layout] = new cvdescriptorset::DescriptorSetLayout(create_info, set_layout);
}

VKAPI_ATTR VkResult VKAPI_CALL
CreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo *pCreateInfo,
                          const VkAllocationCallbacks *pAllocator, VkDescriptorSetLayout *pSetLayout) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip = PreCallValidateCreateDescriptorSetLayout(dev_data, pCreateInfo);
    if (!skip) {
        lock.unlock();
        result = dev_data->dispatch_table.CreateDescriptorSetLayout(device, pCreateInfo, pAllocator, pSetLayout);
        if (VK_SUCCESS == result) {
            lock.lock();
            PostCallRecordCreateDescriptorSetLayout(dev_data, pCreateInfo, *pSetLayout);
        }
    }
    return result;
}

// Used by CreatePipelineLayout and CmdPushConstants.
// Note that the index argument is optional and only used by CreatePipelineLayout.
static bool validatePushConstantRange(const layer_data *dev_data, const uint32_t offset, const uint32_t size,
                                      const char *caller_name, uint32_t index = 0) {
    if (dev_data->instance_data->disabled.push_constant_range)
        return false;
    uint32_t const maxPushConstantsSize = dev_data->phys_dev_properties.properties.limits.maxPushConstantsSize;
    bool skip_call = false;
    // Check that offset + size don't exceed the max.
    // Prevent arithetic overflow here by avoiding addition and testing in this order.
    // TODO : This check combines VALIDATION_ERROR_00877 & 880, need to break out separately
    if ((offset >= maxPushConstantsSize) || (size > maxPushConstantsSize - offset)) {
        // This is a pain just to adapt the log message to the caller, but better to sort it out only when there is a problem.
        if (0 == strcmp(caller_name, "vkCreatePipelineLayout()")) {
            skip_call |=
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                        VALIDATION_ERROR_00877, "DS", "%s call has push constants index %u with offset %u and size %u that "
                                                      "exceeds this device's maxPushConstantSize of %u. %s",
                        caller_name, index, offset, size, maxPushConstantsSize, validation_error_map[VALIDATION_ERROR_00877]);
        } else if (0 == strcmp(caller_name, "vkCmdPushConstants()")) {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                 DRAWSTATE_PUSH_CONSTANTS_ERROR, "DS", "%s call has push constants with offset %u and size %u that "
                                                                       "exceeds this device's maxPushConstantSize of %u.",
                                 caller_name, offset, size, maxPushConstantsSize);
        } else {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                 DRAWSTATE_INTERNAL_ERROR, "DS", "%s caller not supported.", caller_name);
        }
    }
    // size needs to be non-zero and a multiple of 4.
    // TODO : This check combines VALIDATION_ERROR_00878 & 879, need to break out separately
    if ((size == 0) || ((size & 0x3) != 0)) {
        if (0 == strcmp(caller_name, "vkCreatePipelineLayout()")) {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                 VALIDATION_ERROR_00878, "DS", "%s call has push constants index %u with "
                                                               "size %u. Size must be greater than zero and a multiple of 4. %s",
                                 caller_name, index, size, validation_error_map[VALIDATION_ERROR_00878]);
        } else if (0 == strcmp(caller_name, "vkCmdPushConstants()")) {
            skip_call |=
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                        DRAWSTATE_PUSH_CONSTANTS_ERROR, "DS", "%s call has push constants with "
                                                              "size %u. Size must be greater than zero and a multiple of 4.",
                        caller_name, size);
        } else {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                 DRAWSTATE_INTERNAL_ERROR, "DS", "%s caller not supported.", caller_name);
        }
    }
    // offset needs to be a multiple of 4.
    if ((offset & 0x3) != 0) {
        if (0 == strcmp(caller_name, "vkCreatePipelineLayout()")) {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                 DRAWSTATE_PUSH_CONSTANTS_ERROR, "DS", "%s call has push constants index %u with "
                                                                       "offset %u. Offset must be a multiple of 4.",
                                 caller_name, index, offset);
        } else if (0 == strcmp(caller_name, "vkCmdPushConstants()")) {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                 DRAWSTATE_PUSH_CONSTANTS_ERROR, "DS", "%s call has push constants with "
                                                                       "offset %u. Offset must be a multiple of 4.",
                                 caller_name, offset);
        } else {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                 DRAWSTATE_INTERNAL_ERROR, "DS", "%s caller not supported.", caller_name);
        }
    }
    return skip_call;
}

VKAPI_ATTR VkResult VKAPI_CALL CreatePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo *pCreateInfo,
                                                    const VkAllocationCallbacks *pAllocator, VkPipelineLayout *pPipelineLayout) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    // TODO : Add checks for VALIDATION_ERRORS 865-871
    // Push Constant Range checks
    uint32_t i, j;
    for (i = 0; i < pCreateInfo->pushConstantRangeCount; ++i) {
        skip_call |= validatePushConstantRange(dev_data, pCreateInfo->pPushConstantRanges[i].offset,
                                               pCreateInfo->pPushConstantRanges[i].size, "vkCreatePipelineLayout()", i);
        if (0 == pCreateInfo->pPushConstantRanges[i].stageFlags) {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                 DRAWSTATE_PUSH_CONSTANTS_ERROR, "DS", "vkCreatePipelineLayout() call has no stageFlags set.");
        }
    }
    if (skip_call)
        return VK_ERROR_VALIDATION_FAILED_EXT;

    // Each range has been validated.  Now check for overlap between ranges (if they are good).
    // There's no explicit Valid Usage language against this, so issue a warning instead of an error.
    for (i = 0; i < pCreateInfo->pushConstantRangeCount; ++i) {
        for (j = i + 1; j < pCreateInfo->pushConstantRangeCount; ++j) {
            const uint32_t minA = pCreateInfo->pPushConstantRanges[i].offset;
            const uint32_t maxA = minA + pCreateInfo->pPushConstantRanges[i].size;
            const uint32_t minB = pCreateInfo->pPushConstantRanges[j].offset;
            const uint32_t maxB = minB + pCreateInfo->pPushConstantRanges[j].size;
            if ((minA <= minB && maxA > minB) || (minB <= minA && maxB > minA)) {
                skip_call |=
                    log_msg(dev_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                            DRAWSTATE_PUSH_CONSTANTS_ERROR, "DS", "vkCreatePipelineLayout() call has push constants with "
                                                                  "overlapping ranges: %u:[%u, %u), %u:[%u, %u)",
                            i, minA, maxA, j, minB, maxB);
            }
        }
    }

    VkResult result = dev_data->dispatch_table.CreatePipelineLayout(device, pCreateInfo, pAllocator, pPipelineLayout);
    if (VK_SUCCESS == result) {
        std::lock_guard<std::mutex> lock(global_lock);
        PIPELINE_LAYOUT_NODE &plNode = dev_data->pipelineLayoutMap[*pPipelineLayout];
        plNode.layout = *pPipelineLayout;
        plNode.set_layouts.resize(pCreateInfo->setLayoutCount);
        for (i = 0; i < pCreateInfo->setLayoutCount; ++i) {
            plNode.set_layouts[i] = getDescriptorSetLayout(dev_data, pCreateInfo->pSetLayouts[i]);
        }
        plNode.push_constant_ranges.resize(pCreateInfo->pushConstantRangeCount);
        for (i = 0; i < pCreateInfo->pushConstantRangeCount; ++i) {
            plNode.push_constant_ranges[i] = pCreateInfo->pPushConstantRanges[i];
        }
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL
CreateDescriptorPool(VkDevice device, const VkDescriptorPoolCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator,
                     VkDescriptorPool *pDescriptorPool) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->dispatch_table.CreateDescriptorPool(device, pCreateInfo, pAllocator, pDescriptorPool);
    if (VK_SUCCESS == result) {
        if (log_msg(dev_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT,
                    (uint64_t)*pDescriptorPool, __LINE__, DRAWSTATE_OUT_OF_MEMORY, "DS", "Created Descriptor Pool 0x%" PRIxLEAST64,
                    (uint64_t)*pDescriptorPool))
            return VK_ERROR_VALIDATION_FAILED_EXT;
        DESCRIPTOR_POOL_STATE *pNewNode = new DESCRIPTOR_POOL_STATE(*pDescriptorPool, pCreateInfo);
        if (NULL == pNewNode) {
            if (log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT,
                        (uint64_t)*pDescriptorPool, __LINE__, DRAWSTATE_OUT_OF_MEMORY, "DS",
                        "Out of memory while attempting to allocate DESCRIPTOR_POOL_STATE in vkCreateDescriptorPool()"))
                return VK_ERROR_VALIDATION_FAILED_EXT;
        } else {
            std::lock_guard<std::mutex> lock(global_lock);
            dev_data->descriptorPoolMap[*pDescriptorPool] = pNewNode;
        }
    } else {
        // Need to do anything if pool create fails?
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL
ResetDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorPoolResetFlags flags) {
    // TODO : Add checks for VALIDATION_ERROR_00928
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->dispatch_table.ResetDescriptorPool(device, descriptorPool, flags);
    if (VK_SUCCESS == result) {
        std::lock_guard<std::mutex> lock(global_lock);
        clearDescriptorPool(dev_data, device, descriptorPool, flags);
    }
    return result;
}
// Ensure the pool contains enough descriptors and descriptor sets to satisfy
// an allocation request. Fills common_data with the total number of descriptors of each type required,
// as well as DescriptorSetLayout ptrs used for later update.
static bool PreCallValidateAllocateDescriptorSets(layer_data *dev_data, const VkDescriptorSetAllocateInfo *pAllocateInfo,
                                                  cvdescriptorset::AllocateDescriptorSetsData *common_data) {
    if (dev_data->instance_data->disabled.allocate_descriptor_sets)
        return false;
    // All state checks for AllocateDescriptorSets is done in single function
    return cvdescriptorset::ValidateAllocateDescriptorSets(dev_data->report_data, pAllocateInfo, dev_data, common_data);
}
// Allocation state was good and call down chain was made so update state based on allocating descriptor sets
static void PostCallRecordAllocateDescriptorSets(layer_data *dev_data, const VkDescriptorSetAllocateInfo *pAllocateInfo,
                                                 VkDescriptorSet *pDescriptorSets,
                                                 const cvdescriptorset::AllocateDescriptorSetsData *common_data) {
    // All the updates are contained in a single cvdescriptorset function
    cvdescriptorset::PerformAllocateDescriptorSets(pAllocateInfo, pDescriptorSets, common_data, &dev_data->descriptorPoolMap,
                                                   &dev_data->setMap, dev_data);
}

VKAPI_ATTR VkResult VKAPI_CALL
AllocateDescriptorSets(VkDevice device, const VkDescriptorSetAllocateInfo *pAllocateInfo, VkDescriptorSet *pDescriptorSets) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    cvdescriptorset::AllocateDescriptorSetsData common_data(pAllocateInfo->descriptorSetCount);
    bool skip_call = PreCallValidateAllocateDescriptorSets(dev_data, pAllocateInfo, &common_data);
    lock.unlock();

    if (skip_call)
        return VK_ERROR_VALIDATION_FAILED_EXT;

    VkResult result = dev_data->dispatch_table.AllocateDescriptorSets(device, pAllocateInfo, pDescriptorSets);

    if (VK_SUCCESS == result) {
        lock.lock();
        PostCallRecordAllocateDescriptorSets(dev_data, pAllocateInfo, pDescriptorSets, &common_data);
        lock.unlock();
    }
    return result;
}
// Verify state before freeing DescriptorSets
static bool PreCallValidateFreeDescriptorSets(const layer_data *dev_data, VkDescriptorPool pool, uint32_t count,
                                              const VkDescriptorSet *descriptor_sets) {
    if (dev_data->instance_data->disabled.free_descriptor_sets)
        return false;
    bool skip_call = false;
    // First make sure sets being destroyed are not currently in-use
    for (uint32_t i = 0; i < count; ++i)
        skip_call |= validateIdleDescriptorSet(dev_data, descriptor_sets[i], "vkFreeDescriptorSets");

    DESCRIPTOR_POOL_STATE *pool_state = getDescriptorPoolState(dev_data, pool);
    if (pool_state && !(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT & pool_state->createInfo.flags)) {
        // Can't Free from a NON_FREE pool
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT,
                             reinterpret_cast<uint64_t &>(pool), __LINE__, VALIDATION_ERROR_00922, "DS",
                             "It is invalid to call vkFreeDescriptorSets() with a pool created without setting "
                             "VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT. %s",
                             validation_error_map[VALIDATION_ERROR_00922]);
    }
    return skip_call;
}
// Sets have been removed from the pool so update underlying state
static void PostCallRecordFreeDescriptorSets(layer_data *dev_data, VkDescriptorPool pool, uint32_t count,
                                             const VkDescriptorSet *descriptor_sets) {
    DESCRIPTOR_POOL_STATE *pool_state = getDescriptorPoolState(dev_data, pool);
    // Update available descriptor sets in pool
    pool_state->availableSets += count;

    // For each freed descriptor add its resources back into the pool as available and remove from pool and setMap
    for (uint32_t i = 0; i < count; ++i) {
        auto set_state = dev_data->setMap[descriptor_sets[i]];
        uint32_t type_index = 0, descriptor_count = 0;
        for (uint32_t j = 0; j < set_state->GetBindingCount(); ++j) {
            type_index = static_cast<uint32_t>(set_state->GetTypeFromIndex(j));
            descriptor_count = set_state->GetDescriptorCountFromIndex(j);
            pool_state->availableDescriptorTypeCount[type_index] += descriptor_count;
        }
        freeDescriptorSet(dev_data, set_state);
        pool_state->sets.erase(set_state);
    }
}

VKAPI_ATTR VkResult VKAPI_CALL
FreeDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, uint32_t count, const VkDescriptorSet *pDescriptorSets) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    // Make sure that no sets being destroyed are in-flight
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip_call = PreCallValidateFreeDescriptorSets(dev_data, descriptorPool, count, pDescriptorSets);
    lock.unlock();

    if (skip_call)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = dev_data->dispatch_table.FreeDescriptorSets(device, descriptorPool, count, pDescriptorSets);
    if (VK_SUCCESS == result) {
        lock.lock();
        PostCallRecordFreeDescriptorSets(dev_data, descriptorPool, count, pDescriptorSets);
        lock.unlock();
    }
    return result;
}
// TODO : This is a Proof-of-concept for core validation architecture
//  Really we'll want to break out these functions to separate files but
//  keeping it all together here to prove out design
// PreCallValidate* handles validating all of the state prior to calling down chain to UpdateDescriptorSets()
static bool PreCallValidateUpdateDescriptorSets(layer_data *dev_data, uint32_t descriptorWriteCount,
                                                const VkWriteDescriptorSet *pDescriptorWrites, uint32_t descriptorCopyCount,
                                                const VkCopyDescriptorSet *pDescriptorCopies) {
    if (dev_data->instance_data->disabled.update_descriptor_sets)
        return false;
    // First thing to do is perform map look-ups.
    // NOTE : UpdateDescriptorSets is somewhat unique in that it's operating on a number of DescriptorSets
    //  so we can't just do a single map look-up up-front, but do them individually in functions below

    // Now make call(s) that validate state, but don't perform state updates in this function
    // Note, here DescriptorSets is unique in that we don't yet have an instance. Using a helper function in the
    //  namespace which will parse params and make calls into specific class instances
    return cvdescriptorset::ValidateUpdateDescriptorSets(dev_data->report_data, dev_data, descriptorWriteCount, pDescriptorWrites,
                                                         descriptorCopyCount, pDescriptorCopies);
}
// PostCallRecord* handles recording state updates following call down chain to UpdateDescriptorSets()
static void PostCallRecordUpdateDescriptorSets(layer_data *dev_data, uint32_t descriptorWriteCount,
                                               const VkWriteDescriptorSet *pDescriptorWrites, uint32_t descriptorCopyCount,
                                               const VkCopyDescriptorSet *pDescriptorCopies) {
    cvdescriptorset::PerformUpdateDescriptorSets(dev_data, descriptorWriteCount, pDescriptorWrites, descriptorCopyCount,
                                                 pDescriptorCopies);
}

VKAPI_ATTR void VKAPI_CALL
UpdateDescriptorSets(VkDevice device, uint32_t descriptorWriteCount, const VkWriteDescriptorSet *pDescriptorWrites,
                     uint32_t descriptorCopyCount, const VkCopyDescriptorSet *pDescriptorCopies) {
    // Only map look-up at top level is for device-level layer_data
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip_call = PreCallValidateUpdateDescriptorSets(dev_data, descriptorWriteCount, pDescriptorWrites, descriptorCopyCount,
                                                         pDescriptorCopies);
    lock.unlock();
    if (!skip_call) {
        dev_data->dispatch_table.UpdateDescriptorSets(device, descriptorWriteCount, pDescriptorWrites, descriptorCopyCount,
                                                      pDescriptorCopies);
        lock.lock();
        // Since UpdateDescriptorSets() is void, nothing to check prior to updating state
        PostCallRecordUpdateDescriptorSets(dev_data, descriptorWriteCount, pDescriptorWrites, descriptorCopyCount,
                                           pDescriptorCopies);
    }
}

VKAPI_ATTR VkResult VKAPI_CALL
AllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo *pCreateInfo, VkCommandBuffer *pCommandBuffer) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->dispatch_table.AllocateCommandBuffers(device, pCreateInfo, pCommandBuffer);
    if (VK_SUCCESS == result) {
        std::unique_lock<std::mutex> lock(global_lock);
        auto pPool = getCommandPoolNode(dev_data, pCreateInfo->commandPool);

        if (pPool) {
            for (uint32_t i = 0; i < pCreateInfo->commandBufferCount; i++) {
                // Add command buffer to its commandPool map
                pPool->commandBuffers.push_back(pCommandBuffer[i]);
                GLOBAL_CB_NODE *pCB = new GLOBAL_CB_NODE;
                // Add command buffer to map
                dev_data->commandBufferMap[pCommandBuffer[i]] = pCB;
                resetCB(dev_data, pCommandBuffer[i]);
                pCB->createInfo = *pCreateInfo;
                pCB->device = device;
            }
        }
        printCBList(dev_data);
        lock.unlock();
    }
    return result;
}

// Add bindings between the given cmd buffer & framebuffer and the framebuffer's children
static void AddFramebufferBinding(layer_data *dev_data, GLOBAL_CB_NODE *cb_state, FRAMEBUFFER_STATE *fb_state) {
    addCommandBufferBinding(&fb_state->cb_bindings,
                            {reinterpret_cast<uint64_t &>(fb_state->framebuffer), VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT},
                            cb_state);
    for (auto attachment : fb_state->attachments) {
        auto view_state = attachment.view_state;
        if (view_state) {
            AddCommandBufferBindingImageView(dev_data, cb_state, view_state);
        }
        auto rp_state = getRenderPassState(dev_data, fb_state->createInfo.renderPass);
        if (rp_state) {
            addCommandBufferBinding(
                &rp_state->cb_bindings,
                {reinterpret_cast<uint64_t &>(rp_state->renderPass), VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT}, cb_state);
        }
    }
}

VKAPI_ATTR VkResult VKAPI_CALL
BeginCommandBuffer(VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo *pBeginInfo) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    // Validate command buffer level
    GLOBAL_CB_NODE *cb_node = getCBNode(dev_data, commandBuffer);
    if (cb_node) {
        // This implicitly resets the Cmd Buffer so make sure any fence is done and then clear memory references
        if (dev_data->globalInFlightCmdBuffers.count(commandBuffer)) {
            skip_call |=
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)commandBuffer, __LINE__, MEMTRACK_RESET_CB_WHILE_IN_FLIGHT, "MEM",
                        "Calling vkBeginCommandBuffer() on active command buffer 0x%p before it has completed. "
                        "You must check command buffer fence before this call.",
                        commandBuffer);
        }
        clear_cmd_buf_and_mem_references(dev_data, cb_node);
        if (cb_node->createInfo.level != VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
            // Secondary Command Buffer
            const VkCommandBufferInheritanceInfo *pInfo = pBeginInfo->pInheritanceInfo;
            if (!pInfo) {
                skip_call |=
                    log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            reinterpret_cast<uint64_t>(commandBuffer), __LINE__, DRAWSTATE_BEGIN_CB_INVALID_STATE, "DS",
                            "vkBeginCommandBuffer(): Secondary Command Buffer (0x%p) must have inheritance info.",
                            reinterpret_cast<void *>(commandBuffer));
            } else {
                if (pBeginInfo->flags & VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT) {
                    if (!pInfo->renderPass) { // renderpass should NOT be null for a Secondary CB
                        skip_call |= log_msg(
                            dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            reinterpret_cast<uint64_t>(commandBuffer), __LINE__, DRAWSTATE_BEGIN_CB_INVALID_STATE, "DS",
                            "vkBeginCommandBuffer(): Secondary Command Buffers (0x%p) must specify a valid renderpass parameter.",
                            reinterpret_cast<void *>(commandBuffer));
                    }
                    if (!pInfo->framebuffer) { // framebuffer may be null for a Secondary CB, but this affects perf
                        skip_call |= log_msg(
                            dev_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            reinterpret_cast<uint64_t>(commandBuffer), __LINE__, DRAWSTATE_BEGIN_CB_INVALID_STATE, "DS",
                            "vkBeginCommandBuffer(): Secondary Command Buffers (0x%p) may perform better if a "
                            "valid framebuffer parameter is specified.",
                            reinterpret_cast<void *>(commandBuffer));
                    } else {
                        string errorString = "";
                        auto framebuffer = getFramebufferState(dev_data, pInfo->framebuffer);
                        if (framebuffer) {
                            if ((framebuffer->createInfo.renderPass != pInfo->renderPass) &&
                                !verify_renderpass_compatibility(dev_data, framebuffer->renderPassCreateInfo.ptr(),
                                                                 getRenderPassState(dev_data, pInfo->renderPass)->createInfo.ptr(),
                                                                 errorString)) {
                                // renderPass that framebuffer was created with must be compatible with local renderPass
                                skip_call |= log_msg(
                                    dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                    VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, reinterpret_cast<uint64_t>(commandBuffer),
                                    __LINE__, DRAWSTATE_RENDERPASS_INCOMPATIBLE, "DS",
                                    "vkBeginCommandBuffer(): Secondary Command "
                                    "Buffer (0x%p) renderPass (0x%" PRIxLEAST64 ") is incompatible w/ framebuffer "
                                    "(0x%" PRIxLEAST64 ") w/ render pass (0x%" PRIxLEAST64 ") due to: %s",
                                    reinterpret_cast<void *>(commandBuffer), reinterpret_cast<const uint64_t &>(pInfo->renderPass),
                                    reinterpret_cast<const uint64_t &>(pInfo->framebuffer),
                                    reinterpret_cast<uint64_t &>(framebuffer->createInfo.renderPass), errorString.c_str());
                            }
                            // Connect this framebuffer and its children to this cmdBuffer
                            AddFramebufferBinding(dev_data, cb_node, framebuffer);
                        }
                    }
                }
                if ((pInfo->occlusionQueryEnable == VK_FALSE ||
                     dev_data->enabled_features.occlusionQueryPrecise == VK_FALSE) &&
                    (pInfo->queryFlags & VK_QUERY_CONTROL_PRECISE_BIT)) {
                    skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, reinterpret_cast<uint64_t>(commandBuffer),
                                         __LINE__, DRAWSTATE_BEGIN_CB_INVALID_STATE, "DS",
                                         "vkBeginCommandBuffer(): Secondary Command Buffer (0x%p) must not have "
                                         "VK_QUERY_CONTROL_PRECISE_BIT if occulusionQuery is disabled or the device does not "
                                         "support precise occlusion queries.",
                                         reinterpret_cast<void *>(commandBuffer));
                }
            }
            if (pInfo && pInfo->renderPass != VK_NULL_HANDLE) {
                auto renderPass = getRenderPassState(dev_data, pInfo->renderPass);
                if (renderPass) {
                    if (pInfo->subpass >= renderPass->createInfo.subpassCount) {
                        skip_call |= log_msg(
                            dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            (uint64_t)commandBuffer, __LINE__, DRAWSTATE_BEGIN_CB_INVALID_STATE, "DS",
                            "vkBeginCommandBuffer(): Secondary Command Buffers (0x%p) must has a subpass index (%d) "
                            "that is less than the number of subpasses (%d).",
                            (void *)commandBuffer, pInfo->subpass, renderPass->createInfo.subpassCount);
                    }
                }
            }
        }
        if (CB_RECORDING == cb_node->state) {
            skip_call |=
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)commandBuffer, __LINE__, DRAWSTATE_BEGIN_CB_INVALID_STATE, "DS",
                        "vkBeginCommandBuffer(): Cannot call Begin on command buffer (0x%" PRIxLEAST64
                        ") in the RECORDING state. Must first call vkEndCommandBuffer().",
                        (uint64_t)commandBuffer);
        } else if (CB_RECORDED == cb_node->state || (CB_INVALID == cb_node->state && CMD_END == cb_node->cmds.back().type)) {
            VkCommandPool cmdPool = cb_node->createInfo.commandPool;
            auto pPool = getCommandPoolNode(dev_data, cmdPool);
            if (!(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT & pPool->createFlags)) {
                skip_call |=
                    log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            (uint64_t)commandBuffer, __LINE__, DRAWSTATE_INVALID_COMMAND_BUFFER_RESET, "DS",
                            "Call to vkBeginCommandBuffer() on command buffer (0x%" PRIxLEAST64
                            ") attempts to implicitly reset cmdBuffer created from command pool (0x%" PRIxLEAST64
                            ") that does NOT have the VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT bit set.",
                            (uint64_t)commandBuffer, (uint64_t)cmdPool);
            }
            resetCB(dev_data, commandBuffer);
        }
        // Set updated state here in case implicit reset occurs above
        cb_node->state = CB_RECORDING;
        cb_node->beginInfo = *pBeginInfo;
        if (cb_node->beginInfo.pInheritanceInfo) {
            cb_node->inheritanceInfo = *(cb_node->beginInfo.pInheritanceInfo);
            cb_node->beginInfo.pInheritanceInfo = &cb_node->inheritanceInfo;
            // If we are a secondary command-buffer and inheriting.  Update the items we should inherit.
            if ((cb_node->createInfo.level != VK_COMMAND_BUFFER_LEVEL_PRIMARY) &&
                (cb_node->beginInfo.flags & VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT)) {
                cb_node->activeRenderPass = getRenderPassState(dev_data, cb_node->beginInfo.pInheritanceInfo->renderPass);
                cb_node->activeSubpass = cb_node->beginInfo.pInheritanceInfo->subpass;
                cb_node->framebuffers.insert(cb_node->beginInfo.pInheritanceInfo->framebuffer);
            }
        }
    } else {
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                             (uint64_t)commandBuffer, __LINE__, DRAWSTATE_INVALID_COMMAND_BUFFER, "DS",
                             "In vkBeginCommandBuffer() and unable to find CommandBuffer Node for command buffer 0x%p!",
                             (void *)commandBuffer);
    }
    lock.unlock();
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result = dev_data->dispatch_table.BeginCommandBuffer(commandBuffer, pBeginInfo);

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL EndCommandBuffer(VkCommandBuffer commandBuffer) {
    bool skip_call = false;
    VkResult result = VK_SUCCESS;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if ((VK_COMMAND_BUFFER_LEVEL_PRIMARY == pCB->createInfo.level) || !(pCB->beginInfo.flags & VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT)) {
            // This needs spec clarification to update valid usage, see comments in PR:
            // https://github.com/KhronosGroup/Vulkan-LoaderAndValidationLayers/pull/516#discussion_r63013756
            skip_call |= insideRenderPass(dev_data, pCB, "vkEndCommandBuffer");
        }
        skip_call |= addCmd(dev_data, pCB, CMD_END, "vkEndCommandBuffer()");
        for (auto query : pCB->activeQueries) {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                 DRAWSTATE_INVALID_QUERY, "DS",
                                 "Ending command buffer with in progress query: queryPool 0x%" PRIx64 ", index %d",
                                 (uint64_t)(query.pool), query.index);
        }
    }
    if (!skip_call) {
        lock.unlock();
        result = dev_data->dispatch_table.EndCommandBuffer(commandBuffer);
        lock.lock();
        if (VK_SUCCESS == result) {
            pCB->state = CB_RECORDED;
            // Reset CB status flags
            pCB->status = 0;
            printCB(dev_data, commandBuffer);
        }
    } else {
        result = VK_ERROR_VALIDATION_FAILED_EXT;
    }
    lock.unlock();
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL
ResetCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferResetFlags flags) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *pCB = getCBNode(dev_data, commandBuffer);
    VkCommandPool cmdPool = pCB->createInfo.commandPool;
    auto pPool = getCommandPoolNode(dev_data, cmdPool);
    if (!(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT & pPool->createFlags)) {
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                             (uint64_t)commandBuffer, __LINE__, DRAWSTATE_INVALID_COMMAND_BUFFER_RESET, "DS",
                             "Attempt to reset command buffer (0x%" PRIxLEAST64 ") created from command pool (0x%" PRIxLEAST64
                             ") that does NOT have the VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT bit set.",
                             (uint64_t)commandBuffer, (uint64_t)cmdPool);
    }
    skip_call |= checkCommandBufferInFlight(dev_data, pCB, "reset", VALIDATION_ERROR_00092);
    lock.unlock();
    if (skip_call)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = dev_data->dispatch_table.ResetCommandBuffer(commandBuffer, flags);
    if (VK_SUCCESS == result) {
        lock.lock();
        dev_data->globalInFlightCmdBuffers.erase(commandBuffer);
        resetCB(dev_data, commandBuffer);
        lock.unlock();
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL
CmdBindPipeline(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        skip_call |= addCmd(dev_data, pCB, CMD_BINDPIPELINE, "vkCmdBindPipeline()");
        if ((VK_PIPELINE_BIND_POINT_COMPUTE == pipelineBindPoint) && (pCB->activeRenderPass)) {
            skip_call |=
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT,
                        (uint64_t)pipeline, __LINE__, DRAWSTATE_INVALID_RENDERPASS_CMD, "DS",
                        "Incorrectly binding compute pipeline (0x%" PRIxLEAST64 ") during active RenderPass (0x%" PRIxLEAST64 ")",
                        (uint64_t)pipeline, (uint64_t)pCB->activeRenderPass->renderPass);
        }

        PIPELINE_STATE *pPN = getPipelineState(dev_data, pipeline);
        if (pPN) {
            pCB->lastBound[pipelineBindPoint].pipeline_state = pPN;
            set_cb_pso_status(pCB, pPN);
            set_pipeline_state(pPN);
        } else {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT,
                                 (uint64_t)pipeline, __LINE__, DRAWSTATE_INVALID_PIPELINE, "DS",
                                 "Attempt to bind Pipeline 0x%" PRIxLEAST64 " that doesn't exist!", (uint64_t)(pipeline));
        }
        addCommandBufferBinding(&getPipelineState(dev_data, pipeline)->cb_bindings,
                                {reinterpret_cast<uint64_t &>(pipeline), VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT}, pCB);
    }
    lock.unlock();
    if (!skip_call)
        dev_data->dispatch_table.CmdBindPipeline(commandBuffer, pipelineBindPoint, pipeline);
}

VKAPI_ATTR void VKAPI_CALL
CmdSetViewport(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const VkViewport *pViewports) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        skip_call |= addCmd(dev_data, pCB, CMD_SETVIEWPORTSTATE, "vkCmdSetViewport()");
        pCB->viewportMask |= ((1u<<viewportCount) - 1u) << firstViewport;
    }
    lock.unlock();
    if (!skip_call)
        dev_data->dispatch_table.CmdSetViewport(commandBuffer, firstViewport, viewportCount, pViewports);
}

VKAPI_ATTR void VKAPI_CALL
CmdSetScissor(VkCommandBuffer commandBuffer, uint32_t firstScissor, uint32_t scissorCount, const VkRect2D *pScissors) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        skip_call |= addCmd(dev_data, pCB, CMD_SETSCISSORSTATE, "vkCmdSetScissor()");
        pCB->scissorMask |= ((1u<<scissorCount) - 1u) << firstScissor;
    }
    lock.unlock();
    if (!skip_call)
        dev_data->dispatch_table.CmdSetScissor(commandBuffer, firstScissor, scissorCount, pScissors);
}

VKAPI_ATTR void VKAPI_CALL CmdSetLineWidth(VkCommandBuffer commandBuffer, float lineWidth) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        skip_call |= addCmd(dev_data, pCB, CMD_SETLINEWIDTHSTATE, "vkCmdSetLineWidth()");
        pCB->status |= CBSTATUS_LINE_WIDTH_SET;

        PIPELINE_STATE *pPipeTrav = pCB->lastBound[VK_PIPELINE_BIND_POINT_GRAPHICS].pipeline_state;
        if (pPipeTrav != NULL && !isDynamic(pPipeTrav, VK_DYNAMIC_STATE_LINE_WIDTH)) {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, (VkDebugReportObjectTypeEXT)0,
                                 reinterpret_cast<uint64_t &>(commandBuffer), __LINE__, DRAWSTATE_INVALID_SET, "DS",
                                 "vkCmdSetLineWidth called but pipeline was created without VK_DYNAMIC_STATE_LINE_WIDTH "
                                 "flag.  This is undefined behavior and could be ignored.");
        } else {
            skip_call |= verifyLineWidth(dev_data, DRAWSTATE_INVALID_SET, reinterpret_cast<uint64_t &>(commandBuffer), lineWidth);
        }
    }
    lock.unlock();
    if (!skip_call)
        dev_data->dispatch_table.CmdSetLineWidth(commandBuffer, lineWidth);
}

VKAPI_ATTR void VKAPI_CALL
CmdSetDepthBias(VkCommandBuffer commandBuffer, float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        skip_call |= addCmd(dev_data, pCB, CMD_SETDEPTHBIASSTATE, "vkCmdSetDepthBias()");
        pCB->status |= CBSTATUS_DEPTH_BIAS_SET;
    }
    lock.unlock();
    if (!skip_call)
        dev_data->dispatch_table.CmdSetDepthBias(commandBuffer, depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor);
}

VKAPI_ATTR void VKAPI_CALL CmdSetBlendConstants(VkCommandBuffer commandBuffer, const float blendConstants[4]) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        skip_call |= addCmd(dev_data, pCB, CMD_SETBLENDSTATE, "vkCmdSetBlendConstants()");
        pCB->status |= CBSTATUS_BLEND_CONSTANTS_SET;
    }
    lock.unlock();
    if (!skip_call)
        dev_data->dispatch_table.CmdSetBlendConstants(commandBuffer, blendConstants);
}

VKAPI_ATTR void VKAPI_CALL
CmdSetDepthBounds(VkCommandBuffer commandBuffer, float minDepthBounds, float maxDepthBounds) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        skip_call |= addCmd(dev_data, pCB, CMD_SETDEPTHBOUNDSSTATE, "vkCmdSetDepthBounds()");
        pCB->status |= CBSTATUS_DEPTH_BOUNDS_SET;
    }
    lock.unlock();
    if (!skip_call)
        dev_data->dispatch_table.CmdSetDepthBounds(commandBuffer, minDepthBounds, maxDepthBounds);
}

VKAPI_ATTR void VKAPI_CALL
CmdSetStencilCompareMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t compareMask) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        skip_call |= addCmd(dev_data, pCB, CMD_SETSTENCILREADMASKSTATE, "vkCmdSetStencilCompareMask()");
        pCB->status |= CBSTATUS_STENCIL_READ_MASK_SET;
    }
    lock.unlock();
    if (!skip_call)
        dev_data->dispatch_table.CmdSetStencilCompareMask(commandBuffer, faceMask, compareMask);
}

VKAPI_ATTR void VKAPI_CALL
CmdSetStencilWriteMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t writeMask) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        skip_call |= addCmd(dev_data, pCB, CMD_SETSTENCILWRITEMASKSTATE, "vkCmdSetStencilWriteMask()");
        pCB->status |= CBSTATUS_STENCIL_WRITE_MASK_SET;
    }
    lock.unlock();
    if (!skip_call)
        dev_data->dispatch_table.CmdSetStencilWriteMask(commandBuffer, faceMask, writeMask);
}

VKAPI_ATTR void VKAPI_CALL
CmdSetStencilReference(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t reference) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        skip_call |= addCmd(dev_data, pCB, CMD_SETSTENCILREFERENCESTATE, "vkCmdSetStencilReference()");
        pCB->status |= CBSTATUS_STENCIL_REFERENCE_SET;
    }
    lock.unlock();
    if (!skip_call)
        dev_data->dispatch_table.CmdSetStencilReference(commandBuffer, faceMask, reference);
}

VKAPI_ATTR void VKAPI_CALL
CmdBindDescriptorSets(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout,
                      uint32_t firstSet, uint32_t setCount, const VkDescriptorSet *pDescriptorSets, uint32_t dynamicOffsetCount,
                      const uint32_t *pDynamicOffsets) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_RECORDING) {
            // Track total count of dynamic descriptor types to make sure we have an offset for each one
            uint32_t totalDynamicDescriptors = 0;
            string errorString = "";
            uint32_t lastSetIndex = firstSet + setCount - 1;
            if (lastSetIndex >= pCB->lastBound[pipelineBindPoint].boundDescriptorSets.size()) {
                pCB->lastBound[pipelineBindPoint].boundDescriptorSets.resize(lastSetIndex + 1);
                pCB->lastBound[pipelineBindPoint].dynamicOffsets.resize(lastSetIndex + 1);
            }
            auto oldFinalBoundSet = pCB->lastBound[pipelineBindPoint].boundDescriptorSets[lastSetIndex];
            auto pipeline_layout = getPipelineLayout(dev_data, layout);
            for (uint32_t i = 0; i < setCount; i++) {
                cvdescriptorset::DescriptorSet *pSet = getSetNode(dev_data, pDescriptorSets[i]);
                if (pSet) {
                    pCB->lastBound[pipelineBindPoint].pipeline_layout = *pipeline_layout;
                    pCB->lastBound[pipelineBindPoint].boundDescriptorSets[i + firstSet] = pSet;
                    skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT,
                                         VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, (uint64_t)pDescriptorSets[i], __LINE__,
                                         DRAWSTATE_NONE, "DS", "Descriptor Set 0x%" PRIxLEAST64 " bound on pipeline %s",
                                         (uint64_t)pDescriptorSets[i], string_VkPipelineBindPoint(pipelineBindPoint));
                    if (!pSet->IsUpdated() && (pSet->GetTotalDescriptorCount() != 0)) {
                        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT,
                                             VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, (uint64_t)pDescriptorSets[i], __LINE__,
                                             DRAWSTATE_DESCRIPTOR_SET_NOT_UPDATED, "DS",
                                             "Descriptor Set 0x%" PRIxLEAST64
                                             " bound but it was never updated. You may want to either update it or not bind it.",
                                             (uint64_t)pDescriptorSets[i]);
                    }
                    // Verify that set being bound is compatible with overlapping setLayout of pipelineLayout
                    if (!verify_set_layout_compatibility(dev_data, pSet, pipeline_layout, i + firstSet, errorString)) {
                        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                             VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, (uint64_t)pDescriptorSets[i], __LINE__,
                                             DRAWSTATE_PIPELINE_LAYOUTS_INCOMPATIBLE, "DS",
                                             "descriptorSet #%u being bound is not compatible with overlapping descriptorSetLayout "
                                             "at index %u of pipelineLayout 0x%" PRIxLEAST64 " due to: %s",
                                             i, i + firstSet, reinterpret_cast<uint64_t &>(layout), errorString.c_str());
                    }

                    auto setDynamicDescriptorCount = pSet->GetDynamicDescriptorCount();

                    pCB->lastBound[pipelineBindPoint].dynamicOffsets[firstSet + i].clear();

                    if (setDynamicDescriptorCount) {
                        // First make sure we won't overstep bounds of pDynamicOffsets array
                        if ((totalDynamicDescriptors + setDynamicDescriptorCount) > dynamicOffsetCount) {
                            skip_call |=
                                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                        VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, (uint64_t)pDescriptorSets[i], __LINE__,
                                        DRAWSTATE_INVALID_DYNAMIC_OFFSET_COUNT, "DS",
                                        "descriptorSet #%u (0x%" PRIxLEAST64
                                        ") requires %u dynamicOffsets, but only %u dynamicOffsets are left in pDynamicOffsets "
                                        "array. There must be one dynamic offset for each dynamic descriptor being bound.",
                                        i, (uint64_t)pDescriptorSets[i], pSet->GetDynamicDescriptorCount(),
                                        (dynamicOffsetCount - totalDynamicDescriptors));
                        } else { // Validate and store dynamic offsets with the set
                            // Validate Dynamic Offset Minimums
                            uint32_t cur_dyn_offset = totalDynamicDescriptors;
                            for (uint32_t d = 0; d < pSet->GetTotalDescriptorCount(); d++) {
                                if (pSet->GetTypeFromGlobalIndex(d) == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) {
                                    if (vk_safe_modulo(
                                            pDynamicOffsets[cur_dyn_offset],
                                            dev_data->phys_dev_properties.properties.limits.minUniformBufferOffsetAlignment) != 0) {
                                        skip_call |= log_msg(
                                            dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                            VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0, __LINE__,
                                            DRAWSTATE_INVALID_UNIFORM_BUFFER_OFFSET, "DS",
                                            "vkCmdBindDescriptorSets(): pDynamicOffsets[%d] is %d but must be a multiple of "
                                            "device limit minUniformBufferOffsetAlignment 0x%" PRIxLEAST64,
                                            cur_dyn_offset, pDynamicOffsets[cur_dyn_offset],
                                            dev_data->phys_dev_properties.properties.limits.minUniformBufferOffsetAlignment);
                                    }
                                    cur_dyn_offset++;
                                } else if (pSet->GetTypeFromGlobalIndex(d) == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC) {
                                    if (vk_safe_modulo(
                                            pDynamicOffsets[cur_dyn_offset],
                                            dev_data->phys_dev_properties.properties.limits.minStorageBufferOffsetAlignment) != 0) {
                                        skip_call |= log_msg(
                                            dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                            VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0, __LINE__,
                                            DRAWSTATE_INVALID_STORAGE_BUFFER_OFFSET, "DS",
                                            "vkCmdBindDescriptorSets(): pDynamicOffsets[%d] is %d but must be a multiple of "
                                            "device limit minStorageBufferOffsetAlignment 0x%" PRIxLEAST64,
                                            cur_dyn_offset, pDynamicOffsets[cur_dyn_offset],
                                            dev_data->phys_dev_properties.properties.limits.minStorageBufferOffsetAlignment);
                                    }
                                    cur_dyn_offset++;
                                }
                            }

                            pCB->lastBound[pipelineBindPoint].dynamicOffsets[firstSet + i] =
                                std::vector<uint32_t>(pDynamicOffsets + totalDynamicDescriptors,
                                                      pDynamicOffsets + totalDynamicDescriptors + setDynamicDescriptorCount);
                            // Keep running total of dynamic descriptor count to verify at the end
                            totalDynamicDescriptors += setDynamicDescriptorCount;

                        }
                    }
                } else {
                    skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, (uint64_t)pDescriptorSets[i], __LINE__,
                                         DRAWSTATE_INVALID_SET, "DS", "Attempt to bind descriptor set 0x%" PRIxLEAST64
                                         " that doesn't exist!",
                                         (uint64_t)pDescriptorSets[i]);
                }
                skip_call |= addCmd(dev_data, pCB, CMD_BINDDESCRIPTORSETS, "vkCmdBindDescriptorSets()");
                // For any previously bound sets, need to set them to "invalid" if they were disturbed by this update
                if (firstSet > 0) { // Check set #s below the first bound set
                    for (uint32_t i = 0; i < firstSet; ++i) {
                        if (pCB->lastBound[pipelineBindPoint].boundDescriptorSets[i] &&
                            !verify_set_layout_compatibility(dev_data, pCB->lastBound[pipelineBindPoint].boundDescriptorSets[i],
                                                             pipeline_layout, i, errorString)) {
                            skip_call |= log_msg(
                                dev_data->report_data, VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT,
                                VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT,
                                (uint64_t)pCB->lastBound[pipelineBindPoint].boundDescriptorSets[i], __LINE__, DRAWSTATE_NONE, "DS",
                                "DescriptorSet 0x%" PRIxLEAST64
                                " previously bound as set #%u was disturbed by newly bound pipelineLayout (0x%" PRIxLEAST64 ")",
                                (uint64_t)pCB->lastBound[pipelineBindPoint].boundDescriptorSets[i], i, (uint64_t)layout);
                            pCB->lastBound[pipelineBindPoint].boundDescriptorSets[i] = VK_NULL_HANDLE;
                        }
                    }
                }
                // Check if newly last bound set invalidates any remaining bound sets
                if ((pCB->lastBound[pipelineBindPoint].boundDescriptorSets.size() - 1) > (lastSetIndex)) {
                    if (oldFinalBoundSet &&
                        !verify_set_layout_compatibility(dev_data, oldFinalBoundSet, pipeline_layout, lastSetIndex, errorString)) {
                        auto old_set = oldFinalBoundSet->GetSet();
                        skip_call |=
                            log_msg(dev_data->report_data, VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT,
                                    VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, reinterpret_cast<uint64_t &>(old_set), __LINE__,
                                    DRAWSTATE_NONE, "DS", "DescriptorSet 0x%" PRIxLEAST64
                                                          " previously bound as set #%u is incompatible with set 0x%" PRIxLEAST64
                                                          " newly bound as set #%u so set #%u and any subsequent sets were "
                                                          "disturbed by newly bound pipelineLayout (0x%" PRIxLEAST64 ")",
                                    reinterpret_cast<uint64_t &>(old_set), lastSetIndex,
                                    (uint64_t)pCB->lastBound[pipelineBindPoint].boundDescriptorSets[lastSetIndex], lastSetIndex,
                                    lastSetIndex + 1, (uint64_t)layout);
                        pCB->lastBound[pipelineBindPoint].boundDescriptorSets.resize(lastSetIndex + 1);
                    }
                }
            }
            //  dynamicOffsetCount must equal the total number of dynamic descriptors in the sets being bound
            if (totalDynamicDescriptors != dynamicOffsetCount) {
                skip_call |=
                    log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            (uint64_t)commandBuffer, __LINE__, DRAWSTATE_INVALID_DYNAMIC_OFFSET_COUNT, "DS",
                            "Attempting to bind %u descriptorSets with %u dynamic descriptors, but dynamicOffsetCount "
                            "is %u. It should exactly match the number of dynamic descriptors.",
                            setCount, totalDynamicDescriptors, dynamicOffsetCount);
            }
        } else {
            skip_call |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdBindDescriptorSets()");
        }
    }
    lock.unlock();
    if (!skip_call)
        dev_data->dispatch_table.CmdBindDescriptorSets(commandBuffer, pipelineBindPoint, layout, firstSet, setCount,
                                                       pDescriptorSets, dynamicOffsetCount, pDynamicOffsets);
}

VKAPI_ATTR void VKAPI_CALL
CmdBindIndexBuffer(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    // TODO : Somewhere need to verify that IBs have correct usage state flagged
    std::unique_lock<std::mutex> lock(global_lock);

    auto buff_node = getBufferNode(dev_data, buffer);
    auto cb_node = getCBNode(dev_data, commandBuffer);
    if (cb_node && buff_node) {
        skip_call |= ValidateMemoryIsBoundToBuffer(dev_data, buff_node, "vkCmdBindIndexBuffer()");
        std::function<bool()> function = [=]() {
            return ValidateBufferMemoryIsValid(dev_data, buff_node, "vkCmdBindIndexBuffer()");
        };
        cb_node->validate_functions.push_back(function);
        skip_call |= addCmd(dev_data, cb_node, CMD_BINDINDEXBUFFER, "vkCmdBindIndexBuffer()");
        VkDeviceSize offset_align = 0;
        switch (indexType) {
        case VK_INDEX_TYPE_UINT16:
            offset_align = 2;
            break;
        case VK_INDEX_TYPE_UINT32:
            offset_align = 4;
            break;
        default:
            // ParamChecker should catch bad enum, we'll also throw alignment error below if offset_align stays 0
            break;
        }
        if (!offset_align || (offset % offset_align)) {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                 DRAWSTATE_VTX_INDEX_ALIGNMENT_ERROR, "DS",
                                 "vkCmdBindIndexBuffer() offset (0x%" PRIxLEAST64 ") does not fall on alignment (%s) boundary.",
                                 offset, string_VkIndexType(indexType));
        }
        cb_node->status |= CBSTATUS_INDEX_BUFFER_BOUND;
    } else {
        assert(0);
    }
    lock.unlock();
    if (!skip_call)
        dev_data->dispatch_table.CmdBindIndexBuffer(commandBuffer, buffer, offset, indexType);
}

void updateResourceTracking(GLOBAL_CB_NODE *pCB, uint32_t firstBinding, uint32_t bindingCount, const VkBuffer *pBuffers) {
    uint32_t end = firstBinding + bindingCount;
    if (pCB->currentDrawData.buffers.size() < end) {
        pCB->currentDrawData.buffers.resize(end);
    }
    for (uint32_t i = 0; i < bindingCount; ++i) {
        pCB->currentDrawData.buffers[i + firstBinding] = pBuffers[i];
    }
}

static inline void updateResourceTrackingOnDraw(GLOBAL_CB_NODE *pCB) { pCB->drawData.push_back(pCB->currentDrawData); }

VKAPI_ATTR void VKAPI_CALL CmdBindVertexBuffers(VkCommandBuffer commandBuffer, uint32_t firstBinding,
                                                uint32_t bindingCount, const VkBuffer *pBuffers,
                                                const VkDeviceSize *pOffsets) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    // TODO : Somewhere need to verify that VBs have correct usage state flagged
    std::unique_lock<std::mutex> lock(global_lock);

    auto cb_node = getCBNode(dev_data, commandBuffer);
    if (cb_node) {
        for (uint32_t i = 0; i < bindingCount; ++i) {
            auto buff_node = getBufferNode(dev_data, pBuffers[i]);
            assert(buff_node);
            skip_call |= ValidateMemoryIsBoundToBuffer(dev_data, buff_node, "vkCmdBindVertexBuffers()");
            std::function<bool()> function = [=]() {
                return ValidateBufferMemoryIsValid(dev_data, buff_node, "vkCmdBindVertexBuffers()");
            };
            cb_node->validate_functions.push_back(function);
        }
        addCmd(dev_data, cb_node, CMD_BINDVERTEXBUFFER, "vkCmdBindVertexBuffer()");
        updateResourceTracking(cb_node, firstBinding, bindingCount, pBuffers);
    } else {
        skip_call |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdBindVertexBuffer()");
    }
    lock.unlock();
    if (!skip_call)
        dev_data->dispatch_table.CmdBindVertexBuffers(commandBuffer, firstBinding, bindingCount, pBuffers, pOffsets);
}

/* expects global_lock to be held by caller */
static bool markStoreImagesAndBuffersAsWritten(layer_data *dev_data, GLOBAL_CB_NODE *pCB) {
    bool skip_call = false;

    for (auto imageView : pCB->updateImages) {
        auto view_state = getImageViewState(dev_data, imageView);
        if (!view_state)
            continue;

        auto image_state = getImageState(dev_data, view_state->create_info.image);
        assert(image_state);
        std::function<bool()> function = [=]() {
            SetImageMemoryValid(dev_data, image_state, true);
            return false;
        };
        pCB->validate_functions.push_back(function);
    }
    for (auto buffer : pCB->updateBuffers) {
        auto buff_node = getBufferNode(dev_data, buffer);
        assert(buff_node);
        std::function<bool()> function = [=]() {
            SetBufferMemoryValid(dev_data, buff_node, true);
            return false;
        };
        pCB->validate_functions.push_back(function);
    }
    return skip_call;
}

VKAPI_ATTR void VKAPI_CALL CmdDraw(VkCommandBuffer commandBuffer, uint32_t vertexCount, uint32_t instanceCount,
                                   uint32_t firstVertex, uint32_t firstInstance) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        skip_call |= addCmd(dev_data, pCB, CMD_DRAW, "vkCmdDraw()");
        pCB->drawCount[DRAW]++;
        skip_call |= validate_and_update_draw_state(dev_data, pCB, false, VK_PIPELINE_BIND_POINT_GRAPHICS, "vkCmdDraw");
        skip_call |= markStoreImagesAndBuffersAsWritten(dev_data, pCB);
        // TODO : Need to pass commandBuffer as srcObj here
        skip_call |=
            log_msg(dev_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, 0,
                    __LINE__, DRAWSTATE_NONE, "DS", "vkCmdDraw() call 0x%" PRIx64 ", reporting descriptor set state:",
                    g_drawCount[DRAW]++);
        skip_call |= synchAndPrintDSConfig(dev_data, commandBuffer);
        if (!skip_call) {
            updateResourceTrackingOnDraw(pCB);
        }
        skip_call |= outsideRenderPass(dev_data, pCB, "vkCmdDraw");
    }
    lock.unlock();
    if (!skip_call)
        dev_data->dispatch_table.CmdDraw(commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

VKAPI_ATTR void VKAPI_CALL CmdDrawIndexed(VkCommandBuffer commandBuffer, uint32_t indexCount,
                                          uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset,
                                                            uint32_t firstInstance) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    bool skip_call = false;
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        skip_call |= addCmd(dev_data, pCB, CMD_DRAWINDEXED, "vkCmdDrawIndexed()");
        pCB->drawCount[DRAW_INDEXED]++;
        skip_call |= validate_and_update_draw_state(dev_data, pCB, true, VK_PIPELINE_BIND_POINT_GRAPHICS, "vkCmdDrawIndexed");
        skip_call |= markStoreImagesAndBuffersAsWritten(dev_data, pCB);
        // TODO : Need to pass commandBuffer as srcObj here
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT,
                             VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, 0, __LINE__, DRAWSTATE_NONE, "DS",
                             "vkCmdDrawIndexed() call 0x%" PRIx64 ", reporting descriptor set state:",
                             g_drawCount[DRAW_INDEXED]++);
        skip_call |= synchAndPrintDSConfig(dev_data, commandBuffer);
        if (!skip_call) {
            updateResourceTrackingOnDraw(pCB);
        }
        skip_call |= outsideRenderPass(dev_data, pCB, "vkCmdDrawIndexed");
    }
    lock.unlock();
    if (!skip_call)
        dev_data->dispatch_table.CmdDrawIndexed(commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

VKAPI_ATTR void VKAPI_CALL
CmdDrawIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t count, uint32_t stride) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    bool skip_call = false;
    std::unique_lock<std::mutex> lock(global_lock);

    auto cb_node = getCBNode(dev_data, commandBuffer);
    auto buff_node = getBufferNode(dev_data, buffer);
    if (cb_node && buff_node) {
        skip_call |= ValidateMemoryIsBoundToBuffer(dev_data, buff_node, "vkCmdDrawIndirect()");
        AddCommandBufferBindingBuffer(dev_data, cb_node, buff_node);
        skip_call |= addCmd(dev_data, cb_node, CMD_DRAWINDIRECT, "vkCmdDrawIndirect()");
        cb_node->drawCount[DRAW_INDIRECT]++;
        skip_call |= validate_and_update_draw_state(dev_data, cb_node, false, VK_PIPELINE_BIND_POINT_GRAPHICS, "vkCmdDrawIndirect");
        skip_call |= markStoreImagesAndBuffersAsWritten(dev_data, cb_node);
        // TODO : Need to pass commandBuffer as srcObj here
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT,
                             VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, 0, __LINE__, DRAWSTATE_NONE, "DS",
                             "vkCmdDrawIndirect() call 0x%" PRIx64 ", reporting descriptor set state:",
                             g_drawCount[DRAW_INDIRECT]++);
        skip_call |= synchAndPrintDSConfig(dev_data, commandBuffer);
        if (!skip_call) {
            updateResourceTrackingOnDraw(cb_node);
        }
        skip_call |= outsideRenderPass(dev_data, cb_node, "vkCmdDrawIndirect()");
    } else {
        assert(0);
    }
    lock.unlock();
    if (!skip_call)
        dev_data->dispatch_table.CmdDrawIndirect(commandBuffer, buffer, offset, count, stride);
}

VKAPI_ATTR void VKAPI_CALL
CmdDrawIndexedIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t count, uint32_t stride) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);

    auto cb_node = getCBNode(dev_data, commandBuffer);
    auto buff_node = getBufferNode(dev_data, buffer);
    if (cb_node && buff_node) {
        skip_call |= ValidateMemoryIsBoundToBuffer(dev_data, buff_node, "vkCmdDrawIndexedIndirect()");
        AddCommandBufferBindingBuffer(dev_data, cb_node, buff_node);
        skip_call |= addCmd(dev_data, cb_node, CMD_DRAWINDEXEDINDIRECT, "vkCmdDrawIndexedIndirect()");
        cb_node->drawCount[DRAW_INDEXED_INDIRECT]++;
        skip_call |=
            validate_and_update_draw_state(dev_data, cb_node, true, VK_PIPELINE_BIND_POINT_GRAPHICS, "vkCmdDrawIndexedIndirect");
        skip_call |= markStoreImagesAndBuffersAsWritten(dev_data, cb_node);
        // TODO : Need to pass commandBuffer as srcObj here
        skip_call |=
            log_msg(dev_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, 0,
                    __LINE__, DRAWSTATE_NONE, "DS", "vkCmdDrawIndexedIndirect() call 0x%" PRIx64 ", reporting descriptor set state:",
                    g_drawCount[DRAW_INDEXED_INDIRECT]++);
        skip_call |= synchAndPrintDSConfig(dev_data, commandBuffer);
        if (!skip_call) {
            updateResourceTrackingOnDraw(cb_node);
        }
        skip_call |= outsideRenderPass(dev_data, cb_node, "vkCmdDrawIndexedIndirect()");
    } else {
        assert(0);
    }
    lock.unlock();
    if (!skip_call)
        dev_data->dispatch_table.CmdDrawIndexedIndirect(commandBuffer, buffer, offset, count, stride);
}

VKAPI_ATTR void VKAPI_CALL CmdDispatch(VkCommandBuffer commandBuffer, uint32_t x, uint32_t y, uint32_t z) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        skip_call |= validate_and_update_draw_state(dev_data, pCB, false, VK_PIPELINE_BIND_POINT_COMPUTE, "vkCmdDispatch");
        skip_call |= markStoreImagesAndBuffersAsWritten(dev_data, pCB);
        skip_call |= addCmd(dev_data, pCB, CMD_DISPATCH, "vkCmdDispatch()");
        skip_call |= insideRenderPass(dev_data, pCB, "vkCmdDispatch");
    }
    lock.unlock();
    if (!skip_call)
        dev_data->dispatch_table.CmdDispatch(commandBuffer, x, y, z);
}

VKAPI_ATTR void VKAPI_CALL
CmdDispatchIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);

    auto cb_node = getCBNode(dev_data, commandBuffer);
    auto buff_node = getBufferNode(dev_data, buffer);
    if (cb_node && buff_node) {
        skip_call |= ValidateMemoryIsBoundToBuffer(dev_data, buff_node, "vkCmdDispatchIndirect()");
        AddCommandBufferBindingBuffer(dev_data, cb_node, buff_node);
        skip_call |=
            validate_and_update_draw_state(dev_data, cb_node, false, VK_PIPELINE_BIND_POINT_COMPUTE, "vkCmdDispatchIndirect");
        skip_call |= markStoreImagesAndBuffersAsWritten(dev_data, cb_node);
        skip_call |= addCmd(dev_data, cb_node, CMD_DISPATCHINDIRECT, "vkCmdDispatchIndirect()");
        skip_call |= insideRenderPass(dev_data, cb_node, "vkCmdDispatchIndirect()");
    }
    lock.unlock();
    if (!skip_call)
        dev_data->dispatch_table.CmdDispatchIndirect(commandBuffer, buffer, offset);
}

VKAPI_ATTR void VKAPI_CALL CmdCopyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer,
                                         uint32_t regionCount, const VkBufferCopy *pRegions) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);

    auto cb_node = getCBNode(dev_data, commandBuffer);
    auto src_buff_node = getBufferNode(dev_data, srcBuffer);
    auto dst_buff_node = getBufferNode(dev_data, dstBuffer);
    if (cb_node && src_buff_node && dst_buff_node) {
        skip_call |= ValidateMemoryIsBoundToBuffer(dev_data, src_buff_node, "vkCmdCopyBuffer()");
        skip_call |= ValidateMemoryIsBoundToBuffer(dev_data, dst_buff_node, "vkCmdCopyBuffer()");
        // Update bindings between buffers and cmd buffer
        AddCommandBufferBindingBuffer(dev_data, cb_node, src_buff_node);
        AddCommandBufferBindingBuffer(dev_data, cb_node, dst_buff_node);
        // Validate that SRC & DST buffers have correct usage flags set
        skip_call |= ValidateBufferUsageFlags(dev_data, src_buff_node, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true, "vkCmdCopyBuffer()",
                                              "VK_BUFFER_USAGE_TRANSFER_SRC_BIT");
        skip_call |= ValidateBufferUsageFlags(dev_data, dst_buff_node, VK_BUFFER_USAGE_TRANSFER_DST_BIT, true, "vkCmdCopyBuffer()",
                                              "VK_BUFFER_USAGE_TRANSFER_DST_BIT");

        std::function<bool()> function = [=]() {
            return ValidateBufferMemoryIsValid(dev_data, src_buff_node, "vkCmdCopyBuffer()");
        };
        cb_node->validate_functions.push_back(function);
        function = [=]() {
            SetBufferMemoryValid(dev_data, dst_buff_node, true);
            return false;
        };
        cb_node->validate_functions.push_back(function);

        skip_call |= addCmd(dev_data, cb_node, CMD_COPYBUFFER, "vkCmdCopyBuffer()");
        skip_call |= insideRenderPass(dev_data, cb_node, "vkCmdCopyBuffer()");
    } else {
        // Param_checker will flag errors on invalid objects, just assert here as debugging aid
        assert(0);
    }
    lock.unlock();
    if (!skip_call)
        dev_data->dispatch_table.CmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, regionCount, pRegions);
}

static bool VerifySourceImageLayout(layer_data *dev_data, GLOBAL_CB_NODE *cb_node, VkImage srcImage,
                                    VkImageSubresourceLayers subLayers, VkImageLayout srcImageLayout) {
    bool skip_call = false;

    for (uint32_t i = 0; i < subLayers.layerCount; ++i) {
        uint32_t layer = i + subLayers.baseArrayLayer;
        VkImageSubresource sub = {subLayers.aspectMask, subLayers.mipLevel, layer};
        IMAGE_CMD_BUF_LAYOUT_NODE node;
        if (!FindLayout(cb_node, srcImage, sub, node)) {
            SetLayout(cb_node, srcImage, sub, IMAGE_CMD_BUF_LAYOUT_NODE(srcImageLayout, srcImageLayout));
            continue;
        }
        if (node.layout != srcImageLayout) {
            // TODO: Improve log message in the next pass
            skip_call |=
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, 0,
                        __LINE__, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS", "Cannot copy from an image whose source layout is %s "
                                                                        "and doesn't match the current layout %s.",
                        string_VkImageLayout(srcImageLayout), string_VkImageLayout(node.layout));
        }
    }
    if (srcImageLayout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
        if (srcImageLayout == VK_IMAGE_LAYOUT_GENERAL) {
            // TODO : Can we deal with image node from the top of call tree and avoid map look-up here?
            auto image_state = getImageState(dev_data, srcImage);
            if (image_state->createInfo.tiling != VK_IMAGE_TILING_LINEAR) {
                // LAYOUT_GENERAL is allowed, but may not be performance optimal, flag as perf warning.
                skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT,
                                     (VkDebugReportObjectTypeEXT)0, 0, __LINE__, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                     "Layout for input image should be TRANSFER_SRC_OPTIMAL instead of GENERAL.");
            }
        } else {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                 DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS", "Layout for input image is %s but can only be "
                                                                       "TRANSFER_SRC_OPTIMAL or GENERAL.",
                                 string_VkImageLayout(srcImageLayout));
        }
    }
    return skip_call;
}

static bool VerifyDestImageLayout(layer_data *dev_data, GLOBAL_CB_NODE *cb_node, VkImage destImage,
                                  VkImageSubresourceLayers subLayers, VkImageLayout destImageLayout) {
    bool skip_call = false;

    for (uint32_t i = 0; i < subLayers.layerCount; ++i) {
        uint32_t layer = i + subLayers.baseArrayLayer;
        VkImageSubresource sub = {subLayers.aspectMask, subLayers.mipLevel, layer};
        IMAGE_CMD_BUF_LAYOUT_NODE node;
        if (!FindLayout(cb_node, destImage, sub, node)) {
            SetLayout(cb_node, destImage, sub, IMAGE_CMD_BUF_LAYOUT_NODE(destImageLayout, destImageLayout));
            continue;
        }
        if (node.layout != destImageLayout) {
            skip_call |=
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, 0,
                        __LINE__, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS", "Cannot copy from an image whose dest layout is %s and "
                                                                        "doesn't match the current layout %s.",
                        string_VkImageLayout(destImageLayout), string_VkImageLayout(node.layout));
        }
    }
    if (destImageLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        if (destImageLayout == VK_IMAGE_LAYOUT_GENERAL) {
            auto image_state = getImageState(dev_data, destImage);
            if (image_state->createInfo.tiling != VK_IMAGE_TILING_LINEAR) {
                // LAYOUT_GENERAL is allowed, but may not be performance optimal, flag as perf warning.
                skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT,
                                     (VkDebugReportObjectTypeEXT)0, 0, __LINE__, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                     "Layout for output image should be TRANSFER_DST_OPTIMAL instead of GENERAL.");
            }
        } else {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                 DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS", "Layout for output image is %s but can only be "
                                                                       "TRANSFER_DST_OPTIMAL or GENERAL.",
                                 string_VkImageLayout(destImageLayout));
        }
    }
    return skip_call;
}

// Test if two VkExtent3D structs are equivalent
static inline bool IsExtentEqual(const VkExtent3D *extent, const VkExtent3D *other_extent) {
    bool result = true;
    if ((extent->width != other_extent->width) || (extent->height != other_extent->height) ||
        (extent->depth != other_extent->depth)) {
        result = false;
    }
    return result;
}

// Returns the image extent of a specific subresource.
static inline VkExtent3D GetImageSubresourceExtent(const IMAGE_STATE *img, const VkImageSubresourceLayers *subresource) {
    const uint32_t mip = subresource->mipLevel;
    VkExtent3D extent = img->createInfo.extent;
    extent.width = std::max(1U, extent.width >> mip);
    extent.height = std::max(1U, extent.height >> mip);
    extent.depth = std::max(1U, extent.depth >> mip);
    return extent;
}

// Test if the extent argument has all dimensions set to 0.
static inline bool IsExtentZero(const VkExtent3D *extent) {
    return ((extent->width == 0) && (extent->height == 0) && (extent->depth == 0));
}

// Returns the image transfer granularity for a specific image scaled by compressed block size if necessary.
static inline VkExtent3D GetScaledItg(layer_data *dev_data, const GLOBAL_CB_NODE *cb_node, const IMAGE_STATE *img) {
    // Default to (0, 0, 0) granularity in case we can't find the real granularity for the physical device.
    VkExtent3D granularity = { 0, 0, 0 };
    auto pPool = getCommandPoolNode(dev_data, cb_node->createInfo.commandPool);
    if (pPool) {
        granularity = dev_data->phys_dev_properties.queue_family_properties[pPool->queueFamilyIndex].minImageTransferGranularity;
        if (vk_format_is_compressed(img->createInfo.format)) {
            auto block_size = vk_format_compressed_block_size(img->createInfo.format);
            granularity.width *= block_size.width;
            granularity.height *= block_size.height;
        }
    }
    return granularity;
}

// Test elements of a VkExtent3D structure against alignment constraints contained in another VkExtent3D structure
static inline bool IsExtentAligned(const VkExtent3D *extent, const VkExtent3D *granularity) {
    bool valid = true;
    if ((vk_safe_modulo(extent->depth, granularity->depth) != 0) || (vk_safe_modulo(extent->width, granularity->width) != 0) ||
        (vk_safe_modulo(extent->height, granularity->height) != 0)) {
        valid = false;
    }
    return valid;
}

// Check elements of a VkOffset3D structure against a queue family's Image Transfer Granularity values
static inline bool CheckItgOffset(layer_data *dev_data, const GLOBAL_CB_NODE *cb_node, const VkOffset3D *offset,
                                  const VkExtent3D *granularity, const uint32_t i, const char *function, const char *member) {
    bool skip = false;
    VkExtent3D offset_extent = {};
    offset_extent.width = static_cast<uint32_t>(abs(offset->x));
    offset_extent.height = static_cast<uint32_t>(abs(offset->y));
    offset_extent.depth = static_cast<uint32_t>(abs(offset->z));
    if (IsExtentZero(granularity)) {
        // If the queue family image transfer granularity is (0, 0, 0), then the offset must always be (0, 0, 0)
        if (IsExtentZero(&offset_extent) == false) {
            skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                            DRAWSTATE_IMAGE_TRANSFER_GRANULARITY, "DS",
                            "%s: pRegion[%d].%s (x=%d, y=%d, z=%d) must be (x=0, y=0, z=0) "
                            "when the command buffer's queue family image transfer granularity is (w=0, h=0, d=0).",
                            function, i, member, offset->x, offset->y, offset->z);
        }
    } else {
        // If the queue family image transfer granularity is not (0, 0, 0), then the offset dimensions must always be even
        // integer multiples of the image transfer granularity.
        if (IsExtentAligned(&offset_extent, granularity) == false) {
            skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                            DRAWSTATE_IMAGE_TRANSFER_GRANULARITY, "DS",
                            "%s: pRegion[%d].%s (x=%d, y=%d, z=%d) dimensions must be even integer "
                            "multiples of this command buffer's queue family image transfer granularity (w=%d, h=%d, d=%d).",
                            function, i, member, offset->x, offset->y, offset->z, granularity->width, granularity->height,
                            granularity->depth);
        }
    }
    return skip;
}

// Check elements of a VkExtent3D structure against a queue family's Image Transfer Granularity values
static inline bool CheckItgExtent(layer_data *dev_data, const GLOBAL_CB_NODE *cb_node, const VkExtent3D *extent,
                                  const VkOffset3D *offset, const VkExtent3D *granularity, const VkExtent3D *subresource_extent,
                                  const uint32_t i, const char *function, const char *member) {
    bool skip = false;
    if (IsExtentZero(granularity)) {
        // If the queue family image transfer granularity is (0, 0, 0), then the extent must always match the image
        // subresource extent.
        if (IsExtentEqual(extent, subresource_extent) == false) {
            skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                            DRAWSTATE_IMAGE_TRANSFER_GRANULARITY, "DS",
                            "%s: pRegion[%d].%s (w=%d, h=%d, d=%d) must match the image subresource extents (w=%d, h=%d, d=%d) "
                            "when the command buffer's queue family image transfer granularity is (w=0, h=0, d=0).",
                            function, i, member, extent->width, extent->height, extent->depth, subresource_extent->width,
                            subresource_extent->height, subresource_extent->depth);
        }
    } else {
        // If the queue family image transfer granularity is not (0, 0, 0), then the extent dimensions must always be even
        // integer multiples of the image transfer granularity or the offset + extent dimensions must always match the image
        // subresource extent dimensions.
        VkExtent3D offset_extent_sum = {};
        offset_extent_sum.width = static_cast<uint32_t>(abs(offset->x)) + extent->width;
        offset_extent_sum.height = static_cast<uint32_t>(abs(offset->y)) + extent->height;
        offset_extent_sum.depth = static_cast<uint32_t>(abs(offset->z)) + extent->depth;
        if ((IsExtentAligned(extent, granularity) == false) && (IsExtentEqual(&offset_extent_sum, subresource_extent) == false)) {
            skip |=
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                        DRAWSTATE_IMAGE_TRANSFER_GRANULARITY, "DS",
                        "%s: pRegion[%d].%s (w=%d, h=%d, d=%d) dimensions must be even integer multiples of this command buffer's "
                        "queue family image transfer granularity (w=%d, h=%d, d=%d) or offset (x=%d, y=%d, z=%d) + "
                        "extent (w=%d, h=%d, d=%d) must match the image subresource extents (w=%d, h=%d, d=%d).",
                        function, i, member, extent->width, extent->height, extent->depth, granularity->width, granularity->height,
                        granularity->depth, offset->x, offset->y, offset->z, extent->width, extent->height, extent->depth,
                        subresource_extent->width, subresource_extent->height, subresource_extent->depth);
        }
    }
    return skip;
}

// Check a uint32_t width or stride value against a queue family's Image Transfer Granularity width value
static inline bool CheckItgInt(layer_data *dev_data, const GLOBAL_CB_NODE *cb_node, const uint32_t value,
                               const uint32_t granularity, const uint32_t i, const char *function, const char *member) {
    bool skip = false;
    if (vk_safe_modulo(value, granularity) != 0) {
        skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                        DRAWSTATE_IMAGE_TRANSFER_GRANULARITY, "DS",
                        "%s: pRegion[%d].%s (%d) must be an even integer multiple of this command buffer's queue family image "
                        "transfer granularity width (%d).",
                        function, i, member, value, granularity);
    }
    return skip;
}

// Check a VkDeviceSize value against a queue family's Image Transfer Granularity width value
static inline bool CheckItgSize(layer_data *dev_data, const GLOBAL_CB_NODE *cb_node, const VkDeviceSize value,
                                const uint32_t granularity, const uint32_t i, const char *function, const char *member) {
    bool skip = false;
    if (vk_safe_modulo(value, granularity) != 0) {
        skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                        DRAWSTATE_IMAGE_TRANSFER_GRANULARITY, "DS",
                        "%s: pRegion[%d].%s (%" PRIdLEAST64
                        ") must be an even integer multiple of this command buffer's queue family image transfer "
                        "granularity width (%d).",
                        function, i, member, value, granularity);
    }
    return skip;
}

// Check valid usage Image Tranfer Granularity requirements for elements of a VkImageCopy structure
static inline bool ValidateCopyImageTransferGranularityRequirements(layer_data *dev_data, const GLOBAL_CB_NODE *cb_node,
                                                                    const IMAGE_STATE *img, const VkImageCopy *region,
                                                                    const uint32_t i, const char *function) {
    bool skip = false;
    VkExtent3D granularity = GetScaledItg(dev_data, cb_node, img);
    skip |= CheckItgOffset(dev_data, cb_node, &region->srcOffset, &granularity, i, function, "srcOffset");
    skip |= CheckItgOffset(dev_data, cb_node, &region->dstOffset, &granularity, i, function, "dstOffset");
    VkExtent3D subresource_extent = GetImageSubresourceExtent(img, &region->dstSubresource);
    skip |= CheckItgExtent(dev_data, cb_node, &region->extent, &region->dstOffset, &granularity, &subresource_extent, i, function,
                           "extent");
    return skip;
}

// Check valid usage Image Tranfer Granularity requirements for elements of a VkBufferImageCopy structure
static inline bool ValidateCopyBufferImageTransferGranularityRequirements(layer_data *dev_data, const GLOBAL_CB_NODE *cb_node,
                                                                          const IMAGE_STATE *img, const VkBufferImageCopy *region,
                                                                          const uint32_t i, const char *function) {
    bool skip = false;
    VkExtent3D granularity = GetScaledItg(dev_data, cb_node, img);
    skip |= CheckItgSize(dev_data, cb_node, region->bufferOffset, granularity.width, i, function, "bufferOffset");
    skip |= CheckItgInt(dev_data, cb_node, region->bufferRowLength, granularity.width, i, function, "bufferRowLength");
    skip |= CheckItgInt(dev_data, cb_node, region->bufferImageHeight, granularity.width, i, function, "bufferImageHeight");
    skip |= CheckItgOffset(dev_data, cb_node, &region->imageOffset, &granularity, i, function, "imageOffset");
    VkExtent3D subresource_extent = GetImageSubresourceExtent(img, &region->imageSubresource);
    skip |= CheckItgExtent(dev_data, cb_node, &region->imageExtent, &region->imageOffset, &granularity, &subresource_extent, i,
                           function, "imageExtent");
    return skip;
}

VKAPI_ATTR void VKAPI_CALL
CmdCopyImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage,
             VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageCopy *pRegions) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);

    auto cb_node = getCBNode(dev_data, commandBuffer);
    auto src_image_state = getImageState(dev_data, srcImage);
    auto dst_image_state = getImageState(dev_data, dstImage);
    if (cb_node && src_image_state && dst_image_state) {
        skip_call |= ValidateMemoryIsBoundToImage(dev_data, src_image_state, "vkCmdCopyImage()");
        skip_call |= ValidateMemoryIsBoundToImage(dev_data, dst_image_state, "vkCmdCopyImage()");
        // Update bindings between images and cmd buffer
        AddCommandBufferBindingImage(dev_data, cb_node, src_image_state);
        AddCommandBufferBindingImage(dev_data, cb_node, dst_image_state);
        // Validate that SRC & DST images have correct usage flags set
        skip_call |= ValidateImageUsageFlags(dev_data, src_image_state, VK_IMAGE_USAGE_TRANSFER_SRC_BIT, true, "vkCmdCopyImage()",
                                             "VK_IMAGE_USAGE_TRANSFER_SRC_BIT");
        skip_call |= ValidateImageUsageFlags(dev_data, dst_image_state, VK_IMAGE_USAGE_TRANSFER_DST_BIT, true, "vkCmdCopyImage()",
                                             "VK_IMAGE_USAGE_TRANSFER_DST_BIT");
        std::function<bool()> function = [=]() {
            return ValidateImageMemoryIsValid(dev_data, src_image_state, "vkCmdCopyImage()");
        };
        cb_node->validate_functions.push_back(function);
        function = [=]() {
            SetImageMemoryValid(dev_data, dst_image_state, true);
            return false;
        };
        cb_node->validate_functions.push_back(function);

        skip_call |= addCmd(dev_data, cb_node, CMD_COPYIMAGE, "vkCmdCopyImage()");
        skip_call |= insideRenderPass(dev_data, cb_node, "vkCmdCopyImage()");
        for (uint32_t i = 0; i < regionCount; ++i) {
            skip_call |= VerifySourceImageLayout(dev_data, cb_node, srcImage, pRegions[i].srcSubresource, srcImageLayout);
            skip_call |= VerifyDestImageLayout(dev_data, cb_node, dstImage, pRegions[i].dstSubresource, dstImageLayout);
            skip_call |= ValidateCopyImageTransferGranularityRequirements(dev_data, cb_node, dst_image_state, &pRegions[i], i,
                                                                          "vkCmdCopyImage()");
        }
    } else {
        assert(0);
    }
    lock.unlock();
    if (!skip_call)
        dev_data->dispatch_table.CmdCopyImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount,
                                              pRegions);
}

// Validate that an image's sampleCount matches the requirement for a specific API call
static inline bool ValidateImageSampleCount(layer_data *dev_data, IMAGE_STATE *image_state, VkSampleCountFlagBits sample_count,
                                            const char *location) {
    bool skip = false;
    if (image_state->createInfo.samples != sample_count) {
        skip = log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                       reinterpret_cast<uint64_t &>(image_state->image), 0, DRAWSTATE_NUM_SAMPLES_MISMATCH, "DS",
                       "%s for image 0x%" PRIxLEAST64 " was created with a sample count of %s but must be %s.", location,
                       reinterpret_cast<uint64_t &>(image_state->image),
                       string_VkSampleCountFlagBits(image_state->createInfo.samples), string_VkSampleCountFlagBits(sample_count));
    }
    return skip;
}

VKAPI_ATTR void VKAPI_CALL
CmdBlitImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage,
             VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageBlit *pRegions, VkFilter filter) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);

    auto cb_node = getCBNode(dev_data, commandBuffer);
    auto src_image_state = getImageState(dev_data, srcImage);
    auto dst_image_state = getImageState(dev_data, dstImage);
    if (cb_node && src_image_state && dst_image_state) {
        skip_call |= ValidateImageSampleCount(dev_data, src_image_state, VK_SAMPLE_COUNT_1_BIT, "vkCmdBlitImage(): srcImage");
        skip_call |= ValidateImageSampleCount(dev_data, dst_image_state, VK_SAMPLE_COUNT_1_BIT, "vkCmdBlitImage(): dstImage");
        skip_call |= ValidateMemoryIsBoundToImage(dev_data, src_image_state, "vkCmdBlitImage()");
        skip_call |= ValidateMemoryIsBoundToImage(dev_data, dst_image_state, "vkCmdBlitImage()");
        // Update bindings between images and cmd buffer
        AddCommandBufferBindingImage(dev_data, cb_node, src_image_state);
        AddCommandBufferBindingImage(dev_data, cb_node, dst_image_state);
        // Validate that SRC & DST images have correct usage flags set
        skip_call |= ValidateImageUsageFlags(dev_data, src_image_state, VK_IMAGE_USAGE_TRANSFER_SRC_BIT, true, "vkCmdBlitImage()",
                                             "VK_IMAGE_USAGE_TRANSFER_SRC_BIT");
        skip_call |= ValidateImageUsageFlags(dev_data, dst_image_state, VK_IMAGE_USAGE_TRANSFER_DST_BIT, true, "vkCmdBlitImage()",
                                             "VK_IMAGE_USAGE_TRANSFER_DST_BIT");
        std::function<bool()> function = [=]() {
            return ValidateImageMemoryIsValid(dev_data, src_image_state, "vkCmdBlitImage()");
        };
        cb_node->validate_functions.push_back(function);
        function = [=]() {
            SetImageMemoryValid(dev_data, dst_image_state, true);
            return false;
        };
        cb_node->validate_functions.push_back(function);

        skip_call |= addCmd(dev_data, cb_node, CMD_BLITIMAGE, "vkCmdBlitImage()");
        skip_call |= insideRenderPass(dev_data, cb_node, "vkCmdBlitImage()");
    } else {
        assert(0);
    }
    lock.unlock();
    if (!skip_call)
        dev_data->dispatch_table.CmdBlitImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount,
                                              pRegions, filter);
}

VKAPI_ATTR void VKAPI_CALL CmdCopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer srcBuffer,
                                                VkImage dstImage, VkImageLayout dstImageLayout,
                                                uint32_t regionCount, const VkBufferImageCopy *pRegions) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);

    auto cb_node = getCBNode(dev_data, commandBuffer);
    auto src_buff_node = getBufferNode(dev_data, srcBuffer);
    auto dst_image_state = getImageState(dev_data, dstImage);
    if (cb_node && src_buff_node && dst_image_state) {
        skip_call |=
            ValidateImageSampleCount(dev_data, dst_image_state, VK_SAMPLE_COUNT_1_BIT, "vkCmdCopyBufferToImage(): dstImage");
        skip_call |= ValidateMemoryIsBoundToBuffer(dev_data, src_buff_node, "vkCmdCopyBufferToImage()");
        skip_call |= ValidateMemoryIsBoundToImage(dev_data, dst_image_state, "vkCmdCopyBufferToImage()");
        AddCommandBufferBindingBuffer(dev_data, cb_node, src_buff_node);
        AddCommandBufferBindingImage(dev_data, cb_node, dst_image_state);
        skip_call |= ValidateBufferUsageFlags(dev_data, src_buff_node, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true,
                                              "vkCmdCopyBufferToImage()", "VK_BUFFER_USAGE_TRANSFER_SRC_BIT");
        skip_call |= ValidateImageUsageFlags(dev_data, dst_image_state, VK_IMAGE_USAGE_TRANSFER_DST_BIT, true,
                                             "vkCmdCopyBufferToImage()", "VK_IMAGE_USAGE_TRANSFER_DST_BIT");
        std::function<bool()> function = [=]() {
            SetImageMemoryValid(dev_data, dst_image_state, true);
            return false;
        };
        cb_node->validate_functions.push_back(function);
        function = [=]() { return ValidateBufferMemoryIsValid(dev_data, src_buff_node, "vkCmdCopyBufferToImage()"); };
        cb_node->validate_functions.push_back(function);

        skip_call |= addCmd(dev_data, cb_node, CMD_COPYBUFFERTOIMAGE, "vkCmdCopyBufferToImage()");
        skip_call |= insideRenderPass(dev_data, cb_node, "vkCmdCopyBufferToImage()");
        for (uint32_t i = 0; i < regionCount; ++i) {
            skip_call |= VerifyDestImageLayout(dev_data, cb_node, dstImage, pRegions[i].imageSubresource, dstImageLayout);
            skip_call |= ValidateCopyBufferImageTransferGranularityRequirements(dev_data, cb_node, dst_image_state, &pRegions[i], i,
                                                                                "vkCmdCopyBufferToImage()");
        }
    } else {
        assert(0);
    }
    lock.unlock();
    if (!skip_call)
        dev_data->dispatch_table.CmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, dstImageLayout, regionCount, pRegions);
}

VKAPI_ATTR void VKAPI_CALL CmdCopyImageToBuffer(VkCommandBuffer commandBuffer, VkImage srcImage,
                                                VkImageLayout srcImageLayout, VkBuffer dstBuffer,
                                                uint32_t regionCount, const VkBufferImageCopy *pRegions) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);

    auto cb_node = getCBNode(dev_data, commandBuffer);
    auto src_image_state = getImageState(dev_data, srcImage);
    auto dst_buff_node = getBufferNode(dev_data, dstBuffer);
    if (cb_node && src_image_state && dst_buff_node) {
        skip_call |=
            ValidateImageSampleCount(dev_data, src_image_state, VK_SAMPLE_COUNT_1_BIT, "vkCmdCopyImageToBuffer(): srcImage");
        skip_call |= ValidateMemoryIsBoundToImage(dev_data, src_image_state, "vkCmdCopyImageToBuffer()");
        skip_call |= ValidateMemoryIsBoundToBuffer(dev_data, dst_buff_node, "vkCmdCopyImageToBuffer()");
        // Update bindings between buffer/image and cmd buffer
        AddCommandBufferBindingImage(dev_data, cb_node, src_image_state);
        AddCommandBufferBindingBuffer(dev_data, cb_node, dst_buff_node);
        // Validate that SRC image & DST buffer have correct usage flags set
        skip_call |= ValidateImageUsageFlags(dev_data, src_image_state, VK_IMAGE_USAGE_TRANSFER_SRC_BIT, true,
                                             "vkCmdCopyImageToBuffer()", "VK_IMAGE_USAGE_TRANSFER_SRC_BIT");
        skip_call |= ValidateBufferUsageFlags(dev_data, dst_buff_node, VK_BUFFER_USAGE_TRANSFER_DST_BIT, true,
                                              "vkCmdCopyImageToBuffer()", "VK_BUFFER_USAGE_TRANSFER_DST_BIT");
        std::function<bool()> function = [=]() {
            return ValidateImageMemoryIsValid(dev_data, src_image_state, "vkCmdCopyImageToBuffer()");
        };
        cb_node->validate_functions.push_back(function);
        function = [=]() {
            SetBufferMemoryValid(dev_data, dst_buff_node, true);
            return false;
        };
        cb_node->validate_functions.push_back(function);

        skip_call |= addCmd(dev_data, cb_node, CMD_COPYIMAGETOBUFFER, "vkCmdCopyImageToBuffer()");
        skip_call |= insideRenderPass(dev_data, cb_node, "vkCmdCopyImageToBuffer()");
        for (uint32_t i = 0; i < regionCount; ++i) {
            skip_call |= VerifySourceImageLayout(dev_data, cb_node, srcImage, pRegions[i].imageSubresource, srcImageLayout);
            skip_call |= ValidateCopyBufferImageTransferGranularityRequirements(dev_data, cb_node, src_image_state, &pRegions[i], i,
                                                                                "CmdCopyImageToBuffer");
        }
    } else {
        assert(0);
    }
    lock.unlock();
    if (!skip_call)
        dev_data->dispatch_table.CmdCopyImageToBuffer(commandBuffer, srcImage, srcImageLayout, dstBuffer, regionCount, pRegions);
}

VKAPI_ATTR void VKAPI_CALL CmdUpdateBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer,
                                           VkDeviceSize dstOffset, VkDeviceSize dataSize, const uint32_t *pData) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);

    auto cb_node = getCBNode(dev_data, commandBuffer);
    auto dst_buff_node = getBufferNode(dev_data, dstBuffer);
    if (cb_node && dst_buff_node) {
        skip_call |= ValidateMemoryIsBoundToBuffer(dev_data, dst_buff_node, "vkCmdUpdateBuffer()");
        // Update bindings between buffer and cmd buffer
        AddCommandBufferBindingBuffer(dev_data, cb_node, dst_buff_node);
        // Validate that DST buffer has correct usage flags set
        skip_call |= ValidateBufferUsageFlags(dev_data, dst_buff_node, VK_BUFFER_USAGE_TRANSFER_DST_BIT, true,
                                              "vkCmdUpdateBuffer()", "VK_BUFFER_USAGE_TRANSFER_DST_BIT");
        std::function<bool()> function = [=]() {
            SetBufferMemoryValid(dev_data, dst_buff_node, true);
            return false;
        };
        cb_node->validate_functions.push_back(function);

        skip_call |= addCmd(dev_data, cb_node, CMD_UPDATEBUFFER, "vkCmdUpdateBuffer()");
        skip_call |= insideRenderPass(dev_data, cb_node, "vkCmdCopyUpdateBuffer()");
    } else {
        assert(0);
    }
    lock.unlock();
    if (!skip_call)
        dev_data->dispatch_table.CmdUpdateBuffer(commandBuffer, dstBuffer, dstOffset, dataSize, pData);
}

VKAPI_ATTR void VKAPI_CALL
CmdFillBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size, uint32_t data) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);

    auto cb_node = getCBNode(dev_data, commandBuffer);
    auto dst_buff_node = getBufferNode(dev_data, dstBuffer);
    if (cb_node && dst_buff_node) {
        skip_call |= ValidateMemoryIsBoundToBuffer(dev_data, dst_buff_node, "vkCmdFillBuffer()");
        // Update bindings between buffer and cmd buffer
        AddCommandBufferBindingBuffer(dev_data, cb_node, dst_buff_node);
        // Validate that DST buffer has correct usage flags set
        skip_call |= ValidateBufferUsageFlags(dev_data, dst_buff_node, VK_BUFFER_USAGE_TRANSFER_DST_BIT, true, "vkCmdFillBuffer()",
                                              "VK_BUFFER_USAGE_TRANSFER_DST_BIT");
        std::function<bool()> function = [=]() {
            SetBufferMemoryValid(dev_data, dst_buff_node, true);
            return false;
        };
        cb_node->validate_functions.push_back(function);

        skip_call |= addCmd(dev_data, cb_node, CMD_FILLBUFFER, "vkCmdFillBuffer()");
        skip_call |= insideRenderPass(dev_data, cb_node, "vkCmdCopyFillBuffer()");
    } else {
        assert(0);
    }
    lock.unlock();
    if (!skip_call)
        dev_data->dispatch_table.CmdFillBuffer(commandBuffer, dstBuffer, dstOffset, size, data);
}

VKAPI_ATTR void VKAPI_CALL CmdClearAttachments(VkCommandBuffer commandBuffer, uint32_t attachmentCount,
                                               const VkClearAttachment *pAttachments, uint32_t rectCount,
                                               const VkClearRect *pRects) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        skip_call |= addCmd(dev_data, pCB, CMD_CLEARATTACHMENTS, "vkCmdClearAttachments()");
        // Warn if this is issued prior to Draw Cmd and clearing the entire attachment
        if (!hasDrawCmd(pCB) && (pCB->activeRenderPassBeginInfo.renderArea.extent.width == pRects[0].rect.extent.width) &&
            (pCB->activeRenderPassBeginInfo.renderArea.extent.height == pRects[0].rect.extent.height)) {
            // There are times where app needs to use ClearAttachments (generally when reusing a buffer inside of a render pass)
            // Can we make this warning more specific? I'd like to avoid triggering this test if we can tell it's a use that must
            // call CmdClearAttachments
            // Otherwise this seems more like a performance warning.
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT,
                                 VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, reinterpret_cast<uint64_t &>(commandBuffer),
                                 0, DRAWSTATE_CLEAR_CMD_BEFORE_DRAW, "DS",
                                 "vkCmdClearAttachments() issued on command buffer object 0x%" PRIxLEAST64 " prior to any Draw Cmds."
                                 " It is recommended you use RenderPass LOAD_OP_CLEAR on Attachments prior to any Draw.",
                                 (uint64_t)(commandBuffer));
        }
        skip_call |= outsideRenderPass(dev_data, pCB, "vkCmdClearAttachments()");
    }

    // Validate that attachment is in reference list of active subpass
    if (pCB->activeRenderPass) {
        const VkRenderPassCreateInfo *pRPCI = pCB->activeRenderPass->createInfo.ptr();
        const VkSubpassDescription *pSD = &pRPCI->pSubpasses[pCB->activeSubpass];

        for (uint32_t attachment_idx = 0; attachment_idx < attachmentCount; attachment_idx++) {
            const VkClearAttachment *attachment = &pAttachments[attachment_idx];
            if (attachment->aspectMask & VK_IMAGE_ASPECT_COLOR_BIT) {
                if (attachment->colorAttachment >= pSD->colorAttachmentCount) {
                    skip_call |= log_msg(
                        dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)commandBuffer, __LINE__, DRAWSTATE_MISSING_ATTACHMENT_REFERENCE, "DS",
                        "vkCmdClearAttachments() color attachment index %d out of range for active subpass %d; ignored",
                        attachment->colorAttachment, pCB->activeSubpass);
                }
                else if (pSD->pColorAttachments[attachment->colorAttachment].attachment == VK_ATTACHMENT_UNUSED) {
                    skip_call |= log_msg(
                        dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)commandBuffer, __LINE__, DRAWSTATE_MISSING_ATTACHMENT_REFERENCE, "DS",
                        "vkCmdClearAttachments() color attachment index %d is VK_ATTACHMENT_UNUSED; ignored",
                        attachment->colorAttachment);
                }
            } else if (attachment->aspectMask & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)) {
                if (!pSD->pDepthStencilAttachment || // Says no DS will be used in active subpass
                    (pSD->pDepthStencilAttachment->attachment ==
                     VK_ATTACHMENT_UNUSED)) { // Says no DS will be used in active subpass

                    skip_call |= log_msg(
                        dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)commandBuffer, __LINE__, DRAWSTATE_MISSING_ATTACHMENT_REFERENCE, "DS",
                        "vkCmdClearAttachments() depth/stencil clear with no depth/stencil attachment in subpass; ignored");
                }
            }
        }
    }
    lock.unlock();
    if (!skip_call)
        dev_data->dispatch_table.CmdClearAttachments(commandBuffer, attachmentCount, pAttachments, rectCount, pRects);
}

VKAPI_ATTR void VKAPI_CALL CmdClearColorImage(VkCommandBuffer commandBuffer, VkImage image,
                                              VkImageLayout imageLayout, const VkClearColorValue *pColor,
                                              uint32_t rangeCount, const VkImageSubresourceRange *pRanges) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    // TODO : Verify memory is in VK_IMAGE_STATE_CLEAR state

    auto cb_node = getCBNode(dev_data, commandBuffer);
    auto image_state = getImageState(dev_data, image);
    if (cb_node && image_state) {
        skip_call |= ValidateMemoryIsBoundToImage(dev_data, image_state, "vkCmdClearColorImage()");
        AddCommandBufferBindingImage(dev_data, cb_node, image_state);
        std::function<bool()> function = [=]() {
            SetImageMemoryValid(dev_data, image_state, true);
            return false;
        };
        cb_node->validate_functions.push_back(function);

        skip_call |= addCmd(dev_data, cb_node, CMD_CLEARCOLORIMAGE, "vkCmdClearColorImage()");
        skip_call |= insideRenderPass(dev_data, cb_node, "vkCmdClearColorImage()");
    } else {
        assert(0);
    }
    lock.unlock();
    if (!skip_call)
        dev_data->dispatch_table.CmdClearColorImage(commandBuffer, image, imageLayout, pColor, rangeCount, pRanges);
}

VKAPI_ATTR void VKAPI_CALL
CmdClearDepthStencilImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout,
                          const VkClearDepthStencilValue *pDepthStencil, uint32_t rangeCount,
                          const VkImageSubresourceRange *pRanges) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    // TODO : Verify memory is in VK_IMAGE_STATE_CLEAR state

    auto cb_node = getCBNode(dev_data, commandBuffer);
    auto image_state = getImageState(dev_data, image);
    if (cb_node && image_state) {
        skip_call |= ValidateMemoryIsBoundToImage(dev_data, image_state, "vkCmdClearDepthStencilImage()");
        AddCommandBufferBindingImage(dev_data, cb_node, image_state);
        std::function<bool()> function = [=]() {
            SetImageMemoryValid(dev_data, image_state, true);
            return false;
        };
        cb_node->validate_functions.push_back(function);

        skip_call |= addCmd(dev_data, cb_node, CMD_CLEARDEPTHSTENCILIMAGE, "vkCmdClearDepthStencilImage()");
        skip_call |= insideRenderPass(dev_data, cb_node, "vkCmdClearDepthStencilImage()");
    } else {
        assert(0);
    }
    lock.unlock();
    if (!skip_call)
        dev_data->dispatch_table.CmdClearDepthStencilImage(commandBuffer, image, imageLayout, pDepthStencil, rangeCount, pRanges);
}

VKAPI_ATTR void VKAPI_CALL
CmdResolveImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage,
                VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageResolve *pRegions) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);

    auto cb_node = getCBNode(dev_data, commandBuffer);
    auto src_image_state = getImageState(dev_data, srcImage);
    auto dst_image_state = getImageState(dev_data, dstImage);
    if (cb_node && src_image_state && dst_image_state) {
        skip_call |= ValidateMemoryIsBoundToImage(dev_data, src_image_state, "vkCmdResolveImage()");
        skip_call |= ValidateMemoryIsBoundToImage(dev_data, dst_image_state, "vkCmdResolveImage()");
        // Update bindings between images and cmd buffer
        AddCommandBufferBindingImage(dev_data, cb_node, src_image_state);
        AddCommandBufferBindingImage(dev_data, cb_node, dst_image_state);
        std::function<bool()> function = [=]() {
            return ValidateImageMemoryIsValid(dev_data, src_image_state, "vkCmdResolveImage()");
        };
        cb_node->validate_functions.push_back(function);
        function = [=]() {
            SetImageMemoryValid(dev_data, dst_image_state, true);
            return false;
        };
        cb_node->validate_functions.push_back(function);

        skip_call |= addCmd(dev_data, cb_node, CMD_RESOLVEIMAGE, "vkCmdResolveImage()");
        skip_call |= insideRenderPass(dev_data, cb_node, "vkCmdResolveImage()");
    } else {
        assert(0);
    }
    lock.unlock();
    if (!skip_call)
        dev_data->dispatch_table.CmdResolveImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount,
                                                 pRegions);
}

bool setEventStageMask(VkQueue queue, VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE *pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        pCB->eventToStageMap[event] = stageMask;
    }
    auto queue_data = dev_data->queueMap.find(queue);
    if (queue_data != dev_data->queueMap.end()) {
        queue_data->second.eventToStageMap[event] = stageMask;
    }
    return false;
}

VKAPI_ATTR void VKAPI_CALL
CmdSetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        skip_call |= addCmd(dev_data, pCB, CMD_SETEVENT, "vkCmdSetEvent()");
        skip_call |= insideRenderPass(dev_data, pCB, "vkCmdSetEvent");
        auto event_state = getEventNode(dev_data, event);
        if (event_state) {
            addCommandBufferBinding(&event_state->cb_bindings,
                                    {reinterpret_cast<uint64_t &>(event), VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT}, pCB);
            event_state->cb_bindings.insert(pCB);
        }
        pCB->events.push_back(event);
        if (!pCB->waitedEvents.count(event)) {
            pCB->writeEventsBeforeWait.push_back(event);
        }
        std::function<bool(VkQueue)> eventUpdate =
            std::bind(setEventStageMask, std::placeholders::_1, commandBuffer, event, stageMask);
        pCB->eventUpdates.push_back(eventUpdate);
    }
    lock.unlock();
    if (!skip_call)
        dev_data->dispatch_table.CmdSetEvent(commandBuffer, event, stageMask);
}

VKAPI_ATTR void VKAPI_CALL
CmdResetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        skip_call |= addCmd(dev_data, pCB, CMD_RESETEVENT, "vkCmdResetEvent()");
        skip_call |= insideRenderPass(dev_data, pCB, "vkCmdResetEvent");
        auto event_state = getEventNode(dev_data, event);
        if (event_state) {
            addCommandBufferBinding(&event_state->cb_bindings,
                                    {reinterpret_cast<uint64_t &>(event), VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT}, pCB);
            event_state->cb_bindings.insert(pCB);
        }
        pCB->events.push_back(event);
        if (!pCB->waitedEvents.count(event)) {
            pCB->writeEventsBeforeWait.push_back(event);
        }
        std::function<bool(VkQueue)> eventUpdate =
            std::bind(setEventStageMask, std::placeholders::_1, commandBuffer, event, VkPipelineStageFlags(0));
        pCB->eventUpdates.push_back(eventUpdate);
    }
    lock.unlock();
    if (!skip_call)
        dev_data->dispatch_table.CmdResetEvent(commandBuffer, event, stageMask);
}

static bool TransitionImageLayouts(VkCommandBuffer cmdBuffer, uint32_t memBarrierCount,
                                   const VkImageMemoryBarrier *pImgMemBarriers) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE *pCB = getCBNode(dev_data, cmdBuffer);
    bool skip = false;
    uint32_t levelCount = 0;
    uint32_t layerCount = 0;

    for (uint32_t i = 0; i < memBarrierCount; ++i) {
        auto mem_barrier = &pImgMemBarriers[i];
        if (!mem_barrier)
            continue;
        // TODO: Do not iterate over every possibility - consolidate where
        // possible
        ResolveRemainingLevelsLayers(dev_data, &levelCount, &layerCount, mem_barrier->subresourceRange, mem_barrier->image);

        for (uint32_t j = 0; j < levelCount; j++) {
            uint32_t level = mem_barrier->subresourceRange.baseMipLevel + j;
            for (uint32_t k = 0; k < layerCount; k++) {
                uint32_t layer = mem_barrier->subresourceRange.baseArrayLayer + k;
                VkImageSubresource sub = {mem_barrier->subresourceRange.aspectMask, level, layer};
                IMAGE_CMD_BUF_LAYOUT_NODE node;
                if (!FindLayout(pCB, mem_barrier->image, sub, node)) {
                    SetLayout(pCB, mem_barrier->image, sub,
                              IMAGE_CMD_BUF_LAYOUT_NODE(mem_barrier->oldLayout, mem_barrier->newLayout));
                    continue;
                }
                if (mem_barrier->oldLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
                    // TODO: Set memory invalid which is in mem_tracker currently
                } else if (node.layout != mem_barrier->oldLayout) {
                    skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0,
                                    __LINE__, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS", "You cannot transition the layout from %s "
                                                                                    "when current layout is %s.",
                                    string_VkImageLayout(mem_barrier->oldLayout), string_VkImageLayout(node.layout));
                }
                SetLayout(pCB, mem_barrier->image, sub, mem_barrier->newLayout);
            }
        }
    }
    return skip;
}

// Print readable FlagBits in FlagMask
static std::string string_VkAccessFlags(VkAccessFlags accessMask) {
    std::string result;
    std::string separator;

    if (accessMask == 0) {
        result = "[None]";
    } else {
        result = "[";
        for (auto i = 0; i < 32; i++) {
            if (accessMask & (1 << i)) {
                result = result + separator + string_VkAccessFlagBits((VkAccessFlagBits)(1 << i));
                separator = " | ";
            }
        }
        result = result + "]";
    }
    return result;
}

// AccessFlags MUST have 'required_bit' set, and may have one or more of 'optional_bits' set.
// If required_bit is zero, accessMask must have at least one of 'optional_bits' set
// TODO: Add tracking to ensure that at least one barrier has been set for these layout transitions
static bool ValidateMaskBits(const layer_data *my_data, VkCommandBuffer cmdBuffer, const VkAccessFlags &accessMask,
                             const VkImageLayout &layout, VkAccessFlags required_bit, VkAccessFlags optional_bits,
                             const char *type) {
    bool skip_call = false;

    if ((accessMask & required_bit) || (!required_bit && (accessMask & optional_bits))) {
        if (accessMask & ~(required_bit | optional_bits)) {
            // TODO: Verify against Valid Use
            skip_call |=
                log_msg(my_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                        DRAWSTATE_INVALID_BARRIER, "DS", "Additional bits in %s accessMask 0x%X %s are specified when layout is %s.",
                        type, accessMask, string_VkAccessFlags(accessMask).c_str(), string_VkImageLayout(layout));
        }
    } else {
        if (!required_bit) {
            skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                 DRAWSTATE_INVALID_BARRIER, "DS", "%s AccessMask %d %s must contain at least one of access bits %d "
                                                                  "%s when layout is %s, unless the app has previously added a "
                                                                  "barrier for this transition.",
                                 type, accessMask, string_VkAccessFlags(accessMask).c_str(), optional_bits,
                                 string_VkAccessFlags(optional_bits).c_str(), string_VkImageLayout(layout));
        } else {
            std::string opt_bits;
            if (optional_bits != 0) {
                std::stringstream ss;
                ss << optional_bits;
                opt_bits = "and may have optional bits " + ss.str() + ' ' + string_VkAccessFlags(optional_bits);
            }
            skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                 DRAWSTATE_INVALID_BARRIER, "DS", "%s AccessMask %d %s must have required access bit %d %s %s when "
                                                                  "layout is %s, unless the app has previously added a barrier for "
                                                                  "this transition.",
                                 type, accessMask, string_VkAccessFlags(accessMask).c_str(), required_bit,
                                 string_VkAccessFlags(required_bit).c_str(), opt_bits.c_str(), string_VkImageLayout(layout));
        }
    }
    return skip_call;
}

static bool ValidateMaskBitsFromLayouts(const layer_data *my_data, VkCommandBuffer cmdBuffer, const VkAccessFlags &accessMask,
                                        const VkImageLayout &layout, const char *type) {
    bool skip_call = false;
    switch (layout) {
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: {
        skip_call |= ValidateMaskBits(my_data, cmdBuffer, accessMask, layout, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                      VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT, type);
        break;
    }
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL: {
        skip_call |= ValidateMaskBits(my_data, cmdBuffer, accessMask, layout, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                                      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT, type);
        break;
    }
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: {
        skip_call |= ValidateMaskBits(my_data, cmdBuffer, accessMask, layout, VK_ACCESS_TRANSFER_WRITE_BIT, 0, type);
        break;
    }
    case VK_IMAGE_LAYOUT_PREINITIALIZED: {
        skip_call |= ValidateMaskBits(my_data, cmdBuffer, accessMask, layout, VK_ACCESS_HOST_WRITE_BIT, 0, type);
        break;
    }
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL: {
        skip_call |= ValidateMaskBits(my_data, cmdBuffer, accessMask, layout, 0,
                                      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                      VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT, type);
        break;
    }
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: {
        skip_call |= ValidateMaskBits(my_data, cmdBuffer, accessMask, layout, 0,
                                      VK_ACCESS_INPUT_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_READ_BIT, type);
        break;
    }
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: {
        skip_call |= ValidateMaskBits(my_data, cmdBuffer, accessMask, layout, VK_ACCESS_TRANSFER_READ_BIT, 0, type);
        break;
    }
    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: {
        skip_call |= ValidateMaskBits(my_data, cmdBuffer, accessMask, layout, VK_ACCESS_MEMORY_READ_BIT, 0, type);
        break;
    }
    case VK_IMAGE_LAYOUT_UNDEFINED: {
        if (accessMask != 0) {
            // TODO: Verify against Valid Use section spec
            skip_call |=
                log_msg(my_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                        DRAWSTATE_INVALID_BARRIER, "DS", "Additional bits in %s accessMask 0x%X %s are specified when layout is %s.",
                        type, accessMask, string_VkAccessFlags(accessMask).c_str(), string_VkImageLayout(layout));
        }
        break;
    }
    case VK_IMAGE_LAYOUT_GENERAL:
    default: { break; }
    }
    return skip_call;
}

static bool ValidateBarriers(const char *funcName, VkCommandBuffer cmdBuffer, uint32_t memBarrierCount,
                             const VkMemoryBarrier *pMemBarriers, uint32_t bufferBarrierCount,
                             const VkBufferMemoryBarrier *pBufferMemBarriers, uint32_t imageMemBarrierCount,
                             const VkImageMemoryBarrier *pImageMemBarriers) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE *pCB = getCBNode(dev_data, cmdBuffer);
    if (pCB->activeRenderPass && memBarrierCount) {
        if (!pCB->activeRenderPass->hasSelfDependency[pCB->activeSubpass]) {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                 DRAWSTATE_INVALID_BARRIER, "DS", "%s: Barriers cannot be set during subpass %d "
                                                                  "with no self dependency specified.",
                                 funcName, pCB->activeSubpass);
        }
    }
    for (uint32_t i = 0; i < imageMemBarrierCount; ++i) {
        auto mem_barrier = &pImageMemBarriers[i];
        auto image_data = getImageState(dev_data, mem_barrier->image);
        if (image_data) {
            uint32_t src_q_f_index = mem_barrier->srcQueueFamilyIndex;
            uint32_t dst_q_f_index = mem_barrier->dstQueueFamilyIndex;
            if (image_data->createInfo.sharingMode == VK_SHARING_MODE_CONCURRENT) {
                // srcQueueFamilyIndex and dstQueueFamilyIndex must both
                // be VK_QUEUE_FAMILY_IGNORED
                if ((src_q_f_index != VK_QUEUE_FAMILY_IGNORED) || (dst_q_f_index != VK_QUEUE_FAMILY_IGNORED)) {
                    skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0,
                                         __LINE__, DRAWSTATE_INVALID_QUEUE_INDEX, "DS",
                                         "%s: Image Barrier for image 0x%" PRIx64 " was created with sharingMode of "
                                         "VK_SHARING_MODE_CONCURRENT.  Src and dst "
                                         " queueFamilyIndices must be VK_QUEUE_FAMILY_IGNORED.",
                                         funcName, reinterpret_cast<const uint64_t &>(mem_barrier->image));
                }
            } else {
                // Sharing mode is VK_SHARING_MODE_EXCLUSIVE. srcQueueFamilyIndex and
                // dstQueueFamilyIndex must either both be VK_QUEUE_FAMILY_IGNORED,
                // or both be a valid queue family
                if (((src_q_f_index == VK_QUEUE_FAMILY_IGNORED) || (dst_q_f_index == VK_QUEUE_FAMILY_IGNORED)) &&
                    (src_q_f_index != dst_q_f_index)) {
                    skip_call |=
                        log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                DRAWSTATE_INVALID_QUEUE_INDEX, "DS", "%s: Image 0x%" PRIx64 " was created with sharingMode "
                                                                     "of VK_SHARING_MODE_EXCLUSIVE. If one of src- or "
                                                                     "dstQueueFamilyIndex is VK_QUEUE_FAMILY_IGNORED, both "
                                                                     "must be.",
                                funcName, reinterpret_cast<const uint64_t &>(mem_barrier->image));
                } else if (((src_q_f_index != VK_QUEUE_FAMILY_IGNORED) && (dst_q_f_index != VK_QUEUE_FAMILY_IGNORED)) &&
                           ((src_q_f_index >= dev_data->phys_dev_properties.queue_family_properties.size()) ||
                            (dst_q_f_index >= dev_data->phys_dev_properties.queue_family_properties.size()))) {
                    skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0,
                                         __LINE__, DRAWSTATE_INVALID_QUEUE_INDEX, "DS",
                                         "%s: Image 0x%" PRIx64 " was created with sharingMode "
                                         "of VK_SHARING_MODE_EXCLUSIVE, but srcQueueFamilyIndex %d"
                                         " or dstQueueFamilyIndex %d is greater than " PRINTF_SIZE_T_SPECIFIER
                                         "queueFamilies crated for this device.",
                                         funcName, reinterpret_cast<const uint64_t &>(mem_barrier->image), src_q_f_index,
                                         dst_q_f_index, dev_data->phys_dev_properties.queue_family_properties.size());
                }
            }
        }

        if (mem_barrier) {
            if (mem_barrier->oldLayout != mem_barrier->newLayout) {
                skip_call |=
                    ValidateMaskBitsFromLayouts(dev_data, cmdBuffer, mem_barrier->srcAccessMask, mem_barrier->oldLayout, "Source");
                skip_call |=
                    ValidateMaskBitsFromLayouts(dev_data, cmdBuffer, mem_barrier->dstAccessMask, mem_barrier->newLayout, "Dest");
            }
            if (mem_barrier->newLayout == VK_IMAGE_LAYOUT_UNDEFINED || mem_barrier->newLayout == VK_IMAGE_LAYOUT_PREINITIALIZED) {
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                        DRAWSTATE_INVALID_BARRIER, "DS", "%s: Image Layout cannot be transitioned to UNDEFINED or "
                                                         "PREINITIALIZED.",
                        funcName);
            }
            auto image_data = getImageState(dev_data, mem_barrier->image);
            VkFormat format = VK_FORMAT_UNDEFINED;
            uint32_t arrayLayers = 0, mipLevels = 0;
            bool imageFound = false;
            if (image_data) {
                format = image_data->createInfo.format;
                arrayLayers = image_data->createInfo.arrayLayers;
                mipLevels = image_data->createInfo.mipLevels;
                imageFound = true;
            } else if (dev_data->device_extensions.wsi_enabled) {
                auto imageswap_data = getSwapchainFromImage(dev_data, mem_barrier->image);
                if (imageswap_data) {
                    auto swapchain_data = getSwapchainNode(dev_data, imageswap_data);
                    if (swapchain_data) {
                        format = swapchain_data->createInfo.imageFormat;
                        arrayLayers = swapchain_data->createInfo.imageArrayLayers;
                        mipLevels = 1;
                        imageFound = true;
                    }
                }
            }
            if (imageFound) {
                auto aspect_mask = mem_barrier->subresourceRange.aspectMask;
                if (vk_format_is_depth_or_stencil(format)) {
                    if (vk_format_is_depth_and_stencil(format)) {
                        if (!(aspect_mask & VK_IMAGE_ASPECT_DEPTH_BIT) && !(aspect_mask & VK_IMAGE_ASPECT_STENCIL_BIT)) {
                            log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0,
                                    __LINE__, DRAWSTATE_INVALID_BARRIER, "DS",
                                    "%s: Image is a depth and stencil format and thus must "
                                    "have either one or both of VK_IMAGE_ASPECT_DEPTH_BIT and "
                                    "VK_IMAGE_ASPECT_STENCIL_BIT set.",
                                    funcName);
                        }
                    } else if (vk_format_is_depth_only(format)) {
                        if (!(aspect_mask & VK_IMAGE_ASPECT_DEPTH_BIT)) {
                            log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0,
                                    __LINE__, DRAWSTATE_INVALID_BARRIER, "DS", "%s: Image is a depth-only format and thus must "
                                                                               "have VK_IMAGE_ASPECT_DEPTH_BIT set.",
                                    funcName);
                        }
                    } else { // stencil-only case
                        if (!(aspect_mask & VK_IMAGE_ASPECT_STENCIL_BIT)) {
                            log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0,
                                    __LINE__, DRAWSTATE_INVALID_BARRIER, "DS", "%s: Image is a stencil-only format and thus must "
                                                                               "have VK_IMAGE_ASPECT_STENCIL_BIT set.",
                                    funcName);
                        }
                    }
                } else { // image is a color format
                    if (!(aspect_mask & VK_IMAGE_ASPECT_COLOR_BIT)) {
                        log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                DRAWSTATE_INVALID_BARRIER, "DS", "%s: Image is a color format and thus must "
                                                                 "have VK_IMAGE_ASPECT_COLOR_BIT set.",
                                funcName);
                    }
                }
                int layerCount = (mem_barrier->subresourceRange.layerCount == VK_REMAINING_ARRAY_LAYERS)
                                     ? 1
                                     : mem_barrier->subresourceRange.layerCount;
                if ((mem_barrier->subresourceRange.baseArrayLayer + layerCount) > arrayLayers) {
                    log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                            DRAWSTATE_INVALID_BARRIER, "DS", "%s: Subresource must have the sum of the "
                                                             "baseArrayLayer (%d) and layerCount (%d) be less "
                                                             "than or equal to the total number of layers (%d).",
                            funcName, mem_barrier->subresourceRange.baseArrayLayer, mem_barrier->subresourceRange.layerCount,
                            arrayLayers);
                }
                int levelCount = (mem_barrier->subresourceRange.levelCount == VK_REMAINING_MIP_LEVELS)
                                     ? 1
                                     : mem_barrier->subresourceRange.levelCount;
                if ((mem_barrier->subresourceRange.baseMipLevel + levelCount) > mipLevels) {
                    log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                            DRAWSTATE_INVALID_BARRIER, "DS", "%s: Subresource must have the sum of the baseMipLevel "
                                                             "(%d) and levelCount (%d) be less than or equal to "
                                                             "the total number of levels (%d).",
                            funcName, mem_barrier->subresourceRange.baseMipLevel, mem_barrier->subresourceRange.levelCount,
                            mipLevels);
                }
            }
        }
    }
    for (uint32_t i = 0; i < bufferBarrierCount; ++i) {
        auto mem_barrier = &pBufferMemBarriers[i];
        if (pCB->activeRenderPass) {
            skip_call |=
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                        DRAWSTATE_INVALID_BARRIER, "DS", "%s: Buffer Barriers cannot be used during a render pass.", funcName);
        }
        if (!mem_barrier)
            continue;

        // Validate buffer barrier queue family indices
        if ((mem_barrier->srcQueueFamilyIndex != VK_QUEUE_FAMILY_IGNORED &&
             mem_barrier->srcQueueFamilyIndex >= dev_data->phys_dev_properties.queue_family_properties.size()) ||
            (mem_barrier->dstQueueFamilyIndex != VK_QUEUE_FAMILY_IGNORED &&
             mem_barrier->dstQueueFamilyIndex >= dev_data->phys_dev_properties.queue_family_properties.size())) {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                 DRAWSTATE_INVALID_QUEUE_INDEX, "DS",
                                 "%s: Buffer Barrier 0x%" PRIx64 " has QueueFamilyIndex greater "
                                 "than the number of QueueFamilies (" PRINTF_SIZE_T_SPECIFIER ") for this device.",
                                 funcName, reinterpret_cast<const uint64_t &>(mem_barrier->buffer),
                                 dev_data->phys_dev_properties.queue_family_properties.size());
        }

        auto buffer_node = getBufferNode(dev_data, mem_barrier->buffer);
        if (buffer_node) {
            auto buffer_size = buffer_node->binding.size;
            if (mem_barrier->offset >= buffer_size) {
                skip_call |= log_msg(
                    dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                    DRAWSTATE_INVALID_BARRIER, "DS",
                    "%s: Buffer Barrier 0x%" PRIx64 " has offset 0x%" PRIx64 " which is not less than total size 0x%" PRIx64 ".",
                    funcName, reinterpret_cast<const uint64_t &>(mem_barrier->buffer),
                    reinterpret_cast<const uint64_t &>(mem_barrier->offset), reinterpret_cast<const uint64_t &>(buffer_size));
            } else if (mem_barrier->size != VK_WHOLE_SIZE && (mem_barrier->offset + mem_barrier->size > buffer_size)) {
                skip_call |= log_msg(
                    dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                    DRAWSTATE_INVALID_BARRIER, "DS", "%s: Buffer Barrier 0x%" PRIx64 " has offset 0x%" PRIx64 " and size 0x%" PRIx64
                                                     " whose sum is greater than total size 0x%" PRIx64 ".",
                    funcName, reinterpret_cast<const uint64_t &>(mem_barrier->buffer),
                    reinterpret_cast<const uint64_t &>(mem_barrier->offset), reinterpret_cast<const uint64_t &>(mem_barrier->size),
                    reinterpret_cast<const uint64_t &>(buffer_size));
            }
        }
    }
    return skip_call;
}

bool validateEventStageMask(VkQueue queue, GLOBAL_CB_NODE *pCB, uint32_t eventCount, size_t firstEventIndex, VkPipelineStageFlags sourceStageMask) {
    bool skip_call = false;
    VkPipelineStageFlags stageMask = 0;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(queue), layer_data_map);
    for (uint32_t i = 0; i < eventCount; ++i) {
        auto event = pCB->events[firstEventIndex + i];
        auto queue_data = dev_data->queueMap.find(queue);
        if (queue_data == dev_data->queueMap.end())
            return false;
        auto event_data = queue_data->second.eventToStageMap.find(event);
        if (event_data != queue_data->second.eventToStageMap.end()) {
            stageMask |= event_data->second;
        } else {
            auto global_event_data = getEventNode(dev_data, event);
            if (!global_event_data) {
                skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT,
                                     reinterpret_cast<const uint64_t &>(event), __LINE__, DRAWSTATE_INVALID_EVENT, "DS",
                                     "Event 0x%" PRIx64 " cannot be waited on if it has never been set.",
                                     reinterpret_cast<const uint64_t &>(event));
            } else {
                stageMask |= global_event_data->stageMask;
            }
        }
    }
    // TODO: Need to validate that host_bit is only set if set event is called
    // but set event can be called at any time.
    if (sourceStageMask != stageMask && sourceStageMask != (stageMask | VK_PIPELINE_STAGE_HOST_BIT)) {
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                             DRAWSTATE_INVALID_EVENT, "DS", "Submitting cmdbuffer with call to VkCmdWaitEvents "
                                                            "using srcStageMask 0x%X which must be the bitwise "
                                                            "OR of the stageMask parameters used in calls to "
                                                            "vkCmdSetEvent and VK_PIPELINE_STAGE_HOST_BIT if "
                                                            "used with vkSetEvent but instead is 0x%X.",
                             sourceStageMask, stageMask);
    }
    return skip_call;
}

VKAPI_ATTR void VKAPI_CALL
CmdWaitEvents(VkCommandBuffer commandBuffer, uint32_t eventCount, const VkEvent *pEvents, VkPipelineStageFlags sourceStageMask,
              VkPipelineStageFlags dstStageMask, uint32_t memoryBarrierCount, const VkMemoryBarrier *pMemoryBarriers,
              uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier *pBufferMemoryBarriers,
              uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier *pImageMemoryBarriers) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        auto firstEventIndex = pCB->events.size();
        for (uint32_t i = 0; i < eventCount; ++i) {
            auto event_state = getEventNode(dev_data, pEvents[i]);
            if (event_state) {
                addCommandBufferBinding(&event_state->cb_bindings,
                                        {reinterpret_cast<const uint64_t &>(pEvents[i]), VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT},
                                        pCB);
                event_state->cb_bindings.insert(pCB);
            }
            pCB->waitedEvents.insert(pEvents[i]);
            pCB->events.push_back(pEvents[i]);
        }
        std::function<bool(VkQueue)> eventUpdate =
            std::bind(validateEventStageMask, std::placeholders::_1, pCB, eventCount, firstEventIndex, sourceStageMask);
        pCB->eventUpdates.push_back(eventUpdate);
        if (pCB->state == CB_RECORDING) {
            skip_call |= addCmd(dev_data, pCB, CMD_WAITEVENTS, "vkCmdWaitEvents()");
        } else {
            skip_call |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdWaitEvents()");
        }
        skip_call |= TransitionImageLayouts(commandBuffer, imageMemoryBarrierCount, pImageMemoryBarriers);
        skip_call |=
            ValidateBarriers("vkCmdWaitEvents", commandBuffer, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount,
                             pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
    }
    lock.unlock();
    if (!skip_call)
        dev_data->dispatch_table.CmdWaitEvents(commandBuffer, eventCount, pEvents, sourceStageMask, dstStageMask,
                                               memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers,
                                               imageMemoryBarrierCount, pImageMemoryBarriers);
}

VKAPI_ATTR void VKAPI_CALL
CmdPipelineBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
                   VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount, const VkMemoryBarrier *pMemoryBarriers,
                   uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier *pBufferMemoryBarriers,
                   uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier *pImageMemoryBarriers) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        skip_call |= addCmd(dev_data, pCB, CMD_PIPELINEBARRIER, "vkCmdPipelineBarrier()");
        skip_call |= TransitionImageLayouts(commandBuffer, imageMemoryBarrierCount, pImageMemoryBarriers);
        skip_call |=
            ValidateBarriers("vkCmdPipelineBarrier", commandBuffer, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount,
                             pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
    }
    lock.unlock();
    if (!skip_call)
        dev_data->dispatch_table.CmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, dependencyFlags, memoryBarrierCount,
                                                    pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers,
                                                    imageMemoryBarrierCount, pImageMemoryBarriers);
}

bool setQueryState(VkQueue queue, VkCommandBuffer commandBuffer, QueryObject object, bool value) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE *pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        pCB->queryToStateMap[object] = value;
    }
    auto queue_data = dev_data->queueMap.find(queue);
    if (queue_data != dev_data->queueMap.end()) {
        queue_data->second.queryToStateMap[object] = value;
    }
    return false;
}

VKAPI_ATTR void VKAPI_CALL
CmdBeginQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t slot, VkFlags flags) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        QueryObject query = {queryPool, slot};
        pCB->activeQueries.insert(query);
        if (!pCB->startedQueries.count(query)) {
            pCB->startedQueries.insert(query);
        }
        skip_call |= addCmd(dev_data, pCB, CMD_BEGINQUERY, "vkCmdBeginQuery()");
        addCommandBufferBinding(&getQueryPoolNode(dev_data, queryPool)->cb_bindings,
                                {reinterpret_cast<uint64_t &>(queryPool), VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT}, pCB);
    }
    lock.unlock();
    if (!skip_call)
        dev_data->dispatch_table.CmdBeginQuery(commandBuffer, queryPool, slot, flags);
}

VKAPI_ATTR void VKAPI_CALL CmdEndQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t slot) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        QueryObject query = {queryPool, slot};
        if (!pCB->activeQueries.count(query)) {
            skip_call |=
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                        DRAWSTATE_INVALID_QUERY, "DS", "Ending a query before it was started: queryPool 0x%" PRIx64 ", index %d",
                        (uint64_t)(queryPool), slot);
        } else {
            pCB->activeQueries.erase(query);
        }
        std::function<bool(VkQueue)> queryUpdate = std::bind(setQueryState, std::placeholders::_1, commandBuffer, query, true);
        pCB->queryUpdates.push_back(queryUpdate);
        if (pCB->state == CB_RECORDING) {
            skip_call |= addCmd(dev_data, pCB, CMD_ENDQUERY, "VkCmdEndQuery()");
        } else {
            skip_call |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdEndQuery()");
        }
        addCommandBufferBinding(&getQueryPoolNode(dev_data, queryPool)->cb_bindings,
                                {reinterpret_cast<uint64_t &>(queryPool), VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT}, pCB);
    }
    lock.unlock();
    if (!skip_call)
        dev_data->dispatch_table.CmdEndQuery(commandBuffer, queryPool, slot);
}

VKAPI_ATTR void VKAPI_CALL
CmdResetQueryPool(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        for (uint32_t i = 0; i < queryCount; i++) {
            QueryObject query = {queryPool, firstQuery + i};
            pCB->waitedEventsBeforeQueryReset[query] = pCB->waitedEvents;
            std::function<bool(VkQueue)> queryUpdate = std::bind(setQueryState, std::placeholders::_1, commandBuffer, query, false);
            pCB->queryUpdates.push_back(queryUpdate);
        }
        if (pCB->state == CB_RECORDING) {
            skip_call |= addCmd(dev_data, pCB, CMD_RESETQUERYPOOL, "VkCmdResetQueryPool()");
        } else {
            skip_call |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdResetQueryPool()");
        }
        skip_call |= insideRenderPass(dev_data, pCB, "vkCmdQueryPool");
        addCommandBufferBinding(&getQueryPoolNode(dev_data, queryPool)->cb_bindings,
                                {reinterpret_cast<uint64_t &>(queryPool), VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT}, pCB);
    }
    lock.unlock();
    if (!skip_call)
        dev_data->dispatch_table.CmdResetQueryPool(commandBuffer, queryPool, firstQuery, queryCount);
}

bool validateQuery(VkQueue queue, GLOBAL_CB_NODE *pCB, VkQueryPool queryPool, uint32_t queryCount, uint32_t firstQuery) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(pCB->commandBuffer), layer_data_map);
    auto queue_data = dev_data->queueMap.find(queue);
    if (queue_data == dev_data->queueMap.end())
        return false;
    for (uint32_t i = 0; i < queryCount; i++) {
        QueryObject query = {queryPool, firstQuery + i};
        auto query_data = queue_data->second.queryToStateMap.find(query);
        bool fail = false;
        if (query_data != queue_data->second.queryToStateMap.end()) {
            if (!query_data->second) {
                fail = true;
            }
        } else {
            auto global_query_data = dev_data->queryToStateMap.find(query);
            if (global_query_data != dev_data->queryToStateMap.end()) {
                if (!global_query_data->second) {
                    fail = true;
                }
            } else {
                fail = true;
            }
        }
        if (fail) {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                 DRAWSTATE_INVALID_QUERY, "DS",
                                 "Requesting a copy from query to buffer with invalid query: queryPool 0x%" PRIx64 ", index %d",
                                 reinterpret_cast<uint64_t &>(queryPool), firstQuery + i);
        }
    }
    return skip_call;
}

VKAPI_ATTR void VKAPI_CALL
CmdCopyQueryPoolResults(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount,
                        VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize stride, VkQueryResultFlags flags) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);

    auto cb_node = getCBNode(dev_data, commandBuffer);
    auto dst_buff_node = getBufferNode(dev_data, dstBuffer);
    if (cb_node && dst_buff_node) {
        skip_call |= ValidateMemoryIsBoundToBuffer(dev_data, dst_buff_node, "vkCmdCopyQueryPoolResults()");
        // Update bindings between buffer and cmd buffer
        AddCommandBufferBindingBuffer(dev_data, cb_node, dst_buff_node);
        // Validate that DST buffer has correct usage flags set
        skip_call |= ValidateBufferUsageFlags(dev_data, dst_buff_node, VK_BUFFER_USAGE_TRANSFER_DST_BIT, true,
                                              "vkCmdCopyQueryPoolResults()", "VK_BUFFER_USAGE_TRANSFER_DST_BIT");
        std::function<bool()> function = [=]() {
            SetBufferMemoryValid(dev_data, dst_buff_node, true);
            return false;
        };
        cb_node->validate_functions.push_back(function);
        std::function<bool(VkQueue)> queryUpdate =
            std::bind(validateQuery, std::placeholders::_1, cb_node, queryPool, queryCount, firstQuery);
        cb_node->queryUpdates.push_back(queryUpdate);
        if (cb_node->state == CB_RECORDING) {
            skip_call |= addCmd(dev_data, cb_node, CMD_COPYQUERYPOOLRESULTS, "vkCmdCopyQueryPoolResults()");
        } else {
            skip_call |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdCopyQueryPoolResults()");
        }
        skip_call |= insideRenderPass(dev_data, cb_node, "vkCmdCopyQueryPoolResults()");
        addCommandBufferBinding(&getQueryPoolNode(dev_data, queryPool)->cb_bindings,
                                {reinterpret_cast<uint64_t &>(queryPool), VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT}, cb_node);
    } else {
        assert(0);
    }
    lock.unlock();
    if (!skip_call)
        dev_data->dispatch_table.CmdCopyQueryPoolResults(commandBuffer, queryPool, firstQuery, queryCount, dstBuffer, dstOffset,
                                                         stride, flags);
}

VKAPI_ATTR void VKAPI_CALL CmdPushConstants(VkCommandBuffer commandBuffer, VkPipelineLayout layout,
                                            VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size,
                                            const void *pValues) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_RECORDING) {
            skip_call |= addCmd(dev_data, pCB, CMD_PUSHCONSTANTS, "vkCmdPushConstants()");
        } else {
            skip_call |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdPushConstants()");
        }
    }
    skip_call |= validatePushConstantRange(dev_data, offset, size, "vkCmdPushConstants()");
    if (0 == stageFlags) {
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                             DRAWSTATE_PUSH_CONSTANTS_ERROR, "DS", "vkCmdPushConstants() call has no stageFlags set.");
    }

    // Check if push constant update is within any of the ranges with the same stage flags specified in pipeline layout.
    auto pipeline_layout = getPipelineLayout(dev_data, layout);
    // Coalesce adjacent/overlapping pipeline ranges before checking to see if incoming range is
    // contained in the pipeline ranges.
    // Build a {start, end} span list for ranges with matching stage flags.
    const auto &ranges = pipeline_layout->push_constant_ranges;
    struct span {
        uint32_t start;
        uint32_t end;
    };
    std::vector<span> spans;
    spans.reserve(ranges.size());
    for (const auto &iter : ranges) {
        if (iter.stageFlags == stageFlags) {
            spans.push_back({iter.offset, iter.offset + iter.size});
        }
    }
    if (spans.size() == 0) {
        // There were no ranges that matched the stageFlags.
        skip_call |=
            log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                    DRAWSTATE_PUSH_CONSTANTS_ERROR, "DS", "vkCmdPushConstants() stageFlags = 0x%" PRIx32 " do not match "
                                                          "the stageFlags in any of the ranges in pipeline layout 0x%" PRIx64 ".",
                    (uint32_t)stageFlags, (uint64_t)layout);
    } else {
        // Sort span list by start value.
        struct comparer {
            bool operator()(struct span i, struct span j) { return i.start < j.start; }
        } my_comparer;
        std::sort(spans.begin(), spans.end(), my_comparer);

        // Examine two spans at a time.
        std::vector<span>::iterator current = spans.begin();
        std::vector<span>::iterator next = current + 1;
        while (next != spans.end()) {
            if (current->end < next->start) {
                // There is a gap; cannot coalesce. Move to the next two spans.
                ++current;
                ++next;
            } else {
                // Coalesce the two spans.  The start of the next span
                // is within the current span, so pick the larger of
                // the end values to extend the current span.
                // Then delete the next span and set next to the span after it.
                current->end = max(current->end, next->end);
                next = spans.erase(next);
            }
        }

        // Now we can check if the incoming range is within any of the spans.
        bool contained_in_a_range = false;
        for (uint32_t i = 0; i < spans.size(); ++i) {
            if ((offset >= spans[i].start) && ((uint64_t)offset + (uint64_t)size <= (uint64_t)spans[i].end)) {
                contained_in_a_range = true;
                break;
            }
        }
        if (!contained_in_a_range) {
            skip_call |=
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                        DRAWSTATE_PUSH_CONSTANTS_ERROR, "DS", "vkCmdPushConstants() Push constant range [%d, %d) "
                                                              "with stageFlags = 0x%" PRIx32 " "
                                                              "not within flag-matching ranges in pipeline layout 0x%" PRIx64 ".",
                        offset, offset + size, (uint32_t)stageFlags, (uint64_t)layout);
        }
    }
    lock.unlock();
    if (!skip_call)
        dev_data->dispatch_table.CmdPushConstants(commandBuffer, layout, stageFlags, offset, size, pValues);
}

VKAPI_ATTR void VKAPI_CALL
CmdWriteTimestamp(VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage, VkQueryPool queryPool, uint32_t slot) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        QueryObject query = {queryPool, slot};
        std::function<bool(VkQueue)> queryUpdate = std::bind(setQueryState, std::placeholders::_1, commandBuffer, query, true);
        pCB->queryUpdates.push_back(queryUpdate);
        if (pCB->state == CB_RECORDING) {
            skip_call |= addCmd(dev_data, pCB, CMD_WRITETIMESTAMP, "vkCmdWriteTimestamp()");
        } else {
            skip_call |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdWriteTimestamp()");
        }
    }
    lock.unlock();
    if (!skip_call)
        dev_data->dispatch_table.CmdWriteTimestamp(commandBuffer, pipelineStage, queryPool, slot);
}

static bool MatchUsage(layer_data *dev_data, uint32_t count, const VkAttachmentReference *attachments,
                       const VkFramebufferCreateInfo *fbci, VkImageUsageFlagBits usage_flag) {
    bool skip_call = false;

    for (uint32_t attach = 0; attach < count; attach++) {
        if (attachments[attach].attachment != VK_ATTACHMENT_UNUSED) {
            // Attachment counts are verified elsewhere, but prevent an invalid access
            if (attachments[attach].attachment < fbci->attachmentCount) {
                const VkImageView *image_view = &fbci->pAttachments[attachments[attach].attachment];
                auto view_state = getImageViewState(dev_data, *image_view);
                if (view_state) {
                    const VkImageCreateInfo *ici = &getImageState(dev_data, view_state->create_info.image)->createInfo;
                    if (ici != nullptr) {
                        if ((ici->usage & usage_flag) == 0) {
                            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                                 (VkDebugReportObjectTypeEXT)0, 0, __LINE__, DRAWSTATE_INVALID_IMAGE_USAGE, "DS",
                                                 "vkCreateFramebuffer:  Framebuffer Attachment (%d) conflicts with the image's "
                                                 "IMAGE_USAGE flags (%s).",
                                                 attachments[attach].attachment, string_VkImageUsageFlagBits(usage_flag));
                        }
                    }
                }
            }
        }
    }
    return skip_call;
}

// Validate VkFramebufferCreateInfo which includes:
// 1. attachmentCount equals renderPass attachmentCount
// 2. corresponding framebuffer and renderpass attachments have matching formats
// 3. corresponding framebuffer and renderpass attachments have matching sample counts
// 4. fb attachments only have a single mip level
// 5. fb attachment dimensions are each at least as large as the fb
// 6. fb attachments use idenity swizzle
// 7. fb attachments used by renderPass for color/input/ds have correct usage bit set
// 8. fb dimensions are within physical device limits
static bool ValidateFramebufferCreateInfo(layer_data *dev_data, const VkFramebufferCreateInfo *pCreateInfo) {
    bool skip_call = false;

    auto rp_state = getRenderPassState(dev_data, pCreateInfo->renderPass);
    if (rp_state) {
        const VkRenderPassCreateInfo *rpci = rp_state->createInfo.ptr();
        if (rpci->attachmentCount != pCreateInfo->attachmentCount) {
            skip_call |= log_msg(
                dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT,
                reinterpret_cast<const uint64_t &>(pCreateInfo->renderPass), __LINE__, DRAWSTATE_RENDERPASS_INCOMPATIBLE, "DS",
                "vkCreateFramebuffer(): VkFramebufferCreateInfo attachmentCount of %u does not match attachmentCount of %u of "
                "renderPass (0x%" PRIxLEAST64 ") being used to create Framebuffer.",
                pCreateInfo->attachmentCount, rpci->attachmentCount, reinterpret_cast<const uint64_t &>(pCreateInfo->renderPass));
        } else {
            // attachmentCounts match, so make sure corresponding attachment details line up
            const VkImageView *image_views = pCreateInfo->pAttachments;
            for (uint32_t i = 0; i < pCreateInfo->attachmentCount; ++i) {
                auto view_state = getImageViewState(dev_data, image_views[i]);
                auto &ivci = view_state->create_info;
                if (ivci.format != rpci->pAttachments[i].format) {
                    skip_call |= log_msg(
                        dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT,
                        reinterpret_cast<const uint64_t &>(pCreateInfo->renderPass), __LINE__, DRAWSTATE_RENDERPASS_INCOMPATIBLE,
                        "DS", "vkCreateFramebuffer(): VkFramebufferCreateInfo attachment #%u has format of %s that does not match "
                              "the format of "
                              "%s used by the corresponding attachment for renderPass (0x%" PRIxLEAST64 ").",
                        i, string_VkFormat(ivci.format), string_VkFormat(rpci->pAttachments[i].format),
                        reinterpret_cast<const uint64_t &>(pCreateInfo->renderPass));
                }
                const VkImageCreateInfo *ici = &getImageState(dev_data, ivci.image)->createInfo;
                if (ici->samples != rpci->pAttachments[i].samples) {
                    skip_call |= log_msg(
                        dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT,
                        reinterpret_cast<const uint64_t &>(pCreateInfo->renderPass), __LINE__, DRAWSTATE_RENDERPASS_INCOMPATIBLE,
                        "DS", "vkCreateFramebuffer(): VkFramebufferCreateInfo attachment #%u has %s samples that do not match "
                              "the %s samples used by the corresponding attachment for renderPass (0x%" PRIxLEAST64 ").",
                        i, string_VkSampleCountFlagBits(ici->samples), string_VkSampleCountFlagBits(rpci->pAttachments[i].samples),
                        reinterpret_cast<const uint64_t &>(pCreateInfo->renderPass));
                }
                // Verify that view only has a single mip level
                if (ivci.subresourceRange.levelCount != 1) {
                    skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0,
                                         __LINE__, DRAWSTATE_INVALID_FRAMEBUFFER_CREATE_INFO, "DS",
                                         "vkCreateFramebuffer(): VkFramebufferCreateInfo attachment #%u has mip levelCount of %u "
                                         "but only a single mip level (levelCount ==  1) is allowed when creating a Framebuffer.",
                                         i, ivci.subresourceRange.levelCount);
                }
                const uint32_t mip_level = ivci.subresourceRange.baseMipLevel;
                uint32_t mip_width = max(1u, ici->extent.width >> mip_level);
                uint32_t mip_height = max(1u, ici->extent.height >> mip_level);
                if ((ivci.subresourceRange.layerCount < pCreateInfo->layers) || (mip_width < pCreateInfo->width) ||
                    (mip_height < pCreateInfo->height)) {
                    skip_call |=
                        log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0, __LINE__,
                                DRAWSTATE_INVALID_FRAMEBUFFER_CREATE_INFO, "DS",
                                "vkCreateFramebuffer(): VkFramebufferCreateInfo attachment #%u mip level %u has dimensions smaller "
                                "than the corresponding "
                                "framebuffer dimensions. Attachment dimensions must be at least as large. Here are the respective "
                                "dimensions for "
                                "attachment #%u, framebuffer:\n"
                                "width: %u, %u\n"
                                "height: %u, %u\n"
                                "layerCount: %u, %u\n",
                                i, ivci.subresourceRange.baseMipLevel, i, mip_width, pCreateInfo->width, mip_height,
                                pCreateInfo->height, ivci.subresourceRange.layerCount, pCreateInfo->layers);
                }
                if (((ivci.components.r != VK_COMPONENT_SWIZZLE_IDENTITY) && (ivci.components.r != VK_COMPONENT_SWIZZLE_R)) ||
                    ((ivci.components.g != VK_COMPONENT_SWIZZLE_IDENTITY) && (ivci.components.g != VK_COMPONENT_SWIZZLE_G)) ||
                    ((ivci.components.b != VK_COMPONENT_SWIZZLE_IDENTITY) && (ivci.components.b != VK_COMPONENT_SWIZZLE_B)) ||
                    ((ivci.components.a != VK_COMPONENT_SWIZZLE_IDENTITY) && (ivci.components.a != VK_COMPONENT_SWIZZLE_A))) {
                    skip_call |= log_msg(
                        dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0, __LINE__,
                        DRAWSTATE_INVALID_FRAMEBUFFER_CREATE_INFO, "DS",
                        "vkCreateFramebuffer(): VkFramebufferCreateInfo attachment #%u has non-identy swizzle. All framebuffer "
                        "attachments must have been created with the identity swizzle. Here are the actual swizzle values:\n"
                        "r swizzle = %s\n"
                        "g swizzle = %s\n"
                        "b swizzle = %s\n"
                        "a swizzle = %s\n",
                        i, string_VkComponentSwizzle(ivci.components.r), string_VkComponentSwizzle(ivci.components.g),
                        string_VkComponentSwizzle(ivci.components.b), string_VkComponentSwizzle(ivci.components.a));
                }
            }
        }
        // Verify correct attachment usage flags
        for (uint32_t subpass = 0; subpass < rpci->subpassCount; subpass++) {
            // Verify input attachments:
            skip_call |= MatchUsage(dev_data, rpci->pSubpasses[subpass].inputAttachmentCount,
                                    rpci->pSubpasses[subpass].pInputAttachments, pCreateInfo, VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
            // Verify color attachments:
            skip_call |= MatchUsage(dev_data, rpci->pSubpasses[subpass].colorAttachmentCount,
                                    rpci->pSubpasses[subpass].pColorAttachments, pCreateInfo, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
            // Verify depth/stencil attachments:
            if (rpci->pSubpasses[subpass].pDepthStencilAttachment != nullptr) {
                skip_call |= MatchUsage(dev_data, 1, rpci->pSubpasses[subpass].pDepthStencilAttachment, pCreateInfo,
                                        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
            }
        }
    } else {
        skip_call |=
            log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT,
                    reinterpret_cast<const uint64_t &>(pCreateInfo->renderPass), __LINE__, DRAWSTATE_INVALID_RENDERPASS, "DS",
                    "vkCreateFramebuffer(): Attempt to create framebuffer with invalid renderPass (0x%" PRIxLEAST64 ").",
                    reinterpret_cast<const uint64_t &>(pCreateInfo->renderPass));
    }
    // Verify FB dimensions are within physical device limits
    if ((pCreateInfo->height > dev_data->phys_dev_properties.properties.limits.maxFramebufferHeight) ||
        (pCreateInfo->width > dev_data->phys_dev_properties.properties.limits.maxFramebufferWidth) ||
        (pCreateInfo->layers > dev_data->phys_dev_properties.properties.limits.maxFramebufferLayers)) {
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0, __LINE__,
                             DRAWSTATE_INVALID_FRAMEBUFFER_CREATE_INFO, "DS",
                             "vkCreateFramebuffer(): Requested VkFramebufferCreateInfo dimensions exceed physical device limits. "
                             "Here are the respective dimensions: requested, device max:\n"
                             "width: %u, %u\n"
                             "height: %u, %u\n"
                             "layerCount: %u, %u\n",
                             pCreateInfo->width, dev_data->phys_dev_properties.properties.limits.maxFramebufferWidth,
                             pCreateInfo->height, dev_data->phys_dev_properties.properties.limits.maxFramebufferHeight,
                             pCreateInfo->layers, dev_data->phys_dev_properties.properties.limits.maxFramebufferLayers);
    }
    return skip_call;
}

// Validate VkFramebufferCreateInfo state prior to calling down chain to create Framebuffer object
//  Return true if an error is encountered and callback returns true to skip call down chain
//   false indicates that call down chain should proceed
static bool PreCallValidateCreateFramebuffer(layer_data *dev_data, const VkFramebufferCreateInfo *pCreateInfo) {
    // TODO : Verify that renderPass FB is created with is compatible with FB
    bool skip_call = false;
    skip_call |= ValidateFramebufferCreateInfo(dev_data, pCreateInfo);
    return skip_call;
}

// CreateFramebuffer state has been validated and call down chain completed so record new framebuffer object
static void PostCallRecordCreateFramebuffer(layer_data *dev_data, const VkFramebufferCreateInfo *pCreateInfo, VkFramebuffer fb) {
    // Shadow create info and store in map
    std::unique_ptr<FRAMEBUFFER_STATE> fb_state(
        new FRAMEBUFFER_STATE(fb, pCreateInfo, dev_data->renderPassMap[pCreateInfo->renderPass]->createInfo.ptr()));

    for (uint32_t i = 0; i < pCreateInfo->attachmentCount; ++i) {
        VkImageView view = pCreateInfo->pAttachments[i];
        auto view_state = getImageViewState(dev_data, view);
        if (!view_state) {
            continue;
        }
        MT_FB_ATTACHMENT_INFO fb_info;
        fb_info.mem = getImageState(dev_data, view_state->create_info.image)->binding.mem;
        fb_info.view_state = view_state;
        fb_info.image = view_state->create_info.image;
        fb_state->attachments.push_back(fb_info);
    }
    dev_data->frameBufferMap[fb] = std::move(fb_state);
}

VKAPI_ATTR VkResult VKAPI_CALL CreateFramebuffer(VkDevice device, const VkFramebufferCreateInfo *pCreateInfo,
                                                 const VkAllocationCallbacks *pAllocator,
                                                 VkFramebuffer *pFramebuffer) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip_call = PreCallValidateCreateFramebuffer(dev_data, pCreateInfo);
    lock.unlock();

    if (skip_call)
        return VK_ERROR_VALIDATION_FAILED_EXT;

    VkResult result = dev_data->dispatch_table.CreateFramebuffer(device, pCreateInfo, pAllocator, pFramebuffer);

    if (VK_SUCCESS == result) {
        lock.lock();
        PostCallRecordCreateFramebuffer(dev_data, pCreateInfo, *pFramebuffer);
        lock.unlock();
    }
    return result;
}

static bool FindDependency(const int index, const int dependent, const std::vector<DAGNode> &subpass_to_node,
                           std::unordered_set<uint32_t> &processed_nodes) {
    // If we have already checked this node we have not found a dependency path so return false.
    if (processed_nodes.count(index))
        return false;
    processed_nodes.insert(index);
    const DAGNode &node = subpass_to_node[index];
    // Look for a dependency path. If one exists return true else recurse on the previous nodes.
    if (std::find(node.prev.begin(), node.prev.end(), dependent) == node.prev.end()) {
        for (auto elem : node.prev) {
            if (FindDependency(elem, dependent, subpass_to_node, processed_nodes))
                return true;
        }
    } else {
        return true;
    }
    return false;
}

static bool CheckDependencyExists(const layer_data *dev_data, const int subpass, const std::vector<uint32_t> &dependent_subpasses,
                                  const std::vector<DAGNode> &subpass_to_node, bool &skip_call) {
    bool result = true;
    // Loop through all subpasses that share the same attachment and make sure a dependency exists
    for (uint32_t k = 0; k < dependent_subpasses.size(); ++k) {
        if (static_cast<uint32_t>(subpass) == dependent_subpasses[k])
            continue;
        const DAGNode &node = subpass_to_node[subpass];
        // Check for a specified dependency between the two nodes. If one exists we are done.
        auto prev_elem = std::find(node.prev.begin(), node.prev.end(), dependent_subpasses[k]);
        auto next_elem = std::find(node.next.begin(), node.next.end(), dependent_subpasses[k]);
        if (prev_elem == node.prev.end() && next_elem == node.next.end()) {
            // If no dependency exits an implicit dependency still might. If not, throw an error.
            std::unordered_set<uint32_t> processed_nodes;
            if (!(FindDependency(subpass, dependent_subpasses[k], subpass_to_node, processed_nodes) ||
                FindDependency(dependent_subpasses[k], subpass, subpass_to_node, processed_nodes))) {
                skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0,
                                     __LINE__, DRAWSTATE_INVALID_RENDERPASS, "DS",
                                     "A dependency between subpasses %d and %d must exist but one is not specified.", subpass,
                                     dependent_subpasses[k]);
                result = false;
            }
        }
    }
    return result;
}

static bool CheckPreserved(const layer_data *dev_data, const VkRenderPassCreateInfo *pCreateInfo, const int index,
                           const uint32_t attachment, const std::vector<DAGNode> &subpass_to_node, int depth, bool &skip_call) {
    const DAGNode &node = subpass_to_node[index];
    // If this node writes to the attachment return true as next nodes need to preserve the attachment.
    const VkSubpassDescription &subpass = pCreateInfo->pSubpasses[index];
    for (uint32_t j = 0; j < subpass.colorAttachmentCount; ++j) {
        if (attachment == subpass.pColorAttachments[j].attachment)
            return true;
    }
    if (subpass.pDepthStencilAttachment && subpass.pDepthStencilAttachment->attachment != VK_ATTACHMENT_UNUSED) {
        if (attachment == subpass.pDepthStencilAttachment->attachment)
            return true;
    }
    bool result = false;
    // Loop through previous nodes and see if any of them write to the attachment.
    for (auto elem : node.prev) {
        result |= CheckPreserved(dev_data, pCreateInfo, elem, attachment, subpass_to_node, depth + 1, skip_call);
    }
    // If the attachment was written to by a previous node than this node needs to preserve it.
    if (result && depth > 0) {
        const VkSubpassDescription &subpass = pCreateInfo->pSubpasses[index];
        bool has_preserved = false;
        for (uint32_t j = 0; j < subpass.preserveAttachmentCount; ++j) {
            if (subpass.pPreserveAttachments[j] == attachment) {
                has_preserved = true;
                break;
            }
        }
        if (!has_preserved) {
            skip_call |=
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                        DRAWSTATE_INVALID_RENDERPASS, "DS",
                        "Attachment %d is used by a later subpass and must be preserved in subpass %d.", attachment, index);
        }
    }
    return result;
}

template <class T> bool isRangeOverlapping(T offset1, T size1, T offset2, T size2) {
    return (((offset1 + size1) > offset2) && ((offset1 + size1) < (offset2 + size2))) ||
           ((offset1 > offset2) && (offset1 < (offset2 + size2)));
}

bool isRegionOverlapping(VkImageSubresourceRange range1, VkImageSubresourceRange range2) {
    return (isRangeOverlapping(range1.baseMipLevel, range1.levelCount, range2.baseMipLevel, range2.levelCount) &&
            isRangeOverlapping(range1.baseArrayLayer, range1.layerCount, range2.baseArrayLayer, range2.layerCount));
}

static bool ValidateDependencies(const layer_data *dev_data, FRAMEBUFFER_STATE const *framebuffer,
                                 RENDER_PASS_STATE const *renderPass) {
    bool skip_call = false;
    auto const pFramebufferInfo = framebuffer->createInfo.ptr();
    auto const pCreateInfo = renderPass->createInfo.ptr();
    auto const & subpass_to_node = renderPass->subpassToNode;
    std::vector<std::vector<uint32_t>> output_attachment_to_subpass(pCreateInfo->attachmentCount);
    std::vector<std::vector<uint32_t>> input_attachment_to_subpass(pCreateInfo->attachmentCount);
    std::vector<std::vector<uint32_t>> overlapping_attachments(pCreateInfo->attachmentCount);
    // Find overlapping attachments
    for (uint32_t i = 0; i < pCreateInfo->attachmentCount; ++i) {
        for (uint32_t j = i + 1; j < pCreateInfo->attachmentCount; ++j) {
            VkImageView viewi = pFramebufferInfo->pAttachments[i];
            VkImageView viewj = pFramebufferInfo->pAttachments[j];
            if (viewi == viewj) {
                overlapping_attachments[i].push_back(j);
                overlapping_attachments[j].push_back(i);
                continue;
            }
            auto view_state_i = getImageViewState(dev_data, viewi);
            auto view_state_j = getImageViewState(dev_data, viewj);
            if (!view_state_i || !view_state_j) {
                continue;
            }
            auto view_ci_i = view_state_i->create_info;
            auto view_ci_j = view_state_j->create_info;
            if (view_ci_i.image == view_ci_j.image && isRegionOverlapping(view_ci_i.subresourceRange, view_ci_j.subresourceRange)) {
                overlapping_attachments[i].push_back(j);
                overlapping_attachments[j].push_back(i);
                continue;
            }
            auto image_data_i = getImageState(dev_data, view_ci_i.image);
            auto image_data_j = getImageState(dev_data, view_ci_j.image);
            if (!image_data_i || !image_data_j) {
                continue;
            }
            if (image_data_i->binding.mem == image_data_j->binding.mem &&
                isRangeOverlapping(image_data_i->binding.offset, image_data_i->binding.size, image_data_j->binding.offset,
                                   image_data_j->binding.size)) {
                overlapping_attachments[i].push_back(j);
                overlapping_attachments[j].push_back(i);
            }
        }
    }
    for (uint32_t i = 0; i < overlapping_attachments.size(); ++i) {
        uint32_t attachment = i;
        for (auto other_attachment : overlapping_attachments[i]) {
            if (!(pCreateInfo->pAttachments[attachment].flags & VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT)) {
                skip_call |=
                    log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                            DRAWSTATE_INVALID_RENDERPASS, "DS", "Attachment %d aliases attachment %d but doesn't "
                                                                "set VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT.",
                            attachment, other_attachment);
            }
            if (!(pCreateInfo->pAttachments[other_attachment].flags & VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT)) {
                skip_call |=
                    log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                            DRAWSTATE_INVALID_RENDERPASS, "DS", "Attachment %d aliases attachment %d but doesn't "
                                                                "set VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT.",
                            other_attachment, attachment);
            }
        }
    }
    // Find for each attachment the subpasses that use them.
    unordered_set<uint32_t> attachmentIndices;
    for (uint32_t i = 0; i < pCreateInfo->subpassCount; ++i) {
        const VkSubpassDescription &subpass = pCreateInfo->pSubpasses[i];
        attachmentIndices.clear();
        for (uint32_t j = 0; j < subpass.inputAttachmentCount; ++j) {
            uint32_t attachment = subpass.pInputAttachments[j].attachment;
            if (attachment == VK_ATTACHMENT_UNUSED)
                continue;
            input_attachment_to_subpass[attachment].push_back(i);
            for (auto overlapping_attachment : overlapping_attachments[attachment]) {
                input_attachment_to_subpass[overlapping_attachment].push_back(i);
            }
        }
        for (uint32_t j = 0; j < subpass.colorAttachmentCount; ++j) {
            uint32_t attachment = subpass.pColorAttachments[j].attachment;
            if (attachment == VK_ATTACHMENT_UNUSED)
                continue;
            output_attachment_to_subpass[attachment].push_back(i);
            for (auto overlapping_attachment : overlapping_attachments[attachment]) {
                output_attachment_to_subpass[overlapping_attachment].push_back(i);
            }
            attachmentIndices.insert(attachment);
        }
        if (subpass.pDepthStencilAttachment && subpass.pDepthStencilAttachment->attachment != VK_ATTACHMENT_UNUSED) {
            uint32_t attachment = subpass.pDepthStencilAttachment->attachment;
            output_attachment_to_subpass[attachment].push_back(i);
            for (auto overlapping_attachment : overlapping_attachments[attachment]) {
                output_attachment_to_subpass[overlapping_attachment].push_back(i);
            }

            if (attachmentIndices.count(attachment)) {
                skip_call |=
                    log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                            DRAWSTATE_INVALID_RENDERPASS, "DS",
                            "Cannot use same attachment (%u) as both color and depth output in same subpass (%u).", attachment, i);
            }
        }
    }
    // If there is a dependency needed make sure one exists
    for (uint32_t i = 0; i < pCreateInfo->subpassCount; ++i) {
        const VkSubpassDescription &subpass = pCreateInfo->pSubpasses[i];
        // If the attachment is an input then all subpasses that output must have a dependency relationship
        for (uint32_t j = 0; j < subpass.inputAttachmentCount; ++j) {
            uint32_t attachment = subpass.pInputAttachments[j].attachment;
            if (attachment == VK_ATTACHMENT_UNUSED)
                continue;
            CheckDependencyExists(dev_data, i, output_attachment_to_subpass[attachment], subpass_to_node, skip_call);
        }
        // If the attachment is an output then all subpasses that use the attachment must have a dependency relationship
        for (uint32_t j = 0; j < subpass.colorAttachmentCount; ++j) {
            uint32_t attachment = subpass.pColorAttachments[j].attachment;
            if (attachment == VK_ATTACHMENT_UNUSED)
                continue;
            CheckDependencyExists(dev_data, i, output_attachment_to_subpass[attachment], subpass_to_node, skip_call);
            CheckDependencyExists(dev_data, i, input_attachment_to_subpass[attachment], subpass_to_node, skip_call);
        }
        if (subpass.pDepthStencilAttachment && subpass.pDepthStencilAttachment->attachment != VK_ATTACHMENT_UNUSED) {
            const uint32_t &attachment = subpass.pDepthStencilAttachment->attachment;
            CheckDependencyExists(dev_data, i, output_attachment_to_subpass[attachment], subpass_to_node, skip_call);
            CheckDependencyExists(dev_data, i, input_attachment_to_subpass[attachment], subpass_to_node, skip_call);
        }
    }
    // Loop through implicit dependencies, if this pass reads make sure the attachment is preserved for all passes after it was
    // written.
    for (uint32_t i = 0; i < pCreateInfo->subpassCount; ++i) {
        const VkSubpassDescription &subpass = pCreateInfo->pSubpasses[i];
        for (uint32_t j = 0; j < subpass.inputAttachmentCount; ++j) {
            CheckPreserved(dev_data, pCreateInfo, i, subpass.pInputAttachments[j].attachment, subpass_to_node, 0, skip_call);
        }
    }
    return skip_call;
}
// ValidateLayoutVsAttachmentDescription is a general function where we can validate various state associated with the
// VkAttachmentDescription structs that are used by the sub-passes of a renderpass. Initial check is to make sure that
// READ_ONLY layout attachments don't have CLEAR as their loadOp.
static bool ValidateLayoutVsAttachmentDescription(debug_report_data *report_data, const VkImageLayout first_layout,
                                                  const uint32_t attachment,
                                                  const VkAttachmentDescription &attachment_description) {
    bool skip_call = false;
    // Verify that initial loadOp on READ_ONLY attachments is not CLEAR
    if (attachment_description.loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR) {
        if ((first_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL) ||
            (first_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)) {
            skip_call |=
                log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT,
                        VkDebugReportObjectTypeEXT(0), __LINE__, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                        "Cannot clear attachment %d with invalid first layout %s.", attachment, string_VkImageLayout(first_layout));
        }
    }
    return skip_call;
}

static bool ValidateLayouts(const layer_data *dev_data, VkDevice device, const VkRenderPassCreateInfo *pCreateInfo) {
    bool skip = false;

    // Track when we're observing the first use of an attachment
    std::vector<bool> attach_first_use(pCreateInfo->attachmentCount, true);
    for (uint32_t i = 0; i < pCreateInfo->subpassCount; ++i) {
        const VkSubpassDescription &subpass = pCreateInfo->pSubpasses[i];
        for (uint32_t j = 0; j < subpass.colorAttachmentCount; ++j) {
            auto attach_index = subpass.pColorAttachments[j].attachment;
            if (attach_index == VK_ATTACHMENT_UNUSED)
                continue;

            switch (subpass.pColorAttachments[j].layout) {
            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                /* This is ideal. */
                break;

            case VK_IMAGE_LAYOUT_GENERAL:
                /* May not be optimal; TODO: reconsider this warning based on
                 * other constraints?
                 */
                skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT,
                                VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                "Layout for color attachment is GENERAL but should be COLOR_ATTACHMENT_OPTIMAL.");
                break;

            default:
                skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                __LINE__, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                "Layout for color attachment is %s but can only be COLOR_ATTACHMENT_OPTIMAL or GENERAL.",
                                string_VkImageLayout(subpass.pColorAttachments[j].layout));
            }

            if (attach_first_use[attach_index]) {
                skip |= ValidateLayoutVsAttachmentDescription(dev_data->report_data, subpass.pColorAttachments[j].layout,
                                                              attach_index, pCreateInfo->pAttachments[attach_index]);
            }
            attach_first_use[attach_index] = false;
        }
        if (subpass.pDepthStencilAttachment && subpass.pDepthStencilAttachment->attachment != VK_ATTACHMENT_UNUSED) {
            switch (subpass.pDepthStencilAttachment->layout) {
            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
                /* These are ideal. */
                break;

            case VK_IMAGE_LAYOUT_GENERAL:
                /* May not be optimal; TODO: reconsider this warning based on
                 * other constraints? GENERAL can be better than doing a bunch
                 * of transitions.
                 */
                skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT,
                                VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                "GENERAL layout for depth attachment may not give optimal performance.");
                break;

            default:
                /* No other layouts are acceptable */
                skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                __LINE__, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                "Layout for depth attachment is %s but can only be DEPTH_STENCIL_ATTACHMENT_OPTIMAL, "
                                "DEPTH_STENCIL_READ_ONLY_OPTIMAL or GENERAL.",
                                string_VkImageLayout(subpass.pDepthStencilAttachment->layout));
            }

            auto attach_index = subpass.pDepthStencilAttachment->attachment;
            if (attach_first_use[attach_index]) {
                skip |= ValidateLayoutVsAttachmentDescription(dev_data->report_data, subpass.pDepthStencilAttachment->layout,
                                                              attach_index, pCreateInfo->pAttachments[attach_index]);
            }
            attach_first_use[attach_index] = false;
        }
        for (uint32_t j = 0; j < subpass.inputAttachmentCount; ++j) {
            auto attach_index = subpass.pInputAttachments[j].attachment;
            if (attach_index == VK_ATTACHMENT_UNUSED)
                continue;

            switch (subpass.pInputAttachments[j].layout) {
            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                /* These are ideal. */
                break;

            case VK_IMAGE_LAYOUT_GENERAL:
                /* May not be optimal. TODO: reconsider this warning based on
                 * other constraints.
                 */
                skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT,
                                VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                "Layout for input attachment is GENERAL but should be READ_ONLY_OPTIMAL.");
                break;

            default:
                /* No other layouts are acceptable */
                skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                "Layout for input attachment is %s but can only be READ_ONLY_OPTIMAL or GENERAL.",
                                string_VkImageLayout(subpass.pInputAttachments[j].layout));
            }

            if (attach_first_use[attach_index]) {
                skip |= ValidateLayoutVsAttachmentDescription(dev_data->report_data, subpass.pInputAttachments[j].layout,
                                                              attach_index, pCreateInfo->pAttachments[attach_index]);
            }
            attach_first_use[attach_index] = false;
        }
    }
    return skip;
}

static bool CreatePassDAG(const layer_data *dev_data, VkDevice device, const VkRenderPassCreateInfo *pCreateInfo,
                          std::vector<DAGNode> &subpass_to_node, std::vector<bool> &has_self_dependency) {
    bool skip_call = false;
    for (uint32_t i = 0; i < pCreateInfo->subpassCount; ++i) {
        DAGNode &subpass_node = subpass_to_node[i];
        subpass_node.pass = i;
    }
    for (uint32_t i = 0; i < pCreateInfo->dependencyCount; ++i) {
        const VkSubpassDependency &dependency = pCreateInfo->pDependencies[i];
        if (dependency.srcSubpass == VK_SUBPASS_EXTERNAL || dependency.dstSubpass == VK_SUBPASS_EXTERNAL) {
            if (dependency.srcSubpass == dependency.dstSubpass) {
                skip_call |=
                    log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                            DRAWSTATE_INVALID_RENDERPASS, "DS", "The src and dest subpasses cannot both be external.");
            }

            // We don't want to add edges to the DAG for dependencies to/from
            // VK_SUBPASS_EXTERNAL. We don't use them for anything, and their
            // presence complicates other code.
            continue;
        } else if (dependency.srcSubpass > dependency.dstSubpass) {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                 DRAWSTATE_INVALID_RENDERPASS, "DS",
                                 "Depedency graph must be specified such that an earlier pass cannot depend on a later pass.");
        } else if (dependency.srcSubpass == dependency.dstSubpass) {
            has_self_dependency[dependency.srcSubpass] = true;
        }

        subpass_to_node[dependency.dstSubpass].prev.push_back(dependency.srcSubpass);
        subpass_to_node[dependency.srcSubpass].next.push_back(dependency.dstSubpass);
    }
    return skip_call;
}


VKAPI_ATTR VkResult VKAPI_CALL CreateShaderModule(VkDevice device, const VkShaderModuleCreateInfo *pCreateInfo,
                                                  const VkAllocationCallbacks *pAllocator,
                                                  VkShaderModule *pShaderModule) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    bool skip_call = false;

    /* Use SPIRV-Tools validator to try and catch any issues with the module itself */
    spv_context ctx = spvContextCreate(SPV_ENV_VULKAN_1_0);
    spv_const_binary_t binary { pCreateInfo->pCode, pCreateInfo->codeSize / sizeof(uint32_t) };
    spv_diagnostic diag = nullptr;

    auto result = spvValidate(ctx, &binary, &diag);
    if (result != SPV_SUCCESS) {
        skip_call |=
            log_msg(dev_data->report_data, result == SPV_WARNING ? VK_DEBUG_REPORT_WARNING_BIT_EXT : VK_DEBUG_REPORT_ERROR_BIT_EXT,
                    VkDebugReportObjectTypeEXT(0), 0, __LINE__, SHADER_CHECKER_INCONSISTENT_SPIRV, "SC",
                    "SPIR-V module not valid: %s", diag && diag->error ? diag->error : "(no error text)");
    }

    spvDiagnosticDestroy(diag);
    spvContextDestroy(ctx);

    if (skip_call)
        return VK_ERROR_VALIDATION_FAILED_EXT;

    VkResult res = dev_data->dispatch_table.CreateShaderModule(device, pCreateInfo, pAllocator, pShaderModule);

    if (res == VK_SUCCESS) {
        std::lock_guard<std::mutex> lock(global_lock);
        dev_data->shaderModuleMap[*pShaderModule] = unique_ptr<shader_module>(new shader_module(pCreateInfo));
    }
    return res;
}

static bool ValidateAttachmentIndex(layer_data *dev_data, uint32_t attachment, uint32_t attachment_count, const char *type) {
    bool skip_call = false;
    if (attachment >= attachment_count && attachment != VK_ATTACHMENT_UNUSED) {
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                             DRAWSTATE_INVALID_ATTACHMENT_INDEX, "DS",
                             "CreateRenderPass: %s attachment %d cannot be greater than the total number of attachments %d.",
                             type, attachment, attachment_count);
    }
    return skip_call;
}

static bool IsPowerOfTwo(unsigned x) {
    return x && !(x & (x-1));
}

static bool ValidateRenderpassAttachmentUsage(layer_data *dev_data, const VkRenderPassCreateInfo *pCreateInfo) {
    bool skip_call = false;
    for (uint32_t i = 0; i < pCreateInfo->subpassCount; ++i) {
        const VkSubpassDescription &subpass = pCreateInfo->pSubpasses[i];
        if (subpass.pipelineBindPoint != VK_PIPELINE_BIND_POINT_GRAPHICS) {
            skip_call |=
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                        DRAWSTATE_INVALID_RENDERPASS, "DS",
                        "CreateRenderPass: Pipeline bind point for subpass %d must be VK_PIPELINE_BIND_POINT_GRAPHICS.", i);
        }
        for (uint32_t j = 0; j < subpass.preserveAttachmentCount; ++j) {
            uint32_t attachment = subpass.pPreserveAttachments[j];
            if (attachment == VK_ATTACHMENT_UNUSED) {
                skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0,
                                     __LINE__, DRAWSTATE_INVALID_ATTACHMENT_INDEX, "DS",
                                     "CreateRenderPass:  Preserve attachment (%d) must not be VK_ATTACHMENT_UNUSED.", j);
            } else {
                skip_call |= ValidateAttachmentIndex(dev_data, attachment, pCreateInfo->attachmentCount, "Preserve");
            }
        }

        auto subpass_performs_resolve = subpass.pResolveAttachments && std::any_of(
            subpass.pResolveAttachments, subpass.pResolveAttachments + subpass.colorAttachmentCount,
            [](VkAttachmentReference ref) { return ref.attachment != VK_ATTACHMENT_UNUSED; });

        unsigned sample_count = 0;

        for (uint32_t j = 0; j < subpass.colorAttachmentCount; ++j) {
            uint32_t attachment;
            if (subpass.pResolveAttachments) {
                attachment = subpass.pResolveAttachments[j].attachment;
                skip_call |= ValidateAttachmentIndex(dev_data, attachment, pCreateInfo->attachmentCount, "Resolve");

                if (!skip_call && attachment != VK_ATTACHMENT_UNUSED &&
                    pCreateInfo->pAttachments[attachment].samples != VK_SAMPLE_COUNT_1_BIT) {
                    skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0,
                                         __LINE__, DRAWSTATE_INVALID_RENDERPASS, "DS",
                                         "CreateRenderPass:  Subpass %u requests multisample resolve into attachment %u, "
                                         "which must have VK_SAMPLE_COUNT_1_BIT but has %s",
                                         i, attachment, string_VkSampleCountFlagBits(pCreateInfo->pAttachments[attachment].samples));
                }
            }
            attachment = subpass.pColorAttachments[j].attachment;
            skip_call |= ValidateAttachmentIndex(dev_data, attachment, pCreateInfo->attachmentCount, "Color");

            if (!skip_call && attachment != VK_ATTACHMENT_UNUSED) {
                sample_count |= (unsigned)pCreateInfo->pAttachments[attachment].samples;

                if (subpass_performs_resolve &&
                    pCreateInfo->pAttachments[attachment].samples == VK_SAMPLE_COUNT_1_BIT) {
                    skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0,
                                         __LINE__, DRAWSTATE_INVALID_RENDERPASS, "DS",
                                         "CreateRenderPass:  Subpass %u requests multisample resolve from attachment %u "
                                         "which has VK_SAMPLE_COUNT_1_BIT",
                                         i, attachment);
                }
            }
        }

        if (subpass.pDepthStencilAttachment && subpass.pDepthStencilAttachment->attachment != VK_ATTACHMENT_UNUSED) {
            uint32_t attachment = subpass.pDepthStencilAttachment->attachment;
            skip_call |= ValidateAttachmentIndex(dev_data, attachment, pCreateInfo->attachmentCount, "Depth stencil");

            if (!skip_call && attachment != VK_ATTACHMENT_UNUSED) {
                sample_count |= (unsigned)pCreateInfo->pAttachments[attachment].samples;
            }
        }

        for (uint32_t j = 0; j < subpass.inputAttachmentCount; ++j) {
            uint32_t attachment = subpass.pInputAttachments[j].attachment;
            skip_call |= ValidateAttachmentIndex(dev_data, attachment, pCreateInfo->attachmentCount, "Input");
        }

        if (sample_count && !IsPowerOfTwo(sample_count)) {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0,
                                 __LINE__, DRAWSTATE_INVALID_RENDERPASS, "DS",
                                 "CreateRenderPass:  Subpass %u attempts to render to "
                                 "attachments with inconsistent sample counts",
                                 i);
        }
    }
    return skip_call;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateRenderPass(VkDevice device, const VkRenderPassCreateInfo *pCreateInfo,
                                                const VkAllocationCallbacks *pAllocator, VkRenderPass *pRenderPass) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    std::unique_lock<std::mutex> lock(global_lock);

    skip_call |= ValidateLayouts(dev_data, device, pCreateInfo);
    // TODO: As part of wrapping up the mem_tracker/core_validation merge the following routine should be consolidated with
    //       ValidateLayouts.
    skip_call |= ValidateRenderpassAttachmentUsage(dev_data, pCreateInfo);
    lock.unlock();

    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }

    VkResult result = dev_data->dispatch_table.CreateRenderPass(device, pCreateInfo, pAllocator, pRenderPass);

    if (VK_SUCCESS == result) {
        lock.lock();

        std::vector<bool> has_self_dependency(pCreateInfo->subpassCount);
        std::vector<DAGNode> subpass_to_node(pCreateInfo->subpassCount);
        skip_call |= CreatePassDAG(dev_data, device, pCreateInfo, subpass_to_node, has_self_dependency);

        auto render_pass = unique_ptr<RENDER_PASS_STATE>(new RENDER_PASS_STATE(pCreateInfo));
        render_pass->renderPass = *pRenderPass;
        render_pass->hasSelfDependency = has_self_dependency;
        render_pass->subpassToNode = subpass_to_node;

        // TODO: Maybe fill list and then copy instead of locking
        std::unordered_map<uint32_t, bool> &attachment_first_read = render_pass->attachment_first_read;
        std::unordered_map<uint32_t, VkImageLayout> &attachment_first_layout = render_pass->attachment_first_layout;
        for (uint32_t i = 0; i < pCreateInfo->subpassCount; ++i) {
            const VkSubpassDescription &subpass = pCreateInfo->pSubpasses[i];
            for (uint32_t j = 0; j < subpass.colorAttachmentCount; ++j) {
                uint32_t attachment = subpass.pColorAttachments[j].attachment;
                if (!attachment_first_read.count(attachment)) {
                    attachment_first_read.insert(std::make_pair(attachment, false));
                    attachment_first_layout.insert(std::make_pair(attachment, subpass.pColorAttachments[j].layout));
                }
            }
            if (subpass.pDepthStencilAttachment && subpass.pDepthStencilAttachment->attachment != VK_ATTACHMENT_UNUSED) {
                uint32_t attachment = subpass.pDepthStencilAttachment->attachment;
                if (!attachment_first_read.count(attachment)) {
                    attachment_first_read.insert(std::make_pair(attachment, false));
                    attachment_first_layout.insert(std::make_pair(attachment, subpass.pDepthStencilAttachment->layout));
                }
            }
            for (uint32_t j = 0; j < subpass.inputAttachmentCount; ++j) {
                uint32_t attachment = subpass.pInputAttachments[j].attachment;
                if (!attachment_first_read.count(attachment)) {
                    attachment_first_read.insert(std::make_pair(attachment, true));
                    attachment_first_layout.insert(std::make_pair(attachment, subpass.pInputAttachments[j].layout));
                }
            }
        }

        dev_data->renderPassMap[*pRenderPass] = std::move(render_pass);
    }
    return result;
}

static bool VerifyFramebufferAndRenderPassLayouts(layer_data *dev_data, GLOBAL_CB_NODE *pCB, const VkRenderPassBeginInfo *pRenderPassBegin) {
    bool skip_call = false;
    auto const pRenderPassInfo = getRenderPassState(dev_data, pRenderPassBegin->renderPass)->createInfo.ptr();
    auto const & framebufferInfo = dev_data->frameBufferMap[pRenderPassBegin->framebuffer]->createInfo;
    if (pRenderPassInfo->attachmentCount != framebufferInfo.attachmentCount) {
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                             DRAWSTATE_INVALID_RENDERPASS, "DS", "You cannot start a render pass using a framebuffer "
                                                                 "with a different number of attachments.");
    }
    for (uint32_t i = 0; i < pRenderPassInfo->attachmentCount; ++i) {
        const VkImageView &image_view = framebufferInfo.pAttachments[i];
        auto view_state = getImageViewState(dev_data, image_view);
        assert(view_state);
        const VkImage &image = view_state->create_info.image;
        const VkImageSubresourceRange &subRange = view_state->create_info.subresourceRange;
        IMAGE_CMD_BUF_LAYOUT_NODE newNode = {pRenderPassInfo->pAttachments[i].initialLayout,
                                             pRenderPassInfo->pAttachments[i].initialLayout};
        // TODO: Do not iterate over every possibility - consolidate where possible
        for (uint32_t j = 0; j < subRange.levelCount; j++) {
            uint32_t level = subRange.baseMipLevel + j;
            for (uint32_t k = 0; k < subRange.layerCount; k++) {
                uint32_t layer = subRange.baseArrayLayer + k;
                VkImageSubresource sub = {subRange.aspectMask, level, layer};
                IMAGE_CMD_BUF_LAYOUT_NODE node;
                if (!FindLayout(pCB, image, sub, node)) {
                    SetLayout(pCB, image, sub, newNode);
                    continue;
                }
                if (newNode.layout != VK_IMAGE_LAYOUT_UNDEFINED &&
                    newNode.layout != node.layout) {
                    skip_call |=
                        log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                DRAWSTATE_INVALID_RENDERPASS, "DS",
                                "You cannot start a render pass using attachment %u "
                                "where the render pass initial layout is %s and the previous "
                                "known layout of the attachment is %s. The layouts must match, or "
                                "the render pass initial layout for the attachment must be "
                                "VK_IMAGE_LAYOUT_UNDEFINED",
                                i, string_VkImageLayout(newNode.layout), string_VkImageLayout(node.layout));
                }
            }
        }
    }
    return skip_call;
}

static void TransitionAttachmentRefLayout(layer_data *dev_data, GLOBAL_CB_NODE *pCB, FRAMEBUFFER_STATE *pFramebuffer,
                                          VkAttachmentReference ref) {
    if (ref.attachment != VK_ATTACHMENT_UNUSED) {
        auto image_view = pFramebuffer->createInfo.pAttachments[ref.attachment];
        SetLayout(dev_data, pCB, image_view, ref.layout);
    }
}

static void TransitionSubpassLayouts(layer_data *dev_data, GLOBAL_CB_NODE *pCB, const VkRenderPassBeginInfo *pRenderPassBegin,
                                     const int subpass_index) {
    auto renderPass = getRenderPassState(dev_data, pRenderPassBegin->renderPass);
    if (!renderPass)
        return;

    auto framebuffer = getFramebufferState(dev_data, pRenderPassBegin->framebuffer);
    if (!framebuffer)
        return;

    auto const &subpass = renderPass->createInfo.pSubpasses[subpass_index];
    for (uint32_t j = 0; j < subpass.inputAttachmentCount; ++j) {
        TransitionAttachmentRefLayout(dev_data, pCB, framebuffer, subpass.pInputAttachments[j]);
    }
    for (uint32_t j = 0; j < subpass.colorAttachmentCount; ++j) {
        TransitionAttachmentRefLayout(dev_data, pCB, framebuffer, subpass.pColorAttachments[j]);
    }
    if (subpass.pDepthStencilAttachment) {
        TransitionAttachmentRefLayout(dev_data, pCB, framebuffer, *subpass.pDepthStencilAttachment);
    }
}

static bool validatePrimaryCommandBuffer(const layer_data *dev_data, const GLOBAL_CB_NODE *pCB, const std::string &cmd_name) {
    bool skip_call = false;
    if (pCB->createInfo.level != VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                             DRAWSTATE_INVALID_COMMAND_BUFFER, "DS", "Cannot execute command %s on a secondary command buffer.",
                             cmd_name.c_str());
    }
    return skip_call;
}

static void TransitionFinalSubpassLayouts(layer_data *dev_data, GLOBAL_CB_NODE *pCB, const VkRenderPassBeginInfo *pRenderPassBegin) {
    auto renderPass = getRenderPassState(dev_data, pRenderPassBegin->renderPass);
    if (!renderPass)
        return;

    const VkRenderPassCreateInfo *pRenderPassInfo = renderPass->createInfo.ptr();
    auto framebuffer = getFramebufferState(dev_data, pRenderPassBegin->framebuffer);
    if (!framebuffer)
        return;

    for (uint32_t i = 0; i < pRenderPassInfo->attachmentCount; ++i) {
        auto image_view = framebuffer->createInfo.pAttachments[i];
        SetLayout(dev_data, pCB, image_view, pRenderPassInfo->pAttachments[i].finalLayout);
    }
}

static bool VerifyRenderAreaBounds(const layer_data *dev_data, const VkRenderPassBeginInfo *pRenderPassBegin) {
    bool skip_call = false;
    const safe_VkFramebufferCreateInfo *pFramebufferInfo =
        &getFramebufferState(dev_data, pRenderPassBegin->framebuffer)->createInfo;
    if (pRenderPassBegin->renderArea.offset.x < 0 ||
        (pRenderPassBegin->renderArea.offset.x + pRenderPassBegin->renderArea.extent.width) > pFramebufferInfo->width ||
        pRenderPassBegin->renderArea.offset.y < 0 ||
        (pRenderPassBegin->renderArea.offset.y + pRenderPassBegin->renderArea.extent.height) > pFramebufferInfo->height) {
        skip_call |= static_cast<bool>(log_msg(
            dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
            DRAWSTATE_INVALID_RENDER_AREA, "CORE",
            "Cannot execute a render pass with renderArea not within the bound of the "
            "framebuffer. RenderArea: x %d, y %d, width %d, height %d. Framebuffer: width %d, "
            "height %d.",
            pRenderPassBegin->renderArea.offset.x, pRenderPassBegin->renderArea.offset.y, pRenderPassBegin->renderArea.extent.width,
            pRenderPassBegin->renderArea.extent.height, pFramebufferInfo->width, pFramebufferInfo->height));
    }
    return skip_call;
}

// If this is a stencil format, make sure the stencil[Load|Store]Op flag is checked, while if it is a depth/color attachment the
// [load|store]Op flag must be checked
// TODO: The memory valid flag in DEVICE_MEM_INFO should probably be split to track the validity of stencil memory separately.
template <typename T> static bool FormatSpecificLoadAndStoreOpSettings(VkFormat format, T color_depth_op, T stencil_op, T op) {
    if (color_depth_op != op && stencil_op != op) {
        return false;
    }
    bool check_color_depth_load_op = !vk_format_is_stencil_only(format);
    bool check_stencil_load_op = vk_format_is_depth_and_stencil(format) || !check_color_depth_load_op;

    return (((check_color_depth_load_op == true) && (color_depth_op == op)) ||
            ((check_stencil_load_op == true) && (stencil_op == op)));
}

VKAPI_ATTR void VKAPI_CALL
CmdBeginRenderPass(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo *pRenderPassBegin, VkSubpassContents contents) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *cb_node = getCBNode(dev_data, commandBuffer);
    auto renderPass = pRenderPassBegin ? getRenderPassState(dev_data, pRenderPassBegin->renderPass) : nullptr;
    auto framebuffer = pRenderPassBegin ? getFramebufferState(dev_data, pRenderPassBegin->framebuffer) : nullptr;
    if (cb_node) {
        if (renderPass) {
            uint32_t clear_op_size = 0; // Make sure pClearValues is at least as large as last LOAD_OP_CLEAR
            cb_node->activeFramebuffer = pRenderPassBegin->framebuffer;
            for (uint32_t i = 0; i < renderPass->createInfo.attachmentCount; ++i) {
                MT_FB_ATTACHMENT_INFO &fb_info = framebuffer->attachments[i];
                auto pAttachment = &renderPass->createInfo.pAttachments[i];
                if (FormatSpecificLoadAndStoreOpSettings(pAttachment->format, pAttachment->loadOp,
                                                         pAttachment->stencilLoadOp,
                                                         VK_ATTACHMENT_LOAD_OP_CLEAR)) {
                    clear_op_size = static_cast<uint32_t>(i) + 1;
                    std::function<bool()> function = [=]() {
                        SetImageMemoryValid(dev_data, getImageState(dev_data, fb_info.image), true);
                        return false;
                    };
                    cb_node->validate_functions.push_back(function);
                } else if (FormatSpecificLoadAndStoreOpSettings(pAttachment->format, pAttachment->loadOp,
                                                                pAttachment->stencilLoadOp,
                                                                VK_ATTACHMENT_LOAD_OP_DONT_CARE)) {
                    std::function<bool()> function = [=]() {
                        SetImageMemoryValid(dev_data, getImageState(dev_data, fb_info.image), false);
                        return false;
                    };
                    cb_node->validate_functions.push_back(function);
                } else if (FormatSpecificLoadAndStoreOpSettings(pAttachment->format, pAttachment->loadOp,
                                                                pAttachment->stencilLoadOp,
                                                                VK_ATTACHMENT_LOAD_OP_LOAD)) {
                    std::function<bool()> function = [=]() {
                        return ValidateImageMemoryIsValid(dev_data, getImageState(dev_data, fb_info.image),
                                                          "vkCmdBeginRenderPass()");
                    };
                    cb_node->validate_functions.push_back(function);
                }
                if (renderPass->attachment_first_read[i]) {
                    std::function<bool()> function = [=]() {
                        return ValidateImageMemoryIsValid(dev_data, getImageState(dev_data, fb_info.image),
                                                          "vkCmdBeginRenderPass()");
                    };
                    cb_node->validate_functions.push_back(function);
                }
            }
            if (clear_op_size > pRenderPassBegin->clearValueCount) {
                skip_call |=
                    log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT,
                            reinterpret_cast<uint64_t &>(renderPass), __LINE__, VALIDATION_ERROR_00442, "DS",
                            "In vkCmdBeginRenderPass() the VkRenderPassBeginInfo struct has a clearValueCount of %u but there must "
                            "be at least %u "
                            "entries in pClearValues array to account for the highest index attachment in renderPass 0x%" PRIx64
                            " that uses VK_ATTACHMENT_LOAD_OP_CLEAR is %u. Note that the pClearValues array "
                            "is indexed by attachment number so even if some pClearValues entries between 0 and %u correspond to "
                            "attachments that aren't cleared they will be ignored. %s",
                            pRenderPassBegin->clearValueCount, clear_op_size, reinterpret_cast<uint64_t &>(renderPass),
                            clear_op_size, clear_op_size - 1, validation_error_map[VALIDATION_ERROR_00442]);
            }
            skip_call |= VerifyRenderAreaBounds(dev_data, pRenderPassBegin);
            skip_call |= VerifyFramebufferAndRenderPassLayouts(dev_data, cb_node, pRenderPassBegin);
            skip_call |= insideRenderPass(dev_data, cb_node, "vkCmdBeginRenderPass");
            skip_call |= ValidateDependencies(dev_data, framebuffer, renderPass);
            skip_call |= validatePrimaryCommandBuffer(dev_data, cb_node, "vkCmdBeginRenderPass");
            skip_call |= addCmd(dev_data, cb_node, CMD_BEGINRENDERPASS, "vkCmdBeginRenderPass()");
            cb_node->activeRenderPass = renderPass;
            // This is a shallow copy as that is all that is needed for now
            cb_node->activeRenderPassBeginInfo = *pRenderPassBegin;
            cb_node->activeSubpass = 0;
            cb_node->activeSubpassContents = contents;
            cb_node->framebuffers.insert(pRenderPassBegin->framebuffer);
            // Connect this framebuffer and its children to this cmdBuffer
            AddFramebufferBinding(dev_data, cb_node, framebuffer);
            // transition attachments to the correct layouts for the first subpass
            TransitionSubpassLayouts(dev_data, cb_node, &cb_node->activeRenderPassBeginInfo, cb_node->activeSubpass);
        } else {
            skip_call |=
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                        DRAWSTATE_INVALID_RENDERPASS, "DS", "You cannot use a NULL RenderPass object in vkCmdBeginRenderPass()");
        }
    }
    lock.unlock();
    if (!skip_call) {
        dev_data->dispatch_table.CmdBeginRenderPass(commandBuffer, pRenderPassBegin, contents);
    }
}

VKAPI_ATTR void VKAPI_CALL CmdNextSubpass(VkCommandBuffer commandBuffer, VkSubpassContents contents) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        skip_call |= validatePrimaryCommandBuffer(dev_data, pCB, "vkCmdNextSubpass");
        skip_call |= addCmd(dev_data, pCB, CMD_NEXTSUBPASS, "vkCmdNextSubpass()");
        skip_call |= outsideRenderPass(dev_data, pCB, "vkCmdNextSubpass");

        auto subpassCount = pCB->activeRenderPass->createInfo.subpassCount;
        if (pCB->activeSubpass == subpassCount - 1) {
            skip_call |=
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        reinterpret_cast<uint64_t>(commandBuffer), __LINE__, DRAWSTATE_INVALID_SUBPASS_INDEX, "DS",
                        "vkCmdNextSubpass(): Attempted to advance beyond final subpass");
        }
    }
    lock.unlock();

    if (skip_call)
        return;

    dev_data->dispatch_table.CmdNextSubpass(commandBuffer, contents);

    if (pCB) {
      lock.lock();
      pCB->activeSubpass++;
      pCB->activeSubpassContents = contents;
      TransitionSubpassLayouts(dev_data, pCB, &pCB->activeRenderPassBeginInfo, pCB->activeSubpass);
    }
}

VKAPI_ATTR void VKAPI_CALL CmdEndRenderPass(VkCommandBuffer commandBuffer) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    auto pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        RENDER_PASS_STATE *rp_state = pCB->activeRenderPass;
        auto framebuffer = getFramebufferState(dev_data, pCB->activeFramebuffer);
        if (rp_state) {
            if (pCB->activeSubpass != rp_state->createInfo.subpassCount - 1) {
                skip_call |=
                    log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            reinterpret_cast<uint64_t>(commandBuffer), __LINE__, DRAWSTATE_INVALID_SUBPASS_INDEX, "DS",
                            "vkCmdEndRenderPass(): Called before reaching final subpass");
            }

            for (size_t i = 0; i < rp_state->createInfo.attachmentCount; ++i) {
                MT_FB_ATTACHMENT_INFO &fb_info = framebuffer->attachments[i];
                auto pAttachment = &rp_state->createInfo.pAttachments[i];
                if (FormatSpecificLoadAndStoreOpSettings(pAttachment->format, pAttachment->storeOp,
                                                         pAttachment->stencilStoreOp, VK_ATTACHMENT_STORE_OP_STORE)) {
                    std::function<bool()> function = [=]() {
                        SetImageMemoryValid(dev_data, getImageState(dev_data, fb_info.image), true);
                        return false;
                    };
                    pCB->validate_functions.push_back(function);
                } else if (FormatSpecificLoadAndStoreOpSettings(pAttachment->format, pAttachment->storeOp,
                                                                pAttachment->stencilStoreOp,
                                                                VK_ATTACHMENT_STORE_OP_DONT_CARE)) {
                    std::function<bool()> function = [=]() {
                        SetImageMemoryValid(dev_data, getImageState(dev_data, fb_info.image), false);
                        return false;
                    };
                    pCB->validate_functions.push_back(function);
                }
            }
        }
        skip_call |= outsideRenderPass(dev_data, pCB, "vkCmdEndRenderpass");
        skip_call |= validatePrimaryCommandBuffer(dev_data, pCB, "vkCmdEndRenderPass");
        skip_call |= addCmd(dev_data, pCB, CMD_ENDRENDERPASS, "vkCmdEndRenderPass()");
    }
    lock.unlock();

    if (skip_call)
        return;

    dev_data->dispatch_table.CmdEndRenderPass(commandBuffer);

    if (pCB) {
        lock.lock();
        TransitionFinalSubpassLayouts(dev_data, pCB, &pCB->activeRenderPassBeginInfo);
        pCB->activeRenderPass = nullptr;
        pCB->activeSubpass = 0;
        pCB->activeFramebuffer = VK_NULL_HANDLE;
    }
}

static bool logInvalidAttachmentMessage(layer_data *dev_data, VkCommandBuffer secondaryBuffer, uint32_t primaryAttach,
                                        uint32_t secondaryAttach, const char *msg) {
    return log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                   DRAWSTATE_INVALID_SECONDARY_COMMAND_BUFFER, "DS",
                   "vkCmdExecuteCommands() called w/ invalid Secondary Cmd Buffer 0x%" PRIx64 " which has a render pass "
                   "that is not compatible with the Primary Cmd Buffer current render pass. "
                   "Attachment %u is not compatible with %u: %s",
                   reinterpret_cast<uint64_t &>(secondaryBuffer), primaryAttach, secondaryAttach, msg);
}

static bool validateAttachmentCompatibility(layer_data *dev_data, VkCommandBuffer primaryBuffer,
                                            VkRenderPassCreateInfo const *primaryPassCI, uint32_t primaryAttach,
                                            VkCommandBuffer secondaryBuffer, VkRenderPassCreateInfo const *secondaryPassCI,
                                            uint32_t secondaryAttach, bool is_multi) {
    bool skip_call = false;
    if (primaryPassCI->attachmentCount <= primaryAttach) {
        primaryAttach = VK_ATTACHMENT_UNUSED;
    }
    if (secondaryPassCI->attachmentCount <= secondaryAttach) {
        secondaryAttach = VK_ATTACHMENT_UNUSED;
    }
    if (primaryAttach == VK_ATTACHMENT_UNUSED && secondaryAttach == VK_ATTACHMENT_UNUSED) {
        return skip_call;
    }
    if (primaryAttach == VK_ATTACHMENT_UNUSED) {
        skip_call |= logInvalidAttachmentMessage(dev_data, secondaryBuffer, primaryAttach, secondaryAttach,
                                                 "The first is unused while the second is not.");
        return skip_call;
    }
    if (secondaryAttach == VK_ATTACHMENT_UNUSED) {
        skip_call |= logInvalidAttachmentMessage(dev_data, secondaryBuffer, primaryAttach, secondaryAttach,
                                                 "The second is unused while the first is not.");
        return skip_call;
    }
    if (primaryPassCI->pAttachments[primaryAttach].format != secondaryPassCI->pAttachments[secondaryAttach].format) {
        skip_call |=
            logInvalidAttachmentMessage(dev_data, secondaryBuffer, primaryAttach, secondaryAttach, "They have different formats.");
    }
    if (primaryPassCI->pAttachments[primaryAttach].samples != secondaryPassCI->pAttachments[secondaryAttach].samples) {
        skip_call |=
            logInvalidAttachmentMessage(dev_data, secondaryBuffer, primaryAttach, secondaryAttach, "They have different samples.");
    }
    if (is_multi && primaryPassCI->pAttachments[primaryAttach].flags != secondaryPassCI->pAttachments[secondaryAttach].flags) {
        skip_call |=
            logInvalidAttachmentMessage(dev_data, secondaryBuffer, primaryAttach, secondaryAttach, "They have different flags.");
    }
    return skip_call;
}

static bool validateSubpassCompatibility(layer_data *dev_data, VkCommandBuffer primaryBuffer,
                                         VkRenderPassCreateInfo const *primaryPassCI, VkCommandBuffer secondaryBuffer,
                                         VkRenderPassCreateInfo const *secondaryPassCI, const int subpass, bool is_multi) {
    bool skip_call = false;
    const VkSubpassDescription &primary_desc = primaryPassCI->pSubpasses[subpass];
    const VkSubpassDescription &secondary_desc = secondaryPassCI->pSubpasses[subpass];
    uint32_t maxInputAttachmentCount = std::max(primary_desc.inputAttachmentCount, secondary_desc.inputAttachmentCount);
    for (uint32_t i = 0; i < maxInputAttachmentCount; ++i) {
        uint32_t primary_input_attach = VK_ATTACHMENT_UNUSED, secondary_input_attach = VK_ATTACHMENT_UNUSED;
        if (i < primary_desc.inputAttachmentCount) {
            primary_input_attach = primary_desc.pInputAttachments[i].attachment;
        }
        if (i < secondary_desc.inputAttachmentCount) {
            secondary_input_attach = secondary_desc.pInputAttachments[i].attachment;
        }
        skip_call |= validateAttachmentCompatibility(dev_data, primaryBuffer, primaryPassCI, primary_input_attach, secondaryBuffer,
                                                     secondaryPassCI, secondary_input_attach, is_multi);
    }
    uint32_t maxColorAttachmentCount = std::max(primary_desc.colorAttachmentCount, secondary_desc.colorAttachmentCount);
    for (uint32_t i = 0; i < maxColorAttachmentCount; ++i) {
        uint32_t primary_color_attach = VK_ATTACHMENT_UNUSED, secondary_color_attach = VK_ATTACHMENT_UNUSED;
        if (i < primary_desc.colorAttachmentCount) {
            primary_color_attach = primary_desc.pColorAttachments[i].attachment;
        }
        if (i < secondary_desc.colorAttachmentCount) {
            secondary_color_attach = secondary_desc.pColorAttachments[i].attachment;
        }
        skip_call |= validateAttachmentCompatibility(dev_data, primaryBuffer, primaryPassCI, primary_color_attach, secondaryBuffer,
                                                     secondaryPassCI, secondary_color_attach, is_multi);
        uint32_t primary_resolve_attach = VK_ATTACHMENT_UNUSED, secondary_resolve_attach = VK_ATTACHMENT_UNUSED;
        if (i < primary_desc.colorAttachmentCount && primary_desc.pResolveAttachments) {
            primary_resolve_attach = primary_desc.pResolveAttachments[i].attachment;
        }
        if (i < secondary_desc.colorAttachmentCount && secondary_desc.pResolveAttachments) {
            secondary_resolve_attach = secondary_desc.pResolveAttachments[i].attachment;
        }
        skip_call |= validateAttachmentCompatibility(dev_data, primaryBuffer, primaryPassCI, primary_resolve_attach,
                                                     secondaryBuffer, secondaryPassCI, secondary_resolve_attach, is_multi);
    }
    uint32_t primary_depthstencil_attach = VK_ATTACHMENT_UNUSED, secondary_depthstencil_attach = VK_ATTACHMENT_UNUSED;
    if (primary_desc.pDepthStencilAttachment) {
        primary_depthstencil_attach = primary_desc.pDepthStencilAttachment[0].attachment;
    }
    if (secondary_desc.pDepthStencilAttachment) {
        secondary_depthstencil_attach = secondary_desc.pDepthStencilAttachment[0].attachment;
    }
    skip_call |= validateAttachmentCompatibility(dev_data, primaryBuffer, primaryPassCI, primary_depthstencil_attach,
                                                 secondaryBuffer, secondaryPassCI, secondary_depthstencil_attach, is_multi);
    return skip_call;
}

// Verify that given renderPass CreateInfo for primary and secondary command buffers are compatible.
//  This function deals directly with the CreateInfo, there are overloaded versions below that can take the renderPass handle and
//  will then feed into this function
static bool validateRenderPassCompatibility(layer_data *dev_data, VkCommandBuffer primaryBuffer,
                                            VkRenderPassCreateInfo const *primaryPassCI, VkCommandBuffer secondaryBuffer,
                                            VkRenderPassCreateInfo const *secondaryPassCI) {
    bool skip_call = false;

    if (primaryPassCI->subpassCount != secondaryPassCI->subpassCount) {
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                             DRAWSTATE_INVALID_SECONDARY_COMMAND_BUFFER, "DS",
                             "vkCmdExecuteCommands() called w/ invalid secondary Cmd Buffer 0x%" PRIx64
                             " that has a subpassCount of %u that is incompatible with the primary Cmd Buffer 0x%" PRIx64
                             " that has a subpassCount of %u.",
                             reinterpret_cast<uint64_t &>(secondaryBuffer), secondaryPassCI->subpassCount,
                             reinterpret_cast<uint64_t &>(primaryBuffer), primaryPassCI->subpassCount);
    } else {
        for (uint32_t i = 0; i < primaryPassCI->subpassCount; ++i) {
            skip_call |= validateSubpassCompatibility(dev_data, primaryBuffer, primaryPassCI, secondaryBuffer, secondaryPassCI, i,
                                                      primaryPassCI->subpassCount > 1);
        }
    }
    return skip_call;
}

static bool validateFramebuffer(layer_data *dev_data, VkCommandBuffer primaryBuffer, const GLOBAL_CB_NODE *pCB,
                                VkCommandBuffer secondaryBuffer, const GLOBAL_CB_NODE *pSubCB) {
    bool skip_call = false;
    if (!pSubCB->beginInfo.pInheritanceInfo) {
        return skip_call;
    }
    VkFramebuffer primary_fb = pCB->activeFramebuffer;
    VkFramebuffer secondary_fb = pSubCB->beginInfo.pInheritanceInfo->framebuffer;
    if (secondary_fb != VK_NULL_HANDLE) {
        if (primary_fb != secondary_fb) {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                 DRAWSTATE_FRAMEBUFFER_INCOMPATIBLE, "DS",
                                 "vkCmdExecuteCommands() called w/ invalid secondary command buffer 0x%" PRIx64
                                 " which has a framebuffer 0x%" PRIx64
                                 " that is not the same as the primary command buffer's current active framebuffer 0x%" PRIx64 ".",
                                 reinterpret_cast<uint64_t &>(secondaryBuffer), reinterpret_cast<uint64_t &>(secondary_fb),
                                 reinterpret_cast<uint64_t &>(primary_fb));
        }
        auto fb = getFramebufferState(dev_data, secondary_fb);
        if (!fb) {
            skip_call |=
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                        DRAWSTATE_INVALID_SECONDARY_COMMAND_BUFFER, "DS", "vkCmdExecuteCommands() called w/ invalid Cmd Buffer 0x%p "
                                                                          "which has invalid framebuffer 0x%" PRIx64 ".",
                        (void *)secondaryBuffer, (uint64_t)(secondary_fb));
            return skip_call;
        }
        auto cb_renderpass = getRenderPassState(dev_data, pSubCB->beginInfo.pInheritanceInfo->renderPass);
        if (cb_renderpass->renderPass != fb->createInfo.renderPass) {
            skip_call |= validateRenderPassCompatibility(dev_data, secondaryBuffer, fb->renderPassCreateInfo.ptr(), secondaryBuffer,
                                                         cb_renderpass->createInfo.ptr());
        }
    }
    return skip_call;
}

static bool validateSecondaryCommandBufferState(layer_data *dev_data, GLOBAL_CB_NODE *pCB, GLOBAL_CB_NODE *pSubCB) {
    bool skip_call = false;
    unordered_set<int> activeTypes;
    for (auto queryObject : pCB->activeQueries) {
        auto queryPoolData = dev_data->queryPoolMap.find(queryObject.pool);
        if (queryPoolData != dev_data->queryPoolMap.end()) {
            if (queryPoolData->second.createInfo.queryType == VK_QUERY_TYPE_PIPELINE_STATISTICS &&
                pSubCB->beginInfo.pInheritanceInfo) {
                VkQueryPipelineStatisticFlags cmdBufStatistics = pSubCB->beginInfo.pInheritanceInfo->pipelineStatistics;
                if ((cmdBufStatistics & queryPoolData->second.createInfo.pipelineStatistics) != cmdBufStatistics) {
                    skip_call |= log_msg(
                        dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                        DRAWSTATE_INVALID_SECONDARY_COMMAND_BUFFER, "DS",
                        "vkCmdExecuteCommands() called w/ invalid Cmd Buffer 0x%p "
                        "which has invalid active query pool 0x%" PRIx64 ". Pipeline statistics is being queried so the command "
                        "buffer must have all bits set on the queryPool.",
                        reinterpret_cast<void *>(pCB->commandBuffer), reinterpret_cast<const uint64_t &>(queryPoolData->first));
                }
            }
            activeTypes.insert(queryPoolData->second.createInfo.queryType);
        }
    }
    for (auto queryObject : pSubCB->startedQueries) {
        auto queryPoolData = dev_data->queryPoolMap.find(queryObject.pool);
        if (queryPoolData != dev_data->queryPoolMap.end() && activeTypes.count(queryPoolData->second.createInfo.queryType)) {
            skip_call |=
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                        DRAWSTATE_INVALID_SECONDARY_COMMAND_BUFFER, "DS",
                        "vkCmdExecuteCommands() called w/ invalid Cmd Buffer 0x%p "
                        "which has invalid active query pool 0x%" PRIx64 "of type %d but a query of that type has been started on "
                        "secondary Cmd Buffer 0x%p.",
                        reinterpret_cast<void *>(pCB->commandBuffer), reinterpret_cast<const uint64_t &>(queryPoolData->first),
                        queryPoolData->second.createInfo.queryType, reinterpret_cast<void *>(pSubCB->commandBuffer));
        }
    }

    auto primary_pool = getCommandPoolNode(dev_data, pCB->createInfo.commandPool);
    auto secondary_pool = getCommandPoolNode(dev_data, pSubCB->createInfo.commandPool);
    if (primary_pool && secondary_pool && (primary_pool->queueFamilyIndex != secondary_pool->queueFamilyIndex)) {
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                             reinterpret_cast<uint64_t>(pSubCB->commandBuffer), __LINE__, DRAWSTATE_INVALID_QUEUE_FAMILY, "DS",
                             "vkCmdExecuteCommands(): Primary command buffer 0x%" PRIxLEAST64
                             " created in queue family %d has secondary command buffer 0x%" PRIxLEAST64 " created in queue family %d.",
                             reinterpret_cast<uint64_t>(pCB->commandBuffer), primary_pool->queueFamilyIndex,
                             reinterpret_cast<uint64_t>(pSubCB->commandBuffer), secondary_pool->queueFamilyIndex);
    }

    return skip_call;
}

VKAPI_ATTR void VKAPI_CALL
CmdExecuteCommands(VkCommandBuffer commandBuffer, uint32_t commandBuffersCount, const VkCommandBuffer *pCommandBuffers) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        GLOBAL_CB_NODE *pSubCB = NULL;
        for (uint32_t i = 0; i < commandBuffersCount; i++) {
            pSubCB = getCBNode(dev_data, pCommandBuffers[i]);
            if (!pSubCB) {
                skip_call |=
                    log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                            DRAWSTATE_INVALID_SECONDARY_COMMAND_BUFFER, "DS",
                            "vkCmdExecuteCommands() called w/ invalid Cmd Buffer 0x%p in element %u of pCommandBuffers array.",
                            (void *)pCommandBuffers[i], i);
            } else if (VK_COMMAND_BUFFER_LEVEL_PRIMARY == pSubCB->createInfo.level) {
                skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0,
                                     __LINE__, DRAWSTATE_INVALID_SECONDARY_COMMAND_BUFFER, "DS",
                                     "vkCmdExecuteCommands() called w/ Primary Cmd Buffer 0x%p in element %u of pCommandBuffers "
                                     "array. All cmd buffers in pCommandBuffers array must be secondary.",
                                     (void *)pCommandBuffers[i], i);
            } else if (pCB->activeRenderPass) { // Secondary CB w/i RenderPass must have *CONTINUE_BIT set
                auto secondary_rp_state = getRenderPassState(dev_data, pSubCB->beginInfo.pInheritanceInfo->renderPass);
                if (!(pSubCB->beginInfo.flags & VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT)) {
                    skip_call |= log_msg(
                        dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)pCommandBuffers[i], __LINE__, DRAWSTATE_BEGIN_CB_INVALID_STATE, "DS",
                        "vkCmdExecuteCommands(): Secondary Command Buffer (0x%p) executed within render pass (0x%" PRIxLEAST64
                        ") must have had vkBeginCommandBuffer() called w/ VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT set.",
                        (void *)pCommandBuffers[i], (uint64_t)pCB->activeRenderPass->renderPass);
                } else {
                    // Make sure render pass is compatible with parent command buffer pass if has continue
                    if (pCB->activeRenderPass->renderPass != secondary_rp_state->renderPass) {
                        skip_call |=
                            validateRenderPassCompatibility(dev_data, commandBuffer, pCB->activeRenderPass->createInfo.ptr(),
                                                            pCommandBuffers[i], secondary_rp_state->createInfo.ptr());
                    }
                    //  If framebuffer for secondary CB is not NULL, then it must match active FB from primaryCB
                    skip_call |= validateFramebuffer(dev_data, commandBuffer, pCB, pCommandBuffers[i], pSubCB);
                }
                string errorString = "";
                // secondaryCB must have been created w/ RP compatible w/ primaryCB active renderpass
                if ((pCB->activeRenderPass->renderPass != secondary_rp_state->renderPass) &&
                    !verify_renderpass_compatibility(dev_data, pCB->activeRenderPass->createInfo.ptr(),
                                                     secondary_rp_state->createInfo.ptr(), errorString)) {
                    skip_call |= log_msg(
                        dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)pCommandBuffers[i], __LINE__, DRAWSTATE_RENDERPASS_INCOMPATIBLE, "DS",
                        "vkCmdExecuteCommands(): Secondary Command Buffer (0x%p) w/ render pass (0x%" PRIxLEAST64
                        ") is incompatible w/ primary command buffer (0x%p) w/ render pass (0x%" PRIxLEAST64 ") due to: %s",
                        (void *)pCommandBuffers[i], (uint64_t)pSubCB->beginInfo.pInheritanceInfo->renderPass, (void *)commandBuffer,
                        (uint64_t)pCB->activeRenderPass->renderPass, errorString.c_str());
                }
            }
            // TODO(mlentine): Move more logic into this method
            skip_call |= validateSecondaryCommandBufferState(dev_data, pCB, pSubCB);
            skip_call |= validateCommandBufferState(dev_data, pSubCB, "vkCmdExecuteCommands()");
            // Secondary cmdBuffers are considered pending execution starting w/
            // being recorded
            if (!(pSubCB->beginInfo.flags & VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT)) {
                if (dev_data->globalInFlightCmdBuffers.find(pSubCB->commandBuffer) != dev_data->globalInFlightCmdBuffers.end()) {
                    skip_call |= log_msg(
                        dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)(pCB->commandBuffer), __LINE__, DRAWSTATE_INVALID_CB_SIMULTANEOUS_USE, "DS",
                        "Attempt to simultaneously execute command buffer 0x%" PRIxLEAST64
                        " without VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT set!",
                        (uint64_t)(pCB->commandBuffer));
                }
                if (pCB->beginInfo.flags & VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT) {
                    // Warn that non-simultaneous secondary cmd buffer renders primary non-simultaneous
                    skip_call |= log_msg(
                        dev_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)(pCommandBuffers[i]), __LINE__, DRAWSTATE_INVALID_CB_SIMULTANEOUS_USE, "DS",
                        "vkCmdExecuteCommands(): Secondary Command Buffer (0x%" PRIxLEAST64
                        ") does not have VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT set and will cause primary command buffer "
                        "(0x%" PRIxLEAST64 ") to be treated as if it does not have VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT "
                        "set, even though it does.",
                        (uint64_t)(pCommandBuffers[i]), (uint64_t)(pCB->commandBuffer));
                    pCB->beginInfo.flags &= ~VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
                }
            }
            if (!pCB->activeQueries.empty() && !dev_data->enabled_features.inheritedQueries) {
                skip_call |=
                    log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            reinterpret_cast<uint64_t>(pCommandBuffers[i]), __LINE__, DRAWSTATE_INVALID_COMMAND_BUFFER, "DS",
                            "vkCmdExecuteCommands(): Secondary Command Buffer "
                            "(0x%" PRIxLEAST64 ") cannot be submitted with a query in "
                            "flight and inherited queries not "
                            "supported on this device.",
                            reinterpret_cast<uint64_t>(pCommandBuffers[i]));
            }
            pSubCB->primaryCommandBuffer = pCB->commandBuffer;
            pCB->secondaryCommandBuffers.insert(pSubCB->commandBuffer);
            dev_data->globalInFlightCmdBuffers.insert(pSubCB->commandBuffer);
            for (auto &function : pSubCB->queryUpdates) {
                pCB->queryUpdates.push_back(function);
            }
        }
        skip_call |= validatePrimaryCommandBuffer(dev_data, pCB, "vkCmdExecuteComands");
        skip_call |= addCmd(dev_data, pCB, CMD_EXECUTECOMMANDS, "vkCmdExecuteComands()");
    }
    lock.unlock();
    if (!skip_call)
        dev_data->dispatch_table.CmdExecuteCommands(commandBuffer, commandBuffersCount, pCommandBuffers);
}

// For any image objects that overlap mapped memory, verify that their layouts are PREINIT or GENERAL
static bool ValidateMapImageLayouts(VkDevice device, DEVICE_MEM_INFO const *mem_info, VkDeviceSize offset,
                                    VkDeviceSize end_offset) {
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    // Iterate over all bound image ranges and verify that for any that overlap the
    //  map ranges, the layouts are VK_IMAGE_LAYOUT_PREINITIALIZED or VK_IMAGE_LAYOUT_GENERAL
    // TODO : This can be optimized if we store ranges based on starting address and early exit when we pass our range
    for (auto image_handle : mem_info->bound_images) {
        auto img_it = mem_info->bound_ranges.find(image_handle);
        if (img_it != mem_info->bound_ranges.end()) {
            if (rangesIntersect(dev_data, &img_it->second, offset, end_offset)) {
                std::vector<VkImageLayout> layouts;
                if (FindLayouts(dev_data, VkImage(image_handle), layouts)) {
                    for (auto layout : layouts) {
                        if (layout != VK_IMAGE_LAYOUT_PREINITIALIZED && layout != VK_IMAGE_LAYOUT_GENERAL) {
                            skip_call |=
                                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0,
                                        __LINE__, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS", "Cannot map an image with layout %s. Only "
                                                                                        "GENERAL or PREINITIALIZED are supported.",
                                        string_VkImageLayout(layout));
                        }
                    }
                }
            }
        }
    }
    return skip_call;
}

VKAPI_ATTR VkResult VKAPI_CALL
MapMemory(VkDevice device, VkDeviceMemory mem, VkDeviceSize offset, VkDeviceSize size, VkFlags flags, void **ppData) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    bool skip_call = false;
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    std::unique_lock<std::mutex> lock(global_lock);
    DEVICE_MEM_INFO *mem_info = getMemObjInfo(dev_data, mem);
    if (mem_info) {
        // TODO : This could me more fine-grained to track just region that is valid
        mem_info->global_valid = true;
        auto end_offset = (VK_WHOLE_SIZE == size) ? mem_info->alloc_info.allocationSize - 1 : offset + size - 1;
        skip_call |= ValidateMapImageLayouts(device, mem_info, offset, end_offset);
        // TODO : Do we need to create new "bound_range" for the mapped range?
        SetMemRangesValid(dev_data, mem_info, offset, end_offset);
        if ((dev_data->phys_dev_mem_props.memoryTypes[mem_info->alloc_info.memoryTypeIndex].propertyFlags &
             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0) {
            skip_call =
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT,
                        (uint64_t)mem, __LINE__, MEMTRACK_INVALID_STATE, "MEM",
                        "Mapping Memory without VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT set: mem obj 0x%" PRIxLEAST64, (uint64_t)mem);
        }
    }
    skip_call |= ValidateMapMemRange(dev_data, mem, offset, size);
    lock.unlock();

    if (!skip_call) {
        result = dev_data->dispatch_table.MapMemory(device, mem, offset, size, flags, ppData);
        if (VK_SUCCESS == result) {
            lock.lock();
            // TODO : What's the point of this range? See comment on creating new "bound_range" above, which may replace this
            storeMemRanges(dev_data, mem, offset, size);
            initializeAndTrackMemory(dev_data, mem, offset, size, ppData);
            lock.unlock();
        }
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL UnmapMemory(VkDevice device, VkDeviceMemory mem) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    bool skip_call = false;

    std::unique_lock<std::mutex> lock(global_lock);
    skip_call |= deleteMemRanges(dev_data, mem);
    lock.unlock();
    if (!skip_call) {
        dev_data->dispatch_table.UnmapMemory(device, mem);
    }
}

static bool validateMemoryIsMapped(layer_data *dev_data, const char *funcName, uint32_t memRangeCount,
                                   const VkMappedMemoryRange *pMemRanges) {
    bool skip_call = false;
    for (uint32_t i = 0; i < memRangeCount; ++i) {
        auto mem_info = getMemObjInfo(dev_data, pMemRanges[i].memory);
        if (mem_info) {
            if (mem_info->mem_range.offset > pMemRanges[i].offset) {
                skip_call |=
                    log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT,
                            (uint64_t)pMemRanges[i].memory, __LINE__, MEMTRACK_INVALID_MAP, "MEM",
                            "%s: Flush/Invalidate offset (" PRINTF_SIZE_T_SPECIFIER ") is less than Memory Object's offset "
                            "(" PRINTF_SIZE_T_SPECIFIER ").",
                            funcName, static_cast<size_t>(pMemRanges[i].offset), static_cast<size_t>(mem_info->mem_range.offset));
            }

            const uint64_t dev_dataTerminus = (mem_info->mem_range.size == VK_WHOLE_SIZE)
                                                  ? mem_info->alloc_info.allocationSize
                                                  : (mem_info->mem_range.offset + mem_info->mem_range.size);
            if (pMemRanges[i].size != VK_WHOLE_SIZE && (dev_dataTerminus < (pMemRanges[i].offset + pMemRanges[i].size))) {
                skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                     VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, (uint64_t)pMemRanges[i].memory, __LINE__,
                                     MEMTRACK_INVALID_MAP, "MEM", "%s: Flush/Invalidate upper-bound (" PRINTF_SIZE_T_SPECIFIER
                                                                  ") exceeds the Memory Object's upper-bound "
                                                                  "(" PRINTF_SIZE_T_SPECIFIER ").",
                                     funcName, static_cast<size_t>(pMemRanges[i].offset + pMemRanges[i].size),
                                     static_cast<size_t>(dev_dataTerminus));
            }
        }
    }
    return skip_call;
}

static bool ValidateAndCopyNoncoherentMemoryToDriver(layer_data *dev_data, uint32_t memRangeCount,
                                                     const VkMappedMemoryRange *pMemRanges) {
    bool skip_call = false;
    for (uint32_t i = 0; i < memRangeCount; ++i) {
        auto mem_info = getMemObjInfo(dev_data, pMemRanges[i].memory);
        if (mem_info) {
            if (mem_info->shadow_copy) {
                VkDeviceSize size = (mem_info->mem_range.size != VK_WHOLE_SIZE)
                                        ? mem_info->mem_range.size
                                        : (mem_info->alloc_info.allocationSize - mem_info->mem_range.offset);
                char *data = static_cast<char *>(mem_info->shadow_copy);
                for (uint64_t j = 0; j < mem_info->shadow_pad_size; ++j) {
                    if (data[j] != NoncoherentMemoryFillValue) {
                        skip_call |= log_msg(
                            dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT,
                            (uint64_t)pMemRanges[i].memory, __LINE__, MEMTRACK_INVALID_MAP, "MEM",
                            "Memory underflow was detected on mem obj 0x%" PRIxLEAST64, (uint64_t)pMemRanges[i].memory);
                    }
                }
                for (uint64_t j = (size + mem_info->shadow_pad_size); j < (2 * mem_info->shadow_pad_size + size); ++j) {
                    if (data[j] != NoncoherentMemoryFillValue) {
                        skip_call |= log_msg(
                            dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT,
                            (uint64_t)pMemRanges[i].memory, __LINE__, MEMTRACK_INVALID_MAP, "MEM",
                            "Memory overflow was detected on mem obj 0x%" PRIxLEAST64, (uint64_t)pMemRanges[i].memory);
                    }
                }
                memcpy(mem_info->p_driver_data, static_cast<void *>(data + mem_info->shadow_pad_size), (size_t)(size));
            }
        }
    }
    return skip_call;
}

static void CopyNoncoherentMemoryFromDriver(layer_data *dev_data, uint32_t memory_range_count,
                                            const VkMappedMemoryRange *mem_ranges) {
    for (uint32_t i = 0; i < memory_range_count; ++i) {
        auto mem_info = getMemObjInfo(dev_data, mem_ranges[i].memory);
        if (mem_info && mem_info->shadow_copy) {
            VkDeviceSize size = (mem_info->mem_range.size != VK_WHOLE_SIZE)
                                    ? mem_info->mem_range.size
                                    : (mem_info->alloc_info.allocationSize - mem_ranges[i].offset);
            char *data = static_cast<char *>(mem_info->shadow_copy);
            memcpy(data + mem_info->shadow_pad_size, mem_info->p_driver_data, (size_t)(size));
        }
    }
}

VKAPI_ATTR VkResult VKAPI_CALL
FlushMappedMemoryRanges(VkDevice device, uint32_t memRangeCount, const VkMappedMemoryRange *pMemRanges) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    std::unique_lock<std::mutex> lock(global_lock);
    skip_call |= ValidateAndCopyNoncoherentMemoryToDriver(dev_data, memRangeCount, pMemRanges);
    skip_call |= validateMemoryIsMapped(dev_data, "vkFlushMappedMemoryRanges", memRangeCount, pMemRanges);
    lock.unlock();
    if (!skip_call) {
        result = dev_data->dispatch_table.FlushMappedMemoryRanges(device, memRangeCount, pMemRanges);
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL
InvalidateMappedMemoryRanges(VkDevice device, uint32_t memRangeCount, const VkMappedMemoryRange *pMemRanges) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    std::unique_lock<std::mutex> lock(global_lock);
    skip_call |= validateMemoryIsMapped(dev_data, "vkInvalidateMappedMemoryRanges", memRangeCount, pMemRanges);
    lock.unlock();
    if (!skip_call) {
        result = dev_data->dispatch_table.InvalidateMappedMemoryRanges(device, memRangeCount, pMemRanges);
        // Update our shadow copy with modified driver data
        CopyNoncoherentMemoryFromDriver(dev_data, memRangeCount, pMemRanges);
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL BindImageMemory(VkDevice device, VkImage image, VkDeviceMemory mem, VkDeviceSize memoryOffset) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    std::unique_lock<std::mutex> lock(global_lock);
    auto image_state = getImageState(dev_data, image);
    if (image_state) {
        // Track objects tied to memory
        uint64_t image_handle = reinterpret_cast<uint64_t &>(image);
        skip_call = SetMemBinding(dev_data, mem, image_handle, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, "vkBindImageMemory");
        VkMemoryRequirements memRequirements;
        lock.unlock();
        dev_data->dispatch_table.GetImageMemoryRequirements(device, image, &memRequirements);
        lock.lock();

        // Track and validate bound memory range information
        auto mem_info = getMemObjInfo(dev_data, mem);
        if (mem_info) {
            skip_call |= InsertImageMemoryRange(dev_data, image, mem_info, memoryOffset, memRequirements,
                                                image_state->createInfo.tiling == VK_IMAGE_TILING_LINEAR);
            skip_call |= ValidateMemoryTypes(dev_data, mem_info, memRequirements.memoryTypeBits, "vkBindImageMemory");
        }

        print_mem_list(dev_data);
        lock.unlock();
        if (!skip_call) {
            result = dev_data->dispatch_table.BindImageMemory(device, image, mem, memoryOffset);
            lock.lock();
            image_state->binding.mem = mem;
            image_state->binding.offset = memoryOffset;
            image_state->binding.size = memRequirements.size;
            lock.unlock();
        }
    } else {
        log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT,
                reinterpret_cast<const uint64_t &>(image), __LINE__, MEMTRACK_INVALID_OBJECT, "MT",
                "vkBindImageMemory: Cannot find invalid image 0x%" PRIx64 ", has it already been deleted?",
                reinterpret_cast<const uint64_t &>(image));
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL SetEvent(VkDevice device, VkEvent event) {
    bool skip_call = false;
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    auto event_state = getEventNode(dev_data, event);
    if (event_state) {
        event_state->needsSignaled = false;
        event_state->stageMask = VK_PIPELINE_STAGE_HOST_BIT;
        if (event_state->write_in_use) {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT,
                                 reinterpret_cast<const uint64_t &>(event), __LINE__, DRAWSTATE_QUEUE_FORWARD_PROGRESS, "DS",
                                 "Cannot call vkSetEvent() on event 0x%" PRIxLEAST64 " that is already in use by a command buffer.",
                                 reinterpret_cast<const uint64_t &>(event));
        }
    }
    lock.unlock();
    // Host setting event is visible to all queues immediately so update stageMask for any queue that's seen this event
    // TODO : For correctness this needs separate fix to verify that app doesn't make incorrect assumptions about the
    // ordering of this command in relation to vkCmd[Set|Reset]Events (see GH297)
    for (auto queue_data : dev_data->queueMap) {
        auto event_entry = queue_data.second.eventToStageMap.find(event);
        if (event_entry != queue_data.second.eventToStageMap.end()) {
            event_entry->second |= VK_PIPELINE_STAGE_HOST_BIT;
        }
    }
    if (!skip_call)
        result = dev_data->dispatch_table.SetEvent(device, event);
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL
QueueBindSparse(VkQueue queue, uint32_t bindInfoCount, const VkBindSparseInfo *pBindInfo, VkFence fence) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(queue), layer_data_map);
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    std::unique_lock<std::mutex> lock(global_lock);
    auto pFence = getFenceNode(dev_data, fence);
    auto pQueue = getQueueNode(dev_data, queue);

    // First verify that fence is not in use
    skip_call |= ValidateFenceForSubmit(dev_data, pFence);

    if (pFence) {
        SubmitFence(pQueue, pFence, bindInfoCount);
    }

    for (uint32_t bindIdx = 0; bindIdx < bindInfoCount; ++bindIdx) {
        const VkBindSparseInfo &bindInfo = pBindInfo[bindIdx];
        // Track objects tied to memory
        for (uint32_t j = 0; j < bindInfo.bufferBindCount; j++) {
            for (uint32_t k = 0; k < bindInfo.pBufferBinds[j].bindCount; k++) {
                auto sparse_binding = bindInfo.pBufferBinds[j].pBinds[k];
                if (SetSparseMemBinding(dev_data, {sparse_binding.memory, sparse_binding.memoryOffset, sparse_binding.size},
                                        (uint64_t)bindInfo.pBufferBinds[j].buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT,
                                        "vkQueueBindSparse"))
                    skip_call = true;
            }
        }
        for (uint32_t j = 0; j < bindInfo.imageOpaqueBindCount; j++) {
            for (uint32_t k = 0; k < bindInfo.pImageOpaqueBinds[j].bindCount; k++) {
                auto sparse_binding = bindInfo.pImageOpaqueBinds[j].pBinds[k];
                if (SetSparseMemBinding(dev_data, {sparse_binding.memory, sparse_binding.memoryOffset, sparse_binding.size},
                                        (uint64_t)bindInfo.pImageOpaqueBinds[j].image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                                        "vkQueueBindSparse"))
                    skip_call = true;
            }
        }
        for (uint32_t j = 0; j < bindInfo.imageBindCount; j++) {
            for (uint32_t k = 0; k < bindInfo.pImageBinds[j].bindCount; k++) {
                auto sparse_binding = bindInfo.pImageBinds[j].pBinds[k];
                // TODO: This size is broken for non-opaque bindings, need to update to comprehend full sparse binding data
                VkDeviceSize size = sparse_binding.extent.depth * sparse_binding.extent.height * sparse_binding.extent.width * 4;
                if (SetSparseMemBinding(dev_data, {sparse_binding.memory, sparse_binding.memoryOffset, size},
                                        (uint64_t)bindInfo.pImageBinds[j].image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                                        "vkQueueBindSparse"))
                    skip_call = true;
            }
        }

        std::vector<SEMAPHORE_WAIT> semaphore_waits;
        std::vector<VkSemaphore> semaphore_signals;
        for (uint32_t i = 0; i < bindInfo.waitSemaphoreCount; ++i) {
            VkSemaphore semaphore = bindInfo.pWaitSemaphores[i];
            auto pSemaphore = getSemaphoreNode(dev_data, semaphore);
            if (pSemaphore) {
                if (pSemaphore->signaled) {
                    if (pSemaphore->signaler.first != VK_NULL_HANDLE) {
                        semaphore_waits.push_back({semaphore, pSemaphore->signaler.first, pSemaphore->signaler.second});
                        pSemaphore->in_use.fetch_add(1);
                    }
                    pSemaphore->signaler.first = VK_NULL_HANDLE;
                    pSemaphore->signaled = false;
                } else {
                    skip_call |=
                        log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT,
                                reinterpret_cast<const uint64_t &>(semaphore), __LINE__, DRAWSTATE_QUEUE_FORWARD_PROGRESS, "DS",
                                "vkQueueBindSparse: Queue 0x%" PRIx64 " is waiting on semaphore 0x%" PRIx64
                                " that has no way to be signaled.",
                                reinterpret_cast<const uint64_t &>(queue), reinterpret_cast<const uint64_t &>(semaphore));
                }
            }
        }
        for (uint32_t i = 0; i < bindInfo.signalSemaphoreCount; ++i) {
            VkSemaphore semaphore = bindInfo.pSignalSemaphores[i];
            auto pSemaphore = getSemaphoreNode(dev_data, semaphore);
            if (pSemaphore) {
                if (pSemaphore->signaled) {
                    skip_call =
                        log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT,
                                reinterpret_cast<const uint64_t &>(semaphore), __LINE__, DRAWSTATE_QUEUE_FORWARD_PROGRESS, "DS",
                                "vkQueueBindSparse: Queue 0x%" PRIx64 " is signaling semaphore 0x%" PRIx64
                                ", but that semaphore is already signaled.",
                                reinterpret_cast<const uint64_t &>(queue), reinterpret_cast<const uint64_t &>(semaphore));
                }
                else {
                    pSemaphore->signaler.first = queue;
                    pSemaphore->signaler.second = pQueue->seq + pQueue->submissions.size() + 1;
                    pSemaphore->signaled = true;
                    pSemaphore->in_use.fetch_add(1);
                    semaphore_signals.push_back(semaphore);
                }
            }
        }

        pQueue->submissions.emplace_back(std::vector<VkCommandBuffer>(),
                                         semaphore_waits,
                                         semaphore_signals,
                                         bindIdx == bindInfoCount - 1 ? fence : VK_NULL_HANDLE);
    }

    if (pFence && !bindInfoCount) {
        // No work to do, just dropping a fence in the queue by itself.
        pQueue->submissions.emplace_back(std::vector<VkCommandBuffer>(),
                                         std::vector<SEMAPHORE_WAIT>(),
                                         std::vector<VkSemaphore>(),
                                         fence);
    }

    print_mem_list(dev_data);
    lock.unlock();

    if (!skip_call)
        return dev_data->dispatch_table.QueueBindSparse(queue, bindInfoCount, pBindInfo, fence);

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateSemaphore(VkDevice device, const VkSemaphoreCreateInfo *pCreateInfo,
                                               const VkAllocationCallbacks *pAllocator, VkSemaphore *pSemaphore) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->dispatch_table.CreateSemaphore(device, pCreateInfo, pAllocator, pSemaphore);
    if (result == VK_SUCCESS) {
        std::lock_guard<std::mutex> lock(global_lock);
        SEMAPHORE_NODE* sNode = &dev_data->semaphoreMap[*pSemaphore];
        sNode->signaler.first = VK_NULL_HANDLE;
        sNode->signaler.second = 0;
        sNode->signaled = false;
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL
CreateEvent(VkDevice device, const VkEventCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkEvent *pEvent) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->dispatch_table.CreateEvent(device, pCreateInfo, pAllocator, pEvent);
    if (result == VK_SUCCESS) {
        std::lock_guard<std::mutex> lock(global_lock);
        dev_data->eventMap[*pEvent].needsSignaled = false;
        dev_data->eventMap[*pEvent].write_in_use = 0;
        dev_data->eventMap[*pEvent].stageMask = VkPipelineStageFlags(0);
    }
    return result;
}

static bool PreCallValidateCreateSwapchainKHR(layer_data *dev_data, VkSwapchainCreateInfoKHR const *pCreateInfo,
                                              SURFACE_STATE *surface_state, SWAPCHAIN_NODE *old_swapchain_state) {
    auto most_recent_swapchain = surface_state->swapchain ? surface_state->swapchain : surface_state->old_swapchain;

    if (most_recent_swapchain != old_swapchain_state || (surface_state->old_swapchain && surface_state->swapchain)) {
        if (log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                    reinterpret_cast<uint64_t>(dev_data->device), __LINE__, DRAWSTATE_SWAPCHAIN_ALREADY_EXISTS, "DS",
                    "vkCreateSwapchainKHR(): surface has an existing swapchain other than oldSwapchain"))
            return true;
    }
    if (old_swapchain_state && old_swapchain_state->createInfo.surface != pCreateInfo->surface) {
        if (log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT,
                    reinterpret_cast<uint64_t const &>(pCreateInfo->oldSwapchain), __LINE__, DRAWSTATE_SWAPCHAIN_WRONG_SURFACE,
                    "DS", "vkCreateSwapchainKHR(): pCreateInfo->oldSwapchain's surface is not pCreateInfo->surface"))
            return true;
    }

    return false;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR *pCreateInfo,
                                                  const VkAllocationCallbacks *pAllocator,
                                                  VkSwapchainKHR *pSwapchain) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    auto surface_state = getSurfaceState(dev_data->instance_data, pCreateInfo->surface);
    auto old_swapchain_state = getSwapchainNode(dev_data, pCreateInfo->oldSwapchain);

    if (PreCallValidateCreateSwapchainKHR(dev_data, pCreateInfo, surface_state, old_swapchain_state))
        return VK_ERROR_VALIDATION_FAILED_EXT;

    VkResult result = dev_data->dispatch_table.CreateSwapchainKHR(device, pCreateInfo, pAllocator, pSwapchain);

    if (VK_SUCCESS == result) {
        std::lock_guard<std::mutex> lock(global_lock);
        auto swapchain_state = unique_ptr<SWAPCHAIN_NODE>(new SWAPCHAIN_NODE(pCreateInfo, *pSwapchain));
        surface_state->swapchain = swapchain_state.get();
        dev_data->device_extensions.swapchainMap[*pSwapchain] = std::move(swapchain_state);
    } else {
        surface_state->swapchain = nullptr;
    }

    // Spec requires that even if CreateSwapchainKHR fails, oldSwapchain behaves as replaced.
    surface_state->old_swapchain = old_swapchain_state;

    return result;
}

VKAPI_ATTR void VKAPI_CALL
DestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain, const VkAllocationCallbacks *pAllocator) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    bool skip_call = false;

    std::unique_lock<std::mutex> lock(global_lock);
    auto swapchain_data = getSwapchainNode(dev_data, swapchain);
    if (swapchain_data) {
        if (swapchain_data->images.size() > 0) {
            for (auto swapchain_image : swapchain_data->images) {
                auto image_sub = dev_data->imageSubresourceMap.find(swapchain_image);
                if (image_sub != dev_data->imageSubresourceMap.end()) {
                    for (auto imgsubpair : image_sub->second) {
                        auto image_item = dev_data->imageLayoutMap.find(imgsubpair);
                        if (image_item != dev_data->imageLayoutMap.end()) {
                            dev_data->imageLayoutMap.erase(image_item);
                        }
                    }
                    dev_data->imageSubresourceMap.erase(image_sub);
                }
                skip_call =
                    ClearMemoryObjectBindings(dev_data, (uint64_t)swapchain_image, VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT);
                dev_data->imageMap.erase(swapchain_image);
            }
        }

        auto surface_state = getSurfaceState(dev_data->instance_data, swapchain_data->createInfo.surface);
        if (surface_state) {
            if (surface_state->swapchain == swapchain_data)
                surface_state->swapchain = nullptr;
            if (surface_state->old_swapchain == swapchain_data)
                surface_state->old_swapchain = nullptr;
        }

        dev_data->device_extensions.swapchainMap.erase(swapchain);
    }
    lock.unlock();
    if (!skip_call)
        dev_data->dispatch_table.DestroySwapchainKHR(device, swapchain, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL
GetSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain, uint32_t *pCount, VkImage *pSwapchainImages) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->dispatch_table.GetSwapchainImagesKHR(device, swapchain, pCount, pSwapchainImages);

    if (result == VK_SUCCESS && pSwapchainImages != NULL) {
        // This should never happen and is checked by param checker.
        if (!pCount)
            return result;
        std::lock_guard<std::mutex> lock(global_lock);
        const size_t count = *pCount;
        auto swapchain_node = getSwapchainNode(dev_data, swapchain);
        if (swapchain_node && !swapchain_node->images.empty()) {
            // TODO : Not sure I like the memcmp here, but it works
            const bool mismatch = (swapchain_node->images.size() != count ||
                                   memcmp(&swapchain_node->images[0], pSwapchainImages, sizeof(swapchain_node->images[0]) * count));
            if (mismatch) {
                // TODO: Verify against Valid Usage section of extension
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT,
                        (uint64_t)swapchain, __LINE__, MEMTRACK_NONE, "SWAP_CHAIN",
                        "vkGetSwapchainInfoKHR(0x%" PRIx64
                        ", VK_SWAP_CHAIN_INFO_TYPE_PERSISTENT_IMAGES_KHR) returned mismatching data",
                        (uint64_t)(swapchain));
            }
        }
        for (uint32_t i = 0; i < *pCount; ++i) {
            IMAGE_LAYOUT_NODE image_layout_node;
            image_layout_node.layout = VK_IMAGE_LAYOUT_UNDEFINED;
            image_layout_node.format = swapchain_node->createInfo.imageFormat;
            // Add imageMap entries for each swapchain image
            VkImageCreateInfo image_ci = {};
            image_ci.mipLevels = 1;
            image_ci.arrayLayers = swapchain_node->createInfo.imageArrayLayers;
            image_ci.usage = swapchain_node->createInfo.imageUsage;
            image_ci.format = swapchain_node->createInfo.imageFormat;
            image_ci.samples = VK_SAMPLE_COUNT_1_BIT;
            image_ci.extent.width = swapchain_node->createInfo.imageExtent.width;
            image_ci.extent.height = swapchain_node->createInfo.imageExtent.height;
            image_ci.sharingMode = swapchain_node->createInfo.imageSharingMode;
            dev_data->imageMap[pSwapchainImages[i]] = unique_ptr<IMAGE_STATE>(new IMAGE_STATE(pSwapchainImages[i], &image_ci));
            auto &image_state = dev_data->imageMap[pSwapchainImages[i]];
            image_state->valid = false;
            image_state->binding.mem = MEMTRACKER_SWAP_CHAIN_IMAGE_KEY;
            swapchain_node->images.push_back(pSwapchainImages[i]);
            ImageSubresourcePair subpair = {pSwapchainImages[i], false, VkImageSubresource()};
            dev_data->imageSubresourceMap[pSwapchainImages[i]].push_back(subpair);
            dev_data->imageLayoutMap[subpair] = image_layout_node;
            dev_data->device_extensions.imageToSwapchainMap[pSwapchainImages[i]] = swapchain;
        }
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL QueuePresentKHR(VkQueue queue, const VkPresentInfoKHR *pPresentInfo) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(queue), layer_data_map);
    bool skip_call = false;

    std::lock_guard<std::mutex> lock(global_lock);
    for (uint32_t i = 0; i < pPresentInfo->waitSemaphoreCount; ++i) {
        auto pSemaphore = getSemaphoreNode(dev_data, pPresentInfo->pWaitSemaphores[i]);
        if (pSemaphore && !pSemaphore->signaled) {
            skip_call |=
                    log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                            VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, 0, __LINE__, DRAWSTATE_QUEUE_FORWARD_PROGRESS, "DS",
                            "Queue 0x%" PRIx64 " is waiting on semaphore 0x%" PRIx64 " that has no way to be signaled.",
                            reinterpret_cast<uint64_t &>(queue), reinterpret_cast<const uint64_t &>(pPresentInfo->pWaitSemaphores[i]));
        }
    }

    for (uint32_t i = 0; i < pPresentInfo->swapchainCount; ++i) {
        auto swapchain_data = getSwapchainNode(dev_data, pPresentInfo->pSwapchains[i]);
        if (swapchain_data) {
            if (pPresentInfo->pImageIndices[i] >= swapchain_data->images.size()) {
                skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT,
                                     reinterpret_cast<uint64_t const &>(pPresentInfo->pSwapchains[i]), __LINE__, DRAWSTATE_SWAPCHAIN_INVALID_IMAGE,
                                     "DS", "vkQueuePresentKHR: Swapchain image index too large (%u). There are only %u images in this swapchain.",
                                     pPresentInfo->pImageIndices[i], (uint32_t)swapchain_data->images.size());
            }
            else {
                auto image = swapchain_data->images[pPresentInfo->pImageIndices[i]];
                auto image_state = getImageState(dev_data, image);
                skip_call |= ValidateImageMemoryIsValid(dev_data, image_state, "vkQueuePresentKHR()");

                if (!image_state->acquired) {
                    skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT,
                                         reinterpret_cast<uint64_t const &>(pPresentInfo->pSwapchains[i]), __LINE__, DRAWSTATE_SWAPCHAIN_IMAGE_NOT_ACQUIRED,
                                         "DS", "vkQueuePresentKHR: Swapchain image index %u has not been acquired.",
                                         pPresentInfo->pImageIndices[i]);
                }

                vector<VkImageLayout> layouts;
                if (FindLayouts(dev_data, image, layouts)) {
                    for (auto layout : layouts) {
                        if (layout != VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
                            skip_call |=
                                    log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT,
                                            reinterpret_cast<uint64_t &>(queue), __LINE__, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                            "Images passed to present must be in layout "
                                            "VK_IMAGE_LAYOUT_PRESENT_SRC_KHR but is in %s",
                                            string_VkImageLayout(layout));
                        }
                    }
                }
            }
        }
    }

    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }

    VkResult result = dev_data->dispatch_table.QueuePresentKHR(queue, pPresentInfo);

    if (result != VK_ERROR_VALIDATION_FAILED_EXT) {
        // Semaphore waits occur before error generation, if the call reached
        // the ICD. (Confirm?)
        for (uint32_t i = 0; i < pPresentInfo->waitSemaphoreCount; ++i) {
            auto pSemaphore = getSemaphoreNode(dev_data, pPresentInfo->pWaitSemaphores[i]);
            if (pSemaphore) {
                pSemaphore->signaler.first = VK_NULL_HANDLE;
                pSemaphore->signaled = false;
            }
        }

        for (uint32_t i = 0; i < pPresentInfo->swapchainCount; ++i) {
            // Note: this is imperfect, in that we can get confused about what
            // did or didn't succeed-- but if the app does that, it's confused
            // itself just as much.
            auto local_result = pPresentInfo->pResults ? pPresentInfo->pResults[i] : result;

            if (local_result != VK_SUCCESS && local_result != VK_SUBOPTIMAL_KHR)
                continue; // this present didn't actually happen.

            // Mark the image as having been released to the WSI
            auto swapchain_data = getSwapchainNode(dev_data, pPresentInfo->pSwapchains[i]);
            auto image = swapchain_data->images[pPresentInfo->pImageIndices[i]];
            auto image_state = getImageState(dev_data, image);
            image_state->acquired = false;
        }

        // Note: even though presentation is directed to a queue, there is no
        // direct ordering between QP and subsequent work, so QP (and its
        // semaphore waits) /never/ participate in any completion proof.
    }

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateSharedSwapchainsKHR(VkDevice device, uint32_t swapchainCount,
                                                         const VkSwapchainCreateInfoKHR *pCreateInfos,
                                                         const VkAllocationCallbacks *pAllocator, VkSwapchainKHR *pSwapchains) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    VkResult result =
        dev_data->dispatch_table.CreateSharedSwapchainsKHR(device, swapchainCount, pCreateInfos, pAllocator, pSwapchains);
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL AcquireNextImageKHR(VkDevice device, VkSwapchainKHR swapchain, uint64_t timeout,
                                                   VkSemaphore semaphore, VkFence fence, uint32_t *pImageIndex) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    bool skip_call = false;

    std::unique_lock<std::mutex> lock(global_lock);

    if (fence == VK_NULL_HANDLE && semaphore == VK_NULL_HANDLE) {
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                             reinterpret_cast<uint64_t &>(device), __LINE__, DRAWSTATE_SWAPCHAIN_NO_SYNC_FOR_ACQUIRE, "DS",
                             "vkAcquireNextImageKHR: Semaphore and fence cannot both be VK_NULL_HANDLE. There would be no way "
                             "to determine the completion of this operation.");
    }

    auto pSemaphore = getSemaphoreNode(dev_data, semaphore);
    if (pSemaphore && pSemaphore->signaled) {
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT,
                             reinterpret_cast<const uint64_t &>(semaphore), __LINE__, DRAWSTATE_QUEUE_FORWARD_PROGRESS, "DS",
                             "vkAcquireNextImageKHR: Semaphore must not be currently signaled or in a wait state");
    }

    auto pFence = getFenceNode(dev_data, fence);
    if (pFence) {
        skip_call |= ValidateFenceForSubmit(dev_data, pFence);
    }
    lock.unlock();

    if (skip_call)
        return VK_ERROR_VALIDATION_FAILED_EXT;

    VkResult result = dev_data->dispatch_table.AcquireNextImageKHR(device, swapchain, timeout, semaphore, fence, pImageIndex);

    lock.lock();
    if (result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR) {
        if (pFence) {
            pFence->state = FENCE_INFLIGHT;
            pFence->signaler.first = VK_NULL_HANDLE;   // ANI isn't on a queue, so this can't participate in a completion proof.
        }

        // A successful call to AcquireNextImageKHR counts as a signal operation on semaphore
        if (pSemaphore) {
            pSemaphore->signaled = true;
            pSemaphore->signaler.first = VK_NULL_HANDLE;
        }

        // Mark the image as acquired.
        auto swapchain_data = getSwapchainNode(dev_data, swapchain);
        auto image = swapchain_data->images[*pImageIndex];
        auto image_state = getImageState(dev_data, image);
        image_state->acquired = true;
    }
    lock.unlock();

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL EnumeratePhysicalDevices(VkInstance instance, uint32_t *pPhysicalDeviceCount,
                                                        VkPhysicalDevice *pPhysicalDevices) {
    bool skip_call = false;
    instance_layer_data *instance_data = get_my_data_ptr(get_dispatch_key(instance), instance_layer_data_map);

    if (instance_data) {
        // For this instance, flag when vkEnumeratePhysicalDevices goes to QUERY_COUNT and then QUERY_DETAILS
        if (NULL == pPhysicalDevices) {
            instance_data->vkEnumeratePhysicalDevicesState = QUERY_COUNT;
        } else {
            if (UNCALLED == instance_data->vkEnumeratePhysicalDevicesState) {
                // Flag warning here. You can call this without having queried the count, but it may not be
                // robust on platforms with multiple physical devices.
                skip_call |= log_msg(instance_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
                                     0, __LINE__, DEVLIMITS_MISSING_QUERY_COUNT, "DL",
                                     "Call sequence has vkEnumeratePhysicalDevices() w/ non-NULL pPhysicalDevices. You should first "
                                     "call vkEnumeratePhysicalDevices() w/ NULL pPhysicalDevices to query pPhysicalDeviceCount.");
            } // TODO : Could also flag a warning if re-calling this function in QUERY_DETAILS state
            else if (instance_data->physical_devices_count != *pPhysicalDeviceCount) {
                // Having actual count match count from app is not a requirement, so this can be a warning
                skip_call |= log_msg(instance_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT,
                                     VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0, __LINE__, DEVLIMITS_COUNT_MISMATCH, "DL",
                                     "Call to vkEnumeratePhysicalDevices() w/ pPhysicalDeviceCount value %u, but actual count "
                                     "supported by this instance is %u.",
                                     *pPhysicalDeviceCount, instance_data->physical_devices_count);
            }
            instance_data->vkEnumeratePhysicalDevicesState = QUERY_DETAILS;
        }
        if (skip_call) {
            return VK_ERROR_VALIDATION_FAILED_EXT;
        }
        VkResult result = instance_data->dispatch_table.EnumeratePhysicalDevices(instance, pPhysicalDeviceCount, pPhysicalDevices);
        if (NULL == pPhysicalDevices) {
            instance_data->physical_devices_count = *pPhysicalDeviceCount;
        } else if (result == VK_SUCCESS){ // Save physical devices
            for (uint32_t i = 0; i < *pPhysicalDeviceCount; i++) {
                auto & phys_device_state = instance_data->physical_device_map[pPhysicalDevices[i]];
                phys_device_state.phys_device = pPhysicalDevices[i];
                // Init actual features for each physical device
                instance_data->dispatch_table.GetPhysicalDeviceFeatures(pPhysicalDevices[i], &phys_device_state.features);
            }
        }
        return result;
    } else {
        log_msg(instance_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT, 0, __LINE__,
                DEVLIMITS_INVALID_INSTANCE, "DL", "Invalid instance (0x%" PRIxLEAST64 ") passed into vkEnumeratePhysicalDevices().",
                (uint64_t)instance);
    }
    return VK_ERROR_VALIDATION_FAILED_EXT;
}

VKAPI_ATTR void VKAPI_CALL
GetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice physicalDevice, uint32_t *pCount,
    VkQueueFamilyProperties *pQueueFamilyProperties) {
    bool skip_call = false;
    instance_layer_data *instance_data = get_my_data_ptr(get_dispatch_key(physicalDevice), instance_layer_data_map);
    auto physical_device_state = getPhysicalDeviceState(instance_data, physicalDevice);
    if (physical_device_state) {
        if (!pQueueFamilyProperties) {
            physical_device_state->vkGetPhysicalDeviceQueueFamilyPropertiesState = QUERY_COUNT;
        }
        else {
            // Verify that for each physical device, this function is called first with NULL pQueueFamilyProperties ptr in order to
            // get count
            if (UNCALLED == physical_device_state->vkGetPhysicalDeviceQueueFamilyPropertiesState) {
                skip_call |= log_msg(instance_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT,
                    VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0, __LINE__, DEVLIMITS_MISSING_QUERY_COUNT, "DL",
                    "Call sequence has vkGetPhysicalDeviceQueueFamilyProperties() w/ non-NULL "
                    "pQueueFamilyProperties. You should first call vkGetPhysicalDeviceQueueFamilyProperties() w/ "
                    "NULL pQueueFamilyProperties to query pCount.");
            }
            // Then verify that pCount that is passed in on second call matches what was returned
            if (physical_device_state->queueFamilyPropertiesCount != *pCount) {

                // TODO: this is not a requirement of the Valid Usage section for vkGetPhysicalDeviceQueueFamilyProperties, so
                // provide as warning
                skip_call |= log_msg(instance_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT,
                    VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0, __LINE__, DEVLIMITS_COUNT_MISMATCH, "DL",
                    "Call to vkGetPhysicalDeviceQueueFamilyProperties() w/ pCount value %u, but actual count "
                    "supported by this physicalDevice is %u.",
                    *pCount, physical_device_state->queueFamilyPropertiesCount);
            }
            physical_device_state->vkGetPhysicalDeviceQueueFamilyPropertiesState = QUERY_DETAILS;
        }
        if (skip_call) {
            return;
        }
        instance_data->dispatch_table.GetPhysicalDeviceQueueFamilyProperties(physicalDevice, pCount, pQueueFamilyProperties);
        if (!pQueueFamilyProperties) {
            physical_device_state->queueFamilyPropertiesCount = *pCount;
        }
        else { // Save queue family properties
            if (physical_device_state->queue_family_properties.size() < *pCount)
                physical_device_state->queue_family_properties.resize(*pCount);
            for (uint32_t i = 0; i < *pCount; i++) {
                physical_device_state->queue_family_properties[i] = pQueueFamilyProperties[i];
            }
        }
    }
    else {
        log_msg(instance_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0,
            __LINE__, DEVLIMITS_INVALID_PHYSICAL_DEVICE, "DL",
            "Invalid physicalDevice (0x%" PRIxLEAST64 ") passed into vkGetPhysicalDeviceQueueFamilyProperties().",
            (uint64_t)physicalDevice);
    }
}

template<typename TCreateInfo, typename FPtr>
static VkResult CreateSurface(VkInstance instance, TCreateInfo const *pCreateInfo,
                              VkAllocationCallbacks const *pAllocator, VkSurfaceKHR *pSurface,
                              FPtr fptr)
{
    instance_layer_data *instance_data = get_my_data_ptr(get_dispatch_key(instance), instance_layer_data_map);

    // Call down the call chain:
    VkResult result = (instance_data->dispatch_table.*fptr)(instance, pCreateInfo, pAllocator, pSurface);

    if (result == VK_SUCCESS) {
        std::unique_lock<std::mutex> lock(global_lock);
        instance_data->surface_map[*pSurface] = SURFACE_STATE(*pSurface);
        lock.unlock();
    }

    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroySurfaceKHR(VkInstance instance, VkSurfaceKHR surface, const VkAllocationCallbacks *pAllocator) {
    bool skip_call = false;
    instance_layer_data *instance_data = get_my_data_ptr(get_dispatch_key(instance), instance_layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    auto surface_state = getSurfaceState(instance_data, surface);

    if (surface_state) {
        // TODO: track swapchains created from this surface.
        instance_data->surface_map.erase(surface);
    }
    lock.unlock();

    if (!skip_call) {
        // Call down the call chain:
        instance_data->dispatch_table.DestroySurfaceKHR(instance, surface, pAllocator);
    }
}

#ifdef VK_USE_PLATFORM_ANDROID_KHR
VKAPI_ATTR VkResult VKAPI_CALL CreateAndroidSurfaceKHR(VkInstance instance, const VkAndroidSurfaceCreateInfoKHR *pCreateInfo,
                                                       const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
    return CreateSurface(instance, pCreateInfo, pAllocator, pSurface, &VkLayerInstanceDispatchTable::CreateAndroidSurfaceKHR);
}
#endif // VK_USE_PLATFORM_ANDROID_KHR

#ifdef VK_USE_PLATFORM_MIR_KHR
VKAPI_ATTR VkResult VKAPI_CALL CreateMirSurfaceKHR(VkInstance instance, const VkMirSurfaceCreateInfoKHR *pCreateInfo,
                                                   const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
    return CreateSurface(instance, pCreateInfo, pAllocator, pSurface, &VkLayerInstanceDispatchTable::CreateMirSurfaceKHR);
}
#endif // VK_USE_PLATFORM_MIR_KHR

#ifdef VK_USE_PLATFORM_WAYLAND_KHR
VKAPI_ATTR VkResult VKAPI_CALL CreateWaylandSurfaceKHR(VkInstance instance, const VkWaylandSurfaceCreateInfoKHR *pCreateInfo,
                                                       const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
    return CreateSurface(instance, pCreateInfo, pAllocator, pSurface, &VkLayerInstanceDispatchTable::CreateWaylandSurfaceKHR);
}
#endif // VK_USE_PLATFORM_WAYLAND_KHR

#ifdef VK_USE_PLATFORM_WIN32_KHR
VKAPI_ATTR VkResult VKAPI_CALL CreateWin32SurfaceKHR(VkInstance instance, const VkWin32SurfaceCreateInfoKHR *pCreateInfo,
                                                     const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
    return CreateSurface(instance, pCreateInfo, pAllocator, pSurface, &VkLayerInstanceDispatchTable::CreateWin32SurfaceKHR);
}
#endif // VK_USE_PLATFORM_WIN32_KHR

#ifdef VK_USE_PLATFORM_XCB_KHR
VKAPI_ATTR VkResult VKAPI_CALL CreateXcbSurfaceKHR(VkInstance instance, const VkXcbSurfaceCreateInfoKHR *pCreateInfo,
                                                   const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
    return CreateSurface(instance, pCreateInfo, pAllocator, pSurface, &VkLayerInstanceDispatchTable::CreateXcbSurfaceKHR);
}
#endif // VK_USE_PLATFORM_XCB_KHR

#ifdef VK_USE_PLATFORM_XLIB_KHR
VKAPI_ATTR VkResult VKAPI_CALL CreateXlibSurfaceKHR(VkInstance instance, const VkXlibSurfaceCreateInfoKHR *pCreateInfo,
                                                   const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
    return CreateSurface(instance, pCreateInfo, pAllocator, pSurface, &VkLayerInstanceDispatchTable::CreateXlibSurfaceKHR);
}
#endif // VK_USE_PLATFORM_XLIB_KHR


VKAPI_ATTR VkResult VKAPI_CALL
CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT *pCreateInfo,
                             const VkAllocationCallbacks *pAllocator, VkDebugReportCallbackEXT *pMsgCallback) {
    instance_layer_data *instance_data = get_my_data_ptr(get_dispatch_key(instance), instance_layer_data_map);
    VkResult res = instance_data->dispatch_table.CreateDebugReportCallbackEXT(instance, pCreateInfo, pAllocator, pMsgCallback);
    if (VK_SUCCESS == res) {
        std::lock_guard<std::mutex> lock(global_lock);
        res = layer_create_msg_callback(instance_data->report_data, false, pCreateInfo, pAllocator, pMsgCallback);
    }
    return res;
}

VKAPI_ATTR void VKAPI_CALL DestroyDebugReportCallbackEXT(VkInstance instance,
                                                         VkDebugReportCallbackEXT msgCallback,
                                                         const VkAllocationCallbacks *pAllocator) {
    instance_layer_data *instance_data = get_my_data_ptr(get_dispatch_key(instance), instance_layer_data_map);
    instance_data->dispatch_table.DestroyDebugReportCallbackEXT(instance, msgCallback, pAllocator);
    std::lock_guard<std::mutex> lock(global_lock);
    layer_destroy_msg_callback(instance_data->report_data, msgCallback, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL
DebugReportMessageEXT(VkInstance instance, VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t object,
                      size_t location, int32_t msgCode, const char *pLayerPrefix, const char *pMsg) {
    instance_layer_data *instance_data = get_my_data_ptr(get_dispatch_key(instance), instance_layer_data_map);
    instance_data->dispatch_table.DebugReportMessageEXT(instance, flags, objType, object, location, msgCode, pLayerPrefix, pMsg);
}

VKAPI_ATTR VkResult VKAPI_CALL
EnumerateInstanceLayerProperties(uint32_t *pCount, VkLayerProperties *pProperties) {
    return util_GetLayerProperties(1, &global_layer, pCount, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL
EnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t *pCount, VkLayerProperties *pProperties) {
    return util_GetLayerProperties(1, &global_layer, pCount, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL
EnumerateInstanceExtensionProperties(const char *pLayerName, uint32_t *pCount, VkExtensionProperties *pProperties) {
    if (pLayerName && !strcmp(pLayerName, global_layer.layerName))
        return util_GetExtensionProperties(1, instance_extensions, pCount, pProperties);

    return VK_ERROR_LAYER_NOT_PRESENT;
}

VKAPI_ATTR VkResult VKAPI_CALL EnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice,
                                                                  const char *pLayerName, uint32_t *pCount,
                                                                  VkExtensionProperties *pProperties) {
    if (pLayerName && !strcmp(pLayerName, global_layer.layerName))
        return util_GetExtensionProperties(0, NULL, pCount, pProperties);

    assert(physicalDevice);

    instance_layer_data *instance_data = get_my_data_ptr(get_dispatch_key(physicalDevice), instance_layer_data_map);
    return instance_data->dispatch_table.EnumerateDeviceExtensionProperties(physicalDevice, NULL, pCount, pProperties);
}

static PFN_vkVoidFunction
intercept_core_instance_command(const char *name);

static PFN_vkVoidFunction
intercept_core_device_command(const char *name);

static PFN_vkVoidFunction
intercept_khr_swapchain_command(const char *name, VkDevice dev);

static PFN_vkVoidFunction
intercept_khr_surface_command(const char *name, VkInstance instance);

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL GetDeviceProcAddr(VkDevice dev, const char *funcName) {
    PFN_vkVoidFunction proc = intercept_core_device_command(funcName);
    if (proc)
        return proc;

    assert(dev);

    proc = intercept_khr_swapchain_command(funcName, dev);
    if (proc)
        return proc;

    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(dev), layer_data_map);

    auto &table = dev_data->dispatch_table;
    if (!table.GetDeviceProcAddr)
        return nullptr;
    return table.GetDeviceProcAddr(dev, funcName);
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL GetInstanceProcAddr(VkInstance instance, const char *funcName) {
    PFN_vkVoidFunction proc = intercept_core_instance_command(funcName);
    if (!proc)
        proc = intercept_core_device_command(funcName);
    if (!proc)
        proc = intercept_khr_swapchain_command(funcName, VK_NULL_HANDLE);
    if (!proc)
        proc = intercept_khr_surface_command(funcName, instance);
    if (proc)
        return proc;

    assert(instance);

    instance_layer_data *instance_data = get_my_data_ptr(get_dispatch_key(instance), instance_layer_data_map);
    proc = debug_report_get_instance_proc_addr(instance_data->report_data, funcName);
    if (proc)
        return proc;

    auto &table = instance_data->dispatch_table;
    if (!table.GetInstanceProcAddr)
        return nullptr;
    return table.GetInstanceProcAddr(instance, funcName);
}

static PFN_vkVoidFunction
intercept_core_instance_command(const char *name) {
    static const struct {
        const char *name;
        PFN_vkVoidFunction proc;
    } core_instance_commands[] = {
        { "vkGetInstanceProcAddr", reinterpret_cast<PFN_vkVoidFunction>(GetInstanceProcAddr) },
        { "vkGetDeviceProcAddr", reinterpret_cast<PFN_vkVoidFunction>(GetDeviceProcAddr) },
        { "vkCreateInstance", reinterpret_cast<PFN_vkVoidFunction>(CreateInstance) },
        { "vkCreateDevice", reinterpret_cast<PFN_vkVoidFunction>(CreateDevice) },
        { "vkEnumeratePhysicalDevices", reinterpret_cast<PFN_vkVoidFunction>(EnumeratePhysicalDevices) },
        { "vkGetPhysicalDeviceQueueFamilyProperties", reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceQueueFamilyProperties) },
        { "vkDestroyInstance", reinterpret_cast<PFN_vkVoidFunction>(DestroyInstance) },
        { "vkEnumerateInstanceLayerProperties", reinterpret_cast<PFN_vkVoidFunction>(EnumerateInstanceLayerProperties) },
        { "vkEnumerateDeviceLayerProperties", reinterpret_cast<PFN_vkVoidFunction>(EnumerateDeviceLayerProperties) },
        { "vkEnumerateInstanceExtensionProperties", reinterpret_cast<PFN_vkVoidFunction>(EnumerateInstanceExtensionProperties) },
        { "vkEnumerateDeviceExtensionProperties", reinterpret_cast<PFN_vkVoidFunction>(EnumerateDeviceExtensionProperties) },
    };

    for (size_t i = 0; i < ARRAY_SIZE(core_instance_commands); i++) {
        if (!strcmp(core_instance_commands[i].name, name))
            return core_instance_commands[i].proc;
    }

    return nullptr;
}

static PFN_vkVoidFunction
intercept_core_device_command(const char *name) {
    static const struct {
        const char *name;
        PFN_vkVoidFunction proc;
    } core_device_commands[] = {
        {"vkGetDeviceProcAddr", reinterpret_cast<PFN_vkVoidFunction>(GetDeviceProcAddr)},
        {"vkQueueSubmit", reinterpret_cast<PFN_vkVoidFunction>(QueueSubmit)},
        {"vkWaitForFences", reinterpret_cast<PFN_vkVoidFunction>(WaitForFences)},
        {"vkGetFenceStatus", reinterpret_cast<PFN_vkVoidFunction>(GetFenceStatus)},
        {"vkQueueWaitIdle", reinterpret_cast<PFN_vkVoidFunction>(QueueWaitIdle)},
        {"vkDeviceWaitIdle", reinterpret_cast<PFN_vkVoidFunction>(DeviceWaitIdle)},
        {"vkGetDeviceQueue", reinterpret_cast<PFN_vkVoidFunction>(GetDeviceQueue)},
        {"vkDestroyInstance", reinterpret_cast<PFN_vkVoidFunction>(DestroyInstance)},
        {"vkDestroyDevice", reinterpret_cast<PFN_vkVoidFunction>(DestroyDevice)},
        {"vkDestroyFence", reinterpret_cast<PFN_vkVoidFunction>(DestroyFence)},
        {"vkResetFences", reinterpret_cast<PFN_vkVoidFunction>(ResetFences)},
        {"vkDestroySemaphore", reinterpret_cast<PFN_vkVoidFunction>(DestroySemaphore)},
        {"vkDestroyEvent", reinterpret_cast<PFN_vkVoidFunction>(DestroyEvent)},
        {"vkDestroyQueryPool", reinterpret_cast<PFN_vkVoidFunction>(DestroyQueryPool)},
        {"vkDestroyBuffer", reinterpret_cast<PFN_vkVoidFunction>(DestroyBuffer)},
        {"vkDestroyBufferView", reinterpret_cast<PFN_vkVoidFunction>(DestroyBufferView)},
        {"vkDestroyImage", reinterpret_cast<PFN_vkVoidFunction>(DestroyImage)},
        {"vkDestroyImageView", reinterpret_cast<PFN_vkVoidFunction>(DestroyImageView)},
        {"vkDestroyShaderModule", reinterpret_cast<PFN_vkVoidFunction>(DestroyShaderModule)},
        {"vkDestroyPipeline", reinterpret_cast<PFN_vkVoidFunction>(DestroyPipeline)},
        {"vkDestroyPipelineLayout", reinterpret_cast<PFN_vkVoidFunction>(DestroyPipelineLayout)},
        {"vkDestroySampler", reinterpret_cast<PFN_vkVoidFunction>(DestroySampler)},
        {"vkDestroyDescriptorSetLayout", reinterpret_cast<PFN_vkVoidFunction>(DestroyDescriptorSetLayout)},
        {"vkDestroyDescriptorPool", reinterpret_cast<PFN_vkVoidFunction>(DestroyDescriptorPool)},
        {"vkDestroyFramebuffer", reinterpret_cast<PFN_vkVoidFunction>(DestroyFramebuffer)},
        {"vkDestroyRenderPass", reinterpret_cast<PFN_vkVoidFunction>(DestroyRenderPass)},
        {"vkCreateBuffer", reinterpret_cast<PFN_vkVoidFunction>(CreateBuffer)},
        {"vkCreateBufferView", reinterpret_cast<PFN_vkVoidFunction>(CreateBufferView)},
        {"vkCreateImage", reinterpret_cast<PFN_vkVoidFunction>(CreateImage)},
        {"vkCreateImageView", reinterpret_cast<PFN_vkVoidFunction>(CreateImageView)},
        {"vkCreateFence", reinterpret_cast<PFN_vkVoidFunction>(CreateFence)},
        {"vkCreatePipelineCache", reinterpret_cast<PFN_vkVoidFunction>(CreatePipelineCache)},
        {"vkDestroyPipelineCache", reinterpret_cast<PFN_vkVoidFunction>(DestroyPipelineCache)},
        {"vkGetPipelineCacheData", reinterpret_cast<PFN_vkVoidFunction>(GetPipelineCacheData)},
        {"vkMergePipelineCaches", reinterpret_cast<PFN_vkVoidFunction>(MergePipelineCaches)},
        {"vkCreateGraphicsPipelines", reinterpret_cast<PFN_vkVoidFunction>(CreateGraphicsPipelines)},
        {"vkCreateComputePipelines", reinterpret_cast<PFN_vkVoidFunction>(CreateComputePipelines)},
        {"vkCreateSampler", reinterpret_cast<PFN_vkVoidFunction>(CreateSampler)},
        {"vkCreateDescriptorSetLayout", reinterpret_cast<PFN_vkVoidFunction>(CreateDescriptorSetLayout)},
        {"vkCreatePipelineLayout", reinterpret_cast<PFN_vkVoidFunction>(CreatePipelineLayout)},
        {"vkCreateDescriptorPool", reinterpret_cast<PFN_vkVoidFunction>(CreateDescriptorPool)},
        {"vkResetDescriptorPool", reinterpret_cast<PFN_vkVoidFunction>(ResetDescriptorPool)},
        {"vkAllocateDescriptorSets", reinterpret_cast<PFN_vkVoidFunction>(AllocateDescriptorSets)},
        {"vkFreeDescriptorSets", reinterpret_cast<PFN_vkVoidFunction>(FreeDescriptorSets)},
        {"vkUpdateDescriptorSets", reinterpret_cast<PFN_vkVoidFunction>(UpdateDescriptorSets)},
        {"vkCreateCommandPool", reinterpret_cast<PFN_vkVoidFunction>(CreateCommandPool)},
        {"vkDestroyCommandPool", reinterpret_cast<PFN_vkVoidFunction>(DestroyCommandPool)},
        {"vkResetCommandPool", reinterpret_cast<PFN_vkVoidFunction>(ResetCommandPool)},
        {"vkCreateQueryPool", reinterpret_cast<PFN_vkVoidFunction>(CreateQueryPool)},
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
        {"vkCmdBindVertexBuffers", reinterpret_cast<PFN_vkVoidFunction>(CmdBindVertexBuffers)},
        {"vkCmdBindIndexBuffer", reinterpret_cast<PFN_vkVoidFunction>(CmdBindIndexBuffer)},
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
        {"vkCmdCopyQueryPoolResults", reinterpret_cast<PFN_vkVoidFunction>(CmdCopyQueryPoolResults)},
        {"vkCmdPushConstants", reinterpret_cast<PFN_vkVoidFunction>(CmdPushConstants)},
        {"vkCmdWriteTimestamp", reinterpret_cast<PFN_vkVoidFunction>(CmdWriteTimestamp)},
        {"vkCreateFramebuffer", reinterpret_cast<PFN_vkVoidFunction>(CreateFramebuffer)},
        {"vkCreateShaderModule", reinterpret_cast<PFN_vkVoidFunction>(CreateShaderModule)},
        {"vkCreateRenderPass", reinterpret_cast<PFN_vkVoidFunction>(CreateRenderPass)},
        {"vkCmdBeginRenderPass", reinterpret_cast<PFN_vkVoidFunction>(CmdBeginRenderPass)},
        {"vkCmdNextSubpass", reinterpret_cast<PFN_vkVoidFunction>(CmdNextSubpass)},
        {"vkCmdEndRenderPass", reinterpret_cast<PFN_vkVoidFunction>(CmdEndRenderPass)},
        {"vkCmdExecuteCommands", reinterpret_cast<PFN_vkVoidFunction>(CmdExecuteCommands)},
        {"vkSetEvent", reinterpret_cast<PFN_vkVoidFunction>(SetEvent)},
        {"vkMapMemory", reinterpret_cast<PFN_vkVoidFunction>(MapMemory)},
        {"vkUnmapMemory", reinterpret_cast<PFN_vkVoidFunction>(UnmapMemory)},
        {"vkFlushMappedMemoryRanges", reinterpret_cast<PFN_vkVoidFunction>(FlushMappedMemoryRanges)},
        {"vkInvalidateMappedMemoryRanges", reinterpret_cast<PFN_vkVoidFunction>(InvalidateMappedMemoryRanges)},
        {"vkAllocateMemory", reinterpret_cast<PFN_vkVoidFunction>(AllocateMemory)},
        {"vkFreeMemory", reinterpret_cast<PFN_vkVoidFunction>(FreeMemory)},
        {"vkBindBufferMemory", reinterpret_cast<PFN_vkVoidFunction>(BindBufferMemory)},
        {"vkGetBufferMemoryRequirements", reinterpret_cast<PFN_vkVoidFunction>(GetBufferMemoryRequirements)},
        {"vkGetImageMemoryRequirements", reinterpret_cast<PFN_vkVoidFunction>(GetImageMemoryRequirements)},
        {"vkGetQueryPoolResults", reinterpret_cast<PFN_vkVoidFunction>(GetQueryPoolResults)},
        {"vkBindImageMemory", reinterpret_cast<PFN_vkVoidFunction>(BindImageMemory)},
        {"vkQueueBindSparse", reinterpret_cast<PFN_vkVoidFunction>(QueueBindSparse)},
        {"vkCreateSemaphore", reinterpret_cast<PFN_vkVoidFunction>(CreateSemaphore)},
        {"vkCreateEvent", reinterpret_cast<PFN_vkVoidFunction>(CreateEvent)},
    };

    for (size_t i = 0; i < ARRAY_SIZE(core_device_commands); i++) {
        if (!strcmp(core_device_commands[i].name, name))
            return core_device_commands[i].proc;
    }

    return nullptr;
}

static PFN_vkVoidFunction
intercept_khr_swapchain_command(const char *name, VkDevice dev) {
    static const struct {
        const char *name;
        PFN_vkVoidFunction proc;
    } khr_swapchain_commands[] = {
        { "vkCreateSwapchainKHR", reinterpret_cast<PFN_vkVoidFunction>(CreateSwapchainKHR) },
        { "vkDestroySwapchainKHR", reinterpret_cast<PFN_vkVoidFunction>(DestroySwapchainKHR) },
        { "vkGetSwapchainImagesKHR", reinterpret_cast<PFN_vkVoidFunction>(GetSwapchainImagesKHR) },
        { "vkAcquireNextImageKHR", reinterpret_cast<PFN_vkVoidFunction>(AcquireNextImageKHR) },
        { "vkQueuePresentKHR", reinterpret_cast<PFN_vkVoidFunction>(QueuePresentKHR) },
    };
    layer_data *dev_data = nullptr;

    if (dev) {
        dev_data = get_my_data_ptr(get_dispatch_key(dev), layer_data_map);
        if (!dev_data->device_extensions.wsi_enabled)
            return nullptr;
    }

    for (size_t i = 0; i < ARRAY_SIZE(khr_swapchain_commands); i++) {
        if (!strcmp(khr_swapchain_commands[i].name, name))
            return khr_swapchain_commands[i].proc;
    }

    if (dev_data) {
        if (!dev_data->device_extensions.wsi_display_swapchain_enabled)
            return nullptr;
    }

    if (!strcmp("vkCreateSharedSwapchainsKHR", name))
        return reinterpret_cast<PFN_vkVoidFunction>(CreateSharedSwapchainsKHR);

    return nullptr;
}

static PFN_vkVoidFunction
intercept_khr_surface_command(const char *name, VkInstance instance) {
    static const struct {
        const char *name;
        PFN_vkVoidFunction proc;
        bool instance_layer_data::*enable;
    } khr_surface_commands[] = {
#ifdef VK_USE_PLATFORM_ANDROID_KHR
        {"vkCreateAndroidSurfaceKHR", reinterpret_cast<PFN_vkVoidFunction>(CreateAndroidSurfaceKHR),
            &instance_layer_data::androidSurfaceExtensionEnabled},
#endif // VK_USE_PLATFORM_ANDROID_KHR
#ifdef VK_USE_PLATFORM_MIR_KHR
        {"vkCreateMirSurfaceKHR", reinterpret_cast<PFN_vkVoidFunction>(CreateMirSurfaceKHR),
            &instance_layer_data::mirSurfaceExtensionEnabled},
#endif // VK_USE_PLATFORM_MIR_KHR
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
        {"vkCreateWaylandSurfaceKHR", reinterpret_cast<PFN_vkVoidFunction>(CreateWaylandSurfaceKHR),
            &instance_layer_data::waylandSurfaceExtensionEnabled},
#endif // VK_USE_PLATFORM_WAYLAND_KHR
#ifdef VK_USE_PLATFORM_WIN32_KHR
        {"vkCreateWin32SurfaceKHR", reinterpret_cast<PFN_vkVoidFunction>(CreateWin32SurfaceKHR),
            &instance_layer_data::win32SurfaceExtensionEnabled},
#endif // VK_USE_PLATFORM_WIN32_KHR
#ifdef VK_USE_PLATFORM_XCB_KHR
        {"vkCreateXcbSurfaceKHR", reinterpret_cast<PFN_vkVoidFunction>(CreateXcbSurfaceKHR),
            &instance_layer_data::xcbSurfaceExtensionEnabled},
#endif // VK_USE_PLATFORM_XCB_KHR
#ifdef VK_USE_PLATFORM_XLIB_KHR
        {"vkCreateXlibSurfaceKHR", reinterpret_cast<PFN_vkVoidFunction>(CreateXlibSurfaceKHR),
            &instance_layer_data::xlibSurfaceExtensionEnabled},
#endif // VK_USE_PLATFORM_XLIB_KHR
        {"vkDestroySurfaceKHR", reinterpret_cast<PFN_vkVoidFunction>(DestroySurfaceKHR),
            &instance_layer_data::surfaceExtensionEnabled},
    };

    instance_layer_data *instance_data = nullptr;
    if (instance) {
        instance_data = get_my_data_ptr(get_dispatch_key(instance), instance_layer_data_map);
    }

    for (size_t i = 0; i < ARRAY_SIZE(khr_surface_commands); i++) {
        if (!strcmp(khr_surface_commands[i].name, name)) {
            if (instance_data && !(instance_data->*(khr_surface_commands[i].enable)))
                return nullptr;
            return khr_surface_commands[i].proc;
        }
    }

    return nullptr;
}

} // namespace core_validation

// vk_layer_logging.h expects these to be defined

VKAPI_ATTR VkResult VKAPI_CALL
vkCreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT *pCreateInfo,
                               const VkAllocationCallbacks *pAllocator, VkDebugReportCallbackEXT *pMsgCallback) {
    return core_validation::CreateDebugReportCallbackEXT(instance, pCreateInfo, pAllocator, pMsgCallback);
}

VKAPI_ATTR void VKAPI_CALL
vkDestroyDebugReportCallbackEXT(VkInstance instance,
                                VkDebugReportCallbackEXT msgCallback,
                                const VkAllocationCallbacks *pAllocator) {
    core_validation::DestroyDebugReportCallbackEXT(instance, msgCallback, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL
vkDebugReportMessageEXT(VkInstance instance, VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t object,
                        size_t location, int32_t msgCode, const char *pLayerPrefix, const char *pMsg) {
    core_validation::DebugReportMessageEXT(instance, flags, objType, object, location, msgCode, pLayerPrefix, pMsg);
}

// loader-layer interface v0, just wrappers since there is only a layer

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL
vkEnumerateInstanceExtensionProperties(const char *pLayerName, uint32_t *pCount, VkExtensionProperties *pProperties) {
    return core_validation::EnumerateInstanceExtensionProperties(pLayerName, pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL
vkEnumerateInstanceLayerProperties(uint32_t *pCount, VkLayerProperties *pProperties) {
    return core_validation::EnumerateInstanceLayerProperties(pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL
vkEnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t *pCount, VkLayerProperties *pProperties) {
    // the layer command handles VK_NULL_HANDLE just fine internally
    assert(physicalDevice == VK_NULL_HANDLE);
    return core_validation::EnumerateDeviceLayerProperties(VK_NULL_HANDLE, pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice,
                                                                                    const char *pLayerName, uint32_t *pCount,
                                                                                    VkExtensionProperties *pProperties) {
    // the layer command handles VK_NULL_HANDLE just fine internally
    assert(physicalDevice == VK_NULL_HANDLE);
    return core_validation::EnumerateDeviceExtensionProperties(VK_NULL_HANDLE, pLayerName, pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice dev, const char *funcName) {
    return core_validation::GetDeviceProcAddr(dev, funcName);
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char *funcName) {
    return core_validation::GetInstanceProcAddr(instance, funcName);
}
