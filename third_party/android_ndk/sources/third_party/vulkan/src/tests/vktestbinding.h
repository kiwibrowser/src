/*
 * Copyright (c) 2015-2016 The Khronos Group Inc.
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
 * Author: Courtney Goeltzenleuchter <courtney@LunarG.com>
 * Author: Cody Northrop <cody@lunarg.com>
 */

#ifndef VKTESTBINDING_H
#define VKTESTBINDING_H

#include <assert.h>
#include <vector>

#include "vulkan/vulkan.h"

namespace vk_testing {

typedef void (*ErrorCallback)(const char *expr, const char *file, unsigned int line, const char *function);
void set_error_callback(ErrorCallback callback);

class PhysicalDevice;
class Device;
class Queue;
class DeviceMemory;
class Fence;
class Semaphore;
class Event;
class QueryPool;
class Buffer;
class BufferView;
class Image;
class ImageView;
class DepthStencilView;
class Shader;
class Pipeline;
class PipelineDelta;
class Sampler;
class DescriptorSetLayout;
class PipelineLayout;
class DescriptorSetPool;
class DescriptorSet;
class CommandBuffer;
class CommandPool;

std::vector<VkLayerProperties> GetGlobalLayers();
std::vector<VkExtensionProperties> GetGlobalExtensions();
std::vector<VkExtensionProperties> GetGlobalExtensions(const char *pLayerName);

namespace internal {

template <typename T> class Handle {
  public:
    const T &handle() const { return handle_; }
    bool initialized() const { return (handle_ != VK_NULL_HANDLE); }

  protected:
    typedef T handle_type;

    explicit Handle() : handle_(VK_NULL_HANDLE) {}
    explicit Handle(T handle) : handle_(handle) {}

    void init(T handle) {
        assert(!initialized());
        handle_ = handle;
    }

  private:
    // handles are non-copyable
    Handle(const Handle &);
    Handle &operator=(const Handle &);

    T handle_;
};

template <typename T> class NonDispHandle : public Handle<T> {
  protected:
    explicit NonDispHandle() : Handle<T>(), dev_handle_(VK_NULL_HANDLE) {}
    explicit NonDispHandle(VkDevice dev, T handle) : Handle<T>(handle), dev_handle_(dev) {}

    const VkDevice &device() const { return dev_handle_; }

    void init(VkDevice dev, T handle) {
        assert(!Handle<T>::initialized() && dev_handle_ == VK_NULL_HANDLE);
        Handle<T>::init(handle);
        dev_handle_ = dev;
    }

  private:
    VkDevice dev_handle_;
};

} // namespace internal

class PhysicalDevice : public internal::Handle<VkPhysicalDevice> {
  public:
    explicit PhysicalDevice(VkPhysicalDevice phy) : Handle(phy) {
        memory_properties_ = memory_properties();
        device_properties_ = properties();
    }

    VkPhysicalDeviceProperties properties() const;
    VkPhysicalDeviceMemoryProperties memory_properties() const;
    std::vector<VkQueueFamilyProperties> queue_properties() const;
    VkPhysicalDeviceFeatures features() const;

    bool set_memory_type(const uint32_t type_bits, VkMemoryAllocateInfo *info, const VkMemoryPropertyFlags properties,
                         const VkMemoryPropertyFlags forbid = 0) const;

    // vkEnumerateDeviceExtensionProperties()
    std::vector<VkExtensionProperties> extensions() const;
    std::vector<VkExtensionProperties> extensions(const char *pLayerName) const;

    // vkEnumerateLayers()
    std::vector<VkLayerProperties> layers() const;

  private:
    void add_extension_dependencies(uint32_t dependency_count, VkExtensionProperties *depencency_props,
                                    std::vector<VkExtensionProperties> &ext_list);

    VkPhysicalDeviceMemoryProperties memory_properties_;

    VkPhysicalDeviceProperties device_properties_;
};

class Device : public internal::Handle<VkDevice> {
  public:
    explicit Device(VkPhysicalDevice phy) : phy_(phy) {}
    ~Device();

