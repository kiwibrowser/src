// Copyright 2019 The Dawn Authors
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

#include "dawn_native/CommandValidation.h"

#include "common/BitSetIterator.h"
#include "dawn_native/BindGroup.h"
#include "dawn_native/Buffer.h"
#include "dawn_native/CommandBufferStateTracker.h"
#include "dawn_native/Commands.h"
#include "dawn_native/Device.h"
#include "dawn_native/PassResourceUsage.h"
#include "dawn_native/QuerySet.h"
#include "dawn_native/RenderBundle.h"
#include "dawn_native/RenderPipeline.h"

namespace dawn_native {

    namespace {

        inline MaybeError ValidateRenderBundleCommand(CommandIterator* commands,
                                                      Command type,
                                                      CommandBufferStateTracker* commandBufferState,
                                                      const AttachmentState* attachmentState,
                                                      uint64_t* debugGroupStackSize,
                                                      const char* disallowedMessage) {
            switch (type) {
                case Command::Draw: {
                    commands->NextCommand<DrawCmd>();
                    DAWN_TRY(commandBufferState->ValidateCanDraw());
                    break;
                }

                case Command::DrawIndexed: {
                    commands->NextCommand<DrawIndexedCmd>();
                    DAWN_TRY(commandBufferState->ValidateCanDrawIndexed());
                    break;
                }

                case Command::DrawIndirect: {
                    commands->NextCommand<DrawIndirectCmd>();
                    DAWN_TRY(commandBufferState->ValidateCanDraw());
                    break;
                }

                case Command::DrawIndexedIndirect: {
                    commands->NextCommand<DrawIndexedIndirectCmd>();
                    DAWN_TRY(commandBufferState->ValidateCanDrawIndexed());
                    break;
                }

                case Command::InsertDebugMarker: {
                    InsertDebugMarkerCmd* cmd = commands->NextCommand<InsertDebugMarkerCmd>();
                    commands->NextData<char>(cmd->length + 1);
                    break;
                }

                case Command::PopDebugGroup: {
                    commands->NextCommand<PopDebugGroupCmd>();
                    DAWN_TRY(ValidateCanPopDebugGroup(*debugGroupStackSize));
                    *debugGroupStackSize -= 1;
                    break;
                }

                case Command::PushDebugGroup: {
                    PushDebugGroupCmd* cmd = commands->NextCommand<PushDebugGroupCmd>();
                    commands->NextData<char>(cmd->length + 1);
                    *debugGroupStackSize += 1;
                    break;
                }

                case Command::SetRenderPipeline: {
                    SetRenderPipelineCmd* cmd = commands->NextCommand<SetRenderPipelineCmd>();
                    RenderPipelineBase* pipeline = cmd->pipeline.Get();

                    if (DAWN_UNLIKELY(pipeline->GetAttachmentState() != attachmentState)) {
                        return DAWN_VALIDATION_ERROR("Pipeline attachment state is not compatible");
                    }
                    commandBufferState->SetRenderPipeline(pipeline);
                    break;
                }

                case Command::SetBindGroup: {
                    SetBindGroupCmd* cmd = commands->NextCommand<SetBindGroupCmd>();
                    if (cmd->dynamicOffsetCount > 0) {
                        commands->NextData<uint32_t>(cmd->dynamicOffsetCount);
                    }

                    commandBufferState->SetBindGroup(cmd->index, cmd->group.Get());
                    break;
                }

                case Command::SetIndexBuffer: {
                    commands->NextCommand<SetIndexBufferCmd>();
                    commandBufferState->SetIndexBuffer();
                    break;
                }

                case Command::SetVertexBuffer: {
                    SetVertexBufferCmd* cmd = commands->NextCommand<SetVertexBufferCmd>();
                    commandBufferState->SetVertexBuffer(cmd->slot);
                    break;
                }

                default:
                    return DAWN_VALIDATION_ERROR(disallowedMessage);
            }

            return {};
        }

    }  // namespace

