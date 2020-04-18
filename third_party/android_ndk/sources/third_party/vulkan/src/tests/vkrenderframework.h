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
 */

#ifndef VKRENDERFRAMEWORK_H
#define VKRENDERFRAMEWORK_H

#ifdef ANDROID
#include "vktestframeworkandroid.h"
class VkImageObj;
#else
#include "vktestframework.h"
#endif

#include <array>
#include <map>
#include <vector>

using namespace std;

class VkDeviceObj : public vk_testing::Device {
  public:
    VkDeviceObj(uint32_t id, VkPhysicalDevice obj);
    VkDeviceObj(uint32_t id, VkPhysicalDevice obj, std::vector<const char *> &extension_names,
                VkPhysicalDeviceFeatures *features = nullptr);

    VkDevice device() { return handle(); }
    void get_device_queue();

    uint32_t id;
    VkPhysicalDeviceProperties props;
    std::vector<VkQueueFamilyProperties> queue_props;

    VkQueue m_queue;
};

class VkCommandBufferObj;
class VkDepthStencilObj;

class VkRenderFramework : public VkTestFramework {
  public:
    VkRenderFramework();
    ~VkRenderFramework();

    VkInstance instance() { return inst; }
    VkDevice device() { return m_device->device(); }
    VkPhysicalDevice gpu() { return objs[0]; }
    VkRenderPass renderPass() { return m_renderPass; }
    VkFramebuffer framebuffer() { return m_framebuffer; }
    void InitViewport(float width, float height);
    void InitViewport();
    void InitRenderTarget();
    void InitRenderTarget(uint32_t targets);
    void InitRenderTarget(VkImageView *dsBinding);
    void InitRenderTarget(uint32_t targets, VkImageView *dsBinding);
    void InitFramework();
    void InitFramework(std::vector<const char *> instance_layer_names, std::vector<const char *> instance_extension_names,
                       std::vector<const char *> device_extension_names, PFN_vkDebugReportCallbackEXT = NULL,
                       void *userData = NULL);

    void ShutdownFramework();
    void InitState(VkPhysicalDeviceFeatures *features = nullptr);

    const VkRenderPassBeginInfo &renderPassBeginInfo() const { return m_renderPassBeginInfo; }

  protected:
    VkApplicationInfo app_info;
    VkInstance inst;
    VkPhysicalDevice objs[16];
    uint32_t gpu_count;
    VkDeviceObj *m_device;
    VkCommandPool m_commandPool;
    VkCommandBufferObj *m_commandBuffer;
    VkRenderPass m_renderPass;
    VkFramebuffer m_framebuffer;
    std::vector<VkViewport> m_viewports;
    std::vector<VkRect2D> m_scissors;
    float m_lineWidth;
    float m_depthBiasConstantFactor;
    float m_depthBiasClamp;
    float m_depthBiasSlopeFactor;
    float m_blendConstants[4];
    float m_minDepthBounds;
    float m_maxDepthBounds;
    uint32_t m_compareMask;
    uint32_t m_writeMask;
    uint32_t m_reference;
    std::vector<VkClearValue> m_renderPassClearValues;
    VkRenderPassBeginInfo m_renderPassBeginInfo;
    vector<VkImageObj *> m_renderTargets;
    float m_width, m_height;
    VkFormat m_render_target_fmt;
    VkFormat m_depth_stencil_fmt;
    VkClearColorValue m_clear_color;
    bool m_clear_via_load_op;
    float m_depth_clear_color;
    uint32_t m_stencil_clear_color;
    VkDepthStencilObj *m_depthStencil;
    PFN_vkCreateDebugReportCallbackEXT m_CreateDebugReportCallback;
    PFN_vkDestroyDebugReportCallbackEXT m_DestroyDebugReportCallback;
    PFN_vkDebugReportMessageEXT m_DebugReportMessage;
    VkDebugReportCallbackEXT m_globalMsgCallback;
    VkDebugReportCallbackEXT m_devMsgCallback;
    std::vector<const char *> device_extension_names;

    /*
     * SetUp and TearDown are called by the Google Test framework
     * to initialize a test framework based on this class.
     */
    virtual void SetUp() {
        this->app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        this->app_info.pNext = NULL;
        this->app_info.pApplicationName = "base";
        this->app_info.applicationVersion = 1;
        this->app_info.pEngineName = "unittest";
        this->app_info.engineVersion = 1;
        this->app_info.apiVersion = VK_API_VERSION_1_0;

        InitFramework();
    }

