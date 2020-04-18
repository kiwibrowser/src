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
#ifndef CORE_VALIDATION_TYPES_H_
#define CORE_VALIDATION_TYPES_H_

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

#include "vk_safe_struct.h"
#include "vulkan/vulkan.h"
#include <atomic>
#include <functional>
#include <map>
#include <string.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Fwd declarations
namespace cvdescriptorset {
class DescriptorSetLayout;
class DescriptorSet;
};

struct GLOBAL_CB_NODE;

class BASE_NODE {
  public:
    // Track when object is being used by an in-flight command buffer
    std::atomic_int in_use;
    // Track command buffers that this object is bound to
    //  binding initialized when cmd referencing object is bound to command buffer
    //  binding removed when command buffer is reset or destroyed
    // When an object is destroyed, any bound cbs are set to INVALID
    std::unordered_set<GLOBAL_CB_NODE *> cb_bindings;

    BASE_NODE() { in_use.store(0); };
};

// Generic wrapper for vulkan objects
struct VK_OBJECT {
    uint64_t handle;
    VkDebugReportObjectTypeEXT type;
};

inline bool operator==(VK_OBJECT a, VK_OBJECT b) NOEXCEPT { return a.handle == b.handle && a.type == b.type; }

namespace std {
template <> struct hash<VK_OBJECT> {
    size_t operator()(VK_OBJECT obj) const NOEXCEPT { return hash<uint64_t>()(obj.handle) ^ hash<uint32_t>()(obj.type); }
};
}


// Flags describing requirements imposed by the pipeline on a descriptor. These
// can't be checked at pipeline creation time as they depend on the Image or
// ImageView bound.
enum descriptor_req {
    DESCRIPTOR_REQ_VIEW_TYPE_1D = 1 << VK_IMAGE_VIEW_TYPE_1D,
    DESCRIPTOR_REQ_VIEW_TYPE_1D_ARRAY = 1 << VK_IMAGE_VIEW_TYPE_1D_ARRAY,
    DESCRIPTOR_REQ_VIEW_TYPE_2D = 1 << VK_IMAGE_VIEW_TYPE_2D,
    DESCRIPTOR_REQ_VIEW_TYPE_2D_ARRAY = 1 << VK_IMAGE_VIEW_TYPE_2D_ARRAY,
    DESCRIPTOR_REQ_VIEW_TYPE_3D = 1 << VK_IMAGE_VIEW_TYPE_3D,
    DESCRIPTOR_REQ_VIEW_TYPE_CUBE = 1 << VK_IMAGE_VIEW_TYPE_CUBE,
    DESCRIPTOR_REQ_VIEW_TYPE_CUBE_ARRAY = 1 << VK_IMAGE_VIEW_TYPE_CUBE_ARRAY,

    DESCRIPTOR_REQ_ALL_VIEW_TYPE_BITS = (1 << (VK_IMAGE_VIEW_TYPE_END_RANGE + 1)) - 1,

    DESCRIPTOR_REQ_SINGLE_SAMPLE = 2 << VK_IMAGE_VIEW_TYPE_END_RANGE,
    DESCRIPTOR_REQ_MULTI_SAMPLE = DESCRIPTOR_REQ_SINGLE_SAMPLE << 1,
};

struct DESCRIPTOR_POOL_STATE : BASE_NODE {
    VkDescriptorPool pool;
    uint32_t maxSets;       // Max descriptor sets allowed in this pool
    uint32_t availableSets; // Available descriptor sets in this pool

    VkDescriptorPoolCreateInfo createInfo;
    std::unordered_set<cvdescriptorset::DescriptorSet *> sets; // Collection of all sets in this pool
    std::vector<uint32_t> maxDescriptorTypeCount;              // Max # of descriptors of each type in this pool
    std::vector<uint32_t> availableDescriptorTypeCount;        // Available # of descriptors of each type in this pool

