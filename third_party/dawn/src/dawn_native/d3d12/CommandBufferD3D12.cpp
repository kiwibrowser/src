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
#include "dawn_native/CommandEncoder.h"
#include "dawn_native/Commands.h"
#include "dawn_native/d3d12/BindGroupD3D12.h"
#include "dawn_native/d3d12/BindGroupLayoutD3D12.h"
#include "dawn_native/d3d12/BufferD3D12.h"
#include "dawn_native/d3d12/ComputePipelineD3D12.h"
#include "dawn_native/d3d12/DescriptorHeapAllocator.h"
#include "dawn_native/d3d12/DeviceD3D12.h"
#include "dawn_native/d3d12/PipelineLayoutD3D12.h"
#include "dawn_native/d3d12/PlatformFunctions.h"
#include "dawn_native/d3d12/RenderPipelineD3D12.h"
#include "dawn_native/d3d12/ResourceAllocator.h"
#include "dawn_native/d3d12/SamplerD3D12.h"
#include "dawn_native/d3d12/TextureCopySplitter.h"
#include "dawn_native/d3d12/TextureD3D12.h"

#include <deque>

namespace dawn_native { namespace d3d12 {

    namespace {
        DXGI_FORMAT DXGIIndexFormat(dawn::IndexFormat format) {
            switch (format) {
                case dawn::IndexFormat::Uint16:
                    return DXGI_FORMAT_R16_UINT;
                case dawn::IndexFormat::Uint32:
                    return DXGI_FORMAT_R32_UINT;
                default:
                    UNREACHABLE();
            }
        }

        D3D12_TEXTURE_COPY_LOCATION CreateTextureCopyLocationForTexture(const Texture& texture,
                                                                        uint32_t level,
                                                                        uint32_t slice) {
            D3D12_TEXTURE_COPY_LOCATION copyLocation;
            copyLocation.pResource = texture.GetD3D12Resource();
            copyLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            copyLocation.SubresourceIndex = texture.GetSubresourceIndex(level, slice);

            return copyLocation;
        }

        bool CanUseCopyResource(const uint32_t sourceNumMipLevels,
                                const Extent3D& srcSize,
                                const Extent3D& dstSize,
                                const Extent3D& copySize) {
            if (sourceNumMipLevels == 1 && srcSize.width == dstSize.width &&
                srcSize.height == dstSize.height && srcSize.depth == dstSize.depth &&
                srcSize.width == copySize.width && srcSize.height == copySize.height &&
                srcSize.depth == copySize.depth) {
                return true;
            }

            return false;
        }

    }  // anonymous namespace

    class BindGroupStateTracker {
      public:
        BindGroupStateTracker(Device* device) : mDevice(device) {
        }

        void SetInComputePass(bool inCompute_) {
            mInCompute = inCompute_;
        }

        void AllocateDescriptorHeaps(Device* device) {
            // This function should only be called once.
            ASSERT(mCbvSrvUavGPUDescriptorHeap.Get() == nullptr &&
                   mSamplerGPUDescriptorHeap.Get() == nullptr);

            DescriptorHeapAllocator* descriptorHeapAllocator = device->GetDescriptorHeapAllocator();

            if (mCbvSrvUavDescriptorHeapSize > 0) {
                mCbvSrvUavGPUDescriptorHeap = descriptorHeapAllocator->AllocateGPUHeap(
                    D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, mCbvSrvUavDescriptorHeapSize);
            }

            if (mSamplerDescriptorHeapSize > 0) {
                mSamplerGPUDescriptorHeap = descriptorHeapAllocator->AllocateGPUHeap(
                    D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, mSamplerDescriptorHeapSize);
            }

            uint32_t cbvSrvUavDescriptorIndex = 0;
            uint32_t samplerDescriptorIndex = 0;
            for (BindGroup* group : mBindGroupsList) {
                ASSERT(group);
                ASSERT(cbvSrvUavDescriptorIndex +
                           ToBackend(group->GetLayout())->GetCbvUavSrvDescriptorCount() <=
                       mCbvSrvUavDescriptorHeapSize);
                ASSERT(samplerDescriptorIndex +
                           ToBackend(group->GetLayout())->GetSamplerDescriptorCount() <=
                       mSamplerDescriptorHeapSize);
                group->AllocateDescriptors(mCbvSrvUavGPUDescriptorHeap, &cbvSrvUavDescriptorIndex,
                                           mSamplerGPUDescriptorHeap, &samplerDescriptorIndex);
            }

            ASSERT(cbvSrvUavDescriptorIndex == mCbvSrvUavDescriptorHeapSize);
            ASSERT(samplerDescriptorIndex == mSamplerDescriptorHeapSize);
        }

        // This function must only be called before calling AllocateDescriptorHeaps().
        void TrackSetBindGroup(BindGroup* group, uint32_t index, uint32_t indexInSubmit) {
            if (mBindGroups[index] != group) {
                mBindGroups[index] = group;

                if (!group->TestAndSetCounted(mDevice->GetPendingCommandSerial(), indexInSubmit)) {
                    const BindGroupLayout* layout = ToBackend(group->GetLayout());

                    mCbvSrvUavDescriptorHeapSize += layout->GetCbvUavSrvDescriptorCount();
                    mSamplerDescriptorHeapSize += layout->GetSamplerDescriptorCount();
                    mBindGroupsList.push_back(group);
                }
            }
        }