    MaybeError ValidateCanPopDebugGroup(uint64_t debugGroupStackSize) {
        if (debugGroupStackSize == 0) {
            return DAWN_VALIDATION_ERROR("Pop must be balanced by a corresponding Push.");
        }
        return {};
    }

    MaybeError ValidateFinalDebugGroupStackSize(uint64_t debugGroupStackSize) {
        if (debugGroupStackSize != 0) {
            return DAWN_VALIDATION_ERROR("Each Push must be balanced by a corresponding Pop.");
        }
        return {};
    }

    MaybeError ValidateRenderBundle(CommandIterator* commands,
                                    const AttachmentState* attachmentState) {
        CommandBufferStateTracker commandBufferState;
        uint64_t debugGroupStackSize = 0;

        Command type;
        while (commands->NextCommandId(&type)) {
            DAWN_TRY(ValidateRenderBundleCommand(commands, type, &commandBufferState,
                                                 attachmentState, &debugGroupStackSize,
                                                 "Command disallowed inside a render bundle"));
        }

        DAWN_TRY(ValidateFinalDebugGroupStackSize(debugGroupStackSize));
        return {};
    }

    MaybeError ValidateRenderPass(CommandIterator* commands, const BeginRenderPassCmd* renderPass) {
        CommandBufferStateTracker commandBufferState;
        uint64_t debugGroupStackSize = 0;

        Command type;
        while (commands->NextCommandId(&type)) {
            switch (type) {
                case Command::EndRenderPass: {
                    commands->NextCommand<EndRenderPassCmd>();
                    DAWN_TRY(ValidateFinalDebugGroupStackSize(debugGroupStackSize));
                    return {};
                }

                case Command::ExecuteBundles: {
                    ExecuteBundlesCmd* cmd = commands->NextCommand<ExecuteBundlesCmd>();
                    auto bundles = commands->NextData<Ref<RenderBundleBase>>(cmd->count);
                    for (uint32_t i = 0; i < cmd->count; ++i) {
                        if (DAWN_UNLIKELY(renderPass->attachmentState.Get() !=
                                          bundles[i]->GetAttachmentState())) {
                            return DAWN_VALIDATION_ERROR(
                                "Render bundle is not compatible with render pass");
                        }
                    }

                    if (cmd->count > 0) {
                        // Reset state. It is invalidated after render bundle execution.
                        commandBufferState = CommandBufferStateTracker{};
                    }

                    break;
                }

                case Command::SetStencilReference: {
                    commands->NextCommand<SetStencilReferenceCmd>();
                    break;
                }

                case Command::SetBlendColor: {
                    commands->NextCommand<SetBlendColorCmd>();
                    break;
                }

                case Command::SetViewport: {
                    commands->NextCommand<SetViewportCmd>();
                    break;
                }

                case Command::SetScissorRect: {
                    commands->NextCommand<SetScissorRectCmd>();
                    break;
                }

                case Command::WriteTimestamp: {
                    commands->NextCommand<WriteTimestampCmd>();
                    break;
                }

                default:
                    DAWN_TRY(ValidateRenderBundleCommand(
                        commands, type, &commandBufferState, renderPass->attachmentState.Get(),
                        &debugGroupStackSize, "Command disallowed inside a render pass"));
            }
        }

        UNREACHABLE();
        return DAWN_VALIDATION_ERROR("Unfinished render pass");
    }

