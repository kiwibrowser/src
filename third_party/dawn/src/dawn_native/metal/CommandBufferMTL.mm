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

#include "dawn_native/metal/CommandBufferMTL.h"

#include "dawn_native/BindGroupTracker.h"
#include "dawn_native/CommandEncoder.h"
#include "dawn_native/Commands.h"
#include "dawn_native/RenderBundle.h"
#include "dawn_native/metal/BindGroupMTL.h"
#include "dawn_native/metal/BufferMTL.h"
#include "dawn_native/metal/ComputePipelineMTL.h"
#include "dawn_native/metal/DeviceMTL.h"
#include "dawn_native/metal/PipelineLayoutMTL.h"
#include "dawn_native/metal/RenderPipelineMTL.h"
#include "dawn_native/metal/SamplerMTL.h"
#include "dawn_native/metal/TextureMTL.h"
#include "dawn_native/metal/UtilsMetal.h"

namespace dawn_native { namespace metal {

    namespace {

        // Allows this file to use MTLStoreActionStoreAndMultismapleResolve because the logic is
        // first to compute what the "best" Metal render pass descriptor is, then fix it up if we
        // are not on macOS 10.12 (i.e. the EmulateStoreAndMSAAResolve toggle is on).
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunguarded-availability"
        constexpr MTLStoreAction kMTLStoreActionStoreAndMultisampleResolve =
            MTLStoreActionStoreAndMultisampleResolve;
#pragma clang diagnostic pop

        // Creates an autoreleased MTLRenderPassDescriptor matching desc
        MTLRenderPassDescriptor* CreateMTLRenderPassDescriptor(BeginRenderPassCmd* renderPass) {
            MTLRenderPassDescriptor* descriptor = [MTLRenderPassDescriptor renderPassDescriptor];

            for (uint32_t i :
                 IterateBitSet(renderPass->attachmentState->GetColorAttachmentsMask())) {
                auto& attachmentInfo = renderPass->colorAttachments[i];

                switch (attachmentInfo.loadOp) {
                    case wgpu::LoadOp::Clear:
                        descriptor.colorAttachments[i].loadAction = MTLLoadActionClear;
                        descriptor.colorAttachments[i].clearColor = MTLClearColorMake(
                            attachmentInfo.clearColor.r, attachmentInfo.clearColor.g,
                            attachmentInfo.clearColor.b, attachmentInfo.clearColor.a);
                        break;

                    case wgpu::LoadOp::Load:
                        descriptor.colorAttachments[i].loadAction = MTLLoadActionLoad;
                        break;

                    default:
                        UNREACHABLE();
                        break;
                }

                descriptor.colorAttachments[i].texture =
                    ToBackend(attachmentInfo.view->GetTexture())->GetMTLTexture();
                descriptor.colorAttachments[i].level = attachmentInfo.view->GetBaseMipLevel();
                descriptor.colorAttachments[i].slice = attachmentInfo.view->GetBaseArrayLayer();

                bool hasResolveTarget = attachmentInfo.resolveTarget.Get() != nullptr;

                switch (attachmentInfo.storeOp) {
                    case wgpu::StoreOp::Store:
                        if (hasResolveTarget) {
                            descriptor.colorAttachments[i].resolveTexture =
                                ToBackend(attachmentInfo.resolveTarget->GetTexture())
                                    ->GetMTLTexture();
                            descriptor.colorAttachments[i].resolveLevel =
                                attachmentInfo.resolveTarget->GetBaseMipLevel();
                            descriptor.colorAttachments[i].resolveSlice =
                                attachmentInfo.resolveTarget->GetBaseArrayLayer();
                            descriptor.colorAttachments[i].storeAction =
                                kMTLStoreActionStoreAndMultisampleResolve;
                        } else {
                            descriptor.colorAttachments[i].storeAction = MTLStoreActionStore;
                        }
                        break;

                    case wgpu::StoreOp::Clear:
                        descriptor.colorAttachments[i].storeAction = MTLStoreActionDontCare;
                        break;

                    default:
                        UNREACHABLE();
                        break;
                }
            }

            if (renderPass->attachmentState->HasDepthStencilAttachment()) {
                auto& attachmentInfo = renderPass->depthStencilAttachment;

                id<MTLTexture> texture =
                    ToBackend(attachmentInfo.view->GetTexture())->GetMTLTexture();
                const Format& format = attachmentInfo.view->GetTexture()->GetFormat();

                if (format.HasDepth()) {
                    descriptor.depthAttachment.texture = texture;
                    descriptor.depthAttachment.level = attachmentInfo.view->GetBaseMipLevel();
                    descriptor.depthAttachment.slice = attachmentInfo.view->GetBaseArrayLayer();

                    switch (attachmentInfo.depthStoreOp) {
                        case wgpu::StoreOp::Store:
                            descriptor.depthAttachment.storeAction = MTLStoreActionStore;
                            break;

                        case wgpu::StoreOp::Clear:
                            descriptor.depthAttachment.storeAction = MTLStoreActionDontCare;
                            break;

                        default:
                            UNREACHABLE();
                            break;
                    }

                    switch (attachmentInfo.depthLoadOp) {
                        case wgpu::LoadOp::Clear:
                            descriptor.depthAttachment.loadAction = MTLLoadActionClear;
                            descriptor.depthAttachment.clearDepth = attachmentInfo.clearDepth;
                            break;

                        case wgpu::LoadOp::Load:
                            descriptor.depthAttachment.loadAction = MTLLoadActionLoad;
                            break;

                        default:
                            UNREACHABLE();
                            break;
                    }
                }

                if (format.HasStencil()) {
                    descriptor.stencilAttachment.texture = texture;
                    descriptor.stencilAttachment.level = attachmentInfo.view->GetBaseMipLevel();
                    descriptor.stencilAttachment.slice = attachmentInfo.view->GetBaseArrayLayer();

                    switch (attachmentInfo.stencilStoreOp) {
                        case wgpu::StoreOp::Store:
                            descriptor.stencilAttachment.storeAction = MTLStoreActionStore;
                            break;

                        case wgpu::StoreOp::Clear:
                            descriptor.stencilAttachment.storeAction = MTLStoreActionDontCare;
                            break;

                        default:
                            UNREACHABLE();
                            break;
                    }

                    switch (attachmentInfo.stencilLoadOp) {
                        case wgpu::LoadOp::Clear:
                            descriptor.stencilAttachment.loadAction = MTLLoadActionClear;
                            descriptor.stencilAttachment.clearStencil = attachmentInfo.clearStencil;
                            break;

                        case wgpu::LoadOp::Load:
                            descriptor.stencilAttachment.loadAction = MTLLoadActionLoad;
                            break;

                        default:
                            UNREACHABLE();
                            break;
                    }
                }
            }

            return descriptor;
        }

