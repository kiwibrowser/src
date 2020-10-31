// Copyright 2017 The Dawn Authors
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

#include "dawn_native/vulkan/CommandBufferVk.h"

#include "dawn_native/BindGroupAndStorageBarrierTracker.h"
#include "dawn_native/CommandEncoder.h"
#include "dawn_native/CommandValidation.h"
#include "dawn_native/Commands.h"
#include "dawn_native/RenderBundle.h"
#include "dawn_native/vulkan/BindGroupVk.h"
#include "dawn_native/vulkan/BufferVk.h"
#include "dawn_native/vulkan/CommandRecordingContext.h"
#include "dawn_native/vulkan/ComputePipelineVk.h"
#include "dawn_native/vulkan/DeviceVk.h"
#include "dawn_native/vulkan/FencedDeleter.h"
#include "dawn_native/vulkan/PipelineLayoutVk.h"
#include "dawn_native/vulkan/RenderPassCache.h"
#include "dawn_native/vulkan/RenderPipelineVk.h"
#include "dawn_native/vulkan/TextureVk.h"
#include "dawn_native/vulkan/UtilsVulkan.h"
#include "dawn_native/vulkan/VulkanError.h"

namespace dawn_native { namespace vulkan {

    namespace {

        VkIndexType VulkanIndexType(wgpu::IndexFormat format) {
            switch (format) {
                case wgpu::IndexFormat::Uint16:
                    return VK_INDEX_TYPE_UINT16;
                case wgpu::IndexFormat::Uint32:
                    return VK_INDEX_TYPE_UINT32;
                default:
                    UNREACHABLE();
            }
        }

        bool HasSameTextureCopyExtent(const TextureCopy& srcCopy,
                                      const TextureCopy& dstCopy,
                                      const Extent3D& copySize) {
            Extent3D imageExtentSrc = ComputeTextureCopyExtent(srcCopy, copySize);
            Extent3D imageExtentDst = ComputeTextureCopyExtent(dstCopy, copySize);
            return imageExtentSrc.width == imageExtentDst.width &&
                   imageExtentSrc.height == imageExtentDst.height &&
                   imageExtentSrc.depth == imageExtentDst.depth;
        }

        VkImageCopy ComputeImageCopyRegion(const TextureCopy& srcCopy,
                                           const TextureCopy& dstCopy,
                                           const Extent3D& copySize) {
            const Texture* srcTexture = ToBackend(srcCopy.texture.Get());
            const Texture* dstTexture = ToBackend(dstCopy.texture.Get());

            VkImageCopy region;

            // TODO(jiawei.shao@intel.com): support 1D and 3D textures
            ASSERT(srcTexture->GetDimension() == wgpu::TextureDimension::e2D &&
                   dstTexture->GetDimension() == wgpu::TextureDimension::e2D);
            region.srcSubresource.aspectMask = srcTexture->GetVkAspectMask();
            region.srcSubresource.mipLevel = srcCopy.mipLevel;
            region.srcSubresource.baseArrayLayer = srcCopy.origin.z;
            region.srcSubresource.layerCount = copySize.depth;

            region.srcOffset.x = srcCopy.origin.x;
            region.srcOffset.y = srcCopy.origin.y;
            region.srcOffset.z = 0;

            region.dstSubresource.aspectMask = dstTexture->GetVkAspectMask();
            region.dstSubresource.mipLevel = dstCopy.mipLevel;
            region.dstSubresource.baseArrayLayer = dstCopy.origin.z;
            region.dstSubresource.layerCount = copySize.depth;

            region.dstOffset.x = dstCopy.origin.x;
            region.dstOffset.y = dstCopy.origin.y;
            region.dstOffset.z = 0;

            ASSERT(HasSameTextureCopyExtent(srcCopy, dstCopy, copySize));
            Extent3D imageExtent = ComputeTextureCopyExtent(dstCopy, copySize);
            region.extent.width = imageExtent.width;
            region.extent.height = imageExtent.height;
            region.extent.depth = 1;

            return region;
        }

        void ApplyDescriptorSets(
            Device* device,
            VkCommandBuffer commands,
            VkPipelineBindPoint bindPoint,
            VkPipelineLayout pipelineLayout,
            const BindGroupLayoutMask& bindGroupsToApply,
            const ityp::array<BindGroupIndex, BindGroupBase*, kMaxBindGroups>& bindGroups,
            const ityp::array<BindGroupIndex, uint32_t, kMaxBindGroups>& dynamicOffsetCounts,
            const ityp::array<BindGroupIndex,
                              std::array<uint32_t, kMaxDynamicBuffersPerPipelineLayout>,
                              kMaxBindGroups>& dynamicOffsets) {
            for (BindGroupIndex dirtyIndex : IterateBitSet(bindGroupsToApply)) {
                VkDescriptorSet set = ToBackend(bindGroups[dirtyIndex])->GetHandle();
                const uint32_t* dynamicOffset = dynamicOffsetCounts[dirtyIndex] > 0
                                                    ? dynamicOffsets[dirtyIndex].data()
                                                    : nullptr;
                device->fn.CmdBindDescriptorSets(commands, bindPoint, pipelineLayout,
                                                 static_cast<uint32_t>(dirtyIndex), 1, &*set,
                                                 dynamicOffsetCounts[dirtyIndex], dynamicOffset);
            }
        }

        class RenderDescriptorSetTracker : public BindGroupTrackerBase<true, uint32_t> {
          public:
            RenderDescriptorSetTracker() = default;