    MaybeError ValidateComputePass(CommandIterator* commands) {
        CommandBufferStateTracker commandBufferState;
        uint64_t debugGroupStackSize = 0;

        Command type;
        while (commands->NextCommandId(&type)) {
            switch (type) {
                case Command::EndComputePass: {
                    commands->NextCommand<EndComputePassCmd>();
                    DAWN_TRY(ValidateFinalDebugGroupStackSize(debugGroupStackSize));
                    return {};
                }

                case Command::Dispatch: {
                    commands->NextCommand<DispatchCmd>();
                    DAWN_TRY(commandBufferState.ValidateCanDispatch());
                    break;
                }

                case Command::DispatchIndirect: {
                    commands->NextCommand<DispatchIndirectCmd>();
                    DAWN_TRY(commandBufferState.ValidateCanDispatch());
                    break;
                }

                case Command::InsertDebugMarker: {
                    InsertDebugMarkerCmd* cmd = commands->NextCommand<InsertDebugMarkerCmd>();
                    commands->NextData<char>(cmd->length + 1);
                    break;
                }

                case Command::PopDebugGroup: {
                    commands->NextCommand<PopDebugGroupCmd>();
                    DAWN_TRY(ValidateCanPopDebugGroup(debugGroupStackSize));
                    debugGroupStackSize--;
                    break;
                }

                case Command::PushDebugGroup: {
                    PushDebugGroupCmd* cmd = commands->NextCommand<PushDebugGroupCmd>();
                    commands->NextData<char>(cmd->length + 1);
                    debugGroupStackSize++;
                    break;
                }

                case Command::SetComputePipeline: {
                    SetComputePipelineCmd* cmd = commands->NextCommand<SetComputePipelineCmd>();
                    ComputePipelineBase* pipeline = cmd->pipeline.Get();
                    commandBufferState.SetComputePipeline(pipeline);
                    break;
                }

                case Command::SetBindGroup: {
                    SetBindGroupCmd* cmd = commands->NextCommand<SetBindGroupCmd>();
                    if (cmd->dynamicOffsetCount > 0) {
                        commands->NextData<uint32_t>(cmd->dynamicOffsetCount);
                    }
                    commandBufferState.SetBindGroup(cmd->index, cmd->group.Get());
                    break;
                }

                case Command::WriteTimestamp: {
                    commands->NextCommand<WriteTimestampCmd>();
                    break;
                }

                default:
                    return DAWN_VALIDATION_ERROR("Command disallowed inside a compute pass");
            }
        }

        UNREACHABLE();
        return DAWN_VALIDATION_ERROR("Unfinished compute pass");
    }

    // Performs the per-pass usage validation checks
    // This will eventually need to differentiate between render and compute passes.
    // It will be valid to use a buffer both as uniform and storage in the same compute pass.
    MaybeError ValidatePassResourceUsage(const PassResourceUsage& pass) {
        // Buffers can only be used as single-write or multiple read.
        for (size_t i = 0; i < pass.buffers.size(); ++i) {
            const BufferBase* buffer = pass.buffers[i];
            wgpu::BufferUsage usage = pass.bufferUsages[i];

            if (usage & ~buffer->GetUsage()) {
                return DAWN_VALIDATION_ERROR("Buffer missing usage for the pass");
            }

            bool readOnly = (usage & kReadOnlyBufferUsages) == usage;
            bool singleUse = wgpu::HasZeroOrOneBits(usage);

            if (pass.passType == PassType::Render && !readOnly && !singleUse) {
                return DAWN_VALIDATION_ERROR(
                    "Buffer used as writable usage and another usage in pass");
            }
        }

        // Textures can only be used as single-write or multiple read.
        for (size_t i = 0; i < pass.textures.size(); ++i) {
            const TextureBase* texture = pass.textures[i];
            const PassTextureUsage& textureUsage = pass.textureUsages[i];
            wgpu::TextureUsage usage = textureUsage.usage;

            if (usage & ~texture->GetUsage()) {
                return DAWN_VALIDATION_ERROR("Texture missing usage for the pass");
            }

            // TODO (yunchao.he@intel.com): add read/write usage tracking for compute

            // The usage variable for the whole texture is a fast path for texture usage tracking.
            // Because in most cases a texture (with or without subresources) is used as
            // single-write or multiple read, then we can skip iterating the subresources' usages.
            bool readOnly = (usage & kReadOnlyTextureUsages) == usage;
            bool singleUse = wgpu::HasZeroOrOneBits(usage);
            if (pass.passType != PassType::Render || readOnly || singleUse) {
                continue;
            }
            // Inspect the subresources if the usage of the whole texture violates usage validation.
            // Every single subresource can only be used as single-write or multiple read.
            for (wgpu::TextureUsage subresourceUsage : textureUsage.subresourceUsages) {
                bool readOnly = (subresourceUsage & kReadOnlyTextureUsages) == subresourceUsage;
                bool singleUse = wgpu::HasZeroOrOneBits(subresourceUsage);
                if (!readOnly && !singleUse) {
                    return DAWN_VALIDATION_ERROR(
                        "Texture used as writable usage and another usage in render pass");
                }
            }
        }
        return {};
    }

