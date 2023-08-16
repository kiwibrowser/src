// Copyright 2018 The Dawn Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "dawn_native/vulkan/RenderPassCache.h"

#include "common/BitSetIterator.h"
#include "common/HashUtils.h"
#include "dawn_native/vulkan/DeviceVk.h"
#include "dawn_native/vulkan/TextureVk.h"
#include "dawn_native/vulkan/VulkanError.h"

namespace dawn_native { namespace vulkan {

    namespace {
        VkAttachmentLoadOp VulkanAttachmentLoadOp(wgpu::LoadOp op) {
            switch (op) {
                case wgpu::LoadOp::Load:
                    return VK_ATTACHMENT_LOAD_OP_LOAD;
                case wgpu::LoadOp::Clear:
                    return VK_ATTACHMENT_LOAD_OP_CLEAR;
                default:
                    UNREACHABLE();
            }
        }
    }  // anonymous namespace

    // RenderPassCacheQuery

    void RenderPassCacheQuery::SetColor(uint32_t index,
                                        wgpu::TextureFormat format,
                                        wgpu::LoadOp loadOp,
                                        bool hasResolveTarget) {
        colorMask.set(index);
        colorFormats[index] = format;
        colorLoadOp[index] = loadOp;
        resolveTargetMask[index] = hasResolveTarget;
    }

    void RenderPassCacheQuery::SetDepthStencil(wgpu::TextureFormat format,
                                               wgpu::LoadOp depthLoadOp,
                                               wgpu::LoadOp stencilLoadOp) {
        hasDepthStencil = true;
        depthStencilFormat = format;
        this->depthLoadOp = depthLoadOp;
        this->stencilLoadOp = stencilLoadOp;
    }

    void RenderPassCacheQuery::SetSampleCount(uint32_t sampleCount) {
        this->sampleCount = sampleCount;
    }

    // RenderPassCache

    RenderPassCache::RenderPassCache(Device* device) : mDevice(device) {
    }

    RenderPassCache::~RenderPassCache() {
        for (auto it : mCache) {
            mDevice->fn.DestroyRenderPass(mDevice->GetVkDevice(), it.second, nullptr);
        }
        mCache.clear();
    }

    ResultOrError<VkRenderPass> RenderPassCache::GetRenderPass(const RenderPassCacheQuery& query) {
        auto it = mCache.find(query);
        if (it != mCache.end()) {
            return VkRenderPass(it->second);
        }

        VkRenderPass renderPass;
        DAWN_TRY_ASSIGN(renderPass, CreateRenderPassForQuery(query));
        mCache.emplace(query, renderPass);
        return renderPass;
    }