    virtual void TearDown() { ShutdownFramework(); }
};

class VkDescriptorSetObj;
class VkIndexBufferObj;
class VkConstantBufferObj;
class VkPipelineObj;
class VkDescriptorSetObj;

class VkCommandBufferObj : public vk_testing::CommandBuffer {
  public:
    VkCommandBufferObj(VkDeviceObj *device, VkCommandPool pool);
    VkCommandBuffer GetBufferHandle();
    VkResult BeginCommandBuffer();
    VkResult BeginCommandBuffer(VkCommandBufferBeginInfo *pInfo);
    VkResult EndCommandBuffer();
    void PipelineBarrier(VkPipelineStageFlags src_stages, VkPipelineStageFlags dest_stages, VkDependencyFlags dependencyFlags,
                         uint32_t memoryBarrierCount, const VkMemoryBarrier *pMemoryBarriers, uint32_t bufferMemoryBarrierCount,
                         const VkBufferMemoryBarrier *pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount,
                         const VkImageMemoryBarrier *pImageMemoryBarriers);
    void AddRenderTarget(VkImageObj *renderTarget);
    void AddDepthStencil();
    void ClearAllBuffers(VkClearColorValue clear_color, float depth_clear_color, uint32_t stencil_clear_color,
                         VkDepthStencilObj *depthStencilObj);
    void PrepareAttachments();
    void BindPipeline(VkPipelineObj &pipeline);
    void BindDescriptorSet(VkDescriptorSetObj &descriptorSet);
    void BindVertexBuffer(VkConstantBufferObj *vertexBuffer, VkDeviceSize offset, uint32_t binding);
    void BindIndexBuffer(VkIndexBufferObj *indexBuffer, VkDeviceSize offset);
    void BeginRenderPass(const VkRenderPassBeginInfo &info);
    void EndRenderPass();
    void FillBuffer(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize fill_size, uint32_t data);
    void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
    void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset,
                     uint32_t firstInstance);
    void QueueCommandBuffer(bool checkSuccess = true);
    void QueueCommandBuffer(VkFence fence, bool checkSuccess = true);
    void SetViewport(uint32_t firstViewport, uint32_t viewportCount, const VkViewport *pViewports);
    void SetScissor(uint32_t firstScissor, uint32_t scissorCount, const VkRect2D *pScissors);
    void SetLineWidth(float lineWidth);
    void SetDepthBias(float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor);
    void SetBlendConstants(const float blendConstants[4]);
    void SetDepthBounds(float minDepthBounds, float maxDepthBounds);
    void SetStencilReadMask(VkStencilFaceFlags faceMask, uint32_t compareMask);
    void SetStencilWriteMask(VkStencilFaceFlags faceMask, uint32_t writeMask);
    void SetStencilReference(VkStencilFaceFlags faceMask, uint32_t reference);
    void UpdateBuffer(VkBuffer buffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const void *pData);
    void CopyImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout,
                   uint32_t regionCount, const VkImageCopy *pRegions);
    void ResolveImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout,
                      uint32_t regionCount, const VkImageResolve *pRegions);

  protected:
    VkDeviceObj *m_device;
    vector<VkImageObj *> m_renderTargets;
};