    // vkCreateDevice()
    void init(const VkDeviceCreateInfo &info);
    void init(std::vector<const char *> &extensions,
              VkPhysicalDeviceFeatures *features = nullptr); // all queues, all extensions, etc
    void init() {
        std::vector<const char *> extensions;
        init(extensions);
    };

    const PhysicalDevice &phy() const { return phy_; }

    // vkGetDeviceProcAddr()
    PFN_vkVoidFunction get_proc(const char *name) const { return vkGetDeviceProcAddr(handle(), name); }

    // vkGetDeviceQueue()
    const std::vector<Queue *> &graphics_queues() const { return queues_[GRAPHICS]; }
    const std::vector<Queue *> &compute_queues() { return queues_[COMPUTE]; }
    const std::vector<Queue *> &dma_queues() { return queues_[DMA]; }
    uint32_t graphics_queue_node_index_;

    struct Format {
        VkFormat format;
        VkImageTiling tiling;
        VkFlags features;
    };
    // vkGetFormatInfo()
    VkFormatProperties format_properties(VkFormat format);
    const std::vector<Format> &formats() const { return formats_; }

    // vkDeviceWaitIdle()
    void wait();

    // vkWaitForFences()
    VkResult wait(const std::vector<const Fence *> &fences, bool wait_all, uint64_t timeout);
    VkResult wait(const Fence &fence) { return wait(std::vector<const Fence *>(1, &fence), true, (uint64_t)-1); }

    // vkUpdateDescriptorSets()
    void update_descriptor_sets(const std::vector<VkWriteDescriptorSet> &writes, const std::vector<VkCopyDescriptorSet> &copies);
    void update_descriptor_sets(const std::vector<VkWriteDescriptorSet> &writes) {
        return update_descriptor_sets(writes, std::vector<VkCopyDescriptorSet>());
    }

    static VkWriteDescriptorSet write_descriptor_set(const DescriptorSet &set, uint32_t binding, uint32_t array_element,
                                                     VkDescriptorType type, uint32_t count,
                                                     const VkDescriptorImageInfo *image_info);
    static VkWriteDescriptorSet write_descriptor_set(const DescriptorSet &set, uint32_t binding, uint32_t array_element,
                                                     VkDescriptorType type, uint32_t count,
                                                     const VkDescriptorBufferInfo *buffer_info);
    static VkWriteDescriptorSet write_descriptor_set(const DescriptorSet &set, uint32_t binding, uint32_t array_element,
                                                     VkDescriptorType type, uint32_t count, const VkBufferView *buffer_views);
    static VkWriteDescriptorSet write_descriptor_set(const DescriptorSet &set, uint32_t binding, uint32_t array_element,
                                                     VkDescriptorType type, const std::vector<VkDescriptorImageInfo> &image_info);
    static VkWriteDescriptorSet write_descriptor_set(const DescriptorSet &set, uint32_t binding, uint32_t array_element,
                                                     VkDescriptorType type, const std::vector<VkDescriptorBufferInfo> &buffer_info);
    static VkWriteDescriptorSet write_descriptor_set(const DescriptorSet &set, uint32_t binding, uint32_t array_element,
                                                     VkDescriptorType type, const std::vector<VkBufferView> &buffer_views);

    static VkCopyDescriptorSet copy_descriptor_set(const DescriptorSet &src_set, uint32_t src_binding, uint32_t src_array_element,
                                                   const DescriptorSet &dst_set, uint32_t dst_binding, uint32_t dst_array_element,
                                                   uint32_t count);

  private:
    enum QueueIndex {
        GRAPHICS,
        COMPUTE,
        DMA,
        QUEUE_COUNT,
    };

    void init_queues();
    void init_formats();

    PhysicalDevice phy_;

    std::vector<Queue *> queues_[QUEUE_COUNT];
    std::vector<Format> formats_;
};

class Queue : public internal::Handle<VkQueue> {
  public:
    explicit Queue(VkQueue queue, int index) : Handle(queue) { family_index_ = index; }