    ResultOrError<VkRenderPass> RenderPassCache::CreateRenderPassForQuery(
        const RenderPassCacheQuery& query) const {
        // The Vulkan subpasses want to know the layout of the attachments with VkAttachmentRef.
        // Precompute them as they must be pointer-chained in VkSubpassDescription
        std::array<VkAttachmentReference, kMaxColorAttachments> colorAttachmentRefs;
        std::array<VkAttachmentReference, kMaxColorAttachments> resolveAttachmentRefs;
        VkAttachmentReference depthStencilAttachmentRef;

        // Contains the attachment description that will be chained in the create info
        // The order of all attachments in attachmentDescs is "color-depthstencil-resolve".
        constexpr uint32_t kMaxAttachmentCount = kMaxColorAttachments * 2 + 1;
        std::array<VkAttachmentDescription, kMaxAttachmentCount> attachmentDescs = {};

        VkSampleCountFlagBits vkSampleCount = VulkanSampleCount(query.sampleCount);

        uint32_t colorAttachmentIndex = 0;
        for (uint32_t i : IterateBitSet(query.colorMask)) {
            auto& attachmentRef = colorAttachmentRefs[colorAttachmentIndex];
            auto& attachmentDesc = attachmentDescs[colorAttachmentIndex];

            attachmentRef.attachment = colorAttachmentIndex;
            attachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            attachmentDesc.flags = 0;
            attachmentDesc.format = VulkanImageFormat(mDevice, query.colorFormats[i]);
            attachmentDesc.samples = vkSampleCount;
            attachmentDesc.loadOp = VulkanAttachmentLoadOp(query.colorLoadOp[i]);
            attachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            ++colorAttachmentIndex;
        }

        uint32_t attachmentCount = colorAttachmentIndex;
        VkAttachmentReference* depthStencilAttachment = nullptr;
        if (query.hasDepthStencil) {
            auto& attachmentDesc = attachmentDescs[attachmentCount];

            depthStencilAttachment = &depthStencilAttachmentRef;

            depthStencilAttachmentRef.attachment = attachmentCount;
            depthStencilAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            attachmentDesc.flags = 0;
            attachmentDesc.format = VulkanImageFormat(mDevice, query.depthStencilFormat);
            attachmentDesc.samples = vkSampleCount;
            attachmentDesc.loadOp = VulkanAttachmentLoadOp(query.depthLoadOp);
            attachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachmentDesc.stencilLoadOp = VulkanAttachmentLoadOp(query.stencilLoadOp);
            attachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            ++attachmentCount;
        }

        uint32_t resolveAttachmentIndex = 0;
        for (uint32_t i : IterateBitSet(query.resolveTargetMask)) {
            auto& attachmentRef = resolveAttachmentRefs[resolveAttachmentIndex];
            auto& attachmentDesc = attachmentDescs[attachmentCount];

            attachmentRef.attachment = attachmentCount;
            attachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            attachmentDesc.flags = 0;
            attachmentDesc.format = VulkanImageFormat(mDevice, query.colorFormats[i]);
            attachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
            attachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            ++attachmentCount;
            ++resolveAttachmentIndex;
        }

        VkAttachmentReference* resolveTargetAttachmentRefs =
            query.resolveTargetMask.any() ? resolveAttachmentRefs.data() : nullptr;

        // Create the VkSubpassDescription that will be chained in the VkRenderPassCreateInfo
        VkSubpassDescription subpassDesc;
        subpassDesc.flags = 0;
        subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDesc.inputAttachmentCount = 0;
        subpassDesc.pInputAttachments = nullptr;
        subpassDesc.colorAttachmentCount = colorAttachmentIndex;
        subpassDesc.pColorAttachments = colorAttachmentRefs.data();
        subpassDesc.pResolveAttachments = resolveTargetAttachmentRefs;
        subpassDesc.pDepthStencilAttachment = depthStencilAttachment;
        subpassDesc.preserveAttachmentCount = 0;
        subpassDesc.pPreserveAttachments = nullptr;

        // Chain everything in VkRenderPassCreateInfo
        VkRenderPassCreateInfo createInfo;
        createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.attachmentCount = attachmentCount;
        createInfo.pAttachments = attachmentDescs.data();
        createInfo.subpassCount = 1;
        createInfo.pSubpasses = &subpassDesc;
        createInfo.dependencyCount = 0;
        createInfo.pDependencies = nullptr;

        // Create the render pass from the zillion parameters
        VkRenderPass renderPass;
        DAWN_TRY(CheckVkSuccess(mDevice->fn.CreateRenderPass(mDevice->GetVkDevice(), &createInfo,
                                                             nullptr, &*renderPass),
                                "CreateRenderPass"));
        return renderPass;
    }

    // RenderPassCache

    size_t RenderPassCache::CacheFuncs::operator()(const RenderPassCacheQuery& query) const {
        size_t hash = Hash(query.colorMask);

        HashCombine(&hash, Hash(query.resolveTargetMask));

        for (uint32_t i : IterateBitSet(query.colorMask)) {
            HashCombine(&hash, query.colorFormats[i], query.colorLoadOp[i]);
        }

        HashCombine(&hash, query.hasDepthStencil);
        if (query.hasDepthStencil) {
            HashCombine(&hash, query.depthStencilFormat, query.depthLoadOp, query.stencilLoadOp);
        }

        HashCombine(&hash, query.sampleCount);

        return hash;
    }

    bool RenderPassCache::CacheFuncs::operator()(const RenderPassCacheQuery& a,
                                                 const RenderPassCacheQuery& b) const {
        if (a.colorMask != b.colorMask) {
            return false;
        }

        if (a.resolveTargetMask != b.resolveTargetMask) {
            return false;
        }

        if (a.sampleCount != b.sampleCount) {
            return false;
        }

        for (uint32_t i : IterateBitSet(a.colorMask)) {
            if ((a.colorFormats[i] != b.colorFormats[i]) ||
                (a.colorLoadOp[i] != b.colorLoadOp[i])) {
                return false;
            }
        }

        if (a.hasDepthStencil != b.hasDepthStencil) {
            return false;
        }

        if (a.hasDepthStencil) {
            if ((a.depthStencilFormat != b.depthStencilFormat) ||
                (a.depthLoadOp != b.depthLoadOp) || (a.stencilLoadOp != b.stencilLoadOp)) {
                return false;
            }
        }

        return true;
    }
}}  // namespace dawn_native::vulkan