    DESCRIPTOR_POOL_STATE(const VkDescriptorPool pool, const VkDescriptorPoolCreateInfo *pCreateInfo)
        : pool(pool), maxSets(pCreateInfo->maxSets), availableSets(pCreateInfo->maxSets), createInfo(*pCreateInfo),
          maxDescriptorTypeCount(VK_DESCRIPTOR_TYPE_RANGE_SIZE, 0), availableDescriptorTypeCount(VK_DESCRIPTOR_TYPE_RANGE_SIZE, 0) {
        if (createInfo.poolSizeCount) { // Shadow type struct from ptr into local struct
            size_t poolSizeCountSize = createInfo.poolSizeCount * sizeof(VkDescriptorPoolSize);
            createInfo.pPoolSizes = new VkDescriptorPoolSize[poolSizeCountSize];
            memcpy((void *)createInfo.pPoolSizes, pCreateInfo->pPoolSizes, poolSizeCountSize);
            // Now set max counts for each descriptor type based on count of that type times maxSets
            uint32_t i = 0;
            for (i = 0; i < createInfo.poolSizeCount; ++i) {
                uint32_t typeIndex = static_cast<uint32_t>(createInfo.pPoolSizes[i].type);
                // Same descriptor types can appear several times
                maxDescriptorTypeCount[typeIndex] += createInfo.pPoolSizes[i].descriptorCount;
                availableDescriptorTypeCount[typeIndex] = maxDescriptorTypeCount[typeIndex];
            }
        } else {
            createInfo.pPoolSizes = NULL; // Make sure this is NULL so we don't try to clean it up
        }
    }
    ~DESCRIPTOR_POOL_STATE() {
        delete[] createInfo.pPoolSizes;
        // TODO : pSets are currently freed in deletePools function which uses freeShadowUpdateTree function
        //  need to migrate that struct to smart ptrs for auto-cleanup
    }
};

// Generic memory binding struct to track objects bound to objects
struct MEM_BINDING {
    VkDeviceMemory mem;
    VkDeviceSize offset;
    VkDeviceSize size;
};

inline bool operator==(MEM_BINDING a, MEM_BINDING b) NOEXCEPT { return a.mem == b.mem && a.offset == b.offset && a.size == b.size; }

namespace std {
template <> struct hash<MEM_BINDING> {
    size_t operator()(MEM_BINDING mb) const NOEXCEPT {
        auto intermediate = hash<uint64_t>()(reinterpret_cast<uint64_t &>(mb.mem)) ^ hash<uint64_t>()(mb.offset);
        return intermediate ^ hash<uint64_t>()(mb.size);
    }
};
}

// Superclass for bindable object state (currently imagesa and buffers)
class BINDABLE : public BASE_NODE {
  public:
    bool sparse; // Is this object being bound with sparse memory or not?
    // Non-sparse binding data
    MEM_BINDING binding;
    // Sparse binding data, initially just tracking MEM_BINDING per mem object
    //  There's more data for sparse bindings so need better long-term solution
    // TODO : Need to update solution to track all sparse binding data
    std::unordered_set<MEM_BINDING> sparse_bindings;
    BINDABLE() : sparse(false), binding{}, sparse_bindings{}{};
};

class BUFFER_NODE : public BINDABLE {
  public:
    VkBuffer buffer;
    VkBufferCreateInfo createInfo;
    BUFFER_NODE(VkBuffer buff, const VkBufferCreateInfo *pCreateInfo) : buffer(buff), createInfo(*pCreateInfo) {
        if (createInfo.flags & VK_BUFFER_CREATE_SPARSE_BINDING_BIT) {
            sparse = true;
        }
    };

    BUFFER_NODE(BUFFER_NODE const &rh_obj) = delete;
};