    // vkQueueSubmit()
    void submit(const std::vector<const CommandBuffer *> &cmds, Fence &fence);
    void submit(const CommandBuffer &cmd, Fence &fence);
    void submit(const CommandBuffer &cmd);

    // vkQueueWaitIdle()
    void wait();

    int get_family_index() { return family_index_; }

  private:
    int family_index_;
};

class DeviceMemory : public internal::NonDispHandle<VkDeviceMemory> {
  public:
    ~DeviceMemory();

    // vkAllocateMemory()
    void init(const Device &dev, const VkMemoryAllocateInfo &info);

    // vkMapMemory()
    const void *map(VkFlags flags) const;
    void *map(VkFlags flags);
    const void *map() const { return map(0); }
    void *map() { return map(0); }

    // vkUnmapMemory()
    void unmap() const;

    static VkMemoryAllocateInfo alloc_info(VkDeviceSize size, uint32_t memory_type_index);
};

class Fence : public internal::NonDispHandle<VkFence> {
  public:
    ~Fence();

    // vkCreateFence()
    void init(const Device &dev, const VkFenceCreateInfo &info);

    // vkGetFenceStatus()
    VkResult status() const { return vkGetFenceStatus(device(), handle()); }

    static VkFenceCreateInfo create_info(VkFenceCreateFlags flags);
    static VkFenceCreateInfo create_info();
};

class Semaphore : public internal::NonDispHandle<VkSemaphore> {
  public:
    ~Semaphore();

    // vkCreateSemaphore()
    void init(const Device &dev, const VkSemaphoreCreateInfo &info);

    static VkSemaphoreCreateInfo create_info(VkFlags flags);
};

class Event : public internal::NonDispHandle<VkEvent> {
  public:
    ~Event();

    // vkCreateEvent()
    void init(const Device &dev, const VkEventCreateInfo &info);

    // vkGetEventStatus()
    // vkSetEvent()
    // vkResetEvent()
    VkResult status() const { return vkGetEventStatus(device(), handle()); }
    void set();
    void reset();

    static VkEventCreateInfo create_info(VkFlags flags);
};

class QueryPool : public internal::NonDispHandle<VkQueryPool> {
  public:
    ~QueryPool();

    // vkCreateQueryPool()
    void init(const Device &dev, const VkQueryPoolCreateInfo &info);

    // vkGetQueryPoolResults()
    VkResult results(uint32_t first, uint32_t count, size_t size, void *data, size_t stride);

    static VkQueryPoolCreateInfo create_info(VkQueryType type, uint32_t slot_count);
};

class Buffer : public internal::NonDispHandle<VkBuffer> {
  public:
    explicit Buffer() : NonDispHandle() {}
    explicit Buffer(const Device &dev, const VkBufferCreateInfo &info) { init(dev, info); }
    explicit Buffer(const Device &dev, VkDeviceSize size) { init(dev, size); }

    ~Buffer();

    // vkCreateBuffer()
    void init(const Device &dev, const VkBufferCreateInfo &info, VkMemoryPropertyFlags mem_props);
    void init(const Device &dev, const VkBufferCreateInfo &info) { init(dev, info, 0); }
    void init(const Device &dev, VkDeviceSize size, VkMemoryPropertyFlags mem_props) { init(dev, create_info(size, 0), mem_props); }
    void init(const Device &dev, VkDeviceSize size) { init(dev, size, 0); }
    void init_as_src(const Device &dev, VkDeviceSize size, VkMemoryPropertyFlags &reqs) {
        init(dev, create_info(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT), reqs);
    }
    void init_as_dst(const Device &dev, VkDeviceSize size, VkMemoryPropertyFlags &reqs) {
        init(dev, create_info(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT), reqs);
    }
    void init_as_src_and_dst(const Device &dev, VkDeviceSize size, VkMemoryPropertyFlags &reqs) {
        init(dev, create_info(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT), reqs);
    }
    void init_no_mem(const Device &dev, const VkBufferCreateInfo &info);