        // Helper function for Toggle EmulateStoreAndMSAAResolve
        void ResolveInAnotherRenderPass(
            CommandRecordingContext* commandContext,
            const MTLRenderPassDescriptor* mtlRenderPass,
            const std::array<id<MTLTexture>, kMaxColorAttachments>& resolveTextures) {
            MTLRenderPassDescriptor* mtlRenderPassForResolve =
                [MTLRenderPassDescriptor renderPassDescriptor];
            for (uint32_t i = 0; i < kMaxColorAttachments; ++i) {
                if (resolveTextures[i] == nil) {
                    continue;
                }

                mtlRenderPassForResolve.colorAttachments[i].texture =
                    mtlRenderPass.colorAttachments[i].texture;
                mtlRenderPassForResolve.colorAttachments[i].loadAction = MTLLoadActionLoad;
                mtlRenderPassForResolve.colorAttachments[i].storeAction =
                    MTLStoreActionMultisampleResolve;
                mtlRenderPassForResolve.colorAttachments[i].resolveTexture = resolveTextures[i];
                mtlRenderPassForResolve.colorAttachments[i].resolveLevel =
                    mtlRenderPass.colorAttachments[i].resolveLevel;
                mtlRenderPassForResolve.colorAttachments[i].resolveSlice =
                    mtlRenderPass.colorAttachments[i].resolveSlice;
            }

            commandContext->BeginRender(mtlRenderPassForResolve);
            commandContext->EndRender();
        }

        // Helper functions for Toggle AlwaysResolveIntoZeroLevelAndLayer
        id<MTLTexture> CreateResolveTextureForWorkaround(Device* device,
                                                         MTLPixelFormat mtlFormat,
                                                         uint32_t width,
                                                         uint32_t height) {
            MTLTextureDescriptor* mtlDesc = [MTLTextureDescriptor new];
            mtlDesc.textureType = MTLTextureType2D;
            mtlDesc.usage = MTLTextureUsageRenderTarget;
            mtlDesc.pixelFormat = mtlFormat;
            mtlDesc.width = width;
            mtlDesc.height = height;
            mtlDesc.depth = 1;
            mtlDesc.mipmapLevelCount = 1;
            mtlDesc.arrayLength = 1;
            mtlDesc.storageMode = MTLStorageModePrivate;
            mtlDesc.sampleCount = 1;
            id<MTLTexture> resolveTexture =
                [device->GetMTLDevice() newTextureWithDescriptor:mtlDesc];
            [mtlDesc release];
            return resolveTexture;
        }

        void CopyIntoTrueResolveTarget(CommandRecordingContext* commandContext,
                                       id<MTLTexture> mtlTrueResolveTexture,
                                       uint32_t trueResolveLevel,
                                       uint32_t trueResolveSlice,
                                       id<MTLTexture> temporaryResolveTexture,
                                       uint32_t width,
                                       uint32_t height) {
            [commandContext->EnsureBlit() copyFromTexture:temporaryResolveTexture
                                              sourceSlice:0
                                              sourceLevel:0
                                             sourceOrigin:MTLOriginMake(0, 0, 0)
                                               sourceSize:MTLSizeMake(width, height, 1)
                                                toTexture:mtlTrueResolveTexture
                                         destinationSlice:trueResolveSlice
                                         destinationLevel:trueResolveLevel
                                        destinationOrigin:MTLOriginMake(0, 0, 0)];
        }

        // Metal uses a physical addressing mode which means buffers in the shading language are
        // just pointers to the virtual address of their start. This means there is no way to know
        // the length of a buffer to compute the length() of unsized arrays at the end of storage
        // buffers. SPIRV-Cross implements the length() of unsized arrays by requiring an extra
        // buffer that contains the length of other buffers. This structure that keeps track of the
        // length of storage buffers and can apply them to the reserved "buffer length buffer" when
        // needed for a draw or a dispatch.
        struct StorageBufferLengthTracker {
            wgpu::ShaderStage dirtyStages = wgpu::ShaderStage::None;

            // The lengths of buffers are stored as 32bit integers because that is the width the
            // MSL code generated by SPIRV-Cross expects.
            PerStage<std::array<uint32_t, kGenericMetalBufferSlots>> data;

            void Apply(id<MTLRenderCommandEncoder> render, RenderPipeline* pipeline) {
                wgpu::ShaderStage stagesToApply =
                    dirtyStages & pipeline->GetStagesRequiringStorageBufferLength();

                if (stagesToApply == wgpu::ShaderStage::None) {
                    return;
                }

                if (stagesToApply & wgpu::ShaderStage::Vertex) {
                    uint32_t bufferCount = ToBackend(pipeline->GetLayout())
                                               ->GetBufferBindingCount(SingleShaderStage::Vertex);
                    [render setVertexBytes:data[SingleShaderStage::Vertex].data()
                                    length:sizeof(uint32_t) * bufferCount
                                   atIndex:kBufferLengthBufferSlot];
                }

                if (stagesToApply & wgpu::ShaderStage::Fragment) {
                    uint32_t bufferCount = ToBackend(pipeline->GetLayout())
                                               ->GetBufferBindingCount(SingleShaderStage::Fragment);
                    [render setFragmentBytes:data[SingleShaderStage::Fragment].data()
                                      length:sizeof(uint32_t) * bufferCount
                                     atIndex:kBufferLengthBufferSlot];
                }

                // Only mark clean stages that were actually applied.
                dirtyStages ^= stagesToApply;
            }