class BUFFER_VIEW_STATE : public BASE_NODE {
  public:
    VkBufferView buffer_view;
    VkBufferViewCreateInfo create_info;
    BUFFER_VIEW_STATE(VkBufferView bv, const VkBufferViewCreateInfo *ci) : buffer_view(bv), create_info(*ci){};
    BUFFER_VIEW_STATE(const BUFFER_VIEW_STATE &rh_obj) = delete;
};

struct SAMPLER_STATE : public BASE_NODE {
    VkSampler sampler;
    VkSamplerCreateInfo createInfo;

    SAMPLER_STATE(const VkSampler *ps, const VkSamplerCreateInfo *pci) : sampler(*ps), createInfo(*pci){};
};

class IMAGE_STATE : public BINDABLE {
  public:
    VkImage image;
    VkImageCreateInfo createInfo;
    bool valid; // If this is a swapchain image backing memory track valid here as it doesn't have DEVICE_MEM_INFO
    bool acquired;  // If this is a swapchain image, has it been acquired by the app.
    IMAGE_STATE(VkImage img, const VkImageCreateInfo *pCreateInfo)
        : image(img), createInfo(*pCreateInfo), valid(false), acquired(false) {
        if (createInfo.flags & VK_IMAGE_CREATE_SPARSE_BINDING_BIT) {
            sparse = true;
        }
    };

    IMAGE_STATE(IMAGE_STATE const &rh_obj) = delete;
};

class IMAGE_VIEW_STATE : public BASE_NODE {
  public:
    VkImageView image_view;
    VkImageViewCreateInfo create_info;
    IMAGE_VIEW_STATE(VkImageView iv, const VkImageViewCreateInfo *ci) : image_view(iv), create_info(*ci){};
    IMAGE_VIEW_STATE(const IMAGE_VIEW_STATE &rh_obj) = delete;
};

struct MemRange {
    VkDeviceSize offset;
    VkDeviceSize size;
};

struct MEMORY_RANGE {
    uint64_t handle;
    bool image; // True for image, false for buffer
    bool linear; // True for buffers and linear images
    bool valid;  // True if this range is know to be valid
    VkDeviceMemory memory;
    VkDeviceSize start;
    VkDeviceSize size;
    VkDeviceSize end; // Store this pre-computed for simplicity
    // Set of ptrs to every range aliased with this one
    std::unordered_set<MEMORY_RANGE *> aliases;
};

// Data struct for tracking memory object
struct DEVICE_MEM_INFO : public BASE_NODE {
    void *object; // Dispatchable object used to create this memory (device of swapchain)
    bool global_valid; // If allocation is mapped, set to "true" to be picked up by subsequently bound ranges
    VkDeviceMemory mem;
    VkMemoryAllocateInfo alloc_info;
    std::unordered_set<VK_OBJECT> obj_bindings;         // objects bound to this memory
    std::unordered_map<uint64_t, MEMORY_RANGE> bound_ranges;     // Map of object to its binding range
    // Convenience vectors image/buff handles to speed up iterating over images or buffers independently
    std::unordered_set<uint64_t> bound_images;
    std::unordered_set<uint64_t> bound_buffers;

    MemRange mem_range;
    void *shadow_copy_base;     // Base of layer's allocation for guard band, data, and alignment space
    void *shadow_copy;          // Pointer to start of guard-band data before mapped region
    uint64_t shadow_pad_size;   // Size of the guard-band data before and after actual data. It MUST be a
                                // multiple of limits.minMemoryMapAlignment
    void *p_driver_data;        // Pointer to application's actual memory

    DEVICE_MEM_INFO(void *disp_object, const VkDeviceMemory in_mem, const VkMemoryAllocateInfo *p_alloc_info)
        : object(disp_object), global_valid(false), mem(in_mem), alloc_info(*p_alloc_info), mem_range{}, shadow_copy_base(0),
          shadow_copy(0), shadow_pad_size(0), p_driver_data(0){};
};