    MaybeError ValidateTimestampQuery(QuerySetBase* querySet, uint32_t queryIndex) {
        if (querySet->GetQueryType() != wgpu::QueryType::Timestamp) {
            return DAWN_VALIDATION_ERROR("The query type of query set must be Timestamp");
        }

        if (queryIndex >= querySet->GetQueryCount()) {
            return DAWN_VALIDATION_ERROR("Query index exceeds the number of queries in query set");
        }

        return {};
    }

    bool IsRangeOverlapped(uint32_t startA, uint32_t startB, uint32_t length) {
        uint32_t maxStart = std::max(startA, startB);
        uint32_t minStart = std::min(startA, startB);
        return static_cast<uint64_t>(minStart) + static_cast<uint64_t>(length) >
               static_cast<uint64_t>(maxStart);
    }

    uint32_t ComputeRequiredBytesInCopy(const Format& textureFormat,
                                        const Extent3D& copySize,
                                        uint32_t bytesPerRow,
                                        uint32_t rowsPerImage) {
        // Default value for rowsPerImage
        if (rowsPerImage == 0) {
            rowsPerImage = copySize.height;
        }
        ASSERT(rowsPerImage >= copySize.height);
        if (copySize.width == 0 || copySize.height == 0 || copySize.depth == 0) {
            return 0;
        }

        ASSERT(copySize.height >= 1);
        ASSERT(copySize.depth >= 1);

        uint64_t texelBlockRowsPerImage = rowsPerImage / textureFormat.blockHeight;
        uint64_t bytesPerImage = bytesPerRow * texelBlockRowsPerImage;
        uint64_t bytesInLastSlice =
            bytesPerRow * (copySize.height / textureFormat.blockHeight - 1) +
            (copySize.width / textureFormat.blockWidth * textureFormat.blockByteSize);
        return bytesPerImage * (copySize.depth - 1) + bytesInLastSlice;
    }

    MaybeError ValidateCopySizeFitsInBuffer(const Ref<BufferBase>& buffer,
                                            uint64_t offset,
                                            uint64_t size) {
        uint64_t bufferSize = buffer->GetSize();
        bool fitsInBuffer = offset <= bufferSize && (size <= (bufferSize - offset));
        if (!fitsInBuffer) {
            return DAWN_VALIDATION_ERROR("Copy would overflow the buffer");
        }

        return {};
    }

    MaybeError ValidateLinearTextureData(const TextureDataLayout& layout,
                                         uint64_t byteSize,
                                         const Format& format,
                                         const Extent3D& copyExtent) {
        // Validation for the texel block alignments:
        if (layout.rowsPerImage % format.blockHeight != 0) {
            return DAWN_VALIDATION_ERROR(
                "rowsPerImage must be a multiple of compressed texture format block height");
        }

        if (layout.offset % format.blockByteSize != 0) {
            return DAWN_VALIDATION_ERROR("Offset must be a multiple of the texel or block size");
        }

        // Validation for the copy being in-bounds:
        if (layout.rowsPerImage != 0 && layout.rowsPerImage < copyExtent.height) {
            return DAWN_VALIDATION_ERROR("rowsPerImage must not be less than the copy height.");
        }

        // We compute required bytes in copy after validating texel block alignments
        // because the divisibility conditions are necessary for the algorithm to be valid.
        // TODO(tommek@google.com): to match the spec this should only be checked when
        // copyExtent.depth > 1.
        uint32_t requiredBytesInCopy =
            ComputeRequiredBytesInCopy(format, copyExtent, layout.bytesPerRow, layout.rowsPerImage);

        bool fitsInData =
            layout.offset <= byteSize && (requiredBytesInCopy <= (byteSize - layout.offset));
        if (!fitsInData) {
            return DAWN_VALIDATION_ERROR(
                "Required size for texture data layout exceeds the given size");
        }

        // Validation for other members in layout:
        if (layout.bytesPerRow < copyExtent.width / format.blockWidth * format.blockByteSize) {
            return DAWN_VALIDATION_ERROR(
                "bytesPerRow must not be less than the number of bytes per row");
        }

        // TODO(tommek@google.com): to match the spec there should be another condition here
        // on rowsPerImage >= copyExtent.height if copyExtent.depth > 1.

        return {};
    }