            void Apply(id<MTLComputeCommandEncoder> compute, ComputePipeline* pipeline) {
                if (!(dirtyStages & wgpu::ShaderStage::Compute)) {
                    return;
                }

                if (!pipeline->RequiresStorageBufferLength()) {
                    return;
                }

                uint32_t bufferCount = ToBackend(pipeline->GetLayout())
                                           ->GetBufferBindingCount(SingleShaderStage::Compute);
                [compute setBytes:data[SingleShaderStage::Compute].data()
                           length:sizeof(uint32_t) * bufferCount
                          atIndex:kBufferLengthBufferSlot];

                dirtyStages ^= wgpu::ShaderStage::Compute;
            }
        };

        // Keeps track of the dirty bind groups so they can be lazily applied when we know the
        // pipeline state.
        // Bind groups may be inherited because bind groups are packed in the buffer /
        // texture tables in contiguous order.
        class BindGroupTracker : public BindGroupTrackerBase<true, uint64_t> {
          public:
            explicit BindGroupTracker(StorageBufferLengthTracker* lengthTracker)
                : BindGroupTrackerBase(), mLengthTracker(lengthTracker) {
            }

            template <typename Encoder>
            void Apply(Encoder encoder) {
                for (BindGroupIndex index :
                     IterateBitSet(mDirtyBindGroupsObjectChangedOrIsDynamic)) {
                    ApplyBindGroup(encoder, index, ToBackend(mBindGroups[index]),
                                   mDynamicOffsetCounts[index], mDynamicOffsets[index].data(),
                                   ToBackend(mPipelineLayout));
                }
                DidApply();
            }

          private:
            // Handles a call to SetBindGroup, directing the commands to the correct encoder.
            // There is a single function that takes both encoders to factor code. Other approaches
            // like templates wouldn't work because the name of methods are different between the
            // two encoder types.
            void ApplyBindGroupImpl(id<MTLRenderCommandEncoder> render,
                                    id<MTLComputeCommandEncoder> compute,
                                    BindGroupIndex index,
                                    BindGroup* group,
                                    uint32_t dynamicOffsetCount,
                                    uint64_t* dynamicOffsets,
                                    PipelineLayout* pipelineLayout) {
                uint32_t currentDynamicBufferIndex = 0;

                // TODO(kainino@chromium.org): Maintain buffers and offsets arrays in BindGroup
                // so that we only have to do one setVertexBuffers and one setFragmentBuffers
                // call here.
                for (BindingIndex bindingIndex{0};
                     bindingIndex < group->GetLayout()->GetBindingCount(); ++bindingIndex) {
                    const BindingInfo& bindingInfo =
                        group->GetLayout()->GetBindingInfo(bindingIndex);

                    bool hasVertStage =
                        bindingInfo.visibility & wgpu::ShaderStage::Vertex && render != nil;
                    bool hasFragStage =
                        bindingInfo.visibility & wgpu::ShaderStage::Fragment && render != nil;
                    bool hasComputeStage =
                        bindingInfo.visibility & wgpu::ShaderStage::Compute && compute != nil;

                    uint32_t vertIndex = 0;
                    uint32_t fragIndex = 0;
                    uint32_t computeIndex = 0;

                    if (hasVertStage) {
                        vertIndex = pipelineLayout->GetBindingIndexInfo(
                            SingleShaderStage::Vertex)[index][bindingIndex];
                    }
                    if (hasFragStage) {
                        fragIndex = pipelineLayout->GetBindingIndexInfo(
                            SingleShaderStage::Fragment)[index][bindingIndex];
                    }
                    if (hasComputeStage) {
                        computeIndex = pipelineLayout->GetBindingIndexInfo(
                            SingleShaderStage::Compute)[index][bindingIndex];
                    }

                    switch (bindingInfo.type) {
                        case wgpu::BindingType::UniformBuffer:
                        case wgpu::BindingType::StorageBuffer:
                        case wgpu::BindingType::ReadonlyStorageBuffer: {
                            const BufferBinding& binding =
                                group->GetBindingAsBufferBinding(bindingIndex);
                            const id<MTLBuffer> buffer = ToBackend(binding.buffer)->GetMTLBuffer();
                            NSUInteger offset = binding.offset;

                            // TODO(shaobo.yan@intel.com): Record bound buffer status to use
                            // setBufferOffset to achieve better performance.
                            if (bindingInfo.hasDynamicOffset) {
                                offset += dynamicOffsets[currentDynamicBufferIndex];
                                currentDynamicBufferIndex++;
                            }

                            if (hasVertStage) {
                                mLengthTracker->data[SingleShaderStage::Vertex][vertIndex] =
                                    binding.size;
                                mLengthTracker->dirtyStages |= wgpu::ShaderStage::Vertex;
                                [render setVertexBuffers:&buffer
                                                 offsets:&offset
                                               withRange:NSMakeRange(vertIndex, 1)];
                            }
                            if (hasFragStage) {
                                mLengthTracker->data[SingleShaderStage::Fragment][fragIndex] =
                                    binding.size;
                                mLengthTracker->dirtyStages |= wgpu::ShaderStage::Fragment;
                                [render setFragmentBuffers:&buffer
                                                   offsets:&offset
                                                 withRange:NSMakeRange(fragIndex, 1)];
                            }
                            if (hasComputeStage) {
                                mLengthTracker->data[SingleShaderStage::Compute][computeIndex] =
                                    binding.size;
                                mLengthTracker->dirtyStages |= wgpu::ShaderStage::Compute;
                                [compute setBuffers:&buffer
                                            offsets:&offset
                                          withRange:NSMakeRange(computeIndex, 1)];
                            }

                            break;
                        }

                        case wgpu::BindingType::Sampler:
                        case wgpu::BindingType::ComparisonSampler: {
                            auto sampler = ToBackend(group->GetBindingAsSampler(bindingIndex));
                            if (hasVertStage) {
                                [render setVertexSamplerState:sampler->GetMTLSamplerState()
                                                      atIndex:vertIndex];
                            }
                            if (hasFragStage) {
                                [render setFragmentSamplerState:sampler->GetMTLSamplerState()
                                                        atIndex:fragIndex];
                            }
                            if (hasComputeStage) {
                                [compute setSamplerState:sampler->GetMTLSamplerState()
                                                 atIndex:computeIndex];
                            }
                            break;
                        }

                        case wgpu::BindingType::SampledTexture:
                        case wgpu::BindingType::ReadonlyStorageTexture:
                        case wgpu::BindingType::WriteonlyStorageTexture: {
                            auto textureView =
                                ToBackend(group->GetBindingAsTextureView(bindingIndex));
                            if (hasVertStage) {
                                [render setVertexTexture:textureView->GetMTLTexture()
                                                 atIndex:vertIndex];
                            }
                            if (hasFragStage) {
                                [render setFragmentTexture:textureView->GetMTLTexture()
                                                   atIndex:fragIndex];
                            }
                            if (hasComputeStage) {
                                [compute setTexture:textureView->GetMTLTexture()
                                            atIndex:computeIndex];
                            }
                            break;
                        }

                        case wgpu::BindingType::StorageTexture:
                            UNREACHABLE();
                            break;
                    }
                }
            }