class VkConstantBufferObj : public vk_testing::Buffer {
  public:
    VkConstantBufferObj(VkDeviceObj *device,
                        VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    VkConstantBufferObj(VkDeviceObj *device, int constantCount, int constantSize, const void *data,
                        VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    ~VkConstantBufferObj();
    void BufferMemoryBarrier(VkFlags srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_SHADER_WRITE_BIT |
                                                     VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                                                     VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT,
                             VkFlags dstAccessMask = VK_ACCESS_HOST_READ_BIT | VK_ACCESS_INDIRECT_COMMAND_READ_BIT |
                                                     VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
                                                     VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_READ_BIT |
                                                     VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                                     VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_MEMORY_READ_BIT);

    void Bind(VkCommandBuffer commandBuffer, VkDeviceSize offset, uint32_t binding);

    VkDescriptorBufferInfo m_descriptorBufferInfo;

  protected:
    VkDeviceObj *m_device;
    vk_testing::BufferView m_bufferView;
    int m_numVertices;
    int m_stride;
    vk_testing::CommandPool *m_commandPool;
    VkCommandBufferObj *m_commandBuffer;
    vk_testing::Fence m_fence;
};

class VkIndexBufferObj : public VkConstantBufferObj {
  public:
    VkIndexBufferObj(VkDeviceObj *device);
    void CreateAndInitBuffer(int numIndexes, VkIndexType dataFormat, const void *data);
    void Bind(VkCommandBuffer commandBuffer, VkDeviceSize offset);
    VkIndexType GetIndexType();

  protected:
    VkIndexType m_indexType;
};

class VkRenderpassObj {
  public:
    VkRenderpassObj(VkDeviceObj *device);
    ~VkRenderpassObj();
    VkRenderPass handle() { return m_renderpass; }

  protected:
    VkRenderPass m_renderpass;
    VkDevice device;
};

class VkImageObj : public vk_testing::Image {
  public:
    VkImageObj(VkDeviceObj *dev);
    bool IsCompatible(VkFlags usage, VkFlags features);

  public:
    void init(uint32_t w, uint32_t h, VkFormat fmt, VkFlags usage, VkImageTiling tiling = VK_IMAGE_TILING_LINEAR,
              VkMemoryPropertyFlags reqs = 0);

    void init_no_layout(uint32_t w, uint32_t h, VkFormat fmt, VkFlags usage, VkImageTiling tiling = VK_IMAGE_TILING_LINEAR,
                        VkMemoryPropertyFlags reqs = 0);

    //    void clear( CommandBuffer*, uint32_t[4] );

    void layout(VkImageLayout layout) { m_descriptorImageInfo.imageLayout = layout; }

    VkDeviceMemory memory() const { return Image::memory().handle(); }

    void *MapMemory() { return Image::memory().map(); }

    void UnmapMemory() { Image::memory().unmap(); }

    void ImageMemoryBarrier(VkCommandBufferObj *cmd, VkImageAspectFlags aspect, VkFlags output_mask, VkFlags input_mask,
                            VkImageLayout image_layout);

    VkResult CopyImage(VkImageObj &src_image);

    VkImage image() const { return handle(); }

    VkImageView targetView(VkFormat format) {
        if (!m_targetView.initialized()) {
            VkImageViewCreateInfo createView = {};
            createView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createView.image = handle();
            createView.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createView.format = format;
            createView.components.r = VK_COMPONENT_SWIZZLE_R;
            createView.components.g = VK_COMPONENT_SWIZZLE_G;
            createView.components.b = VK_COMPONENT_SWIZZLE_B;
            createView.components.a = VK_COMPONENT_SWIZZLE_A;
            createView.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
            createView.flags = 0;
            m_targetView.init(*m_device, createView);
        }
        return m_targetView.handle();
    }

    void SetLayout(VkCommandBufferObj *cmd_buf, VkImageAspectFlags aspect, VkImageLayout image_layout);
    void SetLayout(VkImageAspectFlags aspect, VkImageLayout image_layout);

    VkImageLayout layout() const { return m_descriptorImageInfo.imageLayout; }
    uint32_t width() const { return extent().width; }
    uint32_t height() const { return extent().height; }
    VkDeviceObj *device() const { return m_device; }

  protected:
    VkDeviceObj *m_device;

    vk_testing::ImageView m_targetView;
    VkDescriptorImageInfo m_descriptorImageInfo;
};

class VkTextureObj : public VkImageObj {
  public:
    VkTextureObj(VkDeviceObj *device, uint32_t *colors = NULL);

    VkDescriptorImageInfo m_imageInfo;

  protected:
    VkDeviceObj *m_device;
    vk_testing::ImageView m_textureView;
    VkDeviceSize m_rowPitch;
};

class VkDepthStencilObj : public VkImageObj {
  public:
    VkDepthStencilObj(VkDeviceObj *device);
    void Init(VkDeviceObj *device, int32_t width, int32_t height, VkFormat format,
              VkImageUsageFlags usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    bool Initialized();
    VkImageView *BindInfo();

  protected:
    VkDeviceObj *m_device;
    bool m_initialized;
    vk_testing::ImageView m_imageView;
    VkFormat m_depth_stencil_fmt;
    VkImageView m_attachmentBindInfo;
};

class VkSamplerObj : public vk_testing::Sampler {
  public:
    VkSamplerObj(VkDeviceObj *device);

  protected:
    VkDeviceObj *m_device;
};

class VkDescriptorSetObj : public vk_testing::DescriptorPool {
  public:
    VkDescriptorSetObj(VkDeviceObj *device);
    ~VkDescriptorSetObj();

    int AppendDummy();
    int AppendBuffer(VkDescriptorType type, VkConstantBufferObj &constantBuffer);
    int AppendSamplerTexture(VkSamplerObj *sampler, VkTextureObj *texture);
    void CreateVKDescriptorSet(VkCommandBufferObj *commandBuffer);

    VkDescriptorSet GetDescriptorSetHandle() const;
    VkPipelineLayout GetPipelineLayout() const;
    int GetTypeCounts() { return m_type_counts.size(); }

  protected:
    VkDeviceObj *m_device;
    std::vector<VkDescriptorSetLayoutBinding> m_layout_bindings;
    std::map<VkDescriptorType, int> m_type_counts;
    int m_nextSlot;

    vector<VkDescriptorImageInfo> m_imageSamplerDescriptors;
    vector<VkWriteDescriptorSet> m_writes;

    vk_testing::DescriptorSetLayout m_layout;
    vk_testing::PipelineLayout m_pipeline_layout;
    vk_testing::DescriptorSet *m_set = NULL;
};

class VkShaderObj : public vk_testing::ShaderModule {
  public:
    VkShaderObj(VkDeviceObj *device, const char *shaderText, VkShaderStageFlagBits stage, VkRenderFramework *framework,
                char const *name = "main");
    VkPipelineShaderStageCreateInfo GetStageCreateInfo() const;

  protected:
    VkPipelineShaderStageCreateInfo stage_info;
    VkShaderStageFlagBits m_stage;
    char const *m_name;
    VkDeviceObj *m_device;
};

class VkPipelineObj : public vk_testing::Pipeline {
  public:
    VkPipelineObj(VkDeviceObj *device);
    void AddShader(VkShaderObj *shaderObj);
    void AddVertexInputAttribs(VkVertexInputAttributeDescription *vi_attrib, uint32_t count);
    void AddVertexInputBindings(VkVertexInputBindingDescription *vi_binding, uint32_t count);
    void AddColorAttachment(uint32_t binding, const VkPipelineColorBlendAttachmentState *att);
    void MakeDynamic(VkDynamicState state);

    void AddColorAttachment() {
        VkPipelineColorBlendAttachmentState att = {};
        att.blendEnable = VK_FALSE;
        att.colorWriteMask = 0xf;
        AddColorAttachment(0, &att);
    }

    void SetDepthStencil(const VkPipelineDepthStencilStateCreateInfo *);
    void SetMSAA(const VkPipelineMultisampleStateCreateInfo *ms_state);
    void SetInputAssembly(const VkPipelineInputAssemblyStateCreateInfo *ia_state);
    void SetRasterization(const VkPipelineRasterizationStateCreateInfo *rs_state);
    void SetTessellation(const VkPipelineTessellationStateCreateInfo *te_state);
    void SetViewport(const vector<VkViewport> viewports);
    void SetScissor(const vector<VkRect2D> scissors);
    VkResult CreateVKPipeline(VkPipelineLayout layout, VkRenderPass render_pass);

  protected:
    VkPipelineVertexInputStateCreateInfo m_vi_state;
    VkPipelineInputAssemblyStateCreateInfo m_ia_state;
    VkPipelineRasterizationStateCreateInfo m_rs_state;
    VkPipelineColorBlendStateCreateInfo m_cb_state;
    VkPipelineDepthStencilStateCreateInfo const *m_ds_state;
    VkPipelineViewportStateCreateInfo m_vp_state;
    VkPipelineMultisampleStateCreateInfo m_ms_state;
    VkPipelineTessellationStateCreateInfo m_te_state;
    vector<VkDynamicState> m_dynamic_state_enables;
    vector<VkViewport> m_viewports;
    vector<VkRect2D> m_scissors;
    VkDeviceObj *m_device;
    vector<VkShaderObj *> m_shaderObjs;
    vector<int> m_vertexBufferBindings;
    vector<VkPipelineColorBlendAttachmentState> m_colorAttachments;
    int m_vertexBufferCount;
};
#endif // VKRENDERFRAMEWORK_H