class SWAPCHAIN_NODE {
  public:
    safe_VkSwapchainCreateInfoKHR createInfo;
    VkSwapchainKHR swapchain;
    std::vector<VkImage> images;
    SWAPCHAIN_NODE(const VkSwapchainCreateInfoKHR *pCreateInfo, VkSwapchainKHR swapchain)
        : createInfo(pCreateInfo), swapchain(swapchain) {}
};

enum DRAW_TYPE {
    DRAW = 0,
    DRAW_INDEXED = 1,
    DRAW_INDIRECT = 2,
    DRAW_INDEXED_INDIRECT = 3,
    DRAW_BEGIN_RANGE = DRAW,
    DRAW_END_RANGE = DRAW_INDEXED_INDIRECT,
    NUM_DRAW_TYPES = (DRAW_END_RANGE - DRAW_BEGIN_RANGE + 1),
};

class IMAGE_CMD_BUF_LAYOUT_NODE {
  public:
    IMAGE_CMD_BUF_LAYOUT_NODE() = default;
    IMAGE_CMD_BUF_LAYOUT_NODE(VkImageLayout initialLayoutInput, VkImageLayout layoutInput)
        : initialLayout(initialLayoutInput), layout(layoutInput) {}

    VkImageLayout initialLayout;
    VkImageLayout layout;
};

// Store the DAG.
struct DAGNode {
    uint32_t pass;
    std::vector<uint32_t> prev;
    std::vector<uint32_t> next;
};

struct RENDER_PASS_STATE : public BASE_NODE {
    VkRenderPass renderPass;
    safe_VkRenderPassCreateInfo createInfo;
    std::vector<bool> hasSelfDependency;
    std::vector<DAGNode> subpassToNode;
    std::unordered_map<uint32_t, bool> attachment_first_read;
    std::unordered_map<uint32_t, VkImageLayout> attachment_first_layout;

    RENDER_PASS_STATE(VkRenderPassCreateInfo const *pCreateInfo) : createInfo(pCreateInfo) {}
};

// Cmd Buffer Tracking
enum CMD_TYPE {
    CMD_BINDPIPELINE,
    CMD_BINDPIPELINEDELTA,
    CMD_SETVIEWPORTSTATE,
    CMD_SETSCISSORSTATE,
    CMD_SETLINEWIDTHSTATE,
    CMD_SETDEPTHBIASSTATE,
    CMD_SETBLENDSTATE,
    CMD_SETDEPTHBOUNDSSTATE,
    CMD_SETSTENCILREADMASKSTATE,
    CMD_SETSTENCILWRITEMASKSTATE,
    CMD_SETSTENCILREFERENCESTATE,
    CMD_BINDDESCRIPTORSETS,
    CMD_BINDINDEXBUFFER,
    CMD_BINDVERTEXBUFFER,
    CMD_DRAW,
    CMD_DRAWINDEXED,
    CMD_DRAWINDIRECT,
    CMD_DRAWINDEXEDINDIRECT,
    CMD_DISPATCH,
    CMD_DISPATCHINDIRECT,
    CMD_COPYBUFFER,
    CMD_COPYIMAGE,
    CMD_BLITIMAGE,
    CMD_COPYBUFFERTOIMAGE,
    CMD_COPYIMAGETOBUFFER,
    CMD_CLONEIMAGEDATA,
    CMD_UPDATEBUFFER,
    CMD_FILLBUFFER,
    CMD_CLEARCOLORIMAGE,
    CMD_CLEARATTACHMENTS,
    CMD_CLEARDEPTHSTENCILIMAGE,
    CMD_RESOLVEIMAGE,
    CMD_SETEVENT,
    CMD_RESETEVENT,
    CMD_WAITEVENTS,
    CMD_PIPELINEBARRIER,
    CMD_BEGINQUERY,
    CMD_ENDQUERY,
    CMD_RESETQUERYPOOL,
    CMD_COPYQUERYPOOLRESULTS,
    CMD_WRITETIMESTAMP,
    CMD_PUSHCONSTANTS,
    CMD_INITATOMICCOUNTERS,
    CMD_LOADATOMICCOUNTERS,
    CMD_SAVEATOMICCOUNTERS,
    CMD_BEGINRENDERPASS,
    CMD_NEXTSUBPASS,
    CMD_ENDRENDERPASS,
    CMD_EXECUTECOMMANDS,
    CMD_END, // Should be last command in any RECORDED cmd buffer
};