            template <typename... Args>
            void ApplyBindGroup(id<MTLRenderCommandEncoder> encoder, Args&&... args) {
                ApplyBindGroupImpl(encoder, nil, std::forward<Args&&>(args)...);
            }

            template <typename... Args>
            void ApplyBindGroup(id<MTLComputeCommandEncoder> encoder, Args&&... args) {
                ApplyBindGroupImpl(nil, encoder, std::forward<Args&&>(args)...);
            }

            StorageBufferLengthTracker* mLengthTracker;
        };

        // Keeps track of the dirty vertex buffer values so they can be lazily applied when we know
        // all the relevant state.
        class VertexBufferTracker {
          public:
            void OnSetVertexBuffer(uint32_t slot, Buffer* buffer, uint64_t offset) {
                mVertexBuffers[slot] = buffer->GetMTLBuffer();
                mVertexBufferOffsets[slot] = offset;

                // Use 64 bit masks and make sure there are no shift UB
                static_assert(kMaxVertexBuffers <= 8 * sizeof(unsigned long long) - 1, "");
                mDirtyVertexBuffers |= 1ull << slot;
            }

            void OnSetPipeline(RenderPipeline* lastPipeline, RenderPipeline* pipeline) {
                // When a new pipeline is bound we must set all the vertex buffers again because
                // they might have been offset by the pipeline layout, and they might be packed
                // differently from the previous pipeline.
                mDirtyVertexBuffers |= pipeline->GetVertexBufferSlotsUsed();
            }

            void Apply(id<MTLRenderCommandEncoder> encoder, RenderPipeline* pipeline) {
                std::bitset<kMaxVertexBuffers> vertexBuffersToApply =
                    mDirtyVertexBuffers & pipeline->GetVertexBufferSlotsUsed();

                for (uint32_t dawnIndex : IterateBitSet(vertexBuffersToApply)) {
                    uint32_t metalIndex = pipeline->GetMtlVertexBufferIndex(dawnIndex);

                    [encoder setVertexBuffers:&mVertexBuffers[dawnIndex]
                                      offsets:&mVertexBufferOffsets[dawnIndex]
                                    withRange:NSMakeRange(metalIndex, 1)];
                }

                mDirtyVertexBuffers.reset();
            }

          private:
            // All the indices in these arrays are Dawn vertex buffer indices
            std::bitset<kMaxVertexBuffers> mDirtyVertexBuffers;
            std::array<id<MTLBuffer>, kMaxVertexBuffers> mVertexBuffers;
            std::array<NSUInteger, kMaxVertexBuffers> mVertexBufferOffsets;
        };

    }  // anonymous namespace

    CommandBuffer::CommandBuffer(CommandEncoder* encoder, const CommandBufferDescriptor* descriptor)
        : CommandBufferBase(encoder, descriptor), mCommands(encoder->AcquireCommands()) {
    }

    CommandBuffer::~CommandBuffer() {
        FreeCommands(&mCommands);
    }

