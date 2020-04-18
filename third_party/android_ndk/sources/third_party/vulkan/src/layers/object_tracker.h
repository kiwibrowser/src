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
 * Author: Mark Lobodzinski <mark@lunarg.com>
 * Author: Jon Ashburn <jon@lunarg.com>
 * Author: Tobin Ehlis <tobin@lunarg.com>
 */

#include <mutex>

#include "vk_enum_string_helper.h"
#include "vk_layer_extension_utils.h"
#include "vk_layer_table.h"
#include "vk_layer_utils.h"
#include "vulkan/vk_layer.h"

namespace object_tracker {

// Object Tracker ERROR codes
enum OBJECT_TRACK_ERROR {
    OBJTRACK_NONE,                     // Used for INFO & other non-error messages
    OBJTRACK_UNKNOWN_OBJECT,           // Updating uses of object that's not in global object list
    OBJTRACK_INTERNAL_ERROR,           // Bug with data tracking within the layer
    OBJTRACK_OBJECT_LEAK,              // OBJECT was not correctly freed/destroyed
    OBJTRACK_INVALID_OBJECT,           // Object used that has never been created
    OBJTRACK_DESCRIPTOR_POOL_MISMATCH, // Descriptor Pools specified incorrectly
    OBJTRACK_COMMAND_POOL_MISMATCH,    // Command Pools specified incorrectly
    OBJTRACK_ALLOCATOR_MISMATCH,       // Created with custom allocator but destroyed without
};

// Object Status -- used to track state of individual objects
typedef VkFlags ObjectStatusFlags;
enum ObjectStatusFlagBits {
    OBJSTATUS_NONE = 0x00000000,                     // No status is set
    OBJSTATUS_FENCE_IS_SUBMITTED = 0x00000001,       // Fence has been submitted
    OBJSTATUS_VIEWPORT_BOUND = 0x00000002,           // Viewport state object has been bound
    OBJSTATUS_RASTER_BOUND = 0x00000004,             // Viewport state object has been bound
    OBJSTATUS_COLOR_BLEND_BOUND = 0x00000008,        // Viewport state object has been bound
    OBJSTATUS_DEPTH_STENCIL_BOUND = 0x00000010,      // Viewport state object has been bound
    OBJSTATUS_GPU_MEM_MAPPED = 0x00000020,           // Memory object is currently mapped
    OBJSTATUS_COMMAND_BUFFER_SECONDARY = 0x00000040, // Command Buffer is of type SECONDARY
    OBJSTATUS_CUSTOM_ALLOCATOR = 0x00000080,         // Allocated with custom allocator
};

// Object and state information structure
struct OBJTRACK_NODE {
    uint64_t handle;                        // Object handle (new)
    VkDebugReportObjectTypeEXT object_type; // Object type identifier
    ObjectStatusFlags status;               // Object state
    uint64_t parent_object;                 // Parent object
};

// Track Queue information
struct OT_QUEUE_INFO {
    uint32_t queue_node_index;
    VkQueue queue;
};

// Layer name string to be logged with validation messages.
const char LayerName[] = "ObjectTracker";

struct instance_extension_enables {
    bool wsi_enabled;
    bool xlib_enabled;
    bool xcb_enabled;
    bool wayland_enabled;
    bool mir_enabled;
    bool android_enabled;
    bool win32_enabled;
};

typedef std::unordered_map<uint64_t, OBJTRACK_NODE *> object_map_type;
struct layer_data {
    VkInstance instance;
    VkPhysicalDevice physical_device;

    uint64_t num_objects[VK_DEBUG_REPORT_OBJECT_TYPE_RANGE_SIZE_EXT + 1];
    uint64_t num_total_objects;

    debug_report_data *report_data;
    std::vector<VkDebugReportCallbackEXT> logging_callback;
    bool wsi_enabled;
    bool wsi_display_swapchain_enabled;
    bool objtrack_extensions_enabled;

    // The following are for keeping track of the temporary callbacks that can
    // be used in vkCreateInstance and vkDestroyInstance:
    uint32_t num_tmp_callbacks;
    VkDebugReportCallbackCreateInfoEXT *tmp_dbg_create_infos;
    VkDebugReportCallbackEXT *tmp_callbacks;

    std::vector<VkQueueFamilyProperties> queue_family_properties;

    // Vector of unordered_maps per object type to hold OBJTRACK_NODE info
    std::vector<object_map_type> object_map;
    // Special-case map for swapchain images
    std::unordered_map<uint64_t, OBJTRACK_NODE *> swapchainImageMap;
    // Map of queue information structures, one per queue
    std::unordered_map<VkQueue, OT_QUEUE_INFO *> queue_info_map;

    // Default constructor
    layer_data()
        : instance(nullptr), physical_device(nullptr), num_objects{}, num_total_objects(0), report_data(nullptr),
          wsi_enabled(false), wsi_display_swapchain_enabled(false), objtrack_extensions_enabled(false), num_tmp_callbacks(0),
          tmp_dbg_create_infos(nullptr), tmp_callbacks(nullptr), object_map{} {
        object_map.resize(VK_DEBUG_REPORT_OBJECT_TYPE_RANGE_SIZE_EXT + 1);
    }
};


static std::unordered_map<void *, struct instance_extension_enables> instanceExtMap;
static std::unordered_map<void *, layer_data *> layer_data_map;
static device_table_map ot_device_table_map;
static instance_table_map ot_instance_table_map;
static std::mutex global_lock;
static uint64_t object_track_index = 0;

// Array of object name strings for OBJECT_TYPE enum conversion
static const char *object_name[VK_DEBUG_REPORT_OBJECT_TYPE_RANGE_SIZE_EXT] = {
    "Unknown",               // VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN
    "Instance",              // VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT
    "Physical Device",       // VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT
    "Device",                // VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT
    "Queue",                 // VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT
    "Semaphore",             // VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT
    "Command Buffer",        // VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT
    "Fence",                 // VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT
    "Device Memory",         // VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT
    "Buffer",                // VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT
    "Image",                 // VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT
    "Event",                 // VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT
    "Query Pool",            // VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT
    "Buffer View",           // VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT
    "Image View",            // VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT
    "Shader Module",         // VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT
    "Pipeline Cache",        // VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT
    "Pipeline Layout",       // VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT
    "Render Pass",           // VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT
    "Pipeline",              // VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT
    "Descriptor Set Layout", // VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT
    "Sampler",               // VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT
    "Descriptor Pool",       // VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT
    "Descriptor Set",        // VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT
    "Framebuffer",           // VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT
    "Command Pool",          // VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT
    "SurfaceKHR",            // VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT
    "SwapchainKHR",          // VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT
    "Debug Report" };        // VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_EXT

#include "vk_dispatch_table_helper.h"

} // namespace object_tracker