    // get the internal memory
    const DeviceMemory &memory() const { return internal_mem_; }
    DeviceMemory &memory() { return internal_mem_; }

    // vkGetObjectMemoryRequirements()
    VkMemoryRequirements memory_requirements() const;

    // vkBindObjectMemory()
    void bind_memory(const DeviceMemory &mem, VkDeviceSize mem_offset);

    static VkBufferCreateInfo create_info(VkDeviceSize size, VkFlags usage);

    VkBufferMemoryBarrier buffer_memory_barrier(VkFlags output_mask, VkFlags input_mask, VkDeviceSize offset,
                                                VkDeviceSize size) const {
        VkBufferMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        barrier.buffer = handle();
        barrier.srcAccessMask = output_mask;
        barrier.dstAccessMask = input_mask;
        barrier.offset = offset;
        barrier.size = size;
        return barrier;
    }

  private:
    VkBufferCreateInfo create_info_;

    DeviceMemory internal_mem_;
};

class BufferView : public internal::NonDispHandle<VkBufferView> {
  public:
    ~BufferView();

    // vkCreateBufferView()
    void init(const Device &dev, const VkBufferViewCreateInfo &info);
};

class Image : public internal::NonDispHandle<VkImage> {
  public:
    explicit Image() : NonDispHandle(), format_features_(0) {}
    explicit Image(const Device &dev, const VkImageCreateInfo &info) : format_features_(0) { init(dev, info); }

    ~Image();

    // vkCreateImage()
    void init(const Device &dev, const VkImageCreateInfo &info, VkMemoryPropertyFlags mem_props);
    void init(const Device &dev, const VkImageCreateInfo &info) { init(dev, info, 0); }
    void init_no_mem(const Device &dev, const VkImageCreateInfo &info);

    // get the internal memory
    const DeviceMemory &memory() const { return internal_mem_; }
    DeviceMemory &memory() { return internal_mem_; }

    // vkGetObjectMemoryRequirements()
    VkMemoryRequirements memory_requirements() const;

    // vkBindObjectMemory()
    void bind_memory(const DeviceMemory &mem, VkDeviceSize mem_offset);

    // vkGetImageSubresourceLayout()
    VkSubresourceLayout subresource_layout(const VkImageSubresource &subres) const;
    VkSubresourceLayout subresource_layout(const VkImageSubresourceLayers &subres) const;

    bool transparent() const;
    bool copyable() const { return (format_features_ & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT); }

    VkImageSubresourceRange subresource_range(VkImageAspectFlagBits aspect) const {
        return subresource_range(create_info_, aspect);
    }
    VkExtent3D extent() const { return create_info_.extent; }
    VkExtent3D extent(uint32_t mip_level) const { return extent(create_info_.extent, mip_level); }
    VkFormat format() const { return create_info_.format; }

    VkImageMemoryBarrier image_memory_barrier(VkFlags output_mask, VkFlags input_mask, VkImageLayout old_layout,
                                              VkImageLayout new_layout, const VkImageSubresourceRange &range) const {
        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcAccessMask = output_mask;
        barrier.dstAccessMask = input_mask;
        barrier.oldLayout = old_layout;
        barrier.newLayout = new_layout;
        barrier.image = handle();
        barrier.subresourceRange = range;
        return barrier;
    }

    static VkImageCreateInfo create_info();
    static VkImageSubresource subresource(VkImageAspectFlags aspect, uint32_t mip_level, uint32_t array_layer);
    static VkImageSubresource subresource(const VkImageSubresourceRange &range, uint32_t mip_level, uint32_t array_layer);
    static VkImageSubresourceLayers subresource(VkImageAspectFlags aspect, uint32_t mip_level, uint32_t array_layer,
                                                uint32_t array_size);
    static VkImageSubresourceLayers subresource(const VkImageSubresourceRange &range, uint32_t mip_level, uint32_t array_layer,
                                                uint32_t array_size);
    static VkImageSubresourceRange subresource_range(VkImageAspectFlags aspect_mask, uint32_t base_mip_level, uint32_t mip_levels,
                                                     uint32_t base_array_layer, uint32_t num_layers);
    static VkImageSubresourceRange subresource_range(const VkImageCreateInfo &info, VkImageAspectFlags aspect_mask);
    static VkImageSubresourceRange subresource_range(const VkImageSubresource &subres);