            void Apply(Device* device,
                       CommandRecordingContext* recordingContext,
                       VkPipelineBindPoint bindPoint) {
                ApplyDescriptorSets(device, recordingContext->commandBuffer, bindPoint,
                                    ToBackend(mPipelineLayout)->GetHandle(),
                                    mDirtyBindGroupsObjectChangedOrIsDynamic, mBindGroups,
                                    mDynamicOffsetCounts, mDynamicOffsets);
                DidApply();
            }
        };

        class ComputeDescriptorSetTracker
            : public BindGroupAndStorageBarrierTrackerBase<true, uint32_t> {
          public:
            ComputeDescriptorSetTracker() = default;

            void Apply(Device* device,
                       CommandRecordingContext* recordingContext,
                       VkPipelineBindPoint bindPoint) {
                ApplyDescriptorSets(device, recordingContext->commandBuffer, bindPoint,
                                    ToBackend(mPipelineLayout)->GetHandle(),
                                    mDirtyBindGroupsObjectChangedOrIsDynamic, mBindGroups,
                                    mDynamicOffsetCounts, mDynamicOffsets);

                for (BindGroupIndex index : IterateBitSet(mBindGroupLayoutsMask)) {
                    for (BindingIndex bindingIndex :
                         IterateBitSet(mBindingsNeedingBarrier[index])) {
                        switch (mBindingTypes[index][bindingIndex]) {
                            case wgpu::BindingType::StorageBuffer:
                                static_cast<Buffer*>(mBindings[index][bindingIndex])
                                    ->TransitionUsageNow(recordingContext,
                                                         wgpu::BufferUsage::Storage);
                                break;

                            case wgpu::BindingType::ReadonlyStorageTexture:
                            case wgpu::BindingType::WriteonlyStorageTexture: {
                                TextureViewBase* view =
                                    static_cast<TextureViewBase*>(mBindings[index][bindingIndex]);
                                ToBackend(view->GetTexture())
                                    ->TransitionUsageNow(recordingContext,
                                                         wgpu::TextureUsage::Storage,
                                                         view->GetSubresourceRange());
                                break;
                            }
                            case wgpu::BindingType::StorageTexture:
                                // Not implemented.

                            case wgpu::BindingType::UniformBuffer:
                            case wgpu::BindingType::ReadonlyStorageBuffer:
                            case wgpu::BindingType::Sampler:
                            case wgpu::BindingType::ComparisonSampler:
                            case wgpu::BindingType::SampledTexture:
                                // Don't require barriers.

                            default:
                                UNREACHABLE();
                                break;
                        }
                    }
                }
                DidApply();
            }
        };

        MaybeError RecordBeginRenderPass(CommandRecordingContext* recordingContext,
                                         Device* device,
                                         BeginRenderPassCmd* renderPass) {
            VkCommandBuffer commands = recordingContext->commandBuffer;

            // Query a VkRenderPass from the cache
            VkRenderPass renderPassVK = VK_NULL_HANDLE;
            {
                RenderPassCacheQuery query;

                for (uint32_t i :
                     IterateBitSet(renderPass->attachmentState->GetColorAttachmentsMask())) {
                    const auto& attachmentInfo = renderPass->colorAttachments[i];

                    bool hasResolveTarget = attachmentInfo.resolveTarget.Get() != nullptr;
                    wgpu::LoadOp loadOp = attachmentInfo.loadOp;

                    query.SetColor(i, attachmentInfo.view->GetFormat().format, loadOp,
                                   hasResolveTarget);
                }

                if (renderPass->attachmentState->HasDepthStencilAttachment()) {
                    const auto& attachmentInfo = renderPass->depthStencilAttachment;

                    query.SetDepthStencil(attachmentInfo.view->GetTexture()->GetFormat().format,
                                          attachmentInfo.depthLoadOp, attachmentInfo.stencilLoadOp);
                }

                query.SetSampleCount(renderPass->attachmentState->GetSampleCount());

                DAWN_TRY_ASSIGN(renderPassVK, device->GetRenderPassCache()->GetRenderPass(query));
            }

            // Create a framebuffer that will be used once for the render pass and gather the clear
            // values for the attachments at the same time.
            std::array<VkClearValue, kMaxColorAttachments + 1> clearValues;
            VkFramebuffer framebuffer = VK_NULL_HANDLE;
            uint32_t attachmentCount = 0;
            {
                // Fill in the attachment info that will be chained in the framebuffer create info.
                std::array<VkImageView, kMaxColorAttachments * 2 + 1> attachments;

                for (uint32_t i :
                     IterateBitSet(renderPass->attachmentState->GetColorAttachmentsMask())) {
                    auto& attachmentInfo = renderPass->colorAttachments[i];
                    TextureView* view = ToBackend(attachmentInfo.view.Get());

                    attachments[attachmentCount] = view->GetHandle();

                    clearValues[attachmentCount].color.float32[0] = attachmentInfo.clearColor.r;
                    clearValues[attachmentCount].color.float32[1] = attachmentInfo.clearColor.g;
                    clearValues[attachmentCount].color.float32[2] = attachmentInfo.clearColor.b;
                    clearValues[attachmentCount].color.float32[3] = attachmentInfo.clearColor.a;

                    attachmentCount++;
                }

                if (renderPass->attachmentState->HasDepthStencilAttachment()) {
                    auto& attachmentInfo = renderPass->depthStencilAttachment;
                    TextureView* view = ToBackend(attachmentInfo.view.Get());

                    attachments[attachmentCount] = view->GetHandle();

                    clearValues[attachmentCount].depthStencil.depth = attachmentInfo.clearDepth;
                    clearValues[attachmentCount].depthStencil.stencil = attachmentInfo.clearStencil;

                    attachmentCount++;
                }

                for (uint32_t i :
                     IterateBitSet(renderPass->attachmentState->GetColorAttachmentsMask())) {
                    if (renderPass->colorAttachments[i].resolveTarget.Get() != nullptr) {
                        TextureView* view =
                            ToBackend(renderPass->colorAttachments[i].resolveTarget.Get());

                        attachments[attachmentCount] = view->GetHandle();

                        attachmentCount++;
                    }
                }

                // Chain attachments and create the framebuffer
                VkFramebufferCreateInfo createInfo;
                createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                createInfo.pNext = nullptr;
                createInfo.flags = 0;
                createInfo.renderPass = renderPassVK;
                createInfo.attachmentCount = attachmentCount;
                createInfo.pAttachments = AsVkArray(attachments.data());
                createInfo.width = renderPass->width;
                createInfo.height = renderPass->height;
                createInfo.layers = 1;

                DAWN_TRY(
                    CheckVkSuccess(device->fn.CreateFramebuffer(device->GetVkDevice(), &createInfo,
                                                                nullptr, &*framebuffer),
                                   "CreateFramebuffer"));

                // We don't reuse VkFramebuffers so mark the framebuffer for deletion as soon as the
                // commands currently being recorded are finished.
                device->GetFencedDeleter()->DeleteWhenUnused(framebuffer);
            }

            VkRenderPassBeginInfo beginInfo;
            beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            beginInfo.pNext = nullptr;
            beginInfo.renderPass = renderPassVK;
            beginInfo.framebuffer = framebuffer;
            beginInfo.renderArea.offset.x = 0;
            beginInfo.renderArea.offset.y = 0;
            beginInfo.renderArea.extent.width = renderPass->width;
            beginInfo.renderArea.extent.height = renderPass->height;
            beginInfo.clearValueCount = attachmentCount;
            beginInfo.pClearValues = clearValues.data();

            device->fn.CmdBeginRenderPass(commands, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

            return {};
        }
    }  // anonymous namespace