// Data structure for holding sequence of cmds in cmd buffer
struct CMD_NODE {
    CMD_TYPE type;
    uint64_t cmdNumber;
};

enum CB_STATE {
    CB_NEW,       // Newly created CB w/o any cmds
    CB_RECORDING, // BeginCB has been called on this CB
    CB_RECORDED,  // EndCB has been called on this CB
    CB_INVALID    // CB had a bound descriptor set destroyed or updated
};

// CB Status -- used to track status of various bindings on cmd buffer objects
typedef VkFlags CBStatusFlags;
enum CBStatusFlagBits {
    // clang-format off
    CBSTATUS_NONE                   = 0x00000000,   // No status is set
    CBSTATUS_LINE_WIDTH_SET         = 0x00000001,   // Line width has been set
    CBSTATUS_DEPTH_BIAS_SET         = 0x00000002,   // Depth bias has been set
    CBSTATUS_BLEND_CONSTANTS_SET    = 0x00000004,   // Blend constants state has been set
    CBSTATUS_DEPTH_BOUNDS_SET       = 0x00000008,   // Depth bounds state object has been set
    CBSTATUS_STENCIL_READ_MASK_SET  = 0x00000010,   // Stencil read mask has been set
    CBSTATUS_STENCIL_WRITE_MASK_SET = 0x00000020,   // Stencil write mask has been set
    CBSTATUS_STENCIL_REFERENCE_SET  = 0x00000040,   // Stencil reference has been set
    CBSTATUS_INDEX_BUFFER_BOUND     = 0x00000080,   // Index buffer has been set
    CBSTATUS_ALL_STATE_SET          = 0x0000007F,   // All state set (intentionally exclude index buffer)
    // clang-format on
};

struct QueryObject {
    VkQueryPool pool;
    uint32_t index;
};

inline bool operator==(const QueryObject &query1, const QueryObject &query2) {
    return (query1.pool == query2.pool && query1.index == query2.index);
}

namespace std {
template <> struct hash<QueryObject> {
    size_t operator()(QueryObject query) const throw() {
        return hash<uint64_t>()((uint64_t)(query.pool)) ^ hash<uint32_t>()(query.index);
    }
};
}
struct DRAW_DATA { std::vector<VkBuffer> buffers; };

struct ImageSubresourcePair {
    VkImage image;
    bool hasSubresource;
    VkImageSubresource subresource;
};

inline bool operator==(const ImageSubresourcePair &img1, const ImageSubresourcePair &img2) {
    if (img1.image != img2.image || img1.hasSubresource != img2.hasSubresource)
        return false;
    return !img1.hasSubresource ||
           (img1.subresource.aspectMask == img2.subresource.aspectMask && img1.subresource.mipLevel == img2.subresource.mipLevel &&
            img1.subresource.arrayLayer == img2.subresource.arrayLayer);
}

namespace std {
template <> struct hash<ImageSubresourcePair> {
    size_t operator()(ImageSubresourcePair img) const throw() {
        size_t hashVal = hash<uint64_t>()(reinterpret_cast<uint64_t &>(img.image));
        hashVal ^= hash<bool>()(img.hasSubresource);
        if (img.hasSubresource) {
            hashVal ^= hash<uint32_t>()(reinterpret_cast<uint32_t &>(img.subresource.aspectMask));
            hashVal ^= hash<uint32_t>()(img.subresource.mipLevel);
            hashVal ^= hash<uint32_t>()(img.subresource.arrayLayer);
        }
        return hashVal;
    }
};
}