    MaybeError CommandBuffer::FillCommands(CommandRecordingContext* commandContext) {
        const std::vector<PassResourceUsage>& passResourceUsages = GetResourceUsages().perPass;
        size_t nextPassNumber = 0;

        auto LazyClearForPass = [](const PassResourceUsage& usages) {
            for (size_t i = 0; i < usages.textures.size(); ++i) {
                Texture* texture = ToBackend(usages.textures[i]);
                // Clear textures that are not output attachments. Output attachments will be
                // cleared in CreateMTLRenderPassDescriptor by setting the loadop to clear when the
                // texture subresource has not been initialized before the render pass.
                if (!(usages.textureUsages[i].usage & wgpu::TextureUsage::OutputAttachment)) {
                    texture->EnsureSubresourceContentInitialized(texture->GetAllSubresources());
                }
            }
        };

        Command type;
        while (mCommands.NextCommandId(&type)) {
            switch (type) {
                case Command::BeginComputePass: {
                    mCommands.NextCommand<BeginComputePassCmd>();

                    LazyClearForPass(passResourceUsages[nextPassNumber]);
                    commandContext->EndBlit();

                    DAWN_TRY(EncodeComputePass(commandContext));

                    nextPassNumber++;
                    break;
                }

                case Command::BeginRenderPass: {
                    BeginRenderPassCmd* cmd = mCommands.NextCommand<BeginRenderPassCmd>();

                    LazyClearForPass(passResourceUsages[nextPassNumber]);
                    commandContext->EndBlit();

                    LazyClearRenderPassAttachments(cmd);
                    MTLRenderPassDescriptor* descriptor = CreateMTLRenderPassDescriptor(cmd);
                    DAWN_TRY(EncodeRenderPass(commandContext, descriptor, cmd->width, cmd->height));

                    nextPassNumber++;
                    break;
                }

                case Command::CopyBufferToBuffer: {
                    CopyBufferToBufferCmd* copy = mCommands.NextCommand<CopyBufferToBufferCmd>();

                    ToBackend(copy->source)->EnsureDataInitialized(commandContext);
                    ToBackend(copy->destination)
                        ->EnsureDataInitializedAsDestination(commandContext,
                                                             copy->destinationOffset, copy->size);

                    [commandContext->EnsureBlit()
                           copyFromBuffer:ToBackend(copy->source)->GetMTLBuffer()
                             sourceOffset:copy->sourceOffset
                                 toBuffer:ToBackend(copy->destination)->GetMTLBuffer()
                        destinationOffset:copy->destinationOffset
                                     size:copy->size];
                    break;
                }

                case Command::CopyBufferToTexture: {
                    CopyBufferToTextureCmd* copy = mCommands.NextCommand<CopyBufferToTextureCmd>();
                    auto& src = copy->source;
                    auto& dst = copy->destination;
                    auto& copySize = copy->copySize;
                    Buffer* buffer = ToBackend(src.buffer.Get());
                    Texture* texture = ToBackend(dst.texture.Get());

                    buffer->EnsureDataInitialized(commandContext);
                    EnsureDestinationTextureInitialized(texture, copy->destination, copy->copySize);

                    TextureBufferCopySplit splitCopies = ComputeTextureBufferCopySplit(
                        texture, dst.mipLevel, dst.origin, copySize, buffer->GetSize(), src.offset,
                        src.bytesPerRow, src.rowsPerImage);

                    for (uint32_t i = 0; i < splitCopies.count; ++i) {
                        const TextureBufferCopySplit::CopyInfo& copyInfo = splitCopies.copies[i];

                        const uint32_t copyBaseLayer = copyInfo.textureOrigin.z;
                        const uint32_t copyLayerCount = copyInfo.copyExtent.depth;
                        const MTLOrigin textureOrigin =
                            MTLOriginMake(copyInfo.textureOrigin.x, copyInfo.textureOrigin.y, 0);
                        const MTLSize copyExtent =
                            MTLSizeMake(copyInfo.copyExtent.width, copyInfo.copyExtent.height, 1);

                        uint64_t bufferOffset = copyInfo.bufferOffset;
                        for (uint32_t copyLayer = copyBaseLayer;
                             copyLayer < copyBaseLayer + copyLayerCount; ++copyLayer) {
                            [commandContext->EnsureBlit() copyFromBuffer:buffer->GetMTLBuffer()
                                                            sourceOffset:bufferOffset
                                                       sourceBytesPerRow:copyInfo.bytesPerRow
                                                     sourceBytesPerImage:copyInfo.bytesPerImage
                                                              sourceSize:copyExtent
                                                               toTexture:texture->GetMTLTexture()
                                                        destinationSlice:copyLayer
                                                        destinationLevel:dst.mipLevel
                                                       destinationOrigin:textureOrigin];
                            bufferOffset += copyInfo.bytesPerImage;
                        }
                    }

                    break;
                }

                case Command::CopyTextureToBuffer: {
                    CopyTextureToBufferCmd* copy = mCommands.NextCommand<CopyTextureToBufferCmd>();
                    auto& src = copy->source;
                    auto& dst = copy->destination;
                    auto& copySize = copy->copySize;
                    Texture* texture = ToBackend(src.texture.Get());
                    Buffer* buffer = ToBackend(dst.buffer.Get());

                    buffer->EnsureDataInitializedAsDestination(commandContext, copy);

                    texture->EnsureSubresourceContentInitialized(
                        GetSubresourcesAffectedByCopy(src, copySize));

                    TextureBufferCopySplit splitCopies = ComputeTextureBufferCopySplit(
                        texture, src.mipLevel, src.origin, copySize, buffer->GetSize(), dst.offset,
                        dst.bytesPerRow, dst.rowsPerImage);

                    for (uint32_t i = 0; i < splitCopies.count; ++i) {
                        const TextureBufferCopySplit::CopyInfo& copyInfo = splitCopies.copies[i];

                        const uint32_t copyBaseLayer = copyInfo.textureOrigin.z;
                        const uint32_t copyLayerCount = copyInfo.copyExtent.depth;
                        const MTLOrigin textureOrigin =
                            MTLOriginMake(copyInfo.textureOrigin.x, copyInfo.textureOrigin.y, 0);
                        const MTLSize copyExtent =
                            MTLSizeMake(copyInfo.copyExtent.width, copyInfo.copyExtent.height, 1);

                        uint64_t bufferOffset = copyInfo.bufferOffset;
                        for (uint32_t copyLayer = copyBaseLayer;
                             copyLayer < copyBaseLayer + copyLayerCount; ++copyLayer) {
                            [commandContext->EnsureBlit() copyFromTexture:texture->GetMTLTexture()
                                                              sourceSlice:copyLayer
                                                              sourceLevel:src.mipLevel
                                                             sourceOrigin:textureOrigin
                                                               sourceSize:copyExtent
                                                                 toBuffer:buffer->GetMTLBuffer()
                                                        destinationOffset:bufferOffset
                                                   destinationBytesPerRow:copyInfo.bytesPerRow
                                                 destinationBytesPerImage:copyInfo.bytesPerImage];
                            bufferOffset += copyInfo.bytesPerImage;
                        }
                    }

                    break;
                }

                case Command::CopyTextureToTexture: {
                    CopyTextureToTextureCmd* copy =
                        mCommands.NextCommand<CopyTextureToTextureCmd>();
                    Texture* srcTexture = ToBackend(copy->source.texture.Get());
                    Texture* dstTexture = ToBackend(copy->destination.texture.Get());

                    srcTexture->EnsureSubresourceContentInitialized(
                        GetSubresourcesAffectedByCopy(copy->source, copy->copySize));
                    EnsureDestinationTextureInitialized(dstTexture, copy->destination,
                                                        copy->copySize);

                    // TODO(jiawei.shao@intel.com): support copies with 1D and 3D textures.
                    ASSERT(srcTexture->GetDimension() == wgpu::TextureDimension::e2D &&
                           dstTexture->GetDimension() == wgpu::TextureDimension::e2D);
                    const MTLSize sizeOneLayer =
                        MTLSizeMake(copy->copySize.width, copy->copySize.height, 1);
                    const MTLOrigin sourceOriginNoLayer =
                        MTLOriginMake(copy->source.origin.x, copy->source.origin.y, 0);
                    const MTLOrigin destinationOriginNoLayer =
                        MTLOriginMake(copy->destination.origin.x, copy->destination.origin.y, 0);

                    for (uint32_t slice = 0; slice < copy->copySize.depth; ++slice) {
                        [commandContext->EnsureBlit()
                              copyFromTexture:srcTexture->GetMTLTexture()
                                  sourceSlice:copy->source.origin.z + slice
                                  sourceLevel:copy->source.mipLevel
                                 sourceOrigin:sourceOriginNoLayer
                                   sourceSize:sizeOneLayer
                                    toTexture:dstTexture->GetMTLTexture()
                             destinationSlice:copy->destination.origin.z + slice
                             destinationLevel:copy->destination.mipLevel
                            destinationOrigin:destinationOriginNoLayer];
                    }
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

        commandContext->EndBlit();
        return {};
    }

    MaybeError CommandBuffer::EncodeComputePass(CommandRecordingContext* commandContext) {
        ComputePipeline* lastPipeline = nullptr;
        StorageBufferLengthTracker storageBufferLengths = {};
        BindGroupTracker bindGroups(&storageBufferLengths);

        id<MTLComputeCommandEncoder> encoder = commandContext->BeginCompute();

        Command type;
        while (mCommands.NextCommandId(&type)) {
            switch (type) {
                case Command::EndComputePass: {
                    mCommands.NextCommand<EndComputePassCmd>();
                    commandContext->EndCompute();
                    return {};
                }

                case Command::Dispatch: {
                    DispatchCmd* dispatch = mCommands.NextCommand<DispatchCmd>();

                    bindGroups.Apply(encoder);
                    storageBufferLengths.Apply(encoder, lastPipeline);

                    [encoder dispatchThreadgroups:MTLSizeMake(dispatch->x, dispatch->y, dispatch->z)
                            threadsPerThreadgroup:lastPipeline->GetLocalWorkGroupSize()];
                    break;
                }

                case Command::DispatchIndirect: {
                    DispatchIndirectCmd* dispatch = mCommands.NextCommand<DispatchIndirectCmd>();

                    bindGroups.Apply(encoder);
                    storageBufferLengths.Apply(encoder, lastPipeline);

                    Buffer* buffer = ToBackend(dispatch->indirectBuffer.Get());
                    id<MTLBuffer> indirectBuffer = buffer->GetMTLBuffer();
                    [encoder dispatchThreadgroupsWithIndirectBuffer:indirectBuffer
                                               indirectBufferOffset:dispatch->indirectOffset
                                              threadsPerThreadgroup:lastPipeline
                                                                        ->GetLocalWorkGroupSize()];
                    break;
                }

                case Command::SetComputePipeline: {
                    SetComputePipelineCmd* cmd = mCommands.NextCommand<SetComputePipelineCmd>();
                    lastPipeline = ToBackend(cmd->pipeline).Get();

                    bindGroups.OnSetPipeline(lastPipeline);

                    lastPipeline->Encode(encoder);
                    break;
                }

                case Command::SetBindGroup: {
                    SetBindGroupCmd* cmd = mCommands.NextCommand<SetBindGroupCmd>();
                    uint32_t* dynamicOffsets = nullptr;
                    if (cmd->dynamicOffsetCount > 0) {
                        dynamicOffsets = mCommands.NextData<uint32_t>(cmd->dynamicOffsetCount);
                    }

                    bindGroups.OnSetBindGroup(cmd->index, ToBackend(cmd->group.Get()),
                                              cmd->dynamicOffsetCount, dynamicOffsets);
                    break;
                }

                case Command::InsertDebugMarker: {
                    InsertDebugMarkerCmd* cmd = mCommands.NextCommand<InsertDebugMarkerCmd>();
                    char* label = mCommands.NextData<char>(cmd->length + 1);
                    NSString* mtlLabel = [[NSString alloc] initWithUTF8String:label];

                    [encoder insertDebugSignpost:mtlLabel];
                    [mtlLabel release];
                    break;
                }

                case Command::PopDebugGroup: {
                    mCommands.NextCommand<PopDebugGroupCmd>();

                    [encoder popDebugGroup];
                    break;
                }

                case Command::PushDebugGroup: {
                    PushDebugGroupCmd* cmd = mCommands.NextCommand<PushDebugGroupCmd>();
                    char* label = mCommands.NextData<char>(cmd->length + 1);
                    NSString* mtlLabel = [[NSString alloc] initWithUTF8String:label];

                    [encoder pushDebugGroup:mtlLabel];
                    [mtlLabel release];
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

    MaybeError CommandBuffer::EncodeRenderPass(CommandRecordingContext* commandContext,
                                               MTLRenderPassDescriptor* mtlRenderPass,
                                               uint32_t width,
                                               uint32_t height) {
        ASSERT(mtlRenderPass);

        Device* device = ToBackend(GetDevice());

        // Handle Toggle AlwaysResolveIntoZeroLevelAndLayer. We must handle this before applying
        // the store + MSAA resolve workaround, otherwise this toggle will never be handled because
        // the resolve texture is removed when applying the store + MSAA resolve workaround.
        if (device->IsToggleEnabled(Toggle::AlwaysResolveIntoZeroLevelAndLayer)) {
            std::array<id<MTLTexture>, kMaxColorAttachments> trueResolveTextures = {};
            std::array<uint32_t, kMaxColorAttachments> trueResolveLevels = {};
            std::array<uint32_t, kMaxColorAttachments> trueResolveSlices = {};

            // Use temporary resolve texture on the resolve targets with non-zero resolveLevel or
            // resolveSlice.
            bool useTemporaryResolveTexture = false;
            std::array<id<MTLTexture>, kMaxColorAttachments> temporaryResolveTextures = {};
            for (uint32_t i = 0; i < kMaxColorAttachments; ++i) {
                if (mtlRenderPass.colorAttachments[i].resolveTexture == nil) {
                    continue;
                }

                if (mtlRenderPass.colorAttachments[i].resolveLevel == 0 &&
                    mtlRenderPass.colorAttachments[i].resolveSlice == 0) {
                    continue;
                }

                trueResolveTextures[i] = mtlRenderPass.colorAttachments[i].resolveTexture;
                trueResolveLevels[i] = mtlRenderPass.colorAttachments[i].resolveLevel;
                trueResolveSlices[i] = mtlRenderPass.colorAttachments[i].resolveSlice;

                const MTLPixelFormat mtlFormat = trueResolveTextures[i].pixelFormat;
                temporaryResolveTextures[i] =
                    CreateResolveTextureForWorkaround(device, mtlFormat, width, height);

                mtlRenderPass.colorAttachments[i].resolveTexture = temporaryResolveTextures[i];
                mtlRenderPass.colorAttachments[i].resolveLevel = 0;
                mtlRenderPass.colorAttachments[i].resolveSlice = 0;
                useTemporaryResolveTexture = true;
            }

            // If we need to use a temporary resolve texture we need to copy the result of MSAA
            // resolve back to the true resolve targets.
            if (useTemporaryResolveTexture) {
                DAWN_TRY(EncodeRenderPass(commandContext, mtlRenderPass, width, height));
                for (uint32_t i = 0; i < kMaxColorAttachments; ++i) {
                    if (trueResolveTextures[i] == nil) {
                        continue;
                    }

                    ASSERT(temporaryResolveTextures[i] != nil);
                    CopyIntoTrueResolveTarget(commandContext, trueResolveTextures[i],
                                              trueResolveLevels[i], trueResolveSlices[i],
                                              temporaryResolveTextures[i], width, height);
                    [temporaryResolveTextures[i] release];
                    temporaryResolveTextures[i] = nil;
                }
                return {};
            }
        }

        // Handle Store + MSAA resolve workaround (Toggle EmulateStoreAndMSAAResolve).
        if (device->IsToggleEnabled(Toggle::EmulateStoreAndMSAAResolve)) {
            bool hasStoreAndMSAAResolve = false;

            // Remove any store + MSAA resolve and remember them.
            std::array<id<MTLTexture>, kMaxColorAttachments> resolveTextures = {};
            for (uint32_t i = 0; i < kMaxColorAttachments; ++i) {
                if (mtlRenderPass.colorAttachments[i].storeAction ==
                    kMTLStoreActionStoreAndMultisampleResolve) {
                    hasStoreAndMSAAResolve = true;
                    resolveTextures[i] = mtlRenderPass.colorAttachments[i].resolveTexture;

                    mtlRenderPass.colorAttachments[i].storeAction = MTLStoreActionStore;
                    mtlRenderPass.colorAttachments[i].resolveTexture = nil;
                }
            }

            // If we found a store + MSAA resolve we need to resolve in a different render pass.
            if (hasStoreAndMSAAResolve) {
                DAWN_TRY(EncodeRenderPass(commandContext, mtlRenderPass, width, height));
                ResolveInAnotherRenderPass(commandContext, mtlRenderPass, resolveTextures);
                return {};
            }
        }

        DAWN_TRY(EncodeRenderPassInternal(commandContext, mtlRenderPass, width, height));
        return {};
    }

    MaybeError CommandBuffer::EncodeRenderPassInternal(CommandRecordingContext* commandContext,
                                                       MTLRenderPassDescriptor* mtlRenderPass,
                                                       uint32_t width,
                                                       uint32_t height) {
        RenderPipeline* lastPipeline = nullptr;
        id<MTLBuffer> indexBuffer = nil;
        uint32_t indexBufferBaseOffset = 0;
        VertexBufferTracker vertexBuffers;
        StorageBufferLengthTracker storageBufferLengths = {};
        BindGroupTracker bindGroups(&storageBufferLengths);

        id<MTLRenderCommandEncoder> encoder = commandContext->BeginRender(mtlRenderPass);

        auto EncodeRenderBundleCommand = [&](CommandIterator* iter, Command type) {
            switch (type) {
                case Command::Draw: {
                    DrawCmd* draw = iter->NextCommand<DrawCmd>();

                    vertexBuffers.Apply(encoder, lastPipeline);
                    bindGroups.Apply(encoder);
                    storageBufferLengths.Apply(encoder, lastPipeline);

                    // The instance count must be non-zero, otherwise no-op
                    if (draw->instanceCount != 0) {
                        // MTLFeatureSet_iOS_GPUFamily3_v1 does not support baseInstance
                        if (draw->firstInstance == 0) {
                            [encoder drawPrimitives:lastPipeline->GetMTLPrimitiveTopology()
                                        vertexStart:draw->firstVertex
                                        vertexCount:draw->vertexCount
                                      instanceCount:draw->instanceCount];
                        } else {
                            [encoder drawPrimitives:lastPipeline->GetMTLPrimitiveTopology()
                                        vertexStart:draw->firstVertex
                                        vertexCount:draw->vertexCount
                                      instanceCount:draw->instanceCount
                                       baseInstance:draw->firstInstance];
                        }
                    }
                    break;
                }

                case Command::DrawIndexed: {
                    DrawIndexedCmd* draw = iter->NextCommand<DrawIndexedCmd>();
                    size_t formatSize =
                        IndexFormatSize(lastPipeline->GetVertexStateDescriptor()->indexFormat);

                    vertexBuffers.Apply(encoder, lastPipeline);
                    bindGroups.Apply(encoder);
                    storageBufferLengths.Apply(encoder, lastPipeline);

                    // The index and instance count must be non-zero, otherwise no-op
                    if (draw->indexCount != 0 && draw->instanceCount != 0) {
                        // MTLFeatureSet_iOS_GPUFamily3_v1 does not support baseInstance and
                        // baseVertex.
                        if (draw->baseVertex == 0 && draw->firstInstance == 0) {
                            [encoder drawIndexedPrimitives:lastPipeline->GetMTLPrimitiveTopology()
                                                indexCount:draw->indexCount
                                                 indexType:lastPipeline->GetMTLIndexType()
                                               indexBuffer:indexBuffer
                                         indexBufferOffset:indexBufferBaseOffset +
                                                           draw->firstIndex * formatSize
                                             instanceCount:draw->instanceCount];
                        } else {
                            [encoder drawIndexedPrimitives:lastPipeline->GetMTLPrimitiveTopology()
                                                indexCount:draw->indexCount
                                                 indexType:lastPipeline->GetMTLIndexType()
                                               indexBuffer:indexBuffer
                                         indexBufferOffset:indexBufferBaseOffset +
                                                           draw->firstIndex * formatSize
                                             instanceCount:draw->instanceCount
                                                baseVertex:draw->baseVertex
                                              baseInstance:draw->firstInstance];
                        }
                    }
                    break;
                }

                case Command::DrawIndirect: {
                    DrawIndirectCmd* draw = iter->NextCommand<DrawIndirectCmd>();

                    vertexBuffers.Apply(encoder, lastPipeline);
                    bindGroups.Apply(encoder);
                    storageBufferLengths.Apply(encoder, lastPipeline);

                    Buffer* buffer = ToBackend(draw->indirectBuffer.Get());
                    id<MTLBuffer> indirectBuffer = buffer->GetMTLBuffer();
                    [encoder drawPrimitives:lastPipeline->GetMTLPrimitiveTopology()
                              indirectBuffer:indirectBuffer
                        indirectBufferOffset:draw->indirectOffset];
                    break;
                }

                case Command::DrawIndexedIndirect: {
                    DrawIndirectCmd* draw = iter->NextCommand<DrawIndirectCmd>();

                    vertexBuffers.Apply(encoder, lastPipeline);
                    bindGroups.Apply(encoder);
                    storageBufferLengths.Apply(encoder, lastPipeline);

                    Buffer* buffer = ToBackend(draw->indirectBuffer.Get());
                    id<MTLBuffer> indirectBuffer = buffer->GetMTLBuffer();
                    [encoder drawIndexedPrimitives:lastPipeline->GetMTLPrimitiveTopology()
                                         indexType:lastPipeline->GetMTLIndexType()
                                       indexBuffer:indexBuffer
                                 indexBufferOffset:indexBufferBaseOffset
                                    indirectBuffer:indirectBuffer
                              indirectBufferOffset:draw->indirectOffset];
                    break;
                }

                case Command::InsertDebugMarker: {
                    InsertDebugMarkerCmd* cmd = iter->NextCommand<InsertDebugMarkerCmd>();
                    char* label = iter->NextData<char>(cmd->length + 1);
                    NSString* mtlLabel = [[NSString alloc] initWithUTF8String:label];

                    [encoder insertDebugSignpost:mtlLabel];
                    [mtlLabel release];
                    break;
                }

                case Command::PopDebugGroup: {
                    iter->NextCommand<PopDebugGroupCmd>();

                    [encoder popDebugGroup];
                    break;
                }

                case Command::PushDebugGroup: {
                    PushDebugGroupCmd* cmd = iter->NextCommand<PushDebugGroupCmd>();
                    char* label = iter->NextData<char>(cmd->length + 1);
                    NSString* mtlLabel = [[NSString alloc] initWithUTF8String:label];

                    [encoder pushDebugGroup:mtlLabel];
                    [mtlLabel release];
                    break;
                }

                case Command::SetRenderPipeline: {
                    SetRenderPipelineCmd* cmd = iter->NextCommand<SetRenderPipelineCmd>();
                    RenderPipeline* newPipeline = ToBackend(cmd->pipeline).Get();

                    vertexBuffers.OnSetPipeline(lastPipeline, newPipeline);
                    bindGroups.OnSetPipeline(newPipeline);

                    [encoder setDepthStencilState:newPipeline->GetMTLDepthStencilState()];
                    [encoder setFrontFacingWinding:newPipeline->GetMTLFrontFace()];
                    [encoder setCullMode:newPipeline->GetMTLCullMode()];
                    newPipeline->Encode(encoder);

                    lastPipeline = newPipeline;
                    break;
                }

                case Command::SetBindGroup: {
                    SetBindGroupCmd* cmd = iter->NextCommand<SetBindGroupCmd>();
                    uint32_t* dynamicOffsets = nullptr;
                    if (cmd->dynamicOffsetCount > 0) {
                        dynamicOffsets = iter->NextData<uint32_t>(cmd->dynamicOffsetCount);
                    }

                    bindGroups.OnSetBindGroup(cmd->index, ToBackend(cmd->group.Get()),
                                              cmd->dynamicOffsetCount, dynamicOffsets);
                    break;
                }

                case Command::SetIndexBuffer: {
                    SetIndexBufferCmd* cmd = iter->NextCommand<SetIndexBufferCmd>();
                    auto b = ToBackend(cmd->buffer.Get());
                    indexBuffer = b->GetMTLBuffer();
                    indexBufferBaseOffset = cmd->offset;
                    break;
                }

                case Command::SetVertexBuffer: {
                    SetVertexBufferCmd* cmd = iter->NextCommand<SetVertexBufferCmd>();

                    vertexBuffers.OnSetVertexBuffer(cmd->slot, ToBackend(cmd->buffer.Get()),
                                                    cmd->offset);
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
                    commandContext->EndRender();
                    return {};
                }

                case Command::SetStencilReference: {
                    SetStencilReferenceCmd* cmd = mCommands.NextCommand<SetStencilReferenceCmd>();
                    [encoder setStencilReferenceValue:cmd->reference];
                    break;
                }

                case Command::SetViewport: {
                    SetViewportCmd* cmd = mCommands.NextCommand<SetViewportCmd>();
                    MTLViewport viewport;
                    viewport.originX = cmd->x;
                    viewport.originY = cmd->y;
                    viewport.width = cmd->width;
                    viewport.height = cmd->height;
                    viewport.znear = cmd->minDepth;
                    viewport.zfar = cmd->maxDepth;

                    [encoder setViewport:viewport];
                    break;
                }

                case Command::SetScissorRect: {
                    SetScissorRectCmd* cmd = mCommands.NextCommand<SetScissorRectCmd>();
                    MTLScissorRect rect;
                    rect.x = cmd->x;
                    rect.y = cmd->y;
                    rect.width = cmd->width;
                    rect.height = cmd->height;

                    // The scissor rect x + width must be <= render pass width
                    if ((rect.x + rect.width) > width) {
                        rect.width = width - rect.x;
                    }
                    // The scissor rect y + height must be <= render pass height
                    if ((rect.y + rect.height > height)) {
                        rect.height = height - rect.y;
                    }

                    [encoder setScissorRect:rect];
                    break;
                }

                case Command::SetBlendColor: {
                    SetBlendColorCmd* cmd = mCommands.NextCommand<SetBlendColorCmd>();
                    [encoder setBlendColorRed:cmd->color.r
                                        green:cmd->color.g
                                         blue:cmd->color.b
                                        alpha:cmd->color.a];
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

}}  // namespace dawn_native::metal