    // static
    CommandBuffer* CommandBuffer::Create(CommandEncoder* encoder,
                                         const CommandBufferDescriptor* descriptor) {
        return new CommandBuffer(encoder, descriptor);
    }

    CommandBuffer::CommandBuffer(CommandEncoder* encoder, const CommandBufferDescriptor* descriptor)
        : CommandBufferBase(encoder, descriptor), mCommands(encoder->AcquireCommands()) {
    }

    CommandBuffer::~CommandBuffer() {
        FreeCommands(&mCommands);
    }

    void CommandBuffer::RecordCopyImageWithTemporaryBuffer(
        CommandRecordingContext* recordingContext,
        const TextureCopy& srcCopy,
        const TextureCopy& dstCopy,
        const Extent3D& copySize) {
        ASSERT(srcCopy.texture->GetFormat().format == dstCopy.texture->GetFormat().format);
        dawn_native::Format format = srcCopy.texture->GetFormat();
        ASSERT(copySize.width % format.blockWidth == 0);
        ASSERT(copySize.height % format.blockHeight == 0);

        // Create the temporary buffer. Note that We don't need to respect WebGPU's 256 alignment
        // because it isn't a hard constraint in Vulkan.
        uint64_t tempBufferSize =
            (copySize.width / format.blockWidth * copySize.height / format.blockHeight) *
            format.blockByteSize;
        BufferDescriptor tempBufferDescriptor;
        tempBufferDescriptor.size = tempBufferSize;
        tempBufferDescriptor.usage = wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst;

        Device* device = ToBackend(GetDevice());
        Ref<Buffer> tempBuffer = AcquireRef(ToBackend(device->CreateBuffer(&tempBufferDescriptor)));

        BufferCopy tempBufferCopy;
        tempBufferCopy.buffer = tempBuffer.Get();
        tempBufferCopy.rowsPerImage = copySize.height;
        tempBufferCopy.offset = 0;
        tempBufferCopy.bytesPerRow = copySize.width / format.blockWidth * format.blockByteSize;

        VkCommandBuffer commands = recordingContext->commandBuffer;
        VkImage srcImage = ToBackend(srcCopy.texture)->GetHandle();
        VkImage dstImage = ToBackend(dstCopy.texture)->GetHandle();

        tempBuffer->TransitionUsageNow(recordingContext, wgpu::BufferUsage::CopyDst);
        VkBufferImageCopy srcToTempBufferRegion =
            ComputeBufferImageCopyRegion(tempBufferCopy, srcCopy, copySize);

        // The Dawn CopySrc usage is always mapped to GENERAL
        device->fn.CmdCopyImageToBuffer(commands, srcImage, VK_IMAGE_LAYOUT_GENERAL,
                                        tempBuffer->GetHandle(), 1, &srcToTempBufferRegion);

        tempBuffer->TransitionUsageNow(recordingContext, wgpu::BufferUsage::CopySrc);
        VkBufferImageCopy tempBufferToDstRegion =
            ComputeBufferImageCopyRegion(tempBufferCopy, dstCopy, copySize);

        // Dawn guarantees dstImage be in the TRANSFER_DST_OPTIMAL layout after the
        // copy command.
        device->fn.CmdCopyBufferToImage(commands, tempBuffer->GetHandle(), dstImage,
                                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                                        &tempBufferToDstRegion);

        recordingContext->tempBuffers.emplace_back(tempBuffer);
    }