// Store layouts and pushconstants for PipelineLayout
struct PIPELINE_LAYOUT_NODE {
    VkPipelineLayout layout;
    std::vector<cvdescriptorset::DescriptorSetLayout const *> set_layouts;
    std::vector<VkPushConstantRange> push_constant_ranges;

    PIPELINE_LAYOUT_NODE() : layout(VK_NULL_HANDLE), set_layouts{}, push_constant_ranges{} {}

    void reset() {
        layout = VK_NULL_HANDLE;
        set_layouts.clear();
        push_constant_ranges.clear();
    }
};

class PIPELINE_STATE : public BASE_NODE {
  public:
    VkPipeline pipeline;
    safe_VkGraphicsPipelineCreateInfo graphicsPipelineCI;
    safe_VkComputePipelineCreateInfo computePipelineCI;
    // Flag of which shader stages are active for this pipeline
    uint32_t active_shaders;
    uint32_t duplicate_shaders;
    // Capture which slots (set#->bindings) are actually used by the shaders of this pipeline
    std::unordered_map<uint32_t, std::map<uint32_t, descriptor_req>> active_slots;
    // Vtx input info (if any)
    std::vector<VkVertexInputBindingDescription> vertexBindingDescriptions;
    std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions;
    std::vector<VkPipelineColorBlendAttachmentState> attachments;
    bool blendConstantsEnabled; // Blend constants enabled for any attachments
    // Store RPCI b/c renderPass may be destroyed after Pipeline creation
    safe_VkRenderPassCreateInfo render_pass_ci;
    PIPELINE_LAYOUT_NODE pipeline_layout;

    // Default constructor
    PIPELINE_STATE()
        : pipeline{}, graphicsPipelineCI{}, computePipelineCI{}, active_shaders(0), duplicate_shaders(0), active_slots(),
          vertexBindingDescriptions(), vertexAttributeDescriptions(), attachments(), blendConstantsEnabled(false), render_pass_ci(),
          pipeline_layout() {}

    void initGraphicsPipeline(const VkGraphicsPipelineCreateInfo *pCreateInfo) {
        graphicsPipelineCI.initialize(pCreateInfo);
        // Make sure compute pipeline is null
        VkComputePipelineCreateInfo emptyComputeCI = {};
        computePipelineCI.initialize(&emptyComputeCI);
        for (uint32_t i = 0; i < pCreateInfo->stageCount; i++) {
            const VkPipelineShaderStageCreateInfo *pPSSCI = &pCreateInfo->pStages[i];
            this->duplicate_shaders |= this->active_shaders & pPSSCI->stage;
            this->active_shaders |= pPSSCI->stage;
        }
        if (pCreateInfo->pVertexInputState) {
            const VkPipelineVertexInputStateCreateInfo *pVICI = pCreateInfo->pVertexInputState;
            if (pVICI->vertexBindingDescriptionCount) {
                this->vertexBindingDescriptions = std::vector<VkVertexInputBindingDescription>(
                    pVICI->pVertexBindingDescriptions, pVICI->pVertexBindingDescriptions + pVICI->vertexBindingDescriptionCount);
            }
            if (pVICI->vertexAttributeDescriptionCount) {
                this->vertexAttributeDescriptions = std::vector<VkVertexInputAttributeDescription>(
                    pVICI->pVertexAttributeDescriptions,
                    pVICI->pVertexAttributeDescriptions + pVICI->vertexAttributeDescriptionCount);
            }
        }
        if (pCreateInfo->pColorBlendState) {
            const VkPipelineColorBlendStateCreateInfo *pCBCI = pCreateInfo->pColorBlendState;
            if (pCBCI->attachmentCount) {
                this->attachments = std::vector<VkPipelineColorBlendAttachmentState>(pCBCI->pAttachments,
                                                                                     pCBCI->pAttachments + pCBCI->attachmentCount);
            }
        }
    }
    void initComputePipeline(const VkComputePipelineCreateInfo *pCreateInfo) {
        computePipelineCI.initialize(pCreateInfo);
        // Make sure gfx pipeline is null
        VkGraphicsPipelineCreateInfo emptyGraphicsCI = {};
        graphicsPipelineCI.initialize(&emptyGraphicsCI);
        switch (computePipelineCI.stage.stage) {
        case VK_SHADER_STAGE_COMPUTE_BIT:
            this->active_shaders |= VK_SHADER_STAGE_COMPUTE_BIT;
            break;
        default:
            // TODO : Flag error
            break;
        }
    }
};