    static VkExtent2D extent(int32_t width, int32_t height);
    static VkExtent2D extent(const VkExtent2D &extent, uint32_t mip_level);
    static VkExtent2D extent(const VkExtent3D &extent);

    static VkExtent3D extent(int32_t width, int32_t height, int32_t depth);
    static VkExtent3D extent(const VkExtent3D &extent, uint32_t mip_level);

  private:
    void init_info(const Device &dev, const VkImageCreateInfo &info);

    VkImageCreateInfo create_info_;
    VkFlags format_features_;

    DeviceMemory internal_mem_;
};

class ImageView : public internal::NonDispHandle<VkImageView> {
  public:
    ~ImageView();

    // vkCreateImageView()
    void init(const Device &dev, const VkImageViewCreateInfo &info);
};

class ShaderModule : public internal::NonDispHandle<VkShaderModule> {
  public:
    ~ShaderModule();

    // vkCreateShaderModule()
    void init(const Device &dev, const VkShaderModuleCreateInfo &info);
    VkResult init_try(const Device &dev, const VkShaderModuleCreateInfo &info);

    static VkShaderModuleCreateInfo create_info(size_t code_size, const uint32_t *code, VkFlags flags);
};

class Pipeline : public internal::NonDispHandle<VkPipeline> {
  public:
    ~Pipeline();

    // vkCreateGraphicsPipeline()
    void init(const Device &dev, const VkGraphicsPipelineCreateInfo &info);
    // vkCreateGraphicsPipelineDerivative()
    void init(const Device &dev, const VkGraphicsPipelineCreateInfo &info, const VkPipeline basePipeline);
    // vkCreateComputePipeline()
    void init(const Device &dev, const VkComputePipelineCreateInfo &info);
    // vkLoadPipeline()
    void init(const Device &dev, size_t size, const void *data);
    // vkLoadPipelineDerivative()
    void init(const Device &dev, size_t size, const void *data, VkPipeline basePipeline);

    // vkCreateGraphicsPipeline with error return
    VkResult init_try(const Device &dev, const VkGraphicsPipelineCreateInfo &info);

    // vkStorePipeline()
    size_t store(size_t size, void *data);
};

class PipelineLayout : public internal::NonDispHandle<VkPipelineLayout> {
  public:
    ~PipelineLayout();

    // vCreatePipelineLayout()
    void init(const Device &dev, VkPipelineLayoutCreateInfo &info, const std::vector<const DescriptorSetLayout *> &layouts);
};

class Sampler : public internal::NonDispHandle<VkSampler> {
  public:
    ~Sampler();

    // vkCreateSampler()
    void init(const Device &dev, const VkSamplerCreateInfo &info);
};

class DescriptorSetLayout : public internal::NonDispHandle<VkDescriptorSetLayout> {
  public:
    ~DescriptorSetLayout();

    // vkCreateDescriptorSetLayout()
    void init(const Device &dev, const VkDescriptorSetLayoutCreateInfo &info);
};

class DescriptorPool : public internal::NonDispHandle<VkDescriptorPool> {
  public:
    ~DescriptorPool();

    // Descriptor sets allocated from this pool will need access to the original
    // object
    VkDescriptorPool GetObj() { return pool_; }

    // vkCreateDescriptorPool()
    void init(const Device &dev, const VkDescriptorPoolCreateInfo &info);

    // vkResetDescriptorPool()
    void reset();

    // vkFreeDescriptorSet()
    void setDynamicUsage(bool isDynamic) { dynamic_usage_ = isDynamic; }
    bool getDynamicUsage() { return dynamic_usage_; }