    MaybeError CommandBuffer::RecordCommands(CommandRecordingContext* recordingContext) {
        Device* device = ToBackend(GetDevice());
        VkCommandBuffer commands = recordingContext->commandBuffer;

        // Records the necessary barriers for the resource usage pre-computed by the frontend
        auto TransitionForPass = [](Device* device, CommandRecordingContext* recordingContext,
                                    const PassResourceUsage& usages) {
            std::vector<VkBufferMemoryBarrier> bufferBarriers;
            std::vector<VkImageMemoryBarrier> imageBarriers;
            VkPipelineStageFlags srcStages = 0;
            VkPipelineStageFlags dstStages = 0;

            for (size_t i = 0; i < usages.buffers.size(); ++i) {
                Buffer* buffer = ToBackend(usages.buffers[i]);
                buffer->TransitionUsageNow(recordingContext, usages.bufferUsages[i],
                                           &bufferBarriers, &srcStages, &dstStages);
            }

            for (size_t i = 0; i < usages.textures.size(); ++i) {
                Texture* texture = ToBackend(usages.textures[i]);
                // Clear textures that are not output attachments. Output attachments will be
                // cleared in RecordBeginRenderPass by setting the loadop to clear when the
                // texture subresource has not been initialized before the render pass.
                if (!(usages.textureUsages[i].usage & wgpu::TextureUsage::OutputAttachment)) {
                    texture->EnsureSubresourceContentInitialized(recordingContext,
                                                                 texture->GetAllSubresources());
                }
                texture->TransitionUsageForPass(recordingContext, usages.textureUsages[i],
                                                &imageBarriers, &srcStages, &dstStages);
            }

            if (bufferBarriers.size() || imageBarriers.size()) {
                device->fn.CmdPipelineBarrier(recordingContext->commandBuffer, srcStages, dstStages,
                                              0, 0, nullptr, bufferBarriers.size(),
                                              bufferBarriers.data(), imageBarriers.size(),
                                              imageBarriers.data());
            }
        };

        const std::vector<PassResourceUsage>& passResourceUsages = GetResourceUsages().perPass;
        size_t nextPassNumber = 0;

        Command type;
        while (mCommands.NextCommandId(&type)) {
            switch (type) {
                case Command::CopyBufferToBuffer: {
                    CopyBufferToBufferCmd* copy = mCommands.NextCommand<CopyBufferToBufferCmd>();

                    Buffer* srcBuffer = ToBackend(copy->source.Get());
                    Buffer* dstBuffer = ToBackend(copy->destination.Get());

                    srcBuffer->EnsureDataInitialized(recordingContext);
                    dstBuffer->EnsureDataInitializedAsDestination(
                        recordingContext, copy->destinationOffset, copy->size);

                    srcBuffer->TransitionUsageNow(recordingContext, wgpu::BufferUsage::CopySrc);
                    dstBuffer->TransitionUsageNow(recordingContext, wgpu::BufferUsage::CopyDst);

                    VkBufferCopy region;
                    region.srcOffset = copy->sourceOffset;
                    region.dstOffset = copy->destinationOffset;
                    region.size = copy->size;

                    VkBuffer srcHandle = srcBuffer->GetHandle();
                    VkBuffer dstHandle = dstBuffer->GetHandle();
                    device->fn.CmdCopyBuffer(commands, srcHandle, dstHandle, 1, &region);
                    break;
                }

                case Command::CopyBufferToTexture: {
                    CopyBufferToTextureCmd* copy = mCommands.NextCommand<CopyBufferToTextureCmd>();
                    auto& src = copy->source;
                    auto& dst = copy->destination;

                    ToBackend(src.buffer)->EnsureDataInitialized(recordingContext);

                    VkBufferImageCopy region =
                        ComputeBufferImageCopyRegion(src, dst, copy->copySize);
                    VkImageSubresourceLayers subresource = region.imageSubresource;

                    ASSERT(dst.texture->GetDimension() == wgpu::TextureDimension::e2D);
                    SubresourceRange range = {subresource.mipLevel, 1, subresource.baseArrayLayer,
                                              subresource.layerCount};
                    if (IsCompleteSubresourceCopiedTo(dst.texture.Get(), copy->copySize,
                                                      subresource.mipLevel)) {
                        // Since texture has been overwritten, it has been "initialized"
                        dst.texture->SetIsSubresourceContentInitialized(true, range);
                    } else {
                        ToBackend(dst.texture)
                            ->EnsureSubresourceContentInitialized(recordingContext, range);
                    }
                    ToBackend(src.buffer)
                        ->TransitionUsageNow(recordingContext, wgpu::BufferUsage::CopySrc);
                    ToBackend(dst.texture)
                        ->TransitionUsageNow(recordingContext, wgpu::TextureUsage::CopyDst, range);
                    VkBuffer srcBuffer = ToBackend(src.buffer)->GetHandle();
                    VkImage dstImage = ToBackend(dst.texture)->GetHandle();

                    // Dawn guarantees dstImage be in the TRANSFER_DST_OPTIMAL layout after the
                    // copy command.
                    device->fn.CmdCopyBufferToImage(commands, srcBuffer, dstImage,
                                                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                                                    &region);
                    break;
                }

                case Command::CopyTextureToBuffer: {
                    CopyTextureToBufferCmd* copy = mCommands.NextCommand<CopyTextureToBufferCmd>();
                    auto& src = copy->source;
                    auto& dst = copy->destination;

                    ToBackend(dst.buffer)
                        ->EnsureDataInitializedAsDestination(recordingContext, copy);

                    VkBufferImageCopy region =
                        ComputeBufferImageCopyRegion(dst, src, copy->copySize);
                    VkImageSubresourceLayers subresource = region.imageSubresource;

                    ASSERT(src.texture->GetDimension() == wgpu::TextureDimension::e2D);
                    const SubresourceRange range = {subresource.mipLevel, 1,
                                                    subresource.baseArrayLayer,
                                                    subresource.layerCount};
                    ToBackend(src.texture)
                        ->EnsureSubresourceContentInitialized(recordingContext, range);

                    ToBackend(src.texture)
                        ->TransitionUsageNow(recordingContext, wgpu::TextureUsage::CopySrc, range);
                    ToBackend(dst.buffer)
                        ->TransitionUsageNow(recordingContext, wgpu::BufferUsage::CopyDst);

                    VkImage srcImage = ToBackend(src.texture)->GetHandle();
                    VkBuffer dstBuffer = ToBackend(dst.buffer)->GetHandle();
                    // The Dawn CopySrc usage is always mapped to GENERAL
                    device->fn.CmdCopyImageToBuffer(commands, srcImage, VK_IMAGE_LAYOUT_GENERAL,
                                                    dstBuffer, 1, &region);
                    break;
                }

                case Command::CopyTextureToTexture: {
                    CopyTextureToTextureCmd* copy =
                        mCommands.NextCommand<CopyTextureToTextureCmd>();
                    TextureCopy& src = copy->source;
                    TextureCopy& dst = copy->destination;
                    SubresourceRange srcRange = GetSubresourcesAffectedByCopy(src, copy->copySize);
                    SubresourceRange dstRange = GetSubresourcesAffectedByCopy(dst, copy->copySize);

                    ToBackend(src.texture)
                        ->EnsureSubresourceContentInitialized(recordingContext, srcRange);
                    if (IsCompleteSubresourceCopiedTo(dst.texture.Get(), copy->copySize,
                                                      dst.mipLevel)) {
                        // Since destination texture has been overwritten, it has been "initialized"
                        dst.texture->SetIsSubresourceContentInitialized(true, dstRange);
                    } else {
                        ToBackend(dst.texture)
                            ->EnsureSubresourceContentInitialized(recordingContext, dstRange);
                    }

                    if (src.texture.Get() == dst.texture.Get() && src.mipLevel == dst.mipLevel) {
                        // When there are overlapped subresources, the layout of the overlapped
                        // subresources should all be GENERAL instead of what we set now. Currently
                        // it is not allowed to copy with overlapped subresources, but we still
                        // add the ASSERT here as a reminder for this possible misuse.
                        ASSERT(
                            !IsRangeOverlapped(src.origin.z, dst.origin.z, copy->copySize.depth));
                    }

                    // TODO after Yunchao's CL
                    ToBackend(src.texture)
                        ->TransitionUsageNow(recordingContext, wgpu::TextureUsage::CopySrc,
                                             srcRange);
                    ToBackend(dst.texture)
                        ->TransitionUsageNow(recordingContext, wgpu::TextureUsage::CopyDst,
                                             dstRange);

                    // In some situations we cannot do texture-to-texture copies with vkCmdCopyImage
                    // because as Vulkan SPEC always validates image copies with the virtual size of
                    // the image subresource, when the extent that fits in the copy region of one
                    // subresource but does not fit in the one of another subresource, we will fail
                    // to find a valid extent to satisfy the requirements on both source and
                    // destination image subresource. For example, when the source is the first
                    // level of a 16x16 texture in BC format, and the destination is the third level
                    // of a 60x60 texture in the same format, neither 16x16 nor 15x15 is valid as
                    // the extent of vkCmdCopyImage.
                    // Our workaround for this issue is replacing the texture-to-texture copy with
                    // one texture-to-buffer copy and one buffer-to-texture copy.
                    bool copyUsingTemporaryBuffer =
                        device->IsToggleEnabled(
                            Toggle::UseTemporaryBufferInCompressedTextureToTextureCopy) &&
                        src.texture->GetFormat().isCompressed &&
                        !HasSameTextureCopyExtent(src, dst, copy->copySize);

                    if (!copyUsingTemporaryBuffer) {
                        VkImage srcImage = ToBackend(src.texture)->GetHandle();
                        VkImage dstImage = ToBackend(dst.texture)->GetHandle();
                        VkImageCopy region = ComputeImageCopyRegion(src, dst, copy->copySize);

                        // Dawn guarantees dstImage be in the TRANSFER_DST_OPTIMAL layout after the
                        // copy command.
                        device->fn.CmdCopyImage(commands, srcImage, VK_IMAGE_LAYOUT_GENERAL,
                                                dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                                                &region);
                    } else {
                        RecordCopyImageWithTemporaryBuffer(recordingContext, src, dst,
                                                           copy->copySize);
                    }

                    break;
                }

                case Command::BeginRenderPass: {
                    BeginRenderPassCmd* cmd = mCommands.NextCommand<BeginRenderPassCmd>();

                    TransitionForPass(device, recordingContext, passResourceUsages[nextPassNumber]);

                    LazyClearRenderPassAttachments(cmd);
                    DAWN_TRY(RecordRenderPass(recordingContext, cmd));

                    nextPassNumber++;
                    break;
                }

                case Command::BeginComputePass: {
                    mCommands.NextCommand<BeginComputePassCmd>();

                    TransitionForPass(device, recordingContext, passResourceUsages[nextPassNumber]);
                    DAWN_TRY(RecordComputePass(recordingContext));

                    nextPassNumber++;
                    break;
                }

                case Command::ResolveQuerySet: {
                    return DAWN_UNIMPLEMENTED_ERROR("Waiting for implementation.");
                }

                case Command::WriteTimestamp: {
                    return DAWN_UNIMPLEMENTED_ERROR("Waiting for implementation.");
                }

                default: {
                    UNREACHABLE();
                    break;
                }
            }
        }

        return {};
    }

