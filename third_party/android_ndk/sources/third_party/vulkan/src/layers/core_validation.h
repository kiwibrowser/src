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
 * Author: Courtney Goeltzenleuchter <courtneygo@google.com>
 * Author: Tobin Ehlis <tobine@google.com>
 * Author: Chris Forbes <chrisf@ijw.co.nz>
 * Author: Mark Lobodzinski <mark@lunarg.com>
 */

#ifndef NOEXCEPT
// Check for noexcept support
#if defined(__clang__)
#if __has_feature(cxx_noexcept)
#define HAS_NOEXCEPT
#endif
#else
#if defined(__GXX_EXPERIMENTAL_CXX0X__) && __GNUC__ * 10 + __GNUC_MINOR__ >= 46
#define HAS_NOEXCEPT
#else
#if defined(_MSC_FULL_VER) && _MSC_FULL_VER >= 190023026 && defined(_HAS_EXCEPTIONS) && _HAS_EXCEPTIONS
#define HAS_NOEXCEPT
#endif
#endif
#endif

#ifdef HAS_NOEXCEPT
#define NOEXCEPT noexcept
#else
#define NOEXCEPT
#endif
#endif

#pragma once
#include "core_validation_error_enums.h"
#include "vk_validation_error_messages.h"
#include "core_validation_types.h"
#include "descriptor_sets.h"
#include "vk_layer_logging.h"
#include "vulkan/vk_layer.h"
#include <atomic>
#include <functional>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <list>
#include <deque>

/*
 * CHECK_DISABLED struct is a container for bools that can block validation checks from being performed.
 * The end goal is to have all checks guarded by a bool. The bools are all "false" by default meaning that all checks
 * are enabled. At CreateInstance time, the user can use the VK_EXT_validation_flags extension to pass in enum values
 * of VkValidationCheckEXT that will selectively disable checks.
 */
struct CHECK_DISABLED {
    bool command_buffer_state;
    bool create_descriptor_set_layout;
    bool destroy_buffer_view; // Skip validation at DestroyBufferView time
    bool destroy_image_view;  // Skip validation at DestroyImageView time
    bool destroy_pipeline;    // Skip validation at DestroyPipeline time
    bool destroy_descriptor_pool; // Skip validation at DestroyDescriptorPool time
    bool destroy_framebuffer;     // Skip validation at DestroyFramebuffer time
    bool destroy_renderpass;      // Skip validation at DestroyRenderpass time
    bool destroy_image;           // Skip validation at DestroyImage time
    bool destroy_sampler;         // Skip validation at DestroySampler time
    bool destroy_command_pool;    // Skip validation at DestroyCommandPool time
    bool destroy_event;           // Skip validation at DestroyEvent time
    bool free_memory;             // Skip validation at FreeMemory time
    bool object_in_use;       // Skip all object in_use checking
    bool idle_descriptor_set; // Skip check to verify that descriptor set is no in-use
    bool push_constant_range; // Skip push constant range checks
    bool free_descriptor_sets; // Skip validation prior to vkFreeDescriptorSets()
    bool allocate_descriptor_sets; // Skip validation prior to vkAllocateDescriptorSets()
    bool update_descriptor_sets;   // Skip validation prior to vkUpdateDescriptorSets()
};

/*
 * MTMTODO : Update this comment
 * Data Structure overview
 *  There are 4 global STL(' maps
 *  cbMap -- map of command Buffer (CB) objects to MT_CB_INFO structures
 *    Each MT_CB_INFO struct has an stl list container with
 *    memory objects that are referenced by this CB
 *  memObjMap -- map of Memory Objects to MT_MEM_OBJ_INFO structures
 *    Each MT_MEM_OBJ_INFO has two stl list containers with:
 *      -- all CBs referencing this mem obj
 *      -- all VK Objects that are bound to this memory
 *  objectMap -- map of objects to MT_OBJ_INFO structures
 *
 * Algorithm overview
 * These are the primary events that should happen related to different objects
 * 1. Command buffers
 *   CREATION - Add object,structure to map
 *   CMD BIND - If mem associated, add mem reference to list container
 *   DESTROY  - Remove from map, decrement (and report) mem references
 * 2. Mem Objects
 *   CREATION - Add object,structure to map
 *   OBJ BIND - Add obj structure to list container for that mem node
 *   CMB BIND - If mem-related add CB structure to list container for that mem node
 *   DESTROY  - Flag as errors any remaining refs and remove from map
 * 3. Generic Objects
 *   MEM BIND - DESTROY any previous binding, Add obj node w/ ref to map, add obj ref to list container for that mem node
 *   DESTROY - If mem bound, remove reference list container for that memInfo, remove object ref from map
 */