    MaybeError ValidateBufferCopyView(DeviceBase const* device,
                                      const BufferCopyView& bufferCopyView) {
        // Should have already been fixed up to not use deprecated fields.
        ASSERT(bufferCopyView.offset == 0 && bufferCopyView.bytesPerRow == 0 &&
               bufferCopyView.rowsPerImage == 0);

        DAWN_TRY(device->ValidateObject(bufferCopyView.buffer));
        if (bufferCopyView.layout.bytesPerRow % kTextureBytesPerRowAlignment != 0) {
            return DAWN_VALIDATION_ERROR("bytesPerRow must be a multiple of 256");
        }

        return {};
    }

    MaybeError ValidateTextureCopyView(DeviceBase const* device,
                                       const TextureCopyView& textureCopy) {
        DAWN_TRY(device->ValidateObject(textureCopy.texture));
        if (textureCopy.mipLevel >= textureCopy.texture->GetNumMipLevels()) {
            return DAWN_VALIDATION_ERROR("mipLevel out of range");
        }

        if (textureCopy.origin.x % textureCopy.texture->GetFormat().blockWidth != 0) {
            return DAWN_VALIDATION_ERROR(
                "Offset.x must be a multiple of compressed texture format block width");
        }

        if (textureCopy.origin.y % textureCopy.texture->GetFormat().blockHeight != 0) {
            return DAWN_VALIDATION_ERROR(
                "Offset.y must be a multiple of compressed texture format block height");
        }

        return {};
    }

    MaybeError ValidateTextureCopyRange(const TextureCopyView& textureCopy,
                                        const Extent3D& copySize) {
        // TODO(jiawei.shao@intel.com): add validations on the texture-to-texture copies within the
        // same texture.
        const TextureBase* texture = textureCopy.texture;

        // Validation for the copy being in-bounds:
        Extent3D mipSize = texture->GetMipLevelPhysicalSize(textureCopy.mipLevel);
        // For 2D textures, include the array layer as depth so it can be checked with other
        // dimensions.
        ASSERT(texture->GetDimension() == wgpu::TextureDimension::e2D);
        mipSize.depth = texture->GetArrayLayers();

        // All texture dimensions are in uint32_t so by doing checks in uint64_t we avoid
        // overflows.
        if (static_cast<uint64_t>(textureCopy.origin.x) + static_cast<uint64_t>(copySize.width) >
                static_cast<uint64_t>(mipSize.width) ||
            static_cast<uint64_t>(textureCopy.origin.y) + static_cast<uint64_t>(copySize.height) >
                static_cast<uint64_t>(mipSize.height) ||
            static_cast<uint64_t>(textureCopy.origin.z) + static_cast<uint64_t>(copySize.depth) >
                static_cast<uint64_t>(mipSize.depth)) {
            return DAWN_VALIDATION_ERROR("Touching outside of the texture");
        }

        // Validation for the texel block alignments:
        if (copySize.width % textureCopy.texture->GetFormat().blockWidth != 0) {
            return DAWN_VALIDATION_ERROR(
                "copySize.width must be a multiple of compressed texture format block width");
        }

        if (copySize.height % textureCopy.texture->GetFormat().blockHeight != 0) {
            return DAWN_VALIDATION_ERROR(
                "copySize.height must be a multiple of compressed texture format block height");
        }

        return {};
    }

}  // namespace dawn_native
