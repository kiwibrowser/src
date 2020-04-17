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

#ifndef DAWNNATIVE_D3D12_DEVICED3D12_H_
#define DAWNNATIVE_D3D12_DEVICED3D12_H_

#include "dawn_native/dawn_platform.h"

#include "common/SerialQueue.h"
#include "dawn_native/Device.h"
#include "dawn_native/d3d12/Forward.h"
#include "dawn_native/d3d12/d3d12_platform.h"

#include <memory>

namespace dawn_native { namespace d3d12 {

    class CommandAllocatorManager;
    class DescriptorHeapAllocator;
    class MapRequestTracker;
    class PlatformFunctions;
    class ResourceAllocator;

    void ASSERT_SUCCESS(HRESULT hr);

    // Definition of backend types
    class Device : public DeviceBase {
      public:
        Device(Adapter* adapter, const DeviceDescriptor* descriptor);
        ~Device();

        MaybeError Initialize();

        CommandBufferBase* CreateCommandBuffer(CommandEncoderBase* encoder) override;

        Serial GetCompletedCommandSerial() const final override;
        Serial GetLastSubmittedCommandSerial() const final override;
        void TickImpl() override;

        ComPtr<ID3D12Device> GetD3D12Device() const;
        ComPtr<ID3D12CommandQueue> GetCommandQueue() const;

        ComPtr<ID3D12CommandSignature> GetDispatchIndirectSignature() const;
        ComPtr<ID3D12CommandSignature> GetDrawIndirectSignature() const;
        ComPtr<ID3D12CommandSignature> GetDrawIndexedIndirectSignature() const;

        DescriptorHeapAllocator* GetDescriptorHeapAllocator() const;
        MapRequestTracker* GetMapRequestTracker() const;
        ResourceAllocator* GetResourceAllocator() const;

        const PlatformFunctions* GetFunctions() const;
        ComPtr<IDXGIFactory4> GetFactory() const;

        void OpenCommandList(ComPtr<ID3D12GraphicsCommandList>* commandList);
        ComPtr<ID3D12GraphicsCommandList> GetPendingCommandList();
        Serial GetPendingCommandSerial() const override;

        void NextSerial();
        void WaitForSerial(Serial serial);

        void ReferenceUntilUnused(ComPtr<IUnknown> object);

        void ExecuteCommandLists(std::initializer_list<ID3D12CommandList*> commandLists);

        ResultOrError<std::unique_ptr<StagingBufferBase>> CreateStagingBuffer(size_t size) override;
        MaybeError CopyFromStagingToBuffer(StagingBufferBase* source,
                                           uint64_t sourceOffset,
                                           BufferBase* destination,
                                           uint64_t destinationOffset,
                                           uint64_t size) override;

      private:
        ResultOrError<BindGroupBase*> CreateBindGroupImpl(
            const BindGroupDescriptor* descriptor) override;
        ResultOrError<BindGroupLayoutBase*> CreateBindGroupLayoutImpl(
            const BindGroupLayoutDescriptor* descriptor) override;
        ResultOrError<BufferBase*> CreateBufferImpl(const BufferDescriptor* descriptor) override;
        ResultOrError<ComputePipelineBase*> CreateComputePipelineImpl(
            const ComputePipelineDescriptor* descriptor) override;
        ResultOrError<PipelineLayoutBase*> CreatePipelineLayoutImpl(
            const PipelineLayoutDescriptor* descriptor) override;
        ResultOrError<QueueBase*> CreateQueueImpl() override;
        ResultOrError<RenderPipelineBase*> CreateRenderPipelineImpl(
            const RenderPipelineDescriptor* descriptor) override;
        ResultOrError<SamplerBase*> CreateSamplerImpl(const SamplerDescriptor* descriptor) override;
        ResultOrError<ShaderModuleBase*> CreateShaderModuleImpl(
            const ShaderModuleDescriptor* descriptor) override;
        ResultOrError<SwapChainBase*> CreateSwapChainImpl(
            const SwapChainDescriptor* descriptor) override;
        ResultOrError<TextureBase*> CreateTextureImpl(const TextureDescriptor* descriptor) override;
        ResultOrError<TextureViewBase*> CreateTextureViewImpl(
            TextureBase* texture,
            const TextureViewDescriptor* descriptor) override;

        Serial mCompletedSerial = 0;
        Serial mLastSubmittedSerial = 0;
        ComPtr<ID3D12Fence> mFence;
        HANDLE mFenceEvent;

        ComPtr<ID3D12Device> mD3d12Device;  // Device is owned by adapter and will not be outlived.
        ComPtr<ID3D12CommandQueue> mCommandQueue;

        ComPtr<ID3D12CommandSignature> mDispatchIndirectSignature;
        ComPtr<ID3D12CommandSignature> mDrawIndirectSignature;
        ComPtr<ID3D12CommandSignature> mDrawIndexedIndirectSignature;

        struct PendingCommandList {
            ComPtr<ID3D12GraphicsCommandList> commandList;
            bool open = false;
        } mPendingCommands;

        SerialQueue<ComPtr<IUnknown>> mUsedComObjectRefs;

        std::unique_ptr<CommandAllocatorManager> mCommandAllocatorManager;
        std::unique_ptr<DescriptorHeapAllocator> mDescriptorHeapAllocator;
        std::unique_ptr<MapRequestTracker> mMapRequestTracker;
        std::unique_ptr<ResourceAllocator> mResourceAllocator;

        dawn_native::PCIInfo mPCIInfo;
    };

}}  // namespace dawn_native::d3d12

#endif  // DAWNNATIVE_D3D12_DEVICED3D12_H_