// TODO : Is there a way to track when Cmd Buffer finishes & remove mem references at that point?
// TODO : Could potentially store a list of freed mem allocs to flag when they're incorrectly used

struct MT_FB_ATTACHMENT_INFO {
    IMAGE_VIEW_STATE *view_state;
    VkImage image;
    VkDeviceMemory mem;
};

struct GENERIC_HEADER {
    VkStructureType sType;
    const void *pNext;
};

struct IMAGE_LAYOUT_NODE {
    VkImageLayout layout;
    VkFormat format;
};

class PHYS_DEV_PROPERTIES_NODE {
  public:
    VkPhysicalDeviceProperties properties;
    std::vector<VkQueueFamilyProperties> queue_family_properties;
};

enum FENCE_STATE { FENCE_UNSIGNALED, FENCE_INFLIGHT, FENCE_RETIRED };

class FENCE_NODE {
  public:
    VkFence fence;
    VkFenceCreateInfo createInfo;
    std::pair<VkQueue, uint64_t> signaler;
    FENCE_STATE state;

    // Default constructor
    FENCE_NODE() : state(FENCE_UNSIGNALED) {}
};

class SEMAPHORE_NODE : public BASE_NODE {
  public:
    std::pair<VkQueue, uint64_t> signaler;
    bool signaled;
};

class EVENT_STATE : public BASE_NODE {
  public:
    int write_in_use;
    bool needsSignaled;
    VkPipelineStageFlags stageMask;
};

class QUEUE_NODE {
  public:
    VkQueue queue;
    uint32_t queueFamilyIndex;
    std::unordered_map<VkEvent, VkPipelineStageFlags> eventToStageMap;
    std::unordered_map<QueryObject, bool> queryToStateMap; // 0 is unavailable, 1 is available

    uint64_t seq;
    std::deque<CB_SUBMISSION> submissions;
};

class QUERY_POOL_NODE : public BASE_NODE {
  public:
    VkQueryPoolCreateInfo createInfo;
};

class FRAMEBUFFER_STATE : public BASE_NODE {
  public:
    VkFramebuffer framebuffer;
    safe_VkFramebufferCreateInfo createInfo;
    safe_VkRenderPassCreateInfo renderPassCreateInfo;
    std::unordered_set<VkCommandBuffer> referencingCmdBuffers;
    std::vector<MT_FB_ATTACHMENT_INFO> attachments;
    FRAMEBUFFER_STATE(VkFramebuffer fb, const VkFramebufferCreateInfo *pCreateInfo, const VkRenderPassCreateInfo *pRPCI)
        : framebuffer(fb), createInfo(pCreateInfo), renderPassCreateInfo(pRPCI){};
};

// Track command pools and their command buffers
struct COMMAND_POOL_NODE : public BASE_NODE {
    VkCommandPoolCreateFlags createFlags;
    uint32_t queueFamilyIndex;
    // TODO: why is this std::list?
    std::list<VkCommandBuffer> commandBuffers; // container of cmd buffers allocated from this pool
};

// Stuff from Device Limits Layer
enum CALL_STATE {
    UNCALLED,      // Function has not been called
    QUERY_COUNT,   // Function called once to query a count
    QUERY_DETAILS, // Function called w/ a count to query details
};

struct PHYSICAL_DEVICE_STATE {
    // Track the call state and array sizes for various query functions
    CALL_STATE vkGetPhysicalDeviceQueueFamilyPropertiesState = UNCALLED;
    uint32_t queueFamilyPropertiesCount = 0;
    CALL_STATE vkGetPhysicalDeviceLayerPropertiesState = UNCALLED;
    CALL_STATE vkGetPhysicalDeviceExtensionPropertiesState = UNCALLED;
    CALL_STATE vkGetPhysicalDeviceFeaturesState = UNCALLED;
    VkPhysicalDeviceFeatures features = {};
    VkPhysicalDevice phys_device = VK_NULL_HANDLE;
    std::vector<VkQueueFamilyProperties> queue_family_properties;
};

struct SURFACE_STATE {
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    SWAPCHAIN_NODE *swapchain = nullptr;
    SWAPCHAIN_NODE *old_swapchain = nullptr;

    SURFACE_STATE() {}
    SURFACE_STATE(VkSurfaceKHR surface)
        : surface(surface) {}
};