// Track last states that are bound per pipeline bind point (Gfx & Compute)
struct LAST_BOUND_STATE {
    PIPELINE_STATE *pipeline_state;
    PIPELINE_LAYOUT_NODE pipeline_layout;
    // Track each set that has been bound
    // Ordered bound set tracking where index is set# that given set is bound to
    std::vector<cvdescriptorset::DescriptorSet *> boundDescriptorSets;
    // one dynamic offset per dynamic descriptor bound to this CB
    std::vector<std::vector<uint32_t>> dynamicOffsets;

    void reset() {
        pipeline_state = nullptr;
        pipeline_layout.reset();
        boundDescriptorSets.clear();
        dynamicOffsets.clear();
    }
};
// Cmd Buffer Wrapper Struct - TODO : This desperately needs its own class
struct GLOBAL_CB_NODE : public BASE_NODE {
    VkCommandBuffer commandBuffer;
    VkCommandBufferAllocateInfo createInfo;
    VkCommandBufferBeginInfo beginInfo;
    VkCommandBufferInheritanceInfo inheritanceInfo;
    VkDevice device;                    // device this CB belongs to
    uint64_t numCmds;                   // number of cmds in this CB
    uint64_t drawCount[NUM_DRAW_TYPES]; // Count of each type of draw in this CB
    CB_STATE state;                     // Track cmd buffer update state
    uint64_t submitCount;               // Number of times CB has been submitted
    CBStatusFlags status;               // Track status of various bindings on cmd buffer
    std::vector<CMD_NODE> cmds;              // vector of commands bound to this command buffer
    // Currently storing "lastBound" objects on per-CB basis
    //  long-term may want to create caches of "lastBound" states and could have
    //  each individual CMD_NODE referencing its own "lastBound" state
    // Store last bound state for Gfx & Compute pipeline bind points
    LAST_BOUND_STATE lastBound[VK_PIPELINE_BIND_POINT_RANGE_SIZE];

    uint32_t viewportMask;
    uint32_t scissorMask;
    VkRenderPassBeginInfo activeRenderPassBeginInfo;
    RENDER_PASS_STATE *activeRenderPass;
    VkSubpassContents activeSubpassContents;
    uint32_t activeSubpass;
    VkFramebuffer activeFramebuffer;
    std::unordered_set<VkFramebuffer> framebuffers;
    // Unified data structs to track objects bound to this command buffer as well as object
    //  dependencies that have been broken : either destroyed objects, or updated descriptor sets
    std::unordered_set<VK_OBJECT> object_bindings;
    std::vector<VK_OBJECT> broken_bindings;

