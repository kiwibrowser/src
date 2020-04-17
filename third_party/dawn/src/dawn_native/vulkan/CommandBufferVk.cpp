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

#include "dawn_native/CommandEncoder.h"
#include "dawn_native/Commands.h"
#include "dawn_native/vulkan/BindGroupVk.h"
#include "dawn_native/vulkan/BufferVk.h"
#include "dawn_native/vulkan/ComputePipelineVk.h"
#include "dawn_native/vulkan/DeviceVk.h"
#include "dawn_native/vulkan/FencedDeleter.h"
#include "dawn_native/vulkan/PipelineLayoutVk.h"
#include "dawn_native/vulkan/RenderPassCache.h"
#include "dawn_native/vulkan/RenderPipelineVk.h"
#include "dawn_native/vulkan/TextureVk.h"

namespace dawn_native { namespace vulkan {

    namespace {

        VkIndexType VulkanIndexType(dawn::IndexFormat format) {
            switch (format) {
                case dawn::IndexFormat::Uint16:
                    return VK_INDEX_TYPE_UINT16;
                case dawn::IndexFormat::Uint32:
                    return VK_INDEX_TYPE_UINT32;
                default:
                    UNREACHABLE();
            }
        }

        VkBufferImageCopy ComputeBufferImageCopyRegion(const BufferCopy& bufferCopy,
                                                       const TextureCopy& textureCopy,
                                                       const Extent3D& copySize) {
            const Texture* texture = ToBackend(textureCopy.texture.Get());

            VkBufferImageCopy region;

            region.bufferOffset = bufferCopy.offset;
            // In Vulkan the row length is in texels while it is in bytes for Dawn
            region.bufferRowLength =
                bufferCopy.rowPitch / TextureFormatPixelSize(texture->GetFormat());
            region.bufferImageHeight = bufferCopy.imageHeight;

            region.imageSubresource.aspectMask = texture->GetVkAspectMask();
            region.imageSubresource.mipLevel = textureCopy.level;
            region.imageSubresource.baseArrayLayer = textureCopy.slice;
            region.imageSubresource.layerCount = 1;

            region.imageOffset.x = textureCopy.origin.x;
            region.imageOffset.y = textureCopy.origin.y;
            region.imageOffset.z = textureCopy.origin.z;

            region.imageExtent.width = copySize.width;
            region.imageExtent.height = copySize.height;
            region.imageExtent.depth = copySize.depth;

            return region;
        }

        VkImageCopy ComputeImageCopyRegion(const TextureCopy& srcCopy,
                                           const TextureCopy& dstCopy,
                                           const Extent3D& copySize) {
            const Texture* srcTexture = ToBackend(srcCopy.texture.Get());
            const Texture* dstTexture = ToBackend(dstCopy.texture.Get());

            VkImageCopy region;

            region.srcSubresource.aspectMask = srcTexture->GetVkAspectMask();
            region.srcSubresource.mipLevel = srcCopy.level;
            region.srcSubresource.baseArrayLayer = srcCopy.slice;
            region.srcSubresource.layerCount = 1;

            region.srcOffset.x = srcCopy.origin.x;
            region.srcOffset.y = srcCopy.origin.y;
            region.srcOffset.z = srcCopy.origin.z;

            region.dstSubresource.aspectMask = dstTexture->GetVkAspectMask();
            region.dstSubresource.mipLevel = dstCopy.level;
            region.dstSubresource.baseArrayLayer = dstCopy.slice;
            region.dstSubresource.layerCount = 1;

            region.dstOffset.x = dstCopy.origin.x;
            region.dstOffset.y = dstCopy.origin.y;
            region.dstOffset.z = dstCopy.origin.z;

            region.extent.width = copySize.width;
            region.extent.height = copySize.height;
            region.extent.depth = copySize.depth;

            return region;
        }