        // This function must only be called before calling AllocateDescriptorHeaps().
        void TrackInheritedGroups(PipelineLayout* oldLayout,
                                  PipelineLayout* newLayout,
                                  uint32_t indexInSubmit) {
            if (oldLayout == nullptr) {
                return;
            }

            uint32_t inheritUntil = oldLayout->GroupsInheritUpTo(newLayout);
            for (uint32_t i = 0; i < inheritUntil; ++i) {
                TrackSetBindGroup(mBindGroups[i], i, indexInSubmit);
            }
        }

        void SetBindGroup(ComPtr<ID3D12GraphicsCommandList> commandList,
                          PipelineLayout* pipelineLayout,
                          BindGroup* group,
                          uint32_t index,
                          bool force = false) {
            if (mBindGroups[index] != group || force) {
                mBindGroups[index] = group;

                uint32_t cbvUavSrvCount =
                    ToBackend(group->GetLayout())->GetCbvUavSrvDescriptorCount();
                uint32_t samplerCount = ToBackend(group->GetLayout())->GetSamplerDescriptorCount();

                if (cbvUavSrvCount > 0) {
                    uint32_t parameterIndex = pipelineLayout->GetCbvUavSrvRootParameterIndex(index);

                    if (mInCompute) {
                        commandList->SetComputeRootDescriptorTable(
                            parameterIndex, mCbvSrvUavGPUDescriptorHeap.GetGPUHandle(
                                                group->GetCbvUavSrvHeapOffset()));
                    } else {
                        commandList->SetGraphicsRootDescriptorTable(
                            parameterIndex, mCbvSrvUavGPUDescriptorHeap.GetGPUHandle(
                                                group->GetCbvUavSrvHeapOffset()));
                    }
                }

                if (samplerCount > 0) {
                    uint32_t parameterIndex = pipelineLayout->GetSamplerRootParameterIndex(index);

                    if (mInCompute) {
                        commandList->SetComputeRootDescriptorTable(
                            parameterIndex,
                            mSamplerGPUDescriptorHeap.GetGPUHandle(group->GetSamplerHeapOffset()));
                    } else {
                        commandList->SetGraphicsRootDescriptorTable(
                            parameterIndex,
                            mSamplerGPUDescriptorHeap.GetGPUHandle(group->GetSamplerHeapOffset()));
                    }
                }
            }
        }

        void SetInheritedBindGroups(ComPtr<ID3D12GraphicsCommandList> commandList,
                                    PipelineLayout* oldLayout,
                                    PipelineLayout* newLayout) {
            if (oldLayout == nullptr) {
                return;
            }

            uint32_t inheritUntil = oldLayout->GroupsInheritUpTo(newLayout);
            for (uint32_t i = 0; i < inheritUntil; ++i) {
                SetBindGroup(commandList, newLayout, mBindGroups[i], i, true);
            }
        }

        void Reset() {
            for (uint32_t i = 0; i < kMaxBindGroups; ++i) {
                mBindGroups[i] = nullptr;
            }
        }

        void SetID3D12DescriptorHeaps(ComPtr<ID3D12GraphicsCommandList> commandList) {
            ASSERT(commandList != nullptr);
            ID3D12DescriptorHeap* descriptorHeaps[2] = {mCbvSrvUavGPUDescriptorHeap.Get(),
                                                        mSamplerGPUDescriptorHeap.Get()};
            if (descriptorHeaps[0] && descriptorHeaps[1]) {
                commandList->SetDescriptorHeaps(2, descriptorHeaps);
            } else if (descriptorHeaps[0]) {
                commandList->SetDescriptorHeaps(1, descriptorHeaps);
            } else if (descriptorHeaps[1]) {
                commandList->SetDescriptorHeaps(1, &descriptorHeaps[1]);
            }
        }

      private:
        uint32_t mCbvSrvUavDescriptorHeapSize = 0;
        uint32_t mSamplerDescriptorHeapSize = 0;
        std::array<BindGroup*, kMaxBindGroups> mBindGroups = {};
        std::deque<BindGroup*> mBindGroupsList = {};
        bool mInCompute = false;

        DescriptorHeapHandle mCbvSrvUavGPUDescriptorHeap = {};
        DescriptorHeapHandle mSamplerGPUDescriptorHeap = {};

        Device* mDevice;
    };

    struct OMSetRenderTargetArgs {
        unsigned int numRTVs = 0;
        std::array<D3D12_CPU_DESCRIPTOR_HANDLE, kMaxColorAttachments> RTVs = {};
        D3D12_CPU_DESCRIPTOR_HANDLE dsv = {};
    };

    class RenderPassDescriptorHeapTracker {
      public:
        RenderPassDescriptorHeapTracker(Device* device) : mDevice(device) {
        }

        // This function must only be called before calling AllocateRTVAndDSVHeaps().
        void TrackRenderPass(const BeginRenderPassCmd* renderPass) {
            DAWN_ASSERT(mRTVHeap.Get() == nullptr && mDSVHeap.Get() == nullptr);

            mNumRTVs += static_cast<uint32_t>(renderPass->colorAttachmentsSet.count());
            if (renderPass->hasDepthStencilAttachment) {
                ++mNumDSVs;
            }
        }

        void AllocateRTVAndDSVHeaps() {
            // This function should only be called once.
            DAWN_ASSERT(mRTVHeap.Get() == nullptr && mDSVHeap.Get() == nullptr);
            DescriptorHeapAllocator* allocator = mDevice->GetDescriptorHeapAllocator();
            if (mNumRTVs > 0) {
                mRTVHeap = allocator->AllocateCPUHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, mNumRTVs);
            }
            if (mNumDSVs > 0) {
                mDSVHeap = allocator->AllocateCPUHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, mNumDSVs);
            }
        }