    MaybeError CommandBuffer::RecordComputePass(CommandRecordingContext* recordingContext) {
        Device* device = ToBackend(GetDevice());
        VkCommandBuffer commands = recordingContext->commandBuffer;

        ComputeDescriptorSetTracker descriptorSets = {};

        Command type;
        while (mCommands.NextCommandId(&type)) {
            switch (type) {
                case Command::EndComputePass: {
                    mCommands.NextCommand<EndComputePassCmd>();
                    return {};
                }

                case Command::Dispatch: {
                    DispatchCmd* dispatch = mCommands.NextCommand<DispatchCmd>();

                    descriptorSets.Apply(device, recordingContext, VK_PIPELINE_BIND_POINT_COMPUTE);
                    device->fn.CmdDispatch(commands, dispatch->x, dispatch->y, dispatch->z);
                    break;
                }

                case Command::DispatchIndirect: {
                    DispatchIndirectCmd* dispatch = mCommands.NextCommand<DispatchIndirectCmd>();
                    VkBuffer indirectBuffer = ToBackend(dispatch->indirectBuffer)->GetHandle();

                    descriptorSets.Apply(device, recordingContext, VK_PIPELINE_BIND_POINT_COMPUTE);
                    device->fn.CmdDispatchIndirect(
                        commands, indirectBuffer,
                        static_cast<VkDeviceSize>(dispatch->indirectOffset));
                    break;
                }

                case Command::SetBindGroup: {
                    SetBindGroupCmd* cmd = mCommands.NextCommand<SetBindGroupCmd>();

                    BindGroup* bindGroup = ToBackend(cmd->group.Get());
                    uint32_t* dynamicOffsets = nullptr;
                    if (cmd->dynamicOffsetCount > 0) {
                        dynamicOffsets = mCommands.NextData<uint32_t>(cmd->dynamicOffsetCount);
                    }

                    descriptorSets.OnSetBindGroup(cmd->index, bindGroup, cmd->dynamicOffsetCount,
                                                  dynamicOffsets);
                    break;
                }

                case Command::SetComputePipeline: {
                    SetComputePipelineCmd* cmd = mCommands.NextCommand<SetComputePipelineCmd>();
                    ComputePipeline* pipeline = ToBackend(cmd->pipeline).Get();

                    device->fn.CmdBindPipeline(commands, VK_PIPELINE_BIND_POINT_COMPUTE,
                                               pipeline->GetHandle());
                    descriptorSets.OnSetPipeline(pipeline);
                    break;
                }

                case Command::InsertDebugMarker: {
                    if (device->GetDeviceInfo().HasExt(DeviceExt::DebugMarker)) {
                        InsertDebugMarkerCmd* cmd = mCommands.NextCommand<InsertDebugMarkerCmd>();
                        const char* label = mCommands.NextData<char>(cmd->length + 1);
                        VkDebugMarkerMarkerInfoEXT markerInfo;
                        markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
                        markerInfo.pNext = nullptr;
                        markerInfo.pMarkerName = label;
                        // Default color to black
                        markerInfo.color[0] = 0.0;
                        markerInfo.color[1] = 0.0;
                        markerInfo.color[2] = 0.0;
                        markerInfo.color[3] = 1.0;
                        device->fn.CmdDebugMarkerInsertEXT(commands, &markerInfo);
                    } else {
                        SkipCommand(&mCommands, Command::InsertDebugMarker);
                    }
                    break;
                }

                case Command::PopDebugGroup: {
                    if (device->GetDeviceInfo().HasExt(DeviceExt::DebugMarker)) {
                        mCommands.NextCommand<PopDebugGroupCmd>();
                        device->fn.CmdDebugMarkerEndEXT(commands);
                    } else {
                        SkipCommand(&mCommands, Command::PopDebugGroup);
                    }
                    break;
                }

                case Command::PushDebugGroup: {
                    if (device->GetDeviceInfo().HasExt(DeviceExt::DebugMarker)) {
                        PushDebugGroupCmd* cmd = mCommands.NextCommand<PushDebugGroupCmd>();
                        const char* label = mCommands.NextData<char>(cmd->length + 1);
                        VkDebugMarkerMarkerInfoEXT markerInfo;
                        markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
                        markerInfo.pNext = nullptr;
                        markerInfo.pMarkerName = label;
                        // Default color to black
                        markerInfo.color[0] = 0.0;
                        markerInfo.color[1] = 0.0;
                        markerInfo.color[2] = 0.0;
                        markerInfo.color[3] = 1.0;
                        device->fn.CmdDebugMarkerBeginEXT(commands, &markerInfo);
                    } else {
                        SkipCommand(&mCommands, Command::PushDebugGroup);
                    }
                    break;
                }

                case Command::WriteTimestamp: {
                    return DAWN_UNIMPLEMENTED_ERROR("Waiting for implementation.");
                }

                default: {
                    UNREACHABLE();
                    break;
                }
            }
        }

        // EndComputePass should have been called
        UNREACHABLE();
    }