        class DescriptorSetTracker {
          public:
            void OnSetBindGroup(uint32_t index,
                                VkDescriptorSet set,
                                uint32_t dynamicOffsetCount,
                                uint64_t* dynamicOffsets) {
                mDirtySets.set(index);
                mSets[index] = set;
                mDynamicOffsetCounts[index] = dynamicOffsetCount;
                if (dynamicOffsetCount > 0) {
                    // Vulkan backend use uint32_t as dynamic offsets type, it is not correct.
                    // Vulkan should use VkDeviceSize. Dawn vulkan backend has to handle this.
                    for (uint32_t i = 0; i < dynamicOffsetCount; ++i) {
                        ASSERT(dynamicOffsets[i] <= std::numeric_limits<uint32_t>::max());
                        mDynamicOffsets[index][i] = static_cast<uint32_t>(dynamicOffsets[i]);
                    }
                }
            }

            void OnPipelineLayoutChange(PipelineLayout* layout) {
                if (layout == mCurrentLayout) {
                    return;
                }

                if (mCurrentLayout == nullptr) {
                    // We're at the beginning of a pass so all bind groups will be set before any
                    // draw / dispatch. Still clear the dirty sets to avoid leftover dirty sets
                    // from previous passes.
                    mDirtySets.reset();
                } else {
                    // Bindgroups that are not inherited will be set again before any draw or
                    // dispatch. Resetting the bits also makes sure we don't have leftover dirty
                    // bindgroups that don't exist in the pipeline layout.
                    mDirtySets &= ~layout->InheritedGroupsMask(mCurrentLayout);
                }
                mCurrentLayout = layout;
            }

            void Flush(Device* device, VkCommandBuffer commands, VkPipelineBindPoint bindPoint) {
                for (uint32_t dirtyIndex : IterateBitSet(mDirtySets)) {
                    device->fn.CmdBindDescriptorSets(
                        commands, bindPoint, mCurrentLayout->GetHandle(), dirtyIndex, 1,
                        &mSets[dirtyIndex], mDynamicOffsetCounts[dirtyIndex],
                        mDynamicOffsetCounts[dirtyIndex] > 0 ? mDynamicOffsets[dirtyIndex].data()
                                                             : nullptr);
                }
                mDirtySets.reset();
            }

          private:
            PipelineLayout* mCurrentLayout = nullptr;
            std::array<VkDescriptorSet, kMaxBindGroups> mSets;
            std::bitset<kMaxBindGroups> mDirtySets;
            std::array<uint32_t, kMaxBindGroups> mDynamicOffsetCounts;
            std::array<std::array<uint32_t, kMaxBindingsPerGroup>, kMaxBindGroups> mDynamicOffsets;
        };