        // TODO(jiawei.shao@intel.com): use hash map <RenderPass, OMSetRenderTargetArgs> as cache to
        // avoid redundant RTV and DSV memory allocations.
        OMSetRenderTargetArgs GetSubpassOMSetRenderTargetArgs(BeginRenderPassCmd* renderPass) {
            OMSetRenderTargetArgs args = {};

            unsigned int rtvIndex = 0;
            uint32_t rtvCount = static_cast<uint32_t>(renderPass->colorAttachmentsSet.count());
            DAWN_ASSERT(mAllocatedRTVs + rtvCount <= mNumRTVs);
            for (uint32_t i : IterateBitSet(renderPass->colorAttachmentsSet)) {
                TextureView* view = ToBackend(renderPass->colorAttachments[i].view).Get();
                D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = mRTVHeap.GetCPUHandle(mAllocatedRTVs);
                D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = view->GetRTVDescriptor();
                mDevice->GetD3D12Device()->CreateRenderTargetView(
                    ToBackend(view->GetTexture())->GetD3D12Resource(), &rtvDesc, rtvHandle);
                args.RTVs[i] = rtvHandle;

                ++rtvIndex;
                ++mAllocatedRTVs;
            }
            args.numRTVs = rtvIndex;

            if (renderPass->hasDepthStencilAttachment) {
                DAWN_ASSERT(mAllocatedDSVs < mNumDSVs);
                TextureView* view = ToBackend(renderPass->depthStencilAttachment.view).Get();
                D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = mDSVHeap.GetCPUHandle(mAllocatedDSVs);
                D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = view->GetDSVDescriptor();
                mDevice->GetD3D12Device()->CreateDepthStencilView(
                    ToBackend(view->GetTexture())->GetD3D12Resource(), &dsvDesc, dsvHandle);
                args.dsv = dsvHandle;

                ++mAllocatedDSVs;
            }

            return args;
        }

        bool IsHeapAllocationCompleted() const {
            return mNumRTVs == mAllocatedRTVs && mNumDSVs == mAllocatedDSVs;
        }

      private:
        Device* mDevice;
        DescriptorHeapHandle mRTVHeap = {};
        DescriptorHeapHandle mDSVHeap = {};
        uint32_t mNumRTVs = 0;
        uint32_t mNumDSVs = 0;