    // vkAllocateDescriptorSets()
    std::vector<DescriptorSet *> alloc_sets(const Device &dev, const std::vector<const DescriptorSetLayout *> &layouts);
    std::vector<DescriptorSet *> alloc_sets(const Device &dev, const DescriptorSetLayout &layout, uint32_t count);
    DescriptorSet *alloc_sets(const Device &dev, const DescriptorSetLayout &layout);

  private:
    VkDescriptorPool pool_;

    // Track whether this pool's usage is VK_DESCRIPTOR_POOL_USAGE_DYNAMIC
    bool dynamic_usage_;
};

class DescriptorSet : public internal::NonDispHandle<VkDescriptorSet> {
  public:
    ~DescriptorSet();

    explicit DescriptorSet() : NonDispHandle() {}
    explicit DescriptorSet(const Device &dev, DescriptorPool *pool, VkDescriptorSet set) : NonDispHandle(dev.handle(), set) {
        containing_pool_ = pool;
    }

  private:
    DescriptorPool *containing_pool_;
};

class CommandPool : public internal::NonDispHandle<VkCommandPool> {
  public:
    ~CommandPool();

    explicit CommandPool() : NonDispHandle() {}
    explicit CommandPool(const Device &dev, const VkCommandPoolCreateInfo &info) { init(dev, info); }

    void init(const Device &dev, const VkCommandPoolCreateInfo &info);

    static VkCommandPoolCreateInfo create_info(uint32_t queue_family_index);
};

inline VkCommandPoolCreateInfo CommandPool::create_info(uint32_t queue_family_index) {
    VkCommandPoolCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.queueFamilyIndex = queue_family_index;
    info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    return info;
}

class CommandBuffer : public internal::Handle<VkCommandBuffer> {
  public:
    ~CommandBuffer();

    explicit CommandBuffer() : Handle() {}
    explicit CommandBuffer(const Device &dev, const VkCommandBufferAllocateInfo &info) { init(dev, info); }

    // vkAllocateCommandBuffers()
    void init(const Device &dev, const VkCommandBufferAllocateInfo &info);

    // vkBeginCommandBuffer()
    void begin(const VkCommandBufferBeginInfo *info);
    void begin();

    // vkEndCommandBuffer()
    // vkResetCommandBuffer()
    void end();
    void reset(VkCommandBufferResetFlags flags);
    void reset() { reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT); }

    static VkCommandBufferAllocateInfo create_info(VkCommandPool const &pool);

  private:
    VkDevice dev_handle_;
    VkCommandPool cmd_pool_;
};

inline VkMemoryAllocateInfo DeviceMemory::alloc_info(VkDeviceSize size, uint32_t memory_type_index) {
    VkMemoryAllocateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    info.allocationSize = size;
    info.memoryTypeIndex = memory_type_index;
    return info;
}

inline VkBufferCreateInfo Buffer::create_info(VkDeviceSize size, VkFlags usage) {
    VkBufferCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size = size;
    info.usage = usage;
    return info;
}

inline VkFenceCreateInfo Fence::create_info(VkFenceCreateFlags flags) {
    VkFenceCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    info.flags = flags;
    return info;
}

inline VkFenceCreateInfo Fence::create_info() {
    VkFenceCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    return info;
}

inline VkSemaphoreCreateInfo Semaphore::create_info(VkFlags flags) {
    VkSemaphoreCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    info.flags = flags;
    return info;
}

inline VkEventCreateInfo Event::create_info(VkFlags flags) {
    VkEventCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
    info.flags = flags;
    return info;
}

inline VkQueryPoolCreateInfo QueryPool::create_info(VkQueryType type, uint32_t slot_count) {
    VkQueryPoolCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    info.queryType = type;
    info.queryCount = slot_count;
    return info;
}