    std::unordered_set<VkEvent> waitedEvents;
    std::vector<VkEvent> writeEventsBeforeWait;
    std::vector<VkEvent> events;
    std::unordered_map<QueryObject, std::unordered_set<VkEvent>> waitedEventsBeforeQueryReset;
    std::unordered_map<QueryObject, bool> queryToStateMap; // 0 is unavailable, 1 is available
    std::unordered_set<QueryObject> activeQueries;
    std::unordered_set<QueryObject> startedQueries;
    std::unordered_map<ImageSubresourcePair, IMAGE_CMD_BUF_LAYOUT_NODE> imageLayoutMap;
    std::unordered_map<VkImage, std::vector<ImageSubresourcePair>> imageSubresourceMap;
    std::unordered_map<VkEvent, VkPipelineStageFlags> eventToStageMap;
    std::vector<DRAW_DATA> drawData;
    DRAW_DATA currentDrawData;
    VkCommandBuffer primaryCommandBuffer;
    // Track images and buffers that are updated by this CB at the point of a draw
    std::unordered_set<VkImageView> updateImages;
    std::unordered_set<VkBuffer> updateBuffers;
    // If cmd buffer is primary, track secondary command buffers pending
    // execution
    std::unordered_set<VkCommandBuffer> secondaryCommandBuffers;
    // MTMTODO : Scrub these data fields and merge active sets w/ lastBound as appropriate
    std::vector<std::function<bool()>> validate_functions;
    std::unordered_set<VkDeviceMemory> memObjs;
    std::vector<std::function<bool(VkQueue)>> eventUpdates;
    std::vector<std::function<bool(VkQueue)>> queryUpdates;
};

struct SEMAPHORE_WAIT {
    VkSemaphore semaphore;
    VkQueue queue;
    uint64_t seq;
};

struct CB_SUBMISSION {
    CB_SUBMISSION(std::vector<VkCommandBuffer> const &cbs, std::vector<SEMAPHORE_WAIT> const &waitSemaphores, std::vector<VkSemaphore> const &signalSemaphores, VkFence fence)
        : cbs(cbs), waitSemaphores(waitSemaphores), signalSemaphores(signalSemaphores), fence(fence) {}

    std::vector<VkCommandBuffer> cbs;
    std::vector<SEMAPHORE_WAIT> waitSemaphores;
    std::vector<VkSemaphore> signalSemaphores;
    VkFence fence;
};

// Fwd declarations of layer_data and helpers to look-up/validate state from layer_data maps
namespace core_validation {
struct layer_data;
cvdescriptorset::DescriptorSet *getSetNode(const layer_data *, VkDescriptorSet);
cvdescriptorset::DescriptorSetLayout const *getDescriptorSetLayout(layer_data const *, VkDescriptorSetLayout);
DESCRIPTOR_POOL_STATE *getDescriptorPoolState(const layer_data *, const VkDescriptorPool);
BUFFER_NODE *getBufferNode(const layer_data *, VkBuffer);
IMAGE_STATE *getImageState(const layer_data *, VkImage);
DEVICE_MEM_INFO *getMemObjInfo(const layer_data *, VkDeviceMemory);
BUFFER_VIEW_STATE *getBufferViewState(const layer_data *, VkBufferView);
SAMPLER_STATE *getSamplerState(const layer_data *, VkSampler);
IMAGE_VIEW_STATE *getImageViewState(const layer_data *, VkImageView);
VkSwapchainKHR getSwapchainFromImage(const layer_data *, VkImage);
SWAPCHAIN_NODE *getSwapchainNode(const layer_data *, VkSwapchainKHR);
void invalidateCommandBuffers(std::unordered_set<GLOBAL_CB_NODE *>, VK_OBJECT);
bool ValidateMemoryIsBoundToBuffer(const layer_data *, const BUFFER_NODE *, const char *);
bool ValidateMemoryIsBoundToImage(const layer_data *, const IMAGE_STATE *, const char *);
void AddCommandBufferBindingSampler(GLOBAL_CB_NODE *, SAMPLER_STATE *);
void AddCommandBufferBindingImage(const layer_data *, GLOBAL_CB_NODE *, IMAGE_STATE *);
void AddCommandBufferBindingImageView(const layer_data *, GLOBAL_CB_NODE *, IMAGE_VIEW_STATE *);
void AddCommandBufferBindingBuffer(const layer_data *, GLOBAL_CB_NODE *, BUFFER_NODE *);
void AddCommandBufferBindingBufferView(const layer_data *, GLOBAL_CB_NODE *, BUFFER_VIEW_STATE *);
}

#endif // CORE_VALIDATION_TYPES_H_