    MaybeError CommandBuffer::RecordRenderPass(CommandRecordingContext* recordingContext,
                                               BeginRenderPassCmd* renderPassCmd) {
        Device* device = ToBackend(GetDevice());
        VkCommandBuffer commands = recordingContext->commandBuffer;

        DAWN_TRY(RecordBeginRenderPass(recordingContext, device, renderPassCmd));

        // Set the default value for the dynamic state
        {
            device->fn.CmdSetLineWidth(commands, 1.0f);
            device->fn.CmdSetDepthBounds(commands, 0.0f, 1.0f);

            device->fn.CmdSetStencilReference(commands, VK_STENCIL_FRONT_AND_BACK, 0);

            float blendConstants[4] = {
                0.0f,
                0.0f,
                0.0f,
                0.0f,
            };
            device->fn.CmdSetBlendConstants(commands, blendConstants);

            // The viewport and scissor default to cover all of the attachments
            VkViewport viewport;
            viewport.x = 0.0f;
            viewport.y = static_cast<float>(renderPassCmd->height);
            viewport.width = static_cast<float>(renderPassCmd->width);
            viewport.height = -static_cast<float>(renderPassCmd->height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            device->fn.CmdSetViewport(commands, 0, 1, &viewport);

            VkRect2D scissorRect;
            scissorRect.offset.x = 0;
            scissorRect.offset.y = 0;
            scissorRect.extent.width = renderPassCmd->width;
            scissorRect.extent.height = renderPassCmd->height;
            device->fn.CmdSetScissor(commands, 0, 1, &scissorRect);
        }

        RenderDescriptorSetTracker descriptorSets = {};
        RenderPipeline* lastPipeline = nullptr;

        auto EncodeRenderBundleCommand = [&](CommandIterator* iter, Command type) {
            switch (type) {
                case Command::Draw: {
                    DrawCmd* draw = iter->NextCommand<DrawCmd>();

                    descriptorSets.Apply(device, recordingContext, VK_PIPELINE_BIND_POINT_GRAPHICS);
                    device->fn.CmdDraw(commands, draw->vertexCount, draw->instanceCount,
                                       draw->firstVertex, draw->firstInstance);
                    break;
                }

                case Command::DrawIndexed: {
                    DrawIndexedCmd* draw = iter->NextCommand<DrawIndexedCmd>();

                    descriptorSets.Apply(device, recordingContext, VK_PIPELINE_BIND_POINT_GRAPHICS);
                    device->fn.CmdDrawIndexed(commands, draw->indexCount, draw->instanceCount,
                                              draw->firstIndex, draw->baseVertex,
                                              draw->firstInstance);
                    break;
                }

                case Command::DrawIndirect: {
                    DrawIndirectCmd* draw = iter->NextCommand<DrawIndirectCmd>();
                    VkBuffer indirectBuffer = ToBackend(draw->indirectBuffer)->GetHandle();

                    descriptorSets.Apply(device, recordingContext, VK_PIPELINE_BIND_POINT_GRAPHICS);
                    device->fn.CmdDrawIndirect(commands, indirectBuffer,
                                               static_cast<VkDeviceSize>(draw->indirectOffset), 1,
                                               0);
                    break;
                }

                case Command::DrawIndexedIndirect: {
                    DrawIndirectCmd* draw = iter->NextCommand<DrawIndirectCmd>();
                    VkBuffer indirectBuffer = ToBackend(draw->indirectBuffer)->GetHandle();

                    descriptorSets.Apply(device, recordingContext, VK_PIPELINE_BIND_POINT_GRAPHICS);
                    device->fn.CmdDrawIndexedIndirect(
                        commands, indirectBuffer, static_cast<VkDeviceSize>(draw->indirectOffset),
                        1, 0);
                    break;
                }

                case Command::InsertDebugMarker: {
                    if (device->GetDeviceInfo().HasExt(DeviceExt::DebugMarker)) {
                        InsertDebugMarkerCmd* cmd = iter->NextCommand<InsertDebugMarkerCmd>();
                        const char* label = iter->NextData<char>(cmd->length + 1);
                        VkDebugMarkerMarkerInfoEXT markerInfo;
                        markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
                        markerInfo.pNext = nullptr;
                        markerInfo.pMarkerName = label;
                        // Default color to black
                        markerInfo.color[0] = 0.0;
                        markerInfo.color[1] = 0.0;
                        markerInfo.color[2] = 0.0;
                        markerInfo.color[3] = 1.0;
                        device->fn.CmdDebugMarkerInsertEXT(commands, &markerInfo);
                    } else {
                        SkipCommand(iter, Command::InsertDebugMarker);
                    }
                    break;
                }

                case Command::PopDebugGroup: {
                    if (device->GetDeviceInfo().HasExt(DeviceExt::DebugMarker)) {
                        iter->NextCommand<PopDebugGroupCmd>();
                        device->fn.CmdDebugMarkerEndEXT(commands);
                    } else {
                        SkipCommand(iter, Command::PopDebugGroup);
                    }
                    break;
                }

                case Command::PushDebugGroup: {
                    if (device->GetDeviceInfo().HasExt(DeviceExt::DebugMarker)) {
                        PushDebugGroupCmd* cmd = iter->NextCommand<PushDebugGroupCmd>();
                        const char* label = iter->NextData<char>(cmd->length + 1);
                        VkDebugMarkerMarkerInfoEXT markerInfo;
                        markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
                        markerInfo.pNext = nullptr;
                        markerInfo.pMarkerName = label;
                        // Default color to black
                        markerInfo.color[0] = 0.0;
                        markerInfo.color[1] = 0.0;
                        markerInfo.color[2] = 0.0;
                        markerInfo.color[3] = 1.0;
                        device->fn.CmdDebugMarkerBeginEXT(commands, &markerInfo);
                    } else {
                        SkipCommand(iter, Command::PushDebugGroup);
                    }
                    break;
                }

                case Command::SetBindGroup: {
                    SetBindGroupCmd* cmd = iter->NextCommand<SetBindGroupCmd>();
                    BindGroup* bindGroup = ToBackend(cmd->group.Get());
                    uint32_t* dynamicOffsets = nullptr;
                    if (cmd->dynamicOffsetCount > 0) {
                        dynamicOffsets = iter->NextData<uint32_t>(cmd->dynamicOffsetCount);
                    }

                    descriptorSets.OnSetBindGroup(cmd->index, bindGroup, cmd->dynamicOffsetCount,
                                                  dynamicOffsets);
                    break;
                }

                case Command::SetIndexBuffer: {
                    SetIndexBufferCmd* cmd = iter->NextCommand<SetIndexBufferCmd>();
                    VkBuffer indexBuffer = ToBackend(cmd->buffer)->GetHandle();

                    // TODO(cwallez@chromium.org): get the index type from the last render pipeline
                    // and rebind if needed on pipeline change
                    ASSERT(lastPipeline != nullptr);
                    VkIndexType indexType =
                        VulkanIndexType(lastPipeline->GetVertexStateDescriptor()->indexFormat);
                    device->fn.CmdBindIndexBuffer(
                        commands, indexBuffer, static_cast<VkDeviceSize>(cmd->offset), indexType);
                    break;
                }

                case Command::SetRenderPipeline: {
                    SetRenderPipelineCmd* cmd = iter->NextCommand<SetRenderPipelineCmd>();
                    RenderPipeline* pipeline = ToBackend(cmd->pipeline).Get();

                    device->fn.CmdBindPipeline(commands, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                               pipeline->GetHandle());
                    lastPipeline = pipeline;

                    descriptorSets.OnSetPipeline(pipeline);
                    break;
                }

                case Command::SetVertexBuffer: {
                    SetVertexBufferCmd* cmd = iter->NextCommand<SetVertexBufferCmd>();
                    VkBuffer buffer = ToBackend(cmd->buffer)->GetHandle();
                    VkDeviceSize offset = static_cast<VkDeviceSize>(cmd->offset);

                    device->fn.CmdBindVertexBuffers(commands, cmd->slot, 1, &*buffer, &offset);
                    break;
                }

                default:
                    UNREACHABLE();
                    break;
            }
        };

        Command type;
        while (mCommands.NextCommandId(&type)) {
            switch (type) {
                case Command::EndRenderPass: {
                    mCommands.NextCommand<EndRenderPassCmd>();
                    device->fn.CmdEndRenderPass(commands);
                    return {};
                }

                case Command::SetBlendColor: {
                    SetBlendColorCmd* cmd = mCommands.NextCommand<SetBlendColorCmd>();
                    float blendConstants[4] = {
                        cmd->color.r,
                        cmd->color.g,
                        cmd->color.b,
                        cmd->color.a,
                    };
                    device->fn.CmdSetBlendConstants(commands, blendConstants);
                    break;
                }

                case Command::SetStencilReference: {
                    SetStencilReferenceCmd* cmd = mCommands.NextCommand<SetStencilReferenceCmd>();
                    device->fn.CmdSetStencilReference(commands, VK_STENCIL_FRONT_AND_BACK,
                                                      cmd->reference);
                    break;
                }

                case Command::SetViewport: {
                    SetViewportCmd* cmd = mCommands.NextCommand<SetViewportCmd>();
                    VkViewport viewport;
                    viewport.x = cmd->x;
                    viewport.y = cmd->y + cmd->height;
                    viewport.width = cmd->width;
                    viewport.height = -cmd->height;
                    viewport.minDepth = cmd->minDepth;
                    viewport.maxDepth = cmd->maxDepth;

                    device->fn.CmdSetViewport(commands, 0, 1, &viewport);
                    break;
                }

                case Command::SetScissorRect: {
                    SetScissorRectCmd* cmd = mCommands.NextCommand<SetScissorRectCmd>();
                    VkRect2D rect;
                    rect.offset.x = cmd->x;
                    rect.offset.y = cmd->y;
                    rect.extent.width = cmd->width;
                    rect.extent.height = cmd->height;

                    device->fn.CmdSetScissor(commands, 0, 1, &rect);
                    break;
                }

                case Command::ExecuteBundles: {
                    ExecuteBundlesCmd* cmd = mCommands.NextCommand<ExecuteBundlesCmd>();
                    auto bundles = mCommands.NextData<Ref<RenderBundleBase>>(cmd->count);

                    for (uint32_t i = 0; i < cmd->count; ++i) {
                        CommandIterator* iter = bundles[i]->GetCommands();
                        iter->Reset();
                        while (iter->NextCommandId(&type)) {
                            EncodeRenderBundleCommand(iter, type);
                        }
                    }
                    break;
                }

                case Command::WriteTimestamp: {
                    return DAWN_UNIMPLEMENTED_ERROR("Waiting for implementation.");
                }

                default: {
                    EncodeRenderBundleCommand(&mCommands, type);
                    break;
                }
            }
        }

        // EndRenderPass should have been called
        UNREACHABLE();
    }

}}  // namespace dawn_native::vulkan