inline VkImageCreateInfo Image::create_info() {
    VkImageCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.extent.width = 1;
    info.extent.height = 1;
    info.extent.depth = 1;
    info.mipLevels = 1;
    info.arrayLayers = 1;
    info.samples = VK_SAMPLE_COUNT_1_BIT;
    return info;
}

inline VkImageSubresource Image::subresource(VkImageAspectFlags aspect, uint32_t mip_level, uint32_t array_layer) {
    VkImageSubresource subres = {};
    if (aspect == 0) {
        assert(!"Invalid VkImageAspectFlags");
    }
    subres.aspectMask = aspect;
    subres.mipLevel = mip_level;
    subres.arrayLayer = array_layer;
    return subres;
}

inline VkImageSubresource Image::subresource(const VkImageSubresourceRange &range, uint32_t mip_level, uint32_t array_layer) {
    return subresource(range.aspectMask, range.baseMipLevel + mip_level, range.baseArrayLayer + array_layer);
}

inline VkImageSubresourceLayers Image::subresource(VkImageAspectFlags aspect, uint32_t mip_level, uint32_t array_layer,
                                                   uint32_t array_size) {
    VkImageSubresourceLayers subres = {};
    switch (aspect) {
    case VK_IMAGE_ASPECT_COLOR_BIT:
    case VK_IMAGE_ASPECT_DEPTH_BIT:
    case VK_IMAGE_ASPECT_STENCIL_BIT:
    case VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT:
        /* valid */
        break;
    default:
        assert(!"Invalid VkImageAspectFlags");
    }
    subres.aspectMask = aspect;
    subres.mipLevel = mip_level;
    subres.baseArrayLayer = array_layer;
    subres.layerCount = array_size;
    return subres;
}

inline VkImageSubresourceLayers Image::subresource(const VkImageSubresourceRange &range, uint32_t mip_level, uint32_t array_layer,
                                                   uint32_t array_size) {
    return subresource(range.aspectMask, range.baseMipLevel + mip_level, range.baseArrayLayer + array_layer, array_size);
}

inline VkImageSubresourceRange Image::subresource_range(VkImageAspectFlags aspect_mask, uint32_t base_mip_level,
                                                        uint32_t mip_levels, uint32_t base_array_layer, uint32_t num_layers) {
    VkImageSubresourceRange range = {};
    if (aspect_mask == 0) {
        assert(!"Invalid VkImageAspectFlags");
    }
    range.aspectMask = aspect_mask;
    range.baseMipLevel = base_mip_level;
    range.levelCount = mip_levels;
    range.baseArrayLayer = base_array_layer;
    range.layerCount = num_layers;
    return range;
}

inline VkImageSubresourceRange Image::subresource_range(const VkImageCreateInfo &info, VkImageAspectFlags aspect_mask) {
    return subresource_range(aspect_mask, 0, info.mipLevels, 0, info.arrayLayers);
}

inline VkImageSubresourceRange Image::subresource_range(const VkImageSubresource &subres) {
    return subresource_range(subres.aspectMask, subres.mipLevel, 1, subres.arrayLayer, 1);
}

inline VkExtent2D Image::extent(int32_t width, int32_t height) {
    VkExtent2D extent = {};
    extent.width = width;
    extent.height = height;
    return extent;
}

inline VkExtent2D Image::extent(const VkExtent2D &extent, uint32_t mip_level) {
    const int32_t width = (extent.width >> mip_level) ? extent.width >> mip_level : 1;
    const int32_t height = (extent.height >> mip_level) ? extent.height >> mip_level : 1;
    return Image::extent(width, height);
}

inline VkExtent2D Image::extent(const VkExtent3D &extent) { return Image::extent(extent.width, extent.height); }

inline VkExtent3D Image::extent(int32_t width, int32_t height, int32_t depth) {
    VkExtent3D extent = {};
    extent.width = width;
    extent.height = height;
    extent.depth = depth;
    return extent;
}

