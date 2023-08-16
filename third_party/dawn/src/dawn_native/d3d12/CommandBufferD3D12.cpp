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

#include "dawn_native/d3d12/CommandBufferD3D12.h"

#include "common/Assert.h"
#include "dawn_native/BindGroupAndStorageBarrierTracker.h"
#include "dawn_native/CommandEncoder.h"
#include "dawn_native/CommandValidation.h"
#include "dawn_native/Commands.h"
#include "dawn_native/RenderBundle.h"
#include "dawn_native/d3d12/BindGroupD3D12.h"
#include "dawn_native/d3d12/BindGroupLayoutD3D12.h"
#include "dawn_native/d3d12/BufferD3D12.h"
#include "dawn_native/d3d12/CommandRecordingContext.h"
#include "dawn_native/d3d12/ComputePipelineD3D12.h"
#include "dawn_native/d3d12/DeviceD3D12.h"
#include "dawn_native/d3d12/PipelineLayoutD3D12.h"
#include "dawn_native/d3d12/PlatformFunctions.h"
#include "dawn_native/d3d12/RenderPassBuilderD3D12.h"
#include "dawn_native/d3d12/RenderPipelineD3D12.h"
#include "dawn_native/d3d12/SamplerD3D12.h"
#include "dawn_native/d3d12/SamplerHeapCacheD3D12.h"
#include "dawn_native/d3d12/ShaderVisibleDescriptorAllocatorD3D12.h"
#include "dawn_native/d3d12/StagingDescriptorAllocatorD3D12.h"
#include "dawn_native/d3d12/TextureCopySplitter.h"
#include "dawn_native/d3d12/TextureD3D12.h"
#include "dawn_native/d3d12/UtilsD3D12.h"

#include <deque>

namespace dawn_native { namespace d3d12 {

    namespace {

        DXGI_FORMAT DXGIIndexFormat(wgpu::IndexFormat format) {
            switch (format) {
                case wgpu::IndexFormat::Uint16:
                    return DXGI_FORMAT_R16_UINT;
                case wgpu::IndexFormat::Uint32:
                    return DXGI_FORMAT_R32_UINT;
                default:
                    UNREACHABLE();
            }
        }

        bool CanUseCopyResource(const Texture* src, const Texture* dst, const Extent3D& copySize) {
            // Checked by validation
            ASSERT(src->GetSampleCount() == dst->GetSampleCount());
            ASSERT(src->GetFormat().format == dst->GetFormat().format);

            const Extent3D& srcSize = src->GetSize();
            const Extent3D& dstSize = dst->GetSize();

            // https://docs.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12graphicscommandlist-copyresource
            // In order to use D3D12's copy resource, the textures must be the same dimensions, and
            // the copy must be of the entire resource.
            // TODO(dawn:129): Support 1D textures.
            return src->GetDimension() == dst->GetDimension() &&  //
                   dst->GetNumMipLevels() == 1 &&                 //
                   src->GetNumMipLevels() == 1 &&      // A copy command is of a single mip, so if a
                                                       // resource has more than one, we definitely
                                                       // cannot use CopyResource.
                   copySize.width == dstSize.width &&  //
                   copySize.width == srcSize.width &&  //
                   copySize.height == dstSize.height &&  //
                   copySize.height == srcSize.height &&  //
                   copySize.depth == dstSize.depth &&    //
                   copySize.depth == srcSize.depth;
        }

        void RecordCopyBufferToTextureFromTextureCopySplit(ID3D12GraphicsCommandList* commandList,
                                                           const Texture2DCopySplit& baseCopySplit,
                                                           Buffer* buffer,
                                                           uint64_t baseOffset,
                                                           uint64_t bufferBytesPerRow,
                                                           Texture* texture,
                                                           uint32_t textureMiplevel,
                                                           uint32_t textureSlice) {
            const D3D12_TEXTURE_COPY_LOCATION textureLocation =
                ComputeTextureCopyLocationForTexture(texture, textureMiplevel, textureSlice);

            const uint64_t offset = baseCopySplit.offset + baseOffset;

            for (uint32_t i = 0; i < baseCopySplit.count; ++i) {
                const Texture2DCopySplit::CopyInfo& info = baseCopySplit.copies[i];

                // TODO(jiawei.shao@intel.com): pre-compute bufferLocation and sourceRegion as
                // members in Texture2DCopySplit::CopyInfo.
                const D3D12_TEXTURE_COPY_LOCATION bufferLocation =
                    ComputeBufferLocationForCopyTextureRegion(texture, buffer->GetD3D12Resource(),
                                                              info.bufferSize, offset,
                                                              bufferBytesPerRow);
                const D3D12_BOX sourceRegion =
                    ComputeD3D12BoxFromOffsetAndSize(info.bufferOffset, info.copySize);

                commandList->CopyTextureRegion(&textureLocation, info.textureOffset.x,
                                               info.textureOffset.y, info.textureOffset.z,
                                               &bufferLocation, &sourceRegion);
            }
        }

        void RecordCopyTextureToBufferFromTextureCopySplit(ID3D12GraphicsCommandList* commandList,
                                                           const Texture2DCopySplit& baseCopySplit,
                                                           Buffer* buffer,
                                                           uint64_t baseOffset,
                                                           uint64_t bufferBytesPerRow,
                                                           Texture* texture,
                                                           uint32_t textureMiplevel,
                                                           uint32_t textureSlice) {
            const D3D12_TEXTURE_COPY_LOCATION textureLocation =
                ComputeTextureCopyLocationForTexture(texture, textureMiplevel, textureSlice);

            const uint64_t offset = baseCopySplit.offset + baseOffset;

            for (uint32_t i = 0; i < baseCopySplit.count; ++i) {
                const Texture2DCopySplit::CopyInfo& info = baseCopySplit.copies[i];

                // TODO(jiawei.shao@intel.com): pre-compute bufferLocation and sourceRegion as
                // members in Texture2DCopySplit::CopyInfo.
                const D3D12_TEXTURE_COPY_LOCATION bufferLocation =
                    ComputeBufferLocationForCopyTextureRegion(texture, buffer->GetD3D12Resource(),
                                                              info.bufferSize, offset,
                                                              bufferBytesPerRow);
                const D3D12_BOX sourceRegion =
                    ComputeD3D12BoxFromOffsetAndSize(info.textureOffset, info.copySize);

                commandList->CopyTextureRegion(&bufferLocation, info.bufferOffset.x,
                                               info.bufferOffset.y, info.bufferOffset.z,
                                               &textureLocation, &sourceRegion);
            }
        }
    }  // anonymous namespace

    class BindGroupStateTracker : public BindGroupAndStorageBarrierTrackerBase<false, uint64_t> {
        using Base = BindGroupAndStorageBarrierTrackerBase;

      public:
        BindGroupStateTracker(Device* device)
            : BindGroupAndStorageBarrierTrackerBase(),
              mDevice(device),
              mViewAllocator(device->GetViewShaderVisibleDescriptorAllocator()),
              mSamplerAllocator(device->GetSamplerShaderVisibleDescriptorAllocator()) {
        }

        void SetInComputePass(bool inCompute_) {
            mInCompute = inCompute_;
        }

        void OnSetPipeline(PipelineBase* pipeline) {
            // Invalidate the root sampler tables previously set in the root signature.
            // This is because changing the pipeline layout also changes the root signature.
            const PipelineLayout* pipelineLayout = ToBackend(pipeline->GetLayout());
            if (mLastAppliedPipelineLayout != pipelineLayout) {
                mBoundRootSamplerTables = {};
            }

            Base::OnSetPipeline(pipeline);
        }