        void RecordBeginRenderPass(VkCommandBuffer commands,
                                   Device* device,
                                   BeginRenderPassCmd* renderPass) {
            // Query a VkRenderPass from the cache
            VkRenderPass renderPassVK = VK_NULL_HANDLE;
            {
                RenderPassCacheQuery query;

                for (uint32_t i : IterateBitSet(renderPass->colorAttachmentsSet)) {
                    const auto& attachmentInfo = renderPass->colorAttachments[i];
                    bool hasResolveTarget = attachmentInfo.resolveTarget.Get() != nullptr;

                    dawn::LoadOp loadOp = attachmentInfo.loadOp;
                    if (loadOp == dawn::LoadOp::Load && attachmentInfo.view->GetTexture() &&
                        !attachmentInfo.view->GetTexture()->IsSubresourceContentInitialized(
                            attachmentInfo.view->GetBaseMipLevel(), 1,
                            attachmentInfo.view->GetBaseArrayLayer(), 1)) {
                        loadOp = dawn::LoadOp::Clear;
                    }

                    query.SetColor(i, attachmentInfo.view->GetFormat(), loadOp, hasResolveTarget);
                }

                if (renderPass->hasDepthStencilAttachment) {
                    const auto& attachmentInfo = renderPass->depthStencilAttachment;
                    query.SetDepthStencil(attachmentInfo.view->GetTexture()->GetFormat(),
                                          attachmentInfo.depthLoadOp, attachmentInfo.stencilLoadOp);
                }

                query.SetSampleCount(renderPass->sampleCount);

                renderPassVK = device->GetRenderPassCache()->GetRenderPass(query);
            }

            // Create a framebuffer that will be used once for the render pass and gather the clear
            // values for the attachments at the same time.
            std::array<VkClearValue, kMaxColorAttachments + 1> clearValues;
            VkFramebuffer framebuffer = VK_NULL_HANDLE;
            uint32_t attachmentCount = 0;
            {
                // Fill in the attachment info that will be chained in the framebuffer create info.
                std::array<VkImageView, kMaxColorAttachments * 2 + 1> attachments;

                for (uint32_t i : IterateBitSet(renderPass->colorAttachmentsSet)) {
                    auto& attachmentInfo = renderPass->colorAttachments[i];
                    TextureView* view = ToBackend(attachmentInfo.view.Get());

                    attachments[attachmentCount] = view->GetHandle();

                    clearValues[attachmentCount].color.float32[0] = attachmentInfo.clearColor.r;
                    clearValues[attachmentCount].color.float32[1] = attachmentInfo.clearColor.g;
                    clearValues[attachmentCount].color.float32[2] = attachmentInfo.clearColor.b;
                    clearValues[attachmentCount].color.float32[3] = attachmentInfo.clearColor.a;

                    attachmentCount++;
                }

                if (renderPass->hasDepthStencilAttachment) {
                    auto& attachmentInfo = renderPass->depthStencilAttachment;
                    TextureView* view = ToBackend(attachmentInfo.view.Get());

                    attachments[attachmentCount] = view->GetHandle();

                    clearValues[attachmentCount].depthStencil.depth = attachmentInfo.clearDepth;
                    clearValues[attachmentCount].depthStencil.stencil = attachmentInfo.clearStencil;

                    attachmentCount++;
                }

                for (uint32_t i : IterateBitSet(renderPass->colorAttachmentsSet)) {
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
                createInfo.pAttachments = attachments.data();
                createInfo.width = renderPass->width;
                createInfo.height = renderPass->height;
                createInfo.layers = 1;

                if (device->fn.CreateFramebuffer(device->GetVkDevice(), &createInfo, nullptr,
                                                 &framebuffer) != VK_SUCCESS) {
                    ASSERT(false);
                }

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
        }
    }  // anonymous namespace

    CommandBuffer::CommandBuffer(Device* device, CommandEncoderBase* encoder)
        : CommandBufferBase(device, encoder), mCommands(encoder->AcquireCommands()) {
    }

    CommandBuffer::~CommandBuffer() {
        FreeCommands(&mCommands);
    }

    void CommandBuffer::RecordCommands(VkCommandBuffer commands) {
        Device* device = ToBackend(GetDevice());

        // Records the necessary barriers for the resource usage pre-computed by the frontend
        auto TransitionForPass = [](VkCommandBuffer commands, const PassResourceUsage& usages) {
            for (size_t i = 0; i < usages.buffers.size(); ++i) {
                Buffer* buffer = ToBackend(usages.buffers[i]);
                buffer->TransitionUsageNow(commands, usages.bufferUsages[i]);
            }
            for (size_t i = 0; i < usages.textures.size(); ++i) {
                Texture* texture = ToBackend(usages.textures[i]);

                // TODO(natlee@microsoft.com): Update clearing here when subresource tracking is
                // implemented
                texture->EnsureSubresourceContentInitialized(
                    commands, 0, texture->GetNumMipLevels(), 0, texture->GetArrayLayers());
                texture->TransitionUsageNow(commands, usages.textureUsages[i]);
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

                    srcBuffer->TransitionUsageNow(commands, dawn::BufferUsageBit::TransferSrc);
                    dstBuffer->TransitionUsageNow(commands, dawn::BufferUsageBit::TransferDst);

                    VkBufferCopy region;
                    region.srcOffset = copy->sourceOffset;
                    region.dstOffset = copy->destinationOffset;
                    region.size = copy->size;

                    VkBuffer srcHandle = srcBuffer->GetHandle();
                    VkBuffer dstHandle = dstBuffer->GetHandle();
                    device->fn.CmdCopyBuffer(commands, srcHandle, dstHandle, 1, &region);
                } break;

                case Command::CopyBufferToTexture: {
                    CopyBufferToTextureCmd* copy = mCommands.NextCommand<CopyBufferToTextureCmd>();
                    auto& src = copy->source;
                    auto& dst = copy->destination;

                    VkBufferImageCopy region =
                        ComputeBufferImageCopyRegion(src, dst, copy->copySize);
                    VkImageSubresourceLayers subresource = region.imageSubresource;

                    if (IsCompleteSubresourceCopiedTo(dst.texture.Get(), copy->copySize,
                                                      subresource.mipLevel)) {
                        // Since texture has been overwritten, it has been "initialized"
                        dst.texture->SetIsSubresourceContentInitialized(
                            subresource.mipLevel, 1, subresource.baseArrayLayer, 1);
                    } else {
                        ToBackend(dst.texture)
                            ->EnsureSubresourceContentInitialized(commands, subresource.mipLevel, 1,
                                                                  subresource.baseArrayLayer, 1);
                    }
                    ToBackend(src.buffer)
                        ->TransitionUsageNow(commands, dawn::BufferUsageBit::TransferSrc);
                    ToBackend(dst.texture)
                        ->TransitionUsageNow(commands, dawn::TextureUsageBit::TransferDst);
                    VkBuffer srcBuffer = ToBackend(src.buffer)->GetHandle();
                    VkImage dstImage = ToBackend(dst.texture)->GetHandle();

                    // The image is written to so the Dawn guarantees make sure it is in the
                    // TRANSFER_DST_OPTIMAL layout
                    device->fn.CmdCopyBufferToImage(commands, srcBuffer, dstImage,
                                                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                                                    &region);
                } break;

                case Command::CopyTextureToBuffer: {
                    CopyTextureToBufferCmd* copy = mCommands.NextCommand<CopyTextureToBufferCmd>();
                    auto& src = copy->source;
                    auto& dst = copy->destination;

                    VkBufferImageCopy region =
                        ComputeBufferImageCopyRegion(dst, src, copy->copySize);
                    VkImageSubresourceLayers subresource = region.imageSubresource;

                    ToBackend(src.texture)
                        ->EnsureSubresourceContentInitialized(commands, subresource.mipLevel, 1,
                                                              subresource.baseArrayLayer, 1);

                    ToBackend(src.texture)
                        ->TransitionUsageNow(commands, dawn::TextureUsageBit::TransferSrc);
                    ToBackend(dst.buffer)
                        ->TransitionUsageNow(commands, dawn::BufferUsageBit::TransferDst);

                    VkImage srcImage = ToBackend(src.texture)->GetHandle();
                    VkBuffer dstBuffer = ToBackend(dst.buffer)->GetHandle();
                    // The Dawn TransferSrc usage is always mapped to GENERAL
                    device->fn.CmdCopyImageToBuffer(commands, srcImage, VK_IMAGE_LAYOUT_GENERAL,
                                                    dstBuffer, 1, &region);
                } break;

                case Command::CopyTextureToTexture: {
                    CopyTextureToTextureCmd* copy =
                        mCommands.NextCommand<CopyTextureToTextureCmd>();
                    TextureCopy& src = copy->source;
                    TextureCopy& dst = copy->destination;

                    VkImageCopy region = ComputeImageCopyRegion(src, dst, copy->copySize);
                    VkImageSubresourceLayers dstSubresource = region.dstSubresource;
                    VkImageSubresourceLayers srcSubresource = region.srcSubresource;

                    ToBackend(src.texture)
                        ->EnsureSubresourceContentInitialized(commands, srcSubresource.mipLevel, 1,
                                                              srcSubresource.baseArrayLayer, 1);
                    if (IsCompleteSubresourceCopiedTo(dst.texture.Get(), copy->copySize,
                                                      dstSubresource.mipLevel)) {
                        // Since destination texture has been overwritten, it has been "initialized"
                        dst.texture->SetIsSubresourceContentInitialized(
                            dstSubresource.mipLevel, 1, dstSubresource.baseArrayLayer, 1);
                    } else {
                        ToBackend(dst.texture)
                            ->EnsureSubresourceContentInitialized(commands, dstSubresource.mipLevel,
                                                                  1, dstSubresource.baseArrayLayer,
                                                                  1);
                    }
                    ToBackend(src.texture)
                        ->TransitionUsageNow(commands, dawn::TextureUsageBit::TransferSrc);
                    ToBackend(dst.texture)
                        ->TransitionUsageNow(commands, dawn::TextureUsageBit::TransferDst);
                    VkImage srcImage = ToBackend(src.texture)->GetHandle();
                    VkImage dstImage = ToBackend(dst.texture)->GetHandle();

                    // The dstImage is written to so the Dawn guarantees make sure it is in the
                    // TRANSFER_DST_OPTIMAL layout
                    device->fn.CmdCopyImage(commands, srcImage, VK_IMAGE_LAYOUT_GENERAL, dstImage,
                                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
                } break;

                case Command::BeginRenderPass: {
                    BeginRenderPassCmd* cmd = mCommands.NextCommand<BeginRenderPassCmd>();

                    TransitionForPass(commands, passResourceUsages[nextPassNumber]);
                    RecordRenderPass(commands, cmd);

                    nextPassNumber++;
                } break;

                case Command::BeginComputePass: {
                    mCommands.NextCommand<BeginComputePassCmd>();

                    TransitionForPass(commands, passResourceUsages[nextPassNumber]);
                    RecordComputePass(commands);

                    nextPassNumber++;
                } break;

                default: { UNREACHABLE(); } break;
            }
        }
    }

    void CommandBuffer::RecordComputePass(VkCommandBuffer commands) {
        Device* device = ToBackend(GetDevice());

        DescriptorSetTracker descriptorSets;

        Command type;
        while (mCommands.NextCommandId(&type)) {
            switch (type) {
                case Command::EndComputePass: {
                    mCommands.NextCommand<EndComputePassCmd>();
                    return;
                } break;

                case Command::Dispatch: {
                    DispatchCmd* dispatch = mCommands.NextCommand<DispatchCmd>();
                    descriptorSets.Flush(device, commands, VK_PIPELINE_BIND_POINT_COMPUTE);
                    device->fn.CmdDispatch(commands, dispatch->x, dispatch->y, dispatch->z);
                } break;

                case Command::DispatchIndirect: {
                    DispatchIndirectCmd* dispatch = mCommands.NextCommand<DispatchIndirectCmd>();
                    VkBuffer indirectBuffer = ToBackend(dispatch->indirectBuffer)->GetHandle();

                    descriptorSets.Flush(device, commands, VK_PIPELINE_BIND_POINT_COMPUTE);
                    device->fn.CmdDispatchIndirect(
                        commands, indirectBuffer,
                        static_cast<VkDeviceSize>(dispatch->indirectOffset));
                } break;

                case Command::SetBindGroup: {
                    SetBindGroupCmd* cmd = mCommands.NextCommand<SetBindGroupCmd>();
                    VkDescriptorSet set = ToBackend(cmd->group.Get())->GetHandle();
                    uint64_t* dynamicOffsets = nullptr;
                    if (cmd->dynamicOffsetCount > 0) {
                        dynamicOffsets = mCommands.NextData<uint64_t>(cmd->dynamicOffsetCount);
                    }

                    descriptorSets.OnSetBindGroup(cmd->index, set, cmd->dynamicOffsetCount,
                                                  dynamicOffsets);
                } break;

                case Command::SetComputePipeline: {
                    SetComputePipelineCmd* cmd = mCommands.NextCommand<SetComputePipelineCmd>();
                    ComputePipeline* pipeline = ToBackend(cmd->pipeline).Get();

                    device->fn.CmdBindPipeline(commands, VK_PIPELINE_BIND_POINT_COMPUTE,
                                               pipeline->GetHandle());
                    descriptorSets.OnPipelineLayoutChange(ToBackend(pipeline->GetLayout()));
                } break;

                default: { UNREACHABLE(); } break;
            }
        }

        // EndComputePass should have been called
        UNREACHABLE();
    }
    void CommandBuffer::RecordRenderPass(VkCommandBuffer commands,
                                         BeginRenderPassCmd* renderPassCmd) {
        Device* device = ToBackend(GetDevice());

        RecordBeginRenderPass(commands, device, renderPassCmd);

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
            viewport.y = 0.0f;
            viewport.width = static_cast<float>(renderPassCmd->width);
            viewport.height = static_cast<float>(renderPassCmd->height);
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

        DescriptorSetTracker descriptorSets;
        RenderPipeline* lastPipeline = nullptr;

        Command type;
        while (mCommands.NextCommandId(&type)) {
            switch (type) {
                case Command::EndRenderPass: {
                    mCommands.NextCommand<EndRenderPassCmd>();
                    device->fn.CmdEndRenderPass(commands);
                    for (uint32_t i : IterateBitSet(renderPassCmd->colorAttachmentsSet)) {
                        auto& attachmentInfo = renderPassCmd->colorAttachments[i];
                        TextureView* view = ToBackend(attachmentInfo.view.Get());
                        switch (attachmentInfo.storeOp) {
                            case dawn::StoreOp::Store: {
                                attachmentInfo.view->GetTexture()
                                    ->SetIsSubresourceContentInitialized(
                                        view->GetBaseMipLevel(), view->GetLevelCount(),
                                        view->GetBaseArrayLayer(), view->GetLayerCount());
                            } break;

                            default: { UNREACHABLE(); } break;
                        }
                    }
                    return;
                } break;

                case Command::Draw: {
                    DrawCmd* draw = mCommands.NextCommand<DrawCmd>();

                    descriptorSets.Flush(device, commands, VK_PIPELINE_BIND_POINT_GRAPHICS);
                    device->fn.CmdDraw(commands, draw->vertexCount, draw->instanceCount,
                                       draw->firstVertex, draw->firstInstance);
                } break;

                case Command::DrawIndexed: {
                    DrawIndexedCmd* draw = mCommands.NextCommand<DrawIndexedCmd>();

                    descriptorSets.Flush(device, commands, VK_PIPELINE_BIND_POINT_GRAPHICS);
                    device->fn.CmdDrawIndexed(commands, draw->indexCount, draw->instanceCount,
                                              draw->firstIndex, draw->baseVertex,
                                              draw->firstInstance);
                } break;

                case Command::DrawIndirect: {
                    DrawIndirectCmd* draw = mCommands.NextCommand<DrawIndirectCmd>();
                    VkBuffer indirectBuffer = ToBackend(draw->indirectBuffer)->GetHandle();

                    descriptorSets.Flush(device, commands, VK_PIPELINE_BIND_POINT_GRAPHICS);
                    device->fn.CmdDrawIndirect(commands, indirectBuffer,
                                               static_cast<VkDeviceSize>(draw->indirectOffset), 1,
                                               0);
                } break;

                case Command::DrawIndexedIndirect: {
                    DrawIndirectCmd* draw = mCommands.NextCommand<DrawIndirectCmd>();
                    VkBuffer indirectBuffer = ToBackend(draw->indirectBuffer)->GetHandle();

                    descriptorSets.Flush(device, commands, VK_PIPELINE_BIND_POINT_GRAPHICS);
                    device->fn.CmdDrawIndexedIndirect(
                        commands, indirectBuffer, static_cast<VkDeviceSize>(draw->indirectOffset),
                        1, 0);
                } break;

                case Command::InsertDebugMarker: {
                    if (device->GetDeviceInfo().debugMarker) {
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
                } break;

                case Command::PopDebugGroup: {
                    if (device->GetDeviceInfo().debugMarker) {
                        mCommands.NextCommand<PopDebugGroupCmd>();
                        device->fn.CmdDebugMarkerEndEXT(commands);
                    } else {
                        SkipCommand(&mCommands, Command::PopDebugGroup);
                    }
                } break;

                case Command::PushDebugGroup: {
                    if (device->GetDeviceInfo().debugMarker) {
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
                } break;

                case Command::SetBindGroup: {
                    SetBindGroupCmd* cmd = mCommands.NextCommand<SetBindGroupCmd>();
                    VkDescriptorSet set = ToBackend(cmd->group.Get())->GetHandle();
                    uint64_t* dynamicOffsets = nullptr;
                    if (cmd->dynamicOffsetCount > 0) {
                        dynamicOffsets = mCommands.NextData<uint64_t>(cmd->dynamicOffsetCount);
                    }

                    descriptorSets.OnSetBindGroup(cmd->index, set, cmd->dynamicOffsetCount,
                                                  dynamicOffsets);
                } break;

                case Command::SetBlendColor: {
                    SetBlendColorCmd* cmd = mCommands.NextCommand<SetBlendColorCmd>();
                    float blendConstants[4] = {
                        cmd->color.r,
                        cmd->color.g,
                        cmd->color.b,
                        cmd->color.a,
                    };
                    device->fn.CmdSetBlendConstants(commands, blendConstants);
                } break;

                case Command::SetIndexBuffer: {
                    SetIndexBufferCmd* cmd = mCommands.NextCommand<SetIndexBufferCmd>();
                    VkBuffer indexBuffer = ToBackend(cmd->buffer)->GetHandle();

                    // TODO(cwallez@chromium.org): get the index type from the last render pipeline
                    // and rebind if needed on pipeline change
                    ASSERT(lastPipeline != nullptr);
                    VkIndexType indexType =
                        VulkanIndexType(lastPipeline->GetVertexInputDescriptor()->indexFormat);
                    device->fn.CmdBindIndexBuffer(
                        commands, indexBuffer, static_cast<VkDeviceSize>(cmd->offset), indexType);
                } break;

                case Command::SetRenderPipeline: {
                    SetRenderPipelineCmd* cmd = mCommands.NextCommand<SetRenderPipelineCmd>();
                    RenderPipeline* pipeline = ToBackend(cmd->pipeline).Get();

                    device->fn.CmdBindPipeline(commands, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                               pipeline->GetHandle());
                    lastPipeline = pipeline;

                    descriptorSets.OnPipelineLayoutChange(ToBackend(pipeline->GetLayout()));
                } break;

                case Command::SetStencilReference: {
                    SetStencilReferenceCmd* cmd = mCommands.NextCommand<SetStencilReferenceCmd>();
                    device->fn.CmdSetStencilReference(commands, VK_STENCIL_FRONT_AND_BACK,
                                                      cmd->reference);
                } break;

                case Command::SetScissorRect: {
                    SetScissorRectCmd* cmd = mCommands.NextCommand<SetScissorRectCmd>();
                    VkRect2D rect;
                    rect.offset.x = cmd->x;
                    rect.offset.y = cmd->y;
                    rect.extent.width = cmd->width;
                    rect.extent.height = cmd->height;

                    device->fn.CmdSetScissor(commands, 0, 1, &rect);
                } break;

                case Command::SetVertexBuffers: {
                    SetVertexBuffersCmd* cmd = mCommands.NextCommand<SetVertexBuffersCmd>();
                    auto buffers = mCommands.NextData<Ref<BufferBase>>(cmd->count);
                    auto offsets = mCommands.NextData<uint64_t>(cmd->count);

                    std::array<VkBuffer, kMaxVertexBuffers> vkBuffers;
                    std::array<VkDeviceSize, kMaxVertexBuffers> vkOffsets;

                    for (uint32_t i = 0; i < cmd->count; ++i) {
                        Buffer* buffer = ToBackend(buffers[i].Get());
                        vkBuffers[i] = buffer->GetHandle();
                        vkOffsets[i] = static_cast<VkDeviceSize>(offsets[i]);
                    }

                    device->fn.CmdBindVertexBuffers(commands, cmd->startSlot, cmd->count,
                                                    vkBuffers.data(), vkOffsets.data());
                } break;

                default: { UNREACHABLE(); } break;
            }
        }

        // EndRenderPass should have been called
        UNREACHABLE();
    }

}}  // namespace dawn_native::vulkan