inline VkExtent3D Image::extent(const VkExtent3D &extent, uint32_t mip_level) {
    const int32_t width = (extent.width >> mip_level) ? extent.width >> mip_level : 1;
    const int32_t height = (extent.height >> mip_level) ? extent.height >> mip_level : 1;
    const int32_t depth = (extent.depth >> mip_level) ? extent.depth >> mip_level : 1;
    return Image::extent(width, height, depth);
}

inline VkShaderModuleCreateInfo ShaderModule::create_info(size_t code_size, const uint32_t *code, VkFlags flags) {
    VkShaderModuleCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = code_size;
    info.pCode = code;
    info.flags = flags;
    return info;
}

inline VkWriteDescriptorSet Device::write_descriptor_set(const DescriptorSet &set, uint32_t binding, uint32_t array_element,
                                                         VkDescriptorType type, uint32_t count,
                                                         const VkDescriptorImageInfo *image_info) {
    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = set.handle();
    write.dstBinding = binding;
    write.dstArrayElement = array_element;
    write.descriptorCount = count;
    write.descriptorType = type;
    write.pImageInfo = image_info;
    return write;
}

inline VkWriteDescriptorSet Device::write_descriptor_set(const DescriptorSet &set, uint32_t binding, uint32_t array_element,
                                                         VkDescriptorType type, uint32_t count,
                                                         const VkDescriptorBufferInfo *buffer_info) {
    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = set.handle();
    write.dstBinding = binding;
    write.dstArrayElement = array_element;
    write.descriptorCount = count;
    write.descriptorType = type;
    write.pBufferInfo = buffer_info;
    return write;
}

inline VkWriteDescriptorSet Device::write_descriptor_set(const DescriptorSet &set, uint32_t binding, uint32_t array_element,
                                                         VkDescriptorType type, uint32_t count, const VkBufferView *buffer_views) {
    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = set.handle();
    write.dstBinding = binding;
    write.dstArrayElement = array_element;
    write.descriptorCount = count;
    write.descriptorType = type;
    write.pTexelBufferView = buffer_views;
    return write;
}

inline VkWriteDescriptorSet Device::write_descriptor_set(const DescriptorSet &set, uint32_t binding, uint32_t array_element,
                                                         VkDescriptorType type,
                                                         const std::vector<VkDescriptorImageInfo> &image_info) {
    return write_descriptor_set(set, binding, array_element, type, image_info.size(), &image_info[0]);
}

inline VkWriteDescriptorSet Device::write_descriptor_set(const DescriptorSet &set, uint32_t binding, uint32_t array_element,
                                                         VkDescriptorType type,
                                                         const std::vector<VkDescriptorBufferInfo> &buffer_info) {
    return write_descriptor_set(set, binding, array_element, type, buffer_info.size(), &buffer_info[0]);
}

inline VkWriteDescriptorSet Device::write_descriptor_set(const DescriptorSet &set, uint32_t binding, uint32_t array_element,
                                                         VkDescriptorType type, const std::vector<VkBufferView> &buffer_views) {
    return write_descriptor_set(set, binding, array_element, type, buffer_views.size(), &buffer_views[0]);
}

inline VkCopyDescriptorSet Device::copy_descriptor_set(const DescriptorSet &src_set, uint32_t src_binding,
                                                       uint32_t src_array_element, const DescriptorSet &dst_set,
                                                       uint32_t dst_binding, uint32_t dst_array_element, uint32_t count) {
    VkCopyDescriptorSet copy = {};
    copy.sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;
    copy.srcSet = src_set.handle();
    copy.srcBinding = src_binding;
    copy.srcArrayElement = src_array_element;
    copy.dstSet = dst_set.handle();
    copy.dstBinding = dst_binding;
    copy.dstArrayElement = dst_array_element;
    copy.descriptorCount = count;

    return copy;
}

inline VkCommandBufferAllocateInfo CommandBuffer::create_info(VkCommandPool const &pool) {
    VkCommandBufferAllocateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.commandPool = pool;
    info.commandBufferCount = 1;
    return info;
}

}; // namespace vk_testing

#endif // VKTESTBINDING_H