        MaybeError Apply(CommandRecordingContext* commandContext) {
            // Bindgroups are allocated in shader-visible descriptor heaps which are managed by a
            // ringbuffer. There can be a single shader-visible descriptor heap of each type bound
            // at any given time. This means that when we switch heaps, all other currently bound
            // bindgroups must be re-populated. Bindgroups can fail allocation gracefully which is
            // the signal to change the bounded heaps.
            // Re-populating all bindgroups after the last one fails causes duplicated allocations
            // to occur on overflow.
            // TODO(bryan.bernhart@intel.com): Consider further optimization.
            bool didCreateBindGroupViews = true;
            bool didCreateBindGroupSamplers = true;
            for (BindGroupIndex index : IterateBitSet(mDirtyBindGroups)) {
                BindGroup* group = ToBackend(mBindGroups[index]);
                didCreateBindGroupViews = group->PopulateViews(mViewAllocator);
                didCreateBindGroupSamplers = group->PopulateSamplers(mDevice, mSamplerAllocator);
                if (!didCreateBindGroupViews && !didCreateBindGroupSamplers) {
                    break;
                }
            }

            ID3D12GraphicsCommandList* commandList = commandContext->GetCommandList();

            if (!didCreateBindGroupViews || !didCreateBindGroupSamplers) {
                if (!didCreateBindGroupViews) {
                    DAWN_TRY(mViewAllocator->AllocateAndSwitchShaderVisibleHeap());
                }

                if (!didCreateBindGroupSamplers) {
                    DAWN_TRY(mSamplerAllocator->AllocateAndSwitchShaderVisibleHeap());
                }

                mDirtyBindGroupsObjectChangedOrIsDynamic |= mBindGroupLayoutsMask;
                mDirtyBindGroups |= mBindGroupLayoutsMask;

                // Must be called before applying the bindgroups.
                SetID3D12DescriptorHeaps(commandList);

                for (BindGroupIndex index : IterateBitSet(mBindGroupLayoutsMask)) {
                    BindGroup* group = ToBackend(mBindGroups[index]);
                    didCreateBindGroupViews = group->PopulateViews(mViewAllocator);
                    didCreateBindGroupSamplers =
                        group->PopulateSamplers(mDevice, mSamplerAllocator);
                    ASSERT(didCreateBindGroupViews);
                    ASSERT(didCreateBindGroupSamplers);
                }
            }

            for (BindGroupIndex index : IterateBitSet(mDirtyBindGroupsObjectChangedOrIsDynamic)) {
                BindGroup* group = ToBackend(mBindGroups[index]);
                ApplyBindGroup(commandList, ToBackend(mPipelineLayout), index, group,
                               mDynamicOffsetCounts[index], mDynamicOffsets[index].data());
            }

            if (mInCompute) {
                for (BindGroupIndex index : IterateBitSet(mBindGroupLayoutsMask)) {
                    for (BindingIndex binding : IterateBitSet(mBindingsNeedingBarrier[index])) {
                        wgpu::BindingType bindingType = mBindingTypes[index][binding];
                        switch (bindingType) {
                            case wgpu::BindingType::StorageBuffer:
                                static_cast<Buffer*>(mBindings[index][binding])
                                    ->TrackUsageAndTransitionNow(commandContext,
                                                                 wgpu::BufferUsage::Storage);
                                break;

                            case wgpu::BindingType::ReadonlyStorageTexture: {
                                TextureViewBase* view =
                                    static_cast<TextureViewBase*>(mBindings[index][binding]);
                                ToBackend(view->GetTexture())
                                    ->TrackUsageAndTransitionNow(commandContext,
                                                                 kReadonlyStorageTexture,
                                                                 view->GetSubresourceRange());
                                break;
                            }
                            case wgpu::BindingType::WriteonlyStorageTexture: {
                                TextureViewBase* view =
                                    static_cast<TextureViewBase*>(mBindings[index][binding]);
                                ToBackend(view->GetTexture())
                                    ->TrackUsageAndTransitionNow(commandContext,
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
            }
            DidApply();

            return {};
        }

        void SetID3D12DescriptorHeaps(ID3D12GraphicsCommandList* commandList) {
            ASSERT(commandList != nullptr);
            std::array<ID3D12DescriptorHeap*, 2> descriptorHeaps = {
                mViewAllocator->GetShaderVisibleHeap(), mSamplerAllocator->GetShaderVisibleHeap()};
            ASSERT(descriptorHeaps[0] != nullptr);
            ASSERT(descriptorHeaps[1] != nullptr);
            commandList->SetDescriptorHeaps(descriptorHeaps.size(), descriptorHeaps.data());
        }

      private:
        void ApplyBindGroup(ID3D12GraphicsCommandList* commandList,
                            const PipelineLayout* pipelineLayout,
                            BindGroupIndex index,
                            BindGroup* group,
                            uint32_t dynamicOffsetCountIn,
                            const uint64_t* dynamicOffsetsIn) {
            ityp::span<BindingIndex, const uint64_t> dynamicOffsets(
                dynamicOffsetsIn, BindingIndex(dynamicOffsetCountIn));
            ASSERT(dynamicOffsets.size() == group->GetLayout()->GetDynamicBufferCount());

            // Usually, the application won't set the same offsets many times,
            // so always try to apply dynamic offsets even if the offsets stay the same
            if (dynamicOffsets.size() != BindingIndex(0)) {
                // Update dynamic offsets.
                // Dynamic buffer bindings are packed at the beginning of the layout.
                for (BindingIndex bindingIndex{0}; bindingIndex < dynamicOffsets.size();
                     ++bindingIndex) {
                    const BindingInfo& bindingInfo =
                        group->GetLayout()->GetBindingInfo(bindingIndex);
                    if (bindingInfo.visibility == wgpu::ShaderStage::None) {
                        // Skip dynamic buffers that are not visible. D3D12 does not have None
                        // visibility.
                        continue;
                    }

                    uint32_t parameterIndex =
                        pipelineLayout->GetDynamicRootParameterIndex(index, bindingIndex);
                    BufferBinding binding = group->GetBindingAsBufferBinding(bindingIndex);

                    // Calculate buffer locations that root descriptors links to. The location
                    // is (base buffer location + initial offset + dynamic offset)
                    uint64_t dynamicOffset = dynamicOffsets[bindingIndex];
                    uint64_t offset = binding.offset + dynamicOffset;
                    D3D12_GPU_VIRTUAL_ADDRESS bufferLocation =
                        ToBackend(binding.buffer)->GetVA() + offset;

                    switch (bindingInfo.type) {
                        case wgpu::BindingType::UniformBuffer:
                            if (mInCompute) {
                                commandList->SetComputeRootConstantBufferView(parameterIndex,
                                                                              bufferLocation);
                            } else {
                                commandList->SetGraphicsRootConstantBufferView(parameterIndex,
                                                                               bufferLocation);
                            }
                            break;
                        case wgpu::BindingType::StorageBuffer:
                            if (mInCompute) {
                                commandList->SetComputeRootUnorderedAccessView(parameterIndex,
                                                                               bufferLocation);
                            } else {
                                commandList->SetGraphicsRootUnorderedAccessView(parameterIndex,
                                                                                bufferLocation);
                            }
                            break;
                        case wgpu::BindingType::ReadonlyStorageBuffer:
                            if (mInCompute) {
                                commandList->SetComputeRootShaderResourceView(parameterIndex,
                                                                              bufferLocation);
                            } else {
                                commandList->SetGraphicsRootShaderResourceView(parameterIndex,
                                                                               bufferLocation);
                            }
                            break;
                        case wgpu::BindingType::SampledTexture:
                        case wgpu::BindingType::Sampler:
                        case wgpu::BindingType::ComparisonSampler:
                        case wgpu::BindingType::StorageTexture:
                        case wgpu::BindingType::ReadonlyStorageTexture:
                        case wgpu::BindingType::WriteonlyStorageTexture:
                            UNREACHABLE();
                            break;
                    }
                }
            }

            // It's not necessary to update descriptor tables if only the dynamic offset changed.
            if (!mDirtyBindGroups[index]) {
                return;
            }

            const uint32_t cbvUavSrvCount =
                ToBackend(group->GetLayout())->GetCbvUavSrvDescriptorCount();
            const uint32_t samplerCount =
                ToBackend(group->GetLayout())->GetSamplerDescriptorCount();

            if (cbvUavSrvCount > 0) {
                uint32_t parameterIndex = pipelineLayout->GetCbvUavSrvRootParameterIndex(index);
                const D3D12_GPU_DESCRIPTOR_HANDLE baseDescriptor = group->GetBaseViewDescriptor();
                if (mInCompute) {
                    commandList->SetComputeRootDescriptorTable(parameterIndex, baseDescriptor);
                } else {
                    commandList->SetGraphicsRootDescriptorTable(parameterIndex, baseDescriptor);
                }
            }

            if (samplerCount > 0) {
                uint32_t parameterIndex = pipelineLayout->GetSamplerRootParameterIndex(index);
                const D3D12_GPU_DESCRIPTOR_HANDLE baseDescriptor =
                    group->GetBaseSamplerDescriptor();
                // Check if the group requires its sampler table to be set in the pipeline.
                // This because sampler heap allocations could be cached and use the same table.
                if (mBoundRootSamplerTables[index].ptr != baseDescriptor.ptr) {
                    if (mInCompute) {
                        commandList->SetComputeRootDescriptorTable(parameterIndex, baseDescriptor);
                    } else {
                        commandList->SetGraphicsRootDescriptorTable(parameterIndex, baseDescriptor);
                    }

                    mBoundRootSamplerTables[index] = baseDescriptor;
                }
            }
        }

        Device* mDevice;

        bool mInCompute = false;

        ityp::array<BindGroupIndex, D3D12_GPU_DESCRIPTOR_HANDLE, kMaxBindGroups>
            mBoundRootSamplerTables = {};

        ShaderVisibleDescriptorAllocator* mViewAllocator;
        ShaderVisibleDescriptorAllocator* mSamplerAllocator;
    };

    namespace {
        class VertexBufferTracker {
          public:
            void OnSetVertexBuffer(uint32_t slot, Buffer* buffer, uint64_t offset, uint64_t size) {
                mStartSlot = std::min(mStartSlot, slot);
                mEndSlot = std::max(mEndSlot, slot + 1);

                auto* d3d12BufferView = &mD3D12BufferViews[slot];
                d3d12BufferView->BufferLocation = buffer->GetVA() + offset;
                d3d12BufferView->SizeInBytes = size;
                // The bufferView stride is set based on the vertex state before a draw.
            }

            void Apply(ID3D12GraphicsCommandList* commandList,
                       const RenderPipeline* renderPipeline) {
                ASSERT(renderPipeline != nullptr);

                std::bitset<kMaxVertexBuffers> vertexBufferSlotsUsed =
                    renderPipeline->GetVertexBufferSlotsUsed();

                uint32_t startSlot = mStartSlot;
                uint32_t endSlot = mEndSlot;

                // If the vertex state has changed, we need to update the StrideInBytes
                // for the D3D12 buffer views. We also need to extend the dirty range to
                // touch all these slots because the stride may have changed.
                if (mLastAppliedRenderPipeline != renderPipeline) {
                    mLastAppliedRenderPipeline = renderPipeline;

                    for (uint32_t slot : IterateBitSet(vertexBufferSlotsUsed)) {
                        startSlot = std::min(startSlot, slot);
                        endSlot = std::max(endSlot, slot + 1);
                        mD3D12BufferViews[slot].StrideInBytes =
                            renderPipeline->GetVertexBuffer(slot).arrayStride;
                    }
                }

                if (endSlot <= startSlot) {
                    return;
                }

                // mD3D12BufferViews is kept up to date with the most recent data passed
                // to SetVertexBuffer. This makes it correct to only track the start
                // and end of the dirty range. When Apply is called,
                // we will at worst set non-dirty vertex buffers in duplicate.
                uint32_t count = endSlot - startSlot;
                commandList->IASetVertexBuffers(startSlot, count, &mD3D12BufferViews[startSlot]);

                mStartSlot = kMaxVertexBuffers;
                mEndSlot = 0;
            }

          private:
            // startSlot and endSlot indicate the range of dirty vertex buffers.
            // If there are multiple calls to SetVertexBuffer, the start and end
            // represent the union of the dirty ranges (the union may have non-dirty
            // data in the middle of the range).
            const RenderPipeline* mLastAppliedRenderPipeline = nullptr;
            uint32_t mStartSlot = kMaxVertexBuffers;
            uint32_t mEndSlot = 0;
            std::array<D3D12_VERTEX_BUFFER_VIEW, kMaxVertexBuffers> mD3D12BufferViews = {};
        };

        class IndexBufferTracker {
          public:
            void OnSetIndexBuffer(Buffer* buffer, uint64_t offset, uint64_t size) {
                mD3D12BufferView.BufferLocation = buffer->GetVA() + offset;
                mD3D12BufferView.SizeInBytes = size;

                // We don't need to dirty the state unless BufferLocation or SizeInBytes
                // change, but most of the time this will always be the case.
                mLastAppliedIndexFormat = DXGI_FORMAT_UNKNOWN;
            }

            void OnSetPipeline(const RenderPipelineBase* pipeline) {
                mD3D12BufferView.Format =
                    DXGIIndexFormat(pipeline->GetVertexStateDescriptor()->indexFormat);
            }

            void Apply(ID3D12GraphicsCommandList* commandList) {
                if (mD3D12BufferView.Format == mLastAppliedIndexFormat) {
                    return;
                }

                commandList->IASetIndexBuffer(&mD3D12BufferView);
                mLastAppliedIndexFormat = mD3D12BufferView.Format;
            }

          private:
            DXGI_FORMAT mLastAppliedIndexFormat = DXGI_FORMAT_UNKNOWN;
            D3D12_INDEX_BUFFER_VIEW mD3D12BufferView = {};
        };

        void ResolveMultisampledRenderPass(CommandRecordingContext* commandContext,
                                           BeginRenderPassCmd* renderPass) {
            ASSERT(renderPass != nullptr);

            for (uint32_t i :
                 IterateBitSet(renderPass->attachmentState->GetColorAttachmentsMask())) {
                TextureViewBase* resolveTarget =
                    renderPass->colorAttachments[i].resolveTarget.Get();
                if (resolveTarget == nullptr) {
                    continue;
                }

                TextureViewBase* colorView = renderPass->colorAttachments[i].view.Get();
                Texture* colorTexture = ToBackend(colorView->GetTexture());
                Texture* resolveTexture = ToBackend(resolveTarget->GetTexture());

                // Transition the usages of the color attachment and resolve target.
                colorTexture->TrackUsageAndTransitionNow(commandContext,
                                                         D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
                                                         colorView->GetSubresourceRange());
                resolveTexture->TrackUsageAndTransitionNow(commandContext,
                                                           D3D12_RESOURCE_STATE_RESOLVE_DEST,
                                                           resolveTarget->GetSubresourceRange());

                // Do MSAA resolve with ResolveSubResource().
                ID3D12Resource* colorTextureHandle = colorTexture->GetD3D12Resource();
                ID3D12Resource* resolveTextureHandle = resolveTexture->GetD3D12Resource();
                const uint32_t resolveTextureSubresourceIndex = resolveTexture->GetSubresourceIndex(
                    resolveTarget->GetBaseMipLevel(), resolveTarget->GetBaseArrayLayer());
                constexpr uint32_t kColorTextureSubresourceIndex = 0;
                commandContext->GetCommandList()->ResolveSubresource(
                    resolveTextureHandle, resolveTextureSubresourceIndex, colorTextureHandle,
                    kColorTextureSubresourceIndex, colorTexture->GetD3D12Format());
            }
        }

    }  // anonymous namespace

    CommandBuffer::CommandBuffer(CommandEncoder* encoder, const CommandBufferDescriptor* descriptor)
        : CommandBufferBase(encoder, descriptor), mCommands(encoder->AcquireCommands()) {
    }

    CommandBuffer::~CommandBuffer() {
        FreeCommands(&mCommands);
    }

    MaybeError CommandBuffer::RecordCommands(CommandRecordingContext* commandContext) {
        Device* device = ToBackend(GetDevice());
        BindGroupStateTracker bindingTracker(device);

        ID3D12GraphicsCommandList* commandList = commandContext->GetCommandList();

        // Make sure we use the correct descriptors for this command list. Could be done once per
        // actual command list but here is ok because there should be few command buffers.
        bindingTracker.SetID3D12DescriptorHeaps(commandList);

        // Records the necessary barriers for the resource usage pre-computed by the frontend
        auto PrepareResourcesForSubmission = [](CommandRecordingContext* commandContext,
                                                const PassResourceUsage& usages) -> bool {
            std::vector<D3D12_RESOURCE_BARRIER> barriers;

            ID3D12GraphicsCommandList* commandList = commandContext->GetCommandList();

            wgpu::BufferUsage bufferUsages = wgpu::BufferUsage::None;

            for (size_t i = 0; i < usages.buffers.size(); ++i) {
                D3D12_RESOURCE_BARRIER barrier;
                if (ToBackend(usages.buffers[i])
                        ->TrackUsageAndGetResourceBarrier(commandContext, &barrier,
                                                          usages.bufferUsages[i])) {
                    barriers.push_back(barrier);
                }
                bufferUsages |= usages.bufferUsages[i];
            }

            for (size_t i = 0; i < usages.textures.size(); ++i) {
                Texture* texture = ToBackend(usages.textures[i]);
                // Clear textures that are not output attachments. Output attachments will be
                // cleared during record render pass if the texture subresource has not been
                // initialized before the render pass.
                if (!(usages.textureUsages[i].usage & wgpu::TextureUsage::OutputAttachment)) {
                    texture->EnsureSubresourceContentInitialized(commandContext,
                                                                 texture->GetAllSubresources());
                }
            }

            wgpu::TextureUsage textureUsages = wgpu::TextureUsage::None;

            for (size_t i = 0; i < usages.textures.size(); ++i) {
                ToBackend(usages.textures[i])
                    ->TrackUsageAndGetResourceBarrierForPass(commandContext, &barriers,
                                                             usages.textureUsages[i]);
                textureUsages |= usages.textureUsages[i].usage;
            }

            if (barriers.size()) {
                commandList->ResourceBarrier(barriers.size(), barriers.data());
            }

            return (bufferUsages & wgpu::BufferUsage::Storage ||
                    textureUsages & wgpu::TextureUsage::Storage);
        };

        const std::vector<PassResourceUsage>& passResourceUsages = GetResourceUsages().perPass;
        uint32_t nextPassNumber = 0;

        Command type;
        while (mCommands.NextCommandId(&type)) {
            switch (type) {
                case Command::BeginComputePass: {
                    mCommands.NextCommand<BeginComputePassCmd>();

                    PrepareResourcesForSubmission(commandContext,
                                                  passResourceUsages[nextPassNumber]);
                    bindingTracker.SetInComputePass(true);
                    DAWN_TRY(RecordComputePass(commandContext, &bindingTracker));

                    nextPassNumber++;
                    break;
                }

                case Command::BeginRenderPass: {
                    BeginRenderPassCmd* beginRenderPassCmd =
                        mCommands.NextCommand<BeginRenderPassCmd>();

                    const bool passHasUAV = PrepareResourcesForSubmission(
                        commandContext, passResourceUsages[nextPassNumber]);
                    bindingTracker.SetInComputePass(false);

                    LazyClearRenderPassAttachments(beginRenderPassCmd);
                    DAWN_TRY(RecordRenderPass(commandContext, &bindingTracker, beginRenderPassCmd,
                                              passHasUAV));

                    nextPassNumber++;
                    break;
                }

                case Command::CopyBufferToBuffer: {
                    CopyBufferToBufferCmd* copy = mCommands.NextCommand<CopyBufferToBufferCmd>();
                    Buffer* srcBuffer = ToBackend(copy->source.Get());
                    Buffer* dstBuffer = ToBackend(copy->destination.Get());

                    DAWN_TRY(srcBuffer->EnsureDataInitialized(commandContext));
                    DAWN_TRY(dstBuffer->EnsureDataInitializedAsDestination(
                        commandContext, copy->destinationOffset, copy->size));

                    srcBuffer->TrackUsageAndTransitionNow(commandContext,
                                                          wgpu::BufferUsage::CopySrc);
                    dstBuffer->TrackUsageAndTransitionNow(commandContext,
                                                          wgpu::BufferUsage::CopyDst);

                    commandList->CopyBufferRegion(
                        dstBuffer->GetD3D12Resource(), copy->destinationOffset,
                        srcBuffer->GetD3D12Resource(), copy->sourceOffset, copy->size);
                    break;
                }

                case Command::CopyBufferToTexture: {
                    CopyBufferToTextureCmd* copy = mCommands.NextCommand<CopyBufferToTextureCmd>();
                    Buffer* buffer = ToBackend(copy->source.buffer.Get());
                    Texture* texture = ToBackend(copy->destination.texture.Get());

                    DAWN_TRY(buffer->EnsureDataInitialized(commandContext));

                    ASSERT(texture->GetDimension() == wgpu::TextureDimension::e2D);
                    SubresourceRange subresources = {copy->destination.mipLevel, 1,
                                                     copy->destination.origin.z,
                                                     copy->copySize.depth};
                    if (IsCompleteSubresourceCopiedTo(texture, copy->copySize,
                                                      copy->destination.mipLevel)) {
                        texture->SetIsSubresourceContentInitialized(true, subresources);
                    } else {
                        texture->EnsureSubresourceContentInitialized(commandContext, subresources);
                    }

                    buffer->TrackUsageAndTransitionNow(commandContext, wgpu::BufferUsage::CopySrc);
                    texture->TrackUsageAndTransitionNow(commandContext, wgpu::TextureUsage::CopyDst,
                                                        subresources);

                    // See comments in ComputeTextureCopySplits() for more details.
                    const TextureCopySplits copySplits = ComputeTextureCopySplits(
                        copy->destination.origin, copy->copySize, texture->GetFormat(),
                        copy->source.offset, copy->source.bytesPerRow, copy->source.rowsPerImage);

                    const uint64_t bytesPerSlice =
                        copy->source.bytesPerRow *
                        (copy->source.rowsPerImage / texture->GetFormat().blockHeight);

                    // copySplits.copies2D[1] is always calculated for the second copy slice with
                    // extra "bytesPerSlice" copy offset compared with the first copy slice. So
                    // here we use an array bufferOffsetsForNextSlice to record the extra offsets
                    // for each copy slice: bufferOffsetsForNextSlice[0] is the extra offset for
                    // the next copy slice that uses copySplits.copies2D[0], and
                    // bufferOffsetsForNextSlice[1] is the extra offset for the next copy slice
                    // that uses copySplits.copies2D[1].
                    std::array<uint64_t, TextureCopySplits::kMaxTextureCopySplits>
                        bufferOffsetsForNextSlice = {{0u, 0u}};
                    for (uint32_t copySlice = 0; copySlice < copy->copySize.depth; ++copySlice) {
                        const uint32_t splitIndex = copySlice % copySplits.copies2D.size();

                        const Texture2DCopySplit& copySplitPerLayerBase =
                            copySplits.copies2D[splitIndex];
                        const uint64_t bufferOffsetForNextSlice =
                            bufferOffsetsForNextSlice[splitIndex];
                        const uint32_t copyTextureLayer = copySlice + copy->destination.origin.z;

                        RecordCopyBufferToTextureFromTextureCopySplit(
                            commandList, copySplitPerLayerBase, buffer, bufferOffsetForNextSlice,
                            copy->source.bytesPerRow, texture, copy->destination.mipLevel,
                            copyTextureLayer);

                        bufferOffsetsForNextSlice[splitIndex] +=
                            bytesPerSlice * copySplits.copies2D.size();
                    }

                    break;
                }

                case Command::CopyTextureToBuffer: {
                    CopyTextureToBufferCmd* copy = mCommands.NextCommand<CopyTextureToBufferCmd>();
                    Texture* texture = ToBackend(copy->source.texture.Get());
                    Buffer* buffer = ToBackend(copy->destination.buffer.Get());

                    DAWN_TRY(buffer->EnsureDataInitializedAsDestination(commandContext, copy));

                    ASSERT(texture->GetDimension() == wgpu::TextureDimension::e2D);
                    SubresourceRange subresources = {copy->source.mipLevel, 1,
                                                     copy->source.origin.z, copy->copySize.depth};
                    texture->EnsureSubresourceContentInitialized(commandContext, subresources);

                    texture->TrackUsageAndTransitionNow(commandContext, wgpu::TextureUsage::CopySrc,
                                                        subresources);
                    buffer->TrackUsageAndTransitionNow(commandContext, wgpu::BufferUsage::CopyDst);

                    // See comments around ComputeTextureCopySplits() for more details.
                    const TextureCopySplits copySplits = ComputeTextureCopySplits(
                        copy->source.origin, copy->copySize, texture->GetFormat(),
                        copy->destination.offset, copy->destination.bytesPerRow,
                        copy->destination.rowsPerImage);

                    const uint64_t bytesPerSlice =
                        copy->destination.bytesPerRow *
                        (copy->destination.rowsPerImage / texture->GetFormat().blockHeight);

                    // copySplits.copies2D[1] is always calculated for the second copy slice with
                    // extra "bytesPerSlice" copy offset compared with the first copy slice. So
                    // here we use an array bufferOffsetsForNextSlice to record the extra offsets
                    // for each copy slice: bufferOffsetsForNextSlice[0] is the extra offset for
                    // the next copy slice that uses copySplits.copies2D[0], and
                    // bufferOffsetsForNextSlice[1] is the extra offset for the next copy slice
                    // that uses copySplits.copies2D[1].
                    std::array<uint64_t, TextureCopySplits::kMaxTextureCopySplits>
                        bufferOffsetsForNextSlice = {{0u, 0u}};
                    for (uint32_t copySlice = 0; copySlice < copy->copySize.depth; ++copySlice) {
                        const uint32_t splitIndex = copySlice % copySplits.copies2D.size();

                        const Texture2DCopySplit& copySplitPerLayerBase =
                            copySplits.copies2D[splitIndex];
                        const uint64_t bufferOffsetForNextSlice =
                            bufferOffsetsForNextSlice[splitIndex];
                        const uint32_t copyTextureLayer = copySlice + copy->source.origin.z;

                        RecordCopyTextureToBufferFromTextureCopySplit(
                            commandList, copySplitPerLayerBase, buffer, bufferOffsetForNextSlice,
                            copy->destination.bytesPerRow, texture, copy->source.mipLevel,
                            copyTextureLayer);

                        bufferOffsetsForNextSlice[splitIndex] +=
                            bytesPerSlice * copySplits.copies2D.size();
                    }

                    break;
                }

                case Command::CopyTextureToTexture: {
                    CopyTextureToTextureCmd* copy =
                        mCommands.NextCommand<CopyTextureToTextureCmd>();

                    Texture* source = ToBackend(copy->source.texture.Get());
                    Texture* destination = ToBackend(copy->destination.texture.Get());
                    SubresourceRange srcRange = {copy->source.mipLevel, 1, copy->source.origin.z,
                                                 copy->copySize.depth};
                    SubresourceRange dstRange = {copy->destination.mipLevel, 1,
                                                 copy->destination.origin.z, copy->copySize.depth};

                    source->EnsureSubresourceContentInitialized(commandContext, srcRange);
                    if (IsCompleteSubresourceCopiedTo(destination, copy->copySize,
                                                      copy->destination.mipLevel)) {
                        destination->SetIsSubresourceContentInitialized(true, dstRange);
                    } else {
                        destination->EnsureSubresourceContentInitialized(commandContext, dstRange);
                    }

                    if (copy->source.texture.Get() == copy->destination.texture.Get() &&
                        copy->source.mipLevel == copy->destination.mipLevel) {
                        // When there are overlapped subresources, the layout of the overlapped
                        // subresources should all be COMMON instead of what we set now. Currently
                        // it is not allowed to copy with overlapped subresources, but we still
                        // add the ASSERT here as a reminder for this possible misuse.
                        ASSERT(!IsRangeOverlapped(copy->source.origin.z, copy->destination.origin.z,
                                                  copy->copySize.depth));
                    }
                    source->TrackUsageAndTransitionNow(commandContext, wgpu::TextureUsage::CopySrc,
                                                       srcRange);
                    destination->TrackUsageAndTransitionNow(commandContext,
                                                            wgpu::TextureUsage::CopyDst, dstRange);

                    if (CanUseCopyResource(source, destination, copy->copySize)) {
                        commandList->CopyResource(destination->GetD3D12Resource(),
                                                  source->GetD3D12Resource());
                    } else {
                        // TODO(jiawei.shao@intel.com): support copying with 1D and 3D textures.
                        ASSERT(source->GetDimension() == wgpu::TextureDimension::e2D &&
                               destination->GetDimension() == wgpu::TextureDimension::e2D);
                        const dawn_native::Extent3D copyExtentOneSlice = {
                            copy->copySize.width, copy->copySize.height, 1u};
                        for (uint32_t slice = 0; slice < copy->copySize.depth; ++slice) {
                            D3D12_TEXTURE_COPY_LOCATION srcLocation =
                                ComputeTextureCopyLocationForTexture(source, copy->source.mipLevel,
                                                                     copy->source.origin.z + slice);

                            D3D12_TEXTURE_COPY_LOCATION dstLocation =
                                ComputeTextureCopyLocationForTexture(
                                    destination, copy->destination.mipLevel,
                                    copy->destination.origin.z + slice);

                            Origin3D sourceOriginInSubresource = copy->source.origin;
                            sourceOriginInSubresource.z = 0;
                            D3D12_BOX sourceRegion = ComputeD3D12BoxFromOffsetAndSize(
                                sourceOriginInSubresource, copyExtentOneSlice);

                            commandList->CopyTextureRegion(&dstLocation, copy->destination.origin.x,
                                                           copy->destination.origin.y, 0,
                                                           &srcLocation, &sourceRegion);
                        }
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

        return {};
    }

    MaybeError CommandBuffer::RecordComputePass(CommandRecordingContext* commandContext,
                                                BindGroupStateTracker* bindingTracker) {
        PipelineLayout* lastLayout = nullptr;
        ID3D12GraphicsCommandList* commandList = commandContext->GetCommandList();

        Command type;
        while (mCommands.NextCommandId(&type)) {
            switch (type) {
                case Command::Dispatch: {
                    DispatchCmd* dispatch = mCommands.NextCommand<DispatchCmd>();

                    DAWN_TRY(bindingTracker->Apply(commandContext));
                    commandList->Dispatch(dispatch->x, dispatch->y, dispatch->z);
                    break;
                }

                case Command::DispatchIndirect: {
                    DispatchIndirectCmd* dispatch = mCommands.NextCommand<DispatchIndirectCmd>();

                    DAWN_TRY(bindingTracker->Apply(commandContext));
                    Buffer* buffer = ToBackend(dispatch->indirectBuffer.Get());
                    ComPtr<ID3D12CommandSignature> signature =
                        ToBackend(GetDevice())->GetDispatchIndirectSignature();
                    commandList->ExecuteIndirect(signature.Get(), 1, buffer->GetD3D12Resource(),
                                                 dispatch->indirectOffset, nullptr, 0);
                    break;
                }

                case Command::EndComputePass: {
                    mCommands.NextCommand<EndComputePassCmd>();
                    return {};
                }

                case Command::SetComputePipeline: {
                    SetComputePipelineCmd* cmd = mCommands.NextCommand<SetComputePipelineCmd>();
                    ComputePipeline* pipeline = ToBackend(cmd->pipeline).Get();
                    PipelineLayout* layout = ToBackend(pipeline->GetLayout());

                    commandList->SetComputeRootSignature(layout->GetRootSignature());
                    commandList->SetPipelineState(pipeline->GetPipelineState());

                    bindingTracker->OnSetPipeline(pipeline);

                    lastLayout = layout;
                    break;
                }

                case Command::SetBindGroup: {
                    SetBindGroupCmd* cmd = mCommands.NextCommand<SetBindGroupCmd>();
                    BindGroup* group = ToBackend(cmd->group.Get());
                    uint32_t* dynamicOffsets = nullptr;

                    if (cmd->dynamicOffsetCount > 0) {
                        dynamicOffsets = mCommands.NextData<uint32_t>(cmd->dynamicOffsetCount);
                    }

                    bindingTracker->OnSetBindGroup(cmd->index, group, cmd->dynamicOffsetCount,
                                                   dynamicOffsets);
                    break;
                }

                case Command::InsertDebugMarker: {
                    InsertDebugMarkerCmd* cmd = mCommands.NextCommand<InsertDebugMarkerCmd>();
                    const char* label = mCommands.NextData<char>(cmd->length + 1);

                    if (ToBackend(GetDevice())->GetFunctions()->IsPIXEventRuntimeLoaded()) {
                        // PIX color is 1 byte per channel in ARGB format
                        constexpr uint64_t kPIXBlackColor = 0xff000000;
                        ToBackend(GetDevice())
                            ->GetFunctions()
                            ->pixSetMarkerOnCommandList(commandList, kPIXBlackColor, label);
                    }
                    break;
                }

                case Command::PopDebugGroup: {
                    mCommands.NextCommand<PopDebugGroupCmd>();

                    if (ToBackend(GetDevice())->GetFunctions()->IsPIXEventRuntimeLoaded()) {
                        ToBackend(GetDevice())
                            ->GetFunctions()
                            ->pixEndEventOnCommandList(commandList);
                    }
                    break;
                }

                case Command::PushDebugGroup: {
                    PushDebugGroupCmd* cmd = mCommands.NextCommand<PushDebugGroupCmd>();
                    const char* label = mCommands.NextData<char>(cmd->length + 1);

                    if (ToBackend(GetDevice())->GetFunctions()->IsPIXEventRuntimeLoaded()) {
                        // PIX color is 1 byte per channel in ARGB format
                        constexpr uint64_t kPIXBlackColor = 0xff000000;
                        ToBackend(GetDevice())
                            ->GetFunctions()
                            ->pixBeginEventOnCommandList(commandList, kPIXBlackColor, label);
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

        return {};
    }

    MaybeError CommandBuffer::SetupRenderPass(CommandRecordingContext* commandContext,
                                              BeginRenderPassCmd* renderPass,
                                              RenderPassBuilder* renderPassBuilder) {
        Device* device = ToBackend(GetDevice());

        for (uint32_t i : IterateBitSet(renderPass->attachmentState->GetColorAttachmentsMask())) {
            RenderPassColorAttachmentInfo& attachmentInfo = renderPass->colorAttachments[i];
            TextureView* view = ToBackend(attachmentInfo.view.Get());

            // Set view attachment.
            CPUDescriptorHeapAllocation rtvAllocation;
            DAWN_TRY_ASSIGN(
                rtvAllocation,
                device->GetRenderTargetViewAllocator()->AllocateTransientCPUDescriptors());

            const D3D12_RENDER_TARGET_VIEW_DESC viewDesc = view->GetRTVDescriptor();
            const D3D12_CPU_DESCRIPTOR_HANDLE baseDescriptor = rtvAllocation.GetBaseDescriptor();

            device->GetD3D12Device()->CreateRenderTargetView(
                ToBackend(view->GetTexture())->GetD3D12Resource(), &viewDesc, baseDescriptor);

            renderPassBuilder->SetRenderTargetView(i, baseDescriptor);

            // Set color load operation.
            renderPassBuilder->SetRenderTargetBeginningAccess(
                i, attachmentInfo.loadOp, attachmentInfo.clearColor, view->GetD3D12Format());

            // Set color store operation.
            if (attachmentInfo.resolveTarget.Get() != nullptr) {
                TextureView* resolveDestinationView = ToBackend(attachmentInfo.resolveTarget.Get());
                Texture* resolveDestinationTexture =
                    ToBackend(resolveDestinationView->GetTexture());

                resolveDestinationTexture->TrackUsageAndTransitionNow(
                    commandContext, D3D12_RESOURCE_STATE_RESOLVE_DEST,
                    resolveDestinationView->GetSubresourceRange());

                renderPassBuilder->SetRenderTargetEndingAccessResolve(i, attachmentInfo.storeOp,
                                                                      view, resolveDestinationView);
            } else {
                renderPassBuilder->SetRenderTargetEndingAccess(i, attachmentInfo.storeOp);
            }
        }

        if (renderPass->attachmentState->HasDepthStencilAttachment()) {
            RenderPassDepthStencilAttachmentInfo& attachmentInfo =
                renderPass->depthStencilAttachment;
            TextureView* view = ToBackend(renderPass->depthStencilAttachment.view.Get());

            // Set depth attachment.
            CPUDescriptorHeapAllocation dsvAllocation;
            DAWN_TRY_ASSIGN(
                dsvAllocation,
                device->GetDepthStencilViewAllocator()->AllocateTransientCPUDescriptors());

            const D3D12_DEPTH_STENCIL_VIEW_DESC viewDesc = view->GetDSVDescriptor();
            const D3D12_CPU_DESCRIPTOR_HANDLE baseDescriptor = dsvAllocation.GetBaseDescriptor();

            device->GetD3D12Device()->CreateDepthStencilView(
                ToBackend(view->GetTexture())->GetD3D12Resource(), &viewDesc, baseDescriptor);

            renderPassBuilder->SetDepthStencilView(baseDescriptor);

            const bool hasDepth = view->GetTexture()->GetFormat().HasDepth();
            const bool hasStencil = view->GetTexture()->GetFormat().HasStencil();

            // Set depth/stencil load operations.
            if (hasDepth) {
                renderPassBuilder->SetDepthAccess(
                    attachmentInfo.depthLoadOp, attachmentInfo.depthStoreOp,
                    attachmentInfo.clearDepth, view->GetD3D12Format());
            } else {
                renderPassBuilder->SetDepthNoAccess();
            }

            if (hasStencil) {
                renderPassBuilder->SetStencilAccess(
                    attachmentInfo.stencilLoadOp, attachmentInfo.stencilStoreOp,
                    attachmentInfo.clearStencil, view->GetD3D12Format());
            } else {
                renderPassBuilder->SetStencilNoAccess();
            }

        } else {
            renderPassBuilder->SetDepthStencilNoAccess();
        }

        return {};
    }

    void CommandBuffer::EmulateBeginRenderPass(CommandRecordingContext* commandContext,
                                               const RenderPassBuilder* renderPassBuilder) const {
        ID3D12GraphicsCommandList* commandList = commandContext->GetCommandList();

        // Clear framebuffer attachments as needed.
        {
            for (uint32_t i = 0; i < renderPassBuilder->GetColorAttachmentCount(); i++) {
                // Load op - color
                if (renderPassBuilder->GetRenderPassRenderTargetDescriptors()[i]
                        .BeginningAccess.Type == D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR) {
                    commandList->ClearRenderTargetView(
                        renderPassBuilder->GetRenderPassRenderTargetDescriptors()[i].cpuDescriptor,
                        renderPassBuilder->GetRenderPassRenderTargetDescriptors()[i]
                            .BeginningAccess.Clear.ClearValue.Color,
                        0, nullptr);
                }
            }

            if (renderPassBuilder->HasDepth()) {
                D3D12_CLEAR_FLAGS clearFlags = {};
                float depthClear = 0.0f;
                uint8_t stencilClear = 0u;

                if (renderPassBuilder->GetRenderPassDepthStencilDescriptor()
                        ->DepthBeginningAccess.Type ==
                    D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR) {
                    clearFlags |= D3D12_CLEAR_FLAG_DEPTH;
                    depthClear = renderPassBuilder->GetRenderPassDepthStencilDescriptor()
                                     ->DepthBeginningAccess.Clear.ClearValue.DepthStencil.Depth;
                }
                if (renderPassBuilder->GetRenderPassDepthStencilDescriptor()
                        ->StencilBeginningAccess.Type ==
                    D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR) {
                    clearFlags |= D3D12_CLEAR_FLAG_STENCIL;
                    stencilClear =
                        renderPassBuilder->GetRenderPassDepthStencilDescriptor()
                            ->StencilBeginningAccess.Clear.ClearValue.DepthStencil.Stencil;
                }

                // TODO(kainino@chromium.org): investigate: should the Dawn clear
                // stencil type be uint8_t?
                if (clearFlags) {
                    commandList->ClearDepthStencilView(
                        renderPassBuilder->GetRenderPassDepthStencilDescriptor()->cpuDescriptor,
                        clearFlags, depthClear, stencilClear, 0, nullptr);
                }
            }
        }

        commandList->OMSetRenderTargets(
            renderPassBuilder->GetColorAttachmentCount(), renderPassBuilder->GetRenderTargetViews(),
            FALSE,
            renderPassBuilder->HasDepth()
                ? &renderPassBuilder->GetRenderPassDepthStencilDescriptor()->cpuDescriptor
                : nullptr);
    }

    MaybeError CommandBuffer::RecordRenderPass(CommandRecordingContext* commandContext,
                                               BindGroupStateTracker* bindingTracker,
                                               BeginRenderPassCmd* renderPass,
                                               const bool passHasUAV) {
        Device* device = ToBackend(GetDevice());
        const bool useRenderPass = device->IsToggleEnabled(Toggle::UseD3D12RenderPass);

        // renderPassBuilder must be scoped to RecordRenderPass because any underlying
        // D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS structs must remain
        // valid until after EndRenderPass() has been called.
        RenderPassBuilder renderPassBuilder(passHasUAV);

        DAWN_TRY(SetupRenderPass(commandContext, renderPass, &renderPassBuilder));

        // Use D3D12's native render pass API if it's available, otherwise emulate the
        // beginning and ending access operations.
        if (useRenderPass) {
            commandContext->GetCommandList4()->BeginRenderPass(
                renderPassBuilder.GetColorAttachmentCount(),
                renderPassBuilder.GetRenderPassRenderTargetDescriptors(),
                renderPassBuilder.HasDepth()
                    ? renderPassBuilder.GetRenderPassDepthStencilDescriptor()
                    : nullptr,
                renderPassBuilder.GetRenderPassFlags());
        } else {
            EmulateBeginRenderPass(commandContext, &renderPassBuilder);
        }

        ID3D12GraphicsCommandList* commandList = commandContext->GetCommandList();

        // Set up default dynamic state
        {
            uint32_t width = renderPass->width;
            uint32_t height = renderPass->height;
            D3D12_VIEWPORT viewport = {
                0.f, 0.f, static_cast<float>(width), static_cast<float>(height), 0.f, 1.f};
            D3D12_RECT scissorRect = {0, 0, static_cast<long>(width), static_cast<long>(height)};
            commandList->RSSetViewports(1, &viewport);
            commandList->RSSetScissorRects(1, &scissorRect);

            static constexpr std::array<float, 4> defaultBlendFactor = {0, 0, 0, 0};
            commandList->OMSetBlendFactor(&defaultBlendFactor[0]);
        }

        RenderPipeline* lastPipeline = nullptr;
        PipelineLayout* lastLayout = nullptr;
        VertexBufferTracker vertexBufferTracker = {};
        IndexBufferTracker indexBufferTracker = {};

        auto EncodeRenderBundleCommand = [&](CommandIterator* iter, Command type) -> MaybeError {
            switch (type) {
                case Command::Draw: {
                    DrawCmd* draw = iter->NextCommand<DrawCmd>();

                    DAWN_TRY(bindingTracker->Apply(commandContext));
                    vertexBufferTracker.Apply(commandList, lastPipeline);
                    commandList->DrawInstanced(draw->vertexCount, draw->instanceCount,
                                               draw->firstVertex, draw->firstInstance);
                    break;
                }

                case Command::DrawIndexed: {
                    DrawIndexedCmd* draw = iter->NextCommand<DrawIndexedCmd>();

                    DAWN_TRY(bindingTracker->Apply(commandContext));
                    indexBufferTracker.Apply(commandList);
                    vertexBufferTracker.Apply(commandList, lastPipeline);
                    commandList->DrawIndexedInstanced(draw->indexCount, draw->instanceCount,
                                                      draw->firstIndex, draw->baseVertex,
                                                      draw->firstInstance);
                    break;
                }

                case Command::DrawIndirect: {
                    DrawIndirectCmd* draw = iter->NextCommand<DrawIndirectCmd>();

                    DAWN_TRY(bindingTracker->Apply(commandContext));
                    vertexBufferTracker.Apply(commandList, lastPipeline);
                    Buffer* buffer = ToBackend(draw->indirectBuffer.Get());
                    ComPtr<ID3D12CommandSignature> signature =
                        ToBackend(GetDevice())->GetDrawIndirectSignature();
                    commandList->ExecuteIndirect(signature.Get(), 1, buffer->GetD3D12Resource(),
                                                 draw->indirectOffset, nullptr, 0);
                    break;
                }

                case Command::DrawIndexedIndirect: {
                    DrawIndexedIndirectCmd* draw = iter->NextCommand<DrawIndexedIndirectCmd>();

                    DAWN_TRY(bindingTracker->Apply(commandContext));
                    indexBufferTracker.Apply(commandList);
                    vertexBufferTracker.Apply(commandList, lastPipeline);
                    Buffer* buffer = ToBackend(draw->indirectBuffer.Get());
                    ComPtr<ID3D12CommandSignature> signature =
                        ToBackend(GetDevice())->GetDrawIndexedIndirectSignature();
                    commandList->ExecuteIndirect(signature.Get(), 1, buffer->GetD3D12Resource(),
                                                 draw->indirectOffset, nullptr, 0);
                    break;
                }

                case Command::InsertDebugMarker: {
                    InsertDebugMarkerCmd* cmd = iter->NextCommand<InsertDebugMarkerCmd>();
                    const char* label = iter->NextData<char>(cmd->length + 1);

                    if (ToBackend(GetDevice())->GetFunctions()->IsPIXEventRuntimeLoaded()) {
                        // PIX color is 1 byte per channel in ARGB format
                        constexpr uint64_t kPIXBlackColor = 0xff000000;
                        ToBackend(GetDevice())
                            ->GetFunctions()
                            ->pixSetMarkerOnCommandList(commandList, kPIXBlackColor, label);
                    }
                    break;
                }

                case Command::PopDebugGroup: {
                    iter->NextCommand<PopDebugGroupCmd>();

                    if (ToBackend(GetDevice())->GetFunctions()->IsPIXEventRuntimeLoaded()) {
                        ToBackend(GetDevice())
                            ->GetFunctions()
                            ->pixEndEventOnCommandList(commandList);
                    }
                    break;
                }

                case Command::PushDebugGroup: {
                    PushDebugGroupCmd* cmd = iter->NextCommand<PushDebugGroupCmd>();
                    const char* label = iter->NextData<char>(cmd->length + 1);

                    if (ToBackend(GetDevice())->GetFunctions()->IsPIXEventRuntimeLoaded()) {
                        // PIX color is 1 byte per channel in ARGB format
                        constexpr uint64_t kPIXBlackColor = 0xff000000;
                        ToBackend(GetDevice())
                            ->GetFunctions()
                            ->pixBeginEventOnCommandList(commandList, kPIXBlackColor, label);
                    }
                    break;
                }

                case Command::SetRenderPipeline: {
                    SetRenderPipelineCmd* cmd = iter->NextCommand<SetRenderPipelineCmd>();
                    RenderPipeline* pipeline = ToBackend(cmd->pipeline).Get();
                    PipelineLayout* layout = ToBackend(pipeline->GetLayout());

                    commandList->SetGraphicsRootSignature(layout->GetRootSignature());
                    commandList->SetPipelineState(pipeline->GetPipelineState());
                    commandList->IASetPrimitiveTopology(pipeline->GetD3D12PrimitiveTopology());

                    bindingTracker->OnSetPipeline(pipeline);
                    indexBufferTracker.OnSetPipeline(pipeline);

                    lastPipeline = pipeline;
                    lastLayout = layout;
                    break;
                }

                case Command::SetBindGroup: {
                    SetBindGroupCmd* cmd = iter->NextCommand<SetBindGroupCmd>();
                    BindGroup* group = ToBackend(cmd->group.Get());
                    uint32_t* dynamicOffsets = nullptr;

                    if (cmd->dynamicOffsetCount > 0) {
                        dynamicOffsets = iter->NextData<uint32_t>(cmd->dynamicOffsetCount);
                    }

                    bindingTracker->OnSetBindGroup(cmd->index, group, cmd->dynamicOffsetCount,
                                                   dynamicOffsets);
                    break;
                }

                case Command::SetIndexBuffer: {
                    SetIndexBufferCmd* cmd = iter->NextCommand<SetIndexBufferCmd>();

                    indexBufferTracker.OnSetIndexBuffer(ToBackend(cmd->buffer.Get()), cmd->offset,
                                                        cmd->size);
                    break;
                }

                case Command::SetVertexBuffer: {
                    SetVertexBufferCmd* cmd = iter->NextCommand<SetVertexBufferCmd>();

                    vertexBufferTracker.OnSetVertexBuffer(cmd->slot, ToBackend(cmd->buffer.Get()),
                                                          cmd->offset, cmd->size);
                    break;
                }

                default:
                    UNREACHABLE();
                    break;
            }
            return {};
        };

        Command type;
        while (mCommands.NextCommandId(&type)) {
            switch (type) {
                case Command::EndRenderPass: {
                    mCommands.NextCommand<EndRenderPassCmd>();
                    if (useRenderPass) {
                        commandContext->GetCommandList4()->EndRenderPass();
                    } else if (renderPass->attachmentState->GetSampleCount() > 1) {
                        ResolveMultisampledRenderPass(commandContext, renderPass);
                    }
                    return {};
                }

                case Command::SetStencilReference: {
                    SetStencilReferenceCmd* cmd = mCommands.NextCommand<SetStencilReferenceCmd>();

                    commandList->OMSetStencilRef(cmd->reference);
                    break;
                }

                case Command::SetViewport: {
                    SetViewportCmd* cmd = mCommands.NextCommand<SetViewportCmd>();
                    D3D12_VIEWPORT viewport;
                    viewport.TopLeftX = cmd->x;
                    viewport.TopLeftY = cmd->y;
                    viewport.Width = cmd->width;
                    viewport.Height = cmd->height;
                    viewport.MinDepth = cmd->minDepth;
                    viewport.MaxDepth = cmd->maxDepth;

                    commandList->RSSetViewports(1, &viewport);
                    break;
                }

                case Command::SetScissorRect: {
                    SetScissorRectCmd* cmd = mCommands.NextCommand<SetScissorRectCmd>();
                    D3D12_RECT rect;
                    rect.left = cmd->x;
                    rect.top = cmd->y;
                    rect.right = cmd->x + cmd->width;
                    rect.bottom = cmd->y + cmd->height;

                    commandList->RSSetScissorRects(1, &rect);
                    break;
                }

                case Command::SetBlendColor: {
                    SetBlendColorCmd* cmd = mCommands.NextCommand<SetBlendColorCmd>();
                    commandList->OMSetBlendFactor(static_cast<const FLOAT*>(&cmd->color.r));
                    break;
                }

                case Command::ExecuteBundles: {
                    ExecuteBundlesCmd* cmd = mCommands.NextCommand<ExecuteBundlesCmd>();
                    auto bundles = mCommands.NextData<Ref<RenderBundleBase>>(cmd->count);

                    for (uint32_t i = 0; i < cmd->count; ++i) {
                        CommandIterator* iter = bundles[i]->GetCommands();
                        iter->Reset();
                        while (iter->NextCommandId(&type)) {
                            DAWN_TRY(EncodeRenderBundleCommand(iter, type));
                        }
                    }
                    break;
                }

                case Command::WriteTimestamp: {
                    return DAWN_UNIMPLEMENTED_ERROR("Waiting for implementation.");
                }

                default: {
                    DAWN_TRY(EncodeRenderBundleCommand(&mCommands, type));
                    break;
                }
            }
        }
        return {};
    }
}}  // namespace dawn_native::d3d12