        uint32_t mAllocatedRTVs = 0;
        uint32_t mAllocatedDSVs = 0;
    };

    namespace {

        void AllocateAndSetDescriptorHeaps(Device* device,
                                           BindGroupStateTracker* bindingTracker,
                                           RenderPassDescriptorHeapTracker* renderPassTracker,
                                           CommandIterator* commands,
                                           uint32_t indexInSubmit) {
            {
                Command type;
                PipelineLayout* lastLayout = nullptr;

                while (commands->NextCommandId(&type)) {
                    switch (type) {
                        case Command::SetComputePipeline: {
                            SetComputePipelineCmd* cmd =
                                commands->NextCommand<SetComputePipelineCmd>();
                            PipelineLayout* layout = ToBackend(cmd->pipeline->GetLayout());
                            bindingTracker->TrackInheritedGroups(lastLayout, layout, indexInSubmit);
                            lastLayout = layout;
                        } break;

                        case Command::SetRenderPipeline: {
                            SetRenderPipelineCmd* cmd =
                                commands->NextCommand<SetRenderPipelineCmd>();
                            PipelineLayout* layout = ToBackend(cmd->pipeline->GetLayout());
                            bindingTracker->TrackInheritedGroups(lastLayout, layout, indexInSubmit);
                            lastLayout = layout;
                        } break;

                        case Command::SetBindGroup: {
                            SetBindGroupCmd* cmd = commands->NextCommand<SetBindGroupCmd>();
                            BindGroup* group = ToBackend(cmd->group.Get());
                            bindingTracker->TrackSetBindGroup(group, cmd->index, indexInSubmit);
                        } break;
                        case Command::BeginRenderPass: {
                            BeginRenderPassCmd* cmd = commands->NextCommand<BeginRenderPassCmd>();
                            renderPassTracker->TrackRenderPass(cmd);
                        } break;
                        default:
                            SkipCommand(commands, type);
                    }
                }

                commands->Reset();
            }

            renderPassTracker->AllocateRTVAndDSVHeaps();
            bindingTracker->AllocateDescriptorHeaps(device);
        }

        void ResolveMultisampledRenderPass(ComPtr<ID3D12GraphicsCommandList> commandList,
                                           BeginRenderPassCmd* renderPass) {
            ASSERT(renderPass != nullptr);

            for (uint32_t i : IterateBitSet(renderPass->colorAttachmentsSet)) {
                TextureViewBase* resolveTarget =
                    renderPass->colorAttachments[i].resolveTarget.Get();
                if (resolveTarget == nullptr) {
                    continue;
                }

                Texture* colorTexture =
                    ToBackend(renderPass->colorAttachments[i].view->GetTexture());
                Texture* resolveTexture = ToBackend(resolveTarget->GetTexture());

                // Transition the usages of the color attachment and resolve target.
                colorTexture->TransitionUsageNow(commandList, D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
                resolveTexture->TransitionUsageNow(commandList, D3D12_RESOURCE_STATE_RESOLVE_DEST);

                // Do MSAA resolve with ResolveSubResource().
                ID3D12Resource* colorTextureHandle = colorTexture->GetD3D12Resource();
                ID3D12Resource* resolveTextureHandle = resolveTexture->GetD3D12Resource();
                const uint32_t resolveTextureSubresourceIndex = resolveTexture->GetSubresourceIndex(
                    resolveTarget->GetBaseMipLevel(), resolveTarget->GetBaseArrayLayer());
                constexpr uint32_t kColorTextureSubresourceIndex = 0;
                commandList->ResolveSubresource(
                    resolveTextureHandle, resolveTextureSubresourceIndex, colorTextureHandle,
                    kColorTextureSubresourceIndex, colorTexture->GetD3D12Format());
            }
        }

    }  // anonymous namespace

    CommandBuffer::CommandBuffer(Device* device, CommandEncoderBase* encoder)
        : CommandBufferBase(device, encoder), mCommands(encoder->AcquireCommands()) {
    }

    CommandBuffer::~CommandBuffer() {
        FreeCommands(&mCommands);
    }

    void CommandBuffer::RecordCommands(ComPtr<ID3D12GraphicsCommandList> commandList,
                                       uint32_t indexInSubmit) {
        Device* device = ToBackend(GetDevice());
        BindGroupStateTracker bindingTracker(device);
        RenderPassDescriptorHeapTracker renderPassTracker(device);

        // Precompute the allocation of bindgroups in descriptor heaps
        // TODO(cwallez@chromium.org): Iterating over all the commands here is inefficient. We
        // should have a system where commands and descriptors are recorded in parallel then the
        // heaps set using a small CommandList inserted just before the main CommandList.
        {
            AllocateAndSetDescriptorHeaps(device, &bindingTracker, &renderPassTracker, &mCommands,
                                          indexInSubmit);
            bindingTracker.Reset();
            bindingTracker.SetID3D12DescriptorHeaps(commandList);
        }

        // Records the necessary barriers for the resource usage pre-computed by the frontend
        auto TransitionForPass = [](ComPtr<ID3D12GraphicsCommandList> commandList,
                                    const PassResourceUsage& usages) {
            std::vector<D3D12_RESOURCE_BARRIER> barriers;

            for (size_t i = 0; i < usages.buffers.size(); ++i) {
                D3D12_RESOURCE_BARRIER barrier;
                if (ToBackend(usages.buffers[i])
                        ->CreateD3D12ResourceBarrierIfNeeded(&barrier, usages.bufferUsages[i])) {
                    barriers.push_back(barrier);
                    ToBackend(usages.buffers[i])->SetUsage(usages.bufferUsages[i]);
                }
            }

            for (size_t i = 0; i < usages.textures.size(); ++i) {
                D3D12_RESOURCE_BARRIER barrier;
                if (ToBackend(usages.textures[i])
                        ->CreateD3D12ResourceBarrierIfNeeded(&barrier, usages.textureUsages[i])) {
                    barriers.push_back(barrier);
                    ToBackend(usages.textures[i])->SetUsage(usages.textureUsages[i]);
                }
            }

            if (barriers.size()) {
                commandList->ResourceBarrier(barriers.size(), barriers.data());
            }
        };

        const std::vector<PassResourceUsage>& passResourceUsages = GetResourceUsages().perPass;
        uint32_t nextPassNumber = 0;

        Command type;
        while (mCommands.NextCommandId(&type)) {
            switch (type) {
                case Command::BeginComputePass: {
                    mCommands.NextCommand<BeginComputePassCmd>();

                    TransitionForPass(commandList, passResourceUsages[nextPassNumber]);
                    bindingTracker.SetInComputePass(true);
                    RecordComputePass(commandList, &bindingTracker);

                    nextPassNumber++;
                } break;

                case Command::BeginRenderPass: {
                    BeginRenderPassCmd* beginRenderPassCmd =
                        mCommands.NextCommand<BeginRenderPassCmd>();

                    TransitionForPass(commandList, passResourceUsages[nextPassNumber]);
                    bindingTracker.SetInComputePass(false);
                    RecordRenderPass(commandList, &bindingTracker, &renderPassTracker,
                                     beginRenderPassCmd);

                    nextPassNumber++;
                } break;

                case Command::CopyBufferToBuffer: {
                    CopyBufferToBufferCmd* copy = mCommands.NextCommand<CopyBufferToBufferCmd>();
                    Buffer* srcBuffer = ToBackend(copy->source.Get());
                    Buffer* dstBuffer = ToBackend(copy->destination.Get());

                    srcBuffer->TransitionUsageNow(commandList, dawn::BufferUsageBit::TransferSrc);
                    dstBuffer->TransitionUsageNow(commandList, dawn::BufferUsageBit::TransferDst);

                    commandList->CopyBufferRegion(
                        dstBuffer->GetD3D12Resource().Get(), copy->destinationOffset,
                        srcBuffer->GetD3D12Resource().Get(), copy->sourceOffset, copy->size);
                } break;

                case Command::CopyBufferToTexture: {
                    CopyBufferToTextureCmd* copy = mCommands.NextCommand<CopyBufferToTextureCmd>();
                    Buffer* buffer = ToBackend(copy->source.buffer.Get());
                    Texture* texture = ToBackend(copy->destination.texture.Get());

                    buffer->TransitionUsageNow(commandList, dawn::BufferUsageBit::TransferSrc);
                    texture->TransitionUsageNow(commandList, dawn::TextureUsageBit::TransferDst);

                    auto copySplit = ComputeTextureCopySplit(
                        copy->destination.origin, copy->copySize,
                        static_cast<uint32_t>(TextureFormatPixelSize(texture->GetFormat())),
                        copy->source.offset, copy->source.rowPitch, copy->source.imageHeight);

                    D3D12_TEXTURE_COPY_LOCATION textureLocation =
                        CreateTextureCopyLocationForTexture(*texture, copy->destination.level,
                                                            copy->destination.slice);

                    for (uint32_t i = 0; i < copySplit.count; ++i) {
                        auto& info = copySplit.copies[i];

                        D3D12_TEXTURE_COPY_LOCATION bufferLocation;
                        bufferLocation.pResource = buffer->GetD3D12Resource().Get();
                        bufferLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
                        bufferLocation.PlacedFootprint.Offset = copySplit.offset;
                        bufferLocation.PlacedFootprint.Footprint.Format = texture->GetD3D12Format();
                        bufferLocation.PlacedFootprint.Footprint.Width = info.bufferSize.width;
                        bufferLocation.PlacedFootprint.Footprint.Height = info.bufferSize.height;
                        bufferLocation.PlacedFootprint.Footprint.Depth = info.bufferSize.depth;
                        bufferLocation.PlacedFootprint.Footprint.RowPitch = copy->source.rowPitch;

                        D3D12_BOX sourceRegion;
                        sourceRegion.left = info.bufferOffset.x;
                        sourceRegion.top = info.bufferOffset.y;
                        sourceRegion.front = info.bufferOffset.z;
                        sourceRegion.right = info.bufferOffset.x + info.copySize.width;
                        sourceRegion.bottom = info.bufferOffset.y + info.copySize.height;
                        sourceRegion.back = info.bufferOffset.z + info.copySize.depth;

                        commandList->CopyTextureRegion(&textureLocation, info.textureOffset.x,
                                                       info.textureOffset.y, info.textureOffset.z,
                                                       &bufferLocation, &sourceRegion);
                    }
                } break;

                case Command::CopyTextureToBuffer: {
                    CopyTextureToBufferCmd* copy = mCommands.NextCommand<CopyTextureToBufferCmd>();
                    Texture* texture = ToBackend(copy->source.texture.Get());
                    Buffer* buffer = ToBackend(copy->destination.buffer.Get());

                    texture->TransitionUsageNow(commandList, dawn::TextureUsageBit::TransferSrc);
                    buffer->TransitionUsageNow(commandList, dawn::BufferUsageBit::TransferDst);

                    auto copySplit = ComputeTextureCopySplit(
                        copy->source.origin, copy->copySize,
                        static_cast<uint32_t>(TextureFormatPixelSize(texture->GetFormat())),
                        copy->destination.offset, copy->destination.rowPitch,
                        copy->destination.imageHeight);

                    D3D12_TEXTURE_COPY_LOCATION textureLocation =
                        CreateTextureCopyLocationForTexture(*texture, copy->source.level,
                                                            copy->source.slice);

                    for (uint32_t i = 0; i < copySplit.count; ++i) {
                        auto& info = copySplit.copies[i];

                        D3D12_TEXTURE_COPY_LOCATION bufferLocation;
                        bufferLocation.pResource = buffer->GetD3D12Resource().Get();
                        bufferLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
                        bufferLocation.PlacedFootprint.Offset = copySplit.offset;
                        bufferLocation.PlacedFootprint.Footprint.Format = texture->GetD3D12Format();
                        bufferLocation.PlacedFootprint.Footprint.Width = info.bufferSize.width;
                        bufferLocation.PlacedFootprint.Footprint.Height = info.bufferSize.height;
                        bufferLocation.PlacedFootprint.Footprint.Depth = info.bufferSize.depth;
                        bufferLocation.PlacedFootprint.Footprint.RowPitch =
                            copy->destination.rowPitch;

                        D3D12_BOX sourceRegion;
                        sourceRegion.left = info.textureOffset.x;
                        sourceRegion.top = info.textureOffset.y;
                        sourceRegion.front = info.textureOffset.z;
                        sourceRegion.right = info.textureOffset.x + info.copySize.width;
                        sourceRegion.bottom = info.textureOffset.y + info.copySize.height;
                        sourceRegion.back = info.textureOffset.z + info.copySize.depth;

                        commandList->CopyTextureRegion(&bufferLocation, info.bufferOffset.x,
                                                       info.bufferOffset.y, info.bufferOffset.z,
                                                       &textureLocation, &sourceRegion);
                    }
                } break;

                case Command::CopyTextureToTexture: {
                    CopyTextureToTextureCmd* copy =
                        mCommands.NextCommand<CopyTextureToTextureCmd>();

                    Texture* source = ToBackend(copy->source.texture.Get());
                    Texture* destination = ToBackend(copy->destination.texture.Get());

                    source->TransitionUsageNow(commandList, dawn::TextureUsageBit::TransferSrc);
                    destination->TransitionUsageNow(commandList,
                                                    dawn::TextureUsageBit::TransferDst);

                    if (CanUseCopyResource(source->GetNumMipLevels(), source->GetSize(),
                                           destination->GetSize(), copy->copySize)) {
                        commandList->CopyResource(destination->GetD3D12Resource(),
                                                  source->GetD3D12Resource());

                    } else {
                        D3D12_TEXTURE_COPY_LOCATION srcLocation =
                            CreateTextureCopyLocationForTexture(*source, copy->source.level,
                                                                copy->source.slice);

                        D3D12_TEXTURE_COPY_LOCATION dstLocation =
                            CreateTextureCopyLocationForTexture(
                                *destination, copy->destination.level, copy->destination.slice);

                        D3D12_BOX sourceRegion;
                        sourceRegion.left = copy->source.origin.x;
                        sourceRegion.top = copy->source.origin.y;
                        sourceRegion.front = copy->source.origin.z;
                        sourceRegion.right = copy->source.origin.x + copy->copySize.width;
                        sourceRegion.bottom = copy->source.origin.y + copy->copySize.height;
                        sourceRegion.back = copy->source.origin.z + copy->copySize.depth;

                        commandList->CopyTextureRegion(
                            &dstLocation, copy->destination.origin.x, copy->destination.origin.y,
                            copy->destination.origin.z, &srcLocation, &sourceRegion);
                    }
                } break;

                default: { UNREACHABLE(); } break;
            }
        }

        DAWN_ASSERT(renderPassTracker.IsHeapAllocationCompleted());
    }

    void CommandBuffer::FlushSetVertexBuffers(ComPtr<ID3D12GraphicsCommandList> commandList,
                                              VertexBuffersInfo* vertexBuffersInfo,
                                              const RenderPipeline* renderPipeline) {
        DAWN_ASSERT(vertexBuffersInfo != nullptr);
        DAWN_ASSERT(renderPipeline != nullptr);

        auto inputsMask = renderPipeline->GetInputsSetMask();

        uint32_t startSlot = vertexBuffersInfo->startSlot;
        uint32_t endSlot = vertexBuffersInfo->endSlot;

        // If the input state has changed, we need to update the StrideInBytes
        // for the D3D12 buffer views. We also need to extend the dirty range to
        // touch all these slots because the stride may have changed.
        if (vertexBuffersInfo->lastRenderPipeline != renderPipeline) {
            vertexBuffersInfo->lastRenderPipeline = renderPipeline;

            for (uint32_t slot : IterateBitSet(inputsMask)) {
                startSlot = std::min(startSlot, slot);
                endSlot = std::max(endSlot, slot + 1);
                vertexBuffersInfo->d3d12BufferViews[slot].StrideInBytes =
                    renderPipeline->GetInput(slot).stride;
            }
        }

        if (endSlot <= startSlot) {
            return;
        }

        // d3d12BufferViews is kept up to date with the most recent data passed
        // to SetVertexBuffers. This makes it correct to only track the start
        // and end of the dirty range. When FlushSetVertexBuffers is called,
        // we will at worst set non-dirty vertex buffers in duplicate.
        uint32_t count = endSlot - startSlot;
        commandList->IASetVertexBuffers(startSlot, count,
                                        &vertexBuffersInfo->d3d12BufferViews[startSlot]);

        vertexBuffersInfo->startSlot = kMaxVertexBuffers;
        vertexBuffersInfo->endSlot = 0;
    }

    void CommandBuffer::RecordComputePass(ComPtr<ID3D12GraphicsCommandList> commandList,
                                          BindGroupStateTracker* bindingTracker) {
        PipelineLayout* lastLayout = nullptr;

        Command type;
        while (mCommands.NextCommandId(&type)) {
            switch (type) {
                case Command::Dispatch: {
                    DispatchCmd* dispatch = mCommands.NextCommand<DispatchCmd>();
                    commandList->Dispatch(dispatch->x, dispatch->y, dispatch->z);
                } break;

                case Command::DispatchIndirect: {
                    DispatchIndirectCmd* dispatch = mCommands.NextCommand<DispatchIndirectCmd>();

                    Buffer* buffer = ToBackend(dispatch->indirectBuffer.Get());
                    ComPtr<ID3D12CommandSignature> signature =
                        ToBackend(GetDevice())->GetDispatchIndirectSignature();
                    commandList->ExecuteIndirect(signature.Get(), 1,
                                                 buffer->GetD3D12Resource().Get(),
                                                 dispatch->indirectOffset, nullptr, 0);
                } break;

                case Command::EndComputePass: {
                    mCommands.NextCommand<EndComputePassCmd>();
                    return;
                } break;

                case Command::SetComputePipeline: {
                    SetComputePipelineCmd* cmd = mCommands.NextCommand<SetComputePipelineCmd>();
                    ComputePipeline* pipeline = ToBackend(cmd->pipeline).Get();
                    PipelineLayout* layout = ToBackend(pipeline->GetLayout());

                    commandList->SetComputeRootSignature(layout->GetRootSignature().Get());
                    commandList->SetPipelineState(pipeline->GetPipelineState().Get());

                    bindingTracker->SetInheritedBindGroups(commandList, lastLayout, layout);
                    lastLayout = layout;
                } break;

                case Command::SetBindGroup: {
                    SetBindGroupCmd* cmd = mCommands.NextCommand<SetBindGroupCmd>();
                    BindGroup* group = ToBackend(cmd->group.Get());
                    bindingTracker->SetBindGroup(commandList, lastLayout, group, cmd->index);
                } break;

                default: { UNREACHABLE(); } break;
            }
        }
    }

    void CommandBuffer::RecordRenderPass(ComPtr<ID3D12GraphicsCommandList> commandList,
                                         BindGroupStateTracker* bindingTracker,
                                         RenderPassDescriptorHeapTracker* renderPassTracker,
                                         BeginRenderPassCmd* renderPass) {
        OMSetRenderTargetArgs args = renderPassTracker->GetSubpassOMSetRenderTargetArgs(renderPass);

        // Clear framebuffer attachments as needed and transition to render target
        {
            for (uint32_t i : IterateBitSet(renderPass->colorAttachmentsSet)) {
                auto& attachmentInfo = renderPass->colorAttachments[i];

                // Load op - color
                if (attachmentInfo.loadOp == dawn::LoadOp::Clear) {
                    D3D12_CPU_DESCRIPTOR_HANDLE handle = args.RTVs[i];
                    commandList->ClearRenderTargetView(handle, &attachmentInfo.clearColor.r, 0,
                                                       nullptr);
                }
            }

            if (renderPass->hasDepthStencilAttachment) {
                auto& attachmentInfo = renderPass->depthStencilAttachment;
                Texture* texture = ToBackend(renderPass->depthStencilAttachment.view->GetTexture());

                // Load op - depth/stencil
                bool doDepthClear = TextureFormatHasDepth(texture->GetFormat()) &&
                                    (attachmentInfo.depthLoadOp == dawn::LoadOp::Clear);
                bool doStencilClear = TextureFormatHasStencil(texture->GetFormat()) &&
                                      (attachmentInfo.stencilLoadOp == dawn::LoadOp::Clear);

                D3D12_CLEAR_FLAGS clearFlags = {};
                if (doDepthClear) {
                    clearFlags |= D3D12_CLEAR_FLAG_DEPTH;
                }
                if (doStencilClear) {
                    clearFlags |= D3D12_CLEAR_FLAG_STENCIL;
                }

                if (clearFlags) {
                    D3D12_CPU_DESCRIPTOR_HANDLE handle = args.dsv;
                    // TODO(kainino@chromium.org): investigate: should the Dawn clear
                    // stencil type be uint8_t?
                    uint8_t clearStencil = static_cast<uint8_t>(attachmentInfo.clearStencil);
                    commandList->ClearDepthStencilView(
                        handle, clearFlags, attachmentInfo.clearDepth, clearStencil, 0, nullptr);
                }
            }
        }

        // Set up render targets
        {
            if (args.dsv.ptr) {
                commandList->OMSetRenderTargets(args.numRTVs, args.RTVs.data(), FALSE, &args.dsv);
            } else {
                commandList->OMSetRenderTargets(args.numRTVs, args.RTVs.data(), FALSE, nullptr);
            }
        }

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
        VertexBuffersInfo vertexBuffersInfo = {};

        Command type;
        while (mCommands.NextCommandId(&type)) {
            switch (type) {
                case Command::EndRenderPass: {
                    mCommands.NextCommand<EndRenderPassCmd>();

                    // TODO(brandon1.jones@intel.com): avoid calling this function and enable MSAA
                    // resolve in D3D12 render pass on the platforms that support this feature.
                    if (renderPass->sampleCount > 1) {
                        ResolveMultisampledRenderPass(commandList, renderPass);
                    }
                    return;
                } break;

                case Command::Draw: {
                    DrawCmd* draw = mCommands.NextCommand<DrawCmd>();

                    FlushSetVertexBuffers(commandList, &vertexBuffersInfo, lastPipeline);
                    commandList->DrawInstanced(draw->vertexCount, draw->instanceCount,
                                               draw->firstVertex, draw->firstInstance);
                } break;

                case Command::DrawIndexed: {
                    DrawIndexedCmd* draw = mCommands.NextCommand<DrawIndexedCmd>();

                    FlushSetVertexBuffers(commandList, &vertexBuffersInfo, lastPipeline);
                    commandList->DrawIndexedInstanced(draw->indexCount, draw->instanceCount,
                                                      draw->firstIndex, draw->baseVertex,
                                                      draw->firstInstance);
                } break;

                case Command::DrawIndirect: {
                    DrawIndirectCmd* draw = mCommands.NextCommand<DrawIndirectCmd>();

                    FlushSetVertexBuffers(commandList, &vertexBuffersInfo, lastPipeline);
                    Buffer* buffer = ToBackend(draw->indirectBuffer.Get());
                    ComPtr<ID3D12CommandSignature> signature =
                        ToBackend(GetDevice())->GetDrawIndirectSignature();
                    commandList->ExecuteIndirect(signature.Get(), 1,
                                                 buffer->GetD3D12Resource().Get(),
                                                 draw->indirectOffset, nullptr, 0);
                } break;

                case Command::DrawIndexedIndirect: {
                    DrawIndexedIndirectCmd* draw = mCommands.NextCommand<DrawIndexedIndirectCmd>();

                    FlushSetVertexBuffers(commandList, &vertexBuffersInfo, lastPipeline);
                    Buffer* buffer = ToBackend(draw->indirectBuffer.Get());
                    ComPtr<ID3D12CommandSignature> signature =
                        ToBackend(GetDevice())->GetDrawIndexedIndirectSignature();
                    commandList->ExecuteIndirect(signature.Get(), 1,
                                                 buffer->GetD3D12Resource().Get(),
                                                 draw->indirectOffset, nullptr, 0);
                } break;

                case Command::InsertDebugMarker: {
                    InsertDebugMarkerCmd* cmd = mCommands.NextCommand<InsertDebugMarkerCmd>();
                    const char* label = mCommands.NextData<char>(cmd->length + 1);

                    if (ToBackend(GetDevice())->GetFunctions()->isPIXEventRuntimeLoaded()) {
                        // PIX color is 1 byte per channel in ARGB format
                        constexpr uint64_t kPIXBlackColor = 0xff000000;
                        ToBackend(GetDevice())
                            ->GetFunctions()
                            ->pixSetMarkerOnCommandList(commandList.Get(), kPIXBlackColor, label);
                    }
                } break;

                case Command::PopDebugGroup: {
                    mCommands.NextCommand<PopDebugGroupCmd>();

                    if (ToBackend(GetDevice())->GetFunctions()->isPIXEventRuntimeLoaded()) {
                        ToBackend(GetDevice())
                            ->GetFunctions()
                            ->pixEndEventOnCommandList(commandList.Get());
                    }
                } break;

                case Command::PushDebugGroup: {
                    PushDebugGroupCmd* cmd = mCommands.NextCommand<PushDebugGroupCmd>();
                    const char* label = mCommands.NextData<char>(cmd->length + 1);

                    if (ToBackend(GetDevice())->GetFunctions()->isPIXEventRuntimeLoaded()) {
                        // PIX color is 1 byte per channel in ARGB format
                        constexpr uint64_t kPIXBlackColor = 0xff000000;
                        ToBackend(GetDevice())
                            ->GetFunctions()
                            ->pixBeginEventOnCommandList(commandList.Get(), kPIXBlackColor, label);
                    }
                } break;

                case Command::SetRenderPipeline: {
                    SetRenderPipelineCmd* cmd = mCommands.NextCommand<SetRenderPipelineCmd>();
                    RenderPipeline* pipeline = ToBackend(cmd->pipeline).Get();
                    PipelineLayout* layout = ToBackend(pipeline->GetLayout());

                    commandList->SetGraphicsRootSignature(layout->GetRootSignature().Get());
                    commandList->SetPipelineState(pipeline->GetPipelineState().Get());
                    commandList->IASetPrimitiveTopology(pipeline->GetD3D12PrimitiveTopology());

                    bindingTracker->SetInheritedBindGroups(commandList, lastLayout, layout);

                    lastPipeline = pipeline;
                    lastLayout = layout;
                } break;

                case Command::SetStencilReference: {
                    SetStencilReferenceCmd* cmd = mCommands.NextCommand<SetStencilReferenceCmd>();

                    commandList->OMSetStencilRef(cmd->reference);
                } break;

                case Command::SetScissorRect: {
                    SetScissorRectCmd* cmd = mCommands.NextCommand<SetScissorRectCmd>();
                    D3D12_RECT rect;
                    rect.left = cmd->x;
                    rect.top = cmd->y;
                    rect.right = cmd->x + cmd->width;
                    rect.bottom = cmd->y + cmd->height;

                    commandList->RSSetScissorRects(1, &rect);
                } break;

                case Command::SetBlendColor: {
                    SetBlendColorCmd* cmd = mCommands.NextCommand<SetBlendColorCmd>();
                    commandList->OMSetBlendFactor(static_cast<const FLOAT*>(&cmd->color.r));
                } break;

                case Command::SetBindGroup: {
                    SetBindGroupCmd* cmd = mCommands.NextCommand<SetBindGroupCmd>();
                    BindGroup* group = ToBackend(cmd->group.Get());
                    bindingTracker->SetBindGroup(commandList, lastLayout, group, cmd->index);
                } break;

                case Command::SetIndexBuffer: {
                    SetIndexBufferCmd* cmd = mCommands.NextCommand<SetIndexBufferCmd>();

                    Buffer* buffer = ToBackend(cmd->buffer.Get());
                    D3D12_INDEX_BUFFER_VIEW bufferView;
                    bufferView.BufferLocation = buffer->GetVA() + cmd->offset;
                    bufferView.SizeInBytes = buffer->GetSize() - cmd->offset;
                    // TODO(cwallez@chromium.org): Make index buffers lazily applied, right now
                    // this will break if the pipeline is changed for one with a different index
                    // format after SetIndexBuffer
                    bufferView.Format =
                        DXGIIndexFormat(lastPipeline->GetVertexInputDescriptor()->indexFormat);

                    commandList->IASetIndexBuffer(&bufferView);
                } break;

                case Command::SetVertexBuffers: {
                    SetVertexBuffersCmd* cmd = mCommands.NextCommand<SetVertexBuffersCmd>();
                    auto buffers = mCommands.NextData<Ref<BufferBase>>(cmd->count);
                    auto offsets = mCommands.NextData<uint64_t>(cmd->count);

                    vertexBuffersInfo.startSlot =
                        std::min(vertexBuffersInfo.startSlot, cmd->startSlot);
                    vertexBuffersInfo.endSlot =
                        std::max(vertexBuffersInfo.endSlot, cmd->startSlot + cmd->count);

                    for (uint32_t i = 0; i < cmd->count; ++i) {
                        Buffer* buffer = ToBackend(buffers[i].Get());
                        auto* d3d12BufferView =
                            &vertexBuffersInfo.d3d12BufferViews[cmd->startSlot + i];
                        d3d12BufferView->BufferLocation = buffer->GetVA() + offsets[i];
                        d3d12BufferView->SizeInBytes = buffer->GetSize() - offsets[i];
                        // The bufferView stride is set based on the input state before a draw.
                    }
                } break;

                default: { UNREACHABLE(); } break;
            }
        }
    }

}}  // namespace dawn_native::d3d12
