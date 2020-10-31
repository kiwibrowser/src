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

#include "common/Constants.h"
#include "common/SerialQueue.h"
#include "dawn_native/BindingInfo.h"
#include "dawn_native/Device.h"
#include "dawn_native/d3d12/CommandRecordingContext.h"
#include "dawn_native/d3d12/D3D12Info.h"
#include "dawn_native/d3d12/Forward.h"
#include "dawn_native/d3d12/ResourceHeapAllocationD3D12.h"

#include <memory>

namespace dawn_native { namespace d3d12 {

    class CommandAllocatorManager;
    class DescriptorHeapAllocator;
    class PlatformFunctions;
    class ResidencyManager;
    class ResourceAllocatorManager;
    class SamplerHeapCache;
    class ShaderVisibleDescriptorAllocator;
    class StagingDescriptorAllocator;

#define ASSERT_SUCCESS(hr)            \
    do {                              \
        HRESULT succeeded = hr;       \
        ASSERT(SUCCEEDED(succeeded)); \
    } while (0)

    // Definition of backend types
    class Device : public DeviceBase {
      public:
        static ResultOrError<Device*> Create(Adapter* adapter, const DeviceDescriptor* descriptor);
        ~Device() override;

        MaybeError Initialize();

        CommandBufferBase* CreateCommandBuffer(CommandEncoder* encoder,
                                               const CommandBufferDescriptor* descriptor) override;

        MaybeError TickImpl() override;

        ID3D12Device* GetD3D12Device() const;
        ComPtr<ID3D12CommandQueue> GetCommandQueue() const;
        ID3D12SharingContract* GetSharingContract() const;

        ComPtr<ID3D12CommandSignature> GetDispatchIndirectSignature() const;
        ComPtr<ID3D12CommandSignature> GetDrawIndirectSignature() const;
        ComPtr<ID3D12CommandSignature> GetDrawIndexedIndirectSignature() const;

        CommandAllocatorManager* GetCommandAllocatorManager() const;
        ResidencyManager* GetResidencyManager() const;

        const PlatformFunctions* GetFunctions() const;
        ComPtr<IDXGIFactory4> GetFactory() const;
        ResultOrError<IDxcLibrary*> GetOrCreateDxcLibrary() const;
        ResultOrError<IDxcCompiler*> GetOrCreateDxcCompiler() const;

        ResultOrError<CommandRecordingContext*> GetPendingCommandContext();

        const D3D12DeviceInfo& GetDeviceInfo() const;

        MaybeError NextSerial();
        MaybeError WaitForSerial(Serial serial);

        void ReferenceUntilUnused(ComPtr<IUnknown> object);

        MaybeError ExecutePendingCommandContext();

        ResultOrError<std::unique_ptr<StagingBufferBase>> CreateStagingBuffer(size_t size) override;
        MaybeError CopyFromStagingToBuffer(StagingBufferBase* source,
                                           uint64_t sourceOffset,
                                           BufferBase* destination,
                                           uint64_t destinationOffset,
                                           uint64_t size) override;

        void CopyFromStagingToBufferImpl(CommandRecordingContext* commandContext,
                                         StagingBufferBase* source,
                                         uint64_t sourceOffset,
                                         BufferBase* destination,
                                         uint64_t destinationOffset,
                                         uint64_t size);

        ResultOrError<ResourceHeapAllocation> AllocateMemory(
            D3D12_HEAP_TYPE heapType,
            const D3D12_RESOURCE_DESC& resourceDescriptor,
            D3D12_RESOURCE_STATES initialUsage);

        void DeallocateMemory(ResourceHeapAllocation& allocation);

        ShaderVisibleDescriptorAllocator* GetViewShaderVisibleDescriptorAllocator() const;
        ShaderVisibleDescriptorAllocator* GetSamplerShaderVisibleDescriptorAllocator() const;

        // Returns nullptr when descriptor count is zero.
        StagingDescriptorAllocator* GetViewStagingDescriptorAllocator(
            uint32_t descriptorCount) const;

        StagingDescriptorAllocator* GetSamplerStagingDescriptorAllocator(
            uint32_t descriptorCount) const;

        SamplerHeapCache* GetSamplerHeapCache();

        StagingDescriptorAllocator* GetRenderTargetViewAllocator() const;

        StagingDescriptorAllocator* GetDepthStencilViewAllocator() const;

        Ref<TextureBase> WrapSharedHandle(const ExternalImageDescriptor* descriptor,
                                          HANDLE sharedHandle,
                                          uint64_t acquireMutexKey,
                                          bool isSwapChainTexture);
        ResultOrError<ComPtr<IDXGIKeyedMutex>> CreateKeyedMutexForTexture(
            ID3D12Resource* d3d12Resource);
        void ReleaseKeyedMutexForTexture(ComPtr<IDXGIKeyedMutex> dxgiKeyedMutex);

        void InitTogglesFromDriver();

      private:
        using DeviceBase::DeviceBase;

        ResultOrError<BindGroupBase*> CreateBindGroupImpl(
            const BindGroupDescriptor* descriptor) override;
        ResultOrError<BindGroupLayoutBase*> CreateBindGroupLayoutImpl(
            const BindGroupLayoutDescriptor* descriptor) override;
        ResultOrError<Ref<BufferBase>> CreateBufferImpl(
            const BufferDescriptor* descriptor) override;
        ResultOrError<ComputePipelineBase*> CreateComputePipelineImpl(
            const ComputePipelineDescriptor* descriptor) override;
        ResultOrError<PipelineLayoutBase*> CreatePipelineLayoutImpl(
            const PipelineLayoutDescriptor* descriptor) override;
        ResultOrError<QuerySetBase*> CreateQuerySetImpl(
            const QuerySetDescriptor* descriptor) override;
        ResultOrError<RenderPipelineBase*> CreateRenderPipelineImpl(
            const RenderPipelineDescriptor* descriptor) override;
        ResultOrError<SamplerBase*> CreateSamplerImpl(const SamplerDescriptor* descriptor) override;
        ResultOrError<ShaderModuleBase*> CreateShaderModuleImpl(
            const ShaderModuleDescriptor* descriptor) override;
        ResultOrError<SwapChainBase*> CreateSwapChainImpl(
            const SwapChainDescriptor* descriptor) override;
        ResultOrError<NewSwapChainBase*> CreateSwapChainImpl(
            Surface* surface,
            NewSwapChainBase* previousSwapChain,
            const SwapChainDescriptor* descriptor) override;
        ResultOrError<Ref<TextureBase>> CreateTextureImpl(
            const TextureDescriptor* descriptor) override;
        ResultOrError<TextureViewBase*> CreateTextureViewImpl(
            TextureBase* texture,
            const TextureViewDescriptor* descriptor) override;

        void ShutDownImpl() override;
        MaybeError WaitForIdleForDestruction() override;

        MaybeError CheckDebugLayerAndGenerateErrors();

        ComPtr<ID3D12Fence> mFence;
        HANDLE mFenceEvent = nullptr;
        Serial CheckAndUpdateCompletedSerials() override;

        ComPtr<ID3D12Device> mD3d12Device;  // Device is owned by adapter and will not be outlived.
        ComPtr<ID3D12CommandQueue> mCommandQueue;
        ComPtr<ID3D12SharingContract> mD3d12SharingContract;

        // 11on12 device and device context corresponding to mCommandQueue
        ComPtr<ID3D11On12Device> mD3d11On12Device;
        ComPtr<ID3D11DeviceContext2> mD3d11On12DeviceContext;

        ComPtr<ID3D12CommandSignature> mDispatchIndirectSignature;
        ComPtr<ID3D12CommandSignature> mDrawIndirectSignature;
        ComPtr<ID3D12CommandSignature> mDrawIndexedIndirectSignature;

        CommandRecordingContext mPendingCommands;

        SerialQueue<ComPtr<IUnknown>> mUsedComObjectRefs;

        std::unique_ptr<CommandAllocatorManager> mCommandAllocatorManager;
        std::unique_ptr<ResourceAllocatorManager> mResourceAllocatorManager;
        std::unique_ptr<ResidencyManager> mResidencyManager;

        static constexpr uint32_t kMaxSamplerDescriptorsPerBindGroup =
            3 * kMaxSamplersPerShaderStage;
        static constexpr uint32_t kMaxViewDescriptorsPerBindGroup =
            kMaxBindingsPerPipelineLayout - kMaxSamplerDescriptorsPerBindGroup;

        static constexpr uint32_t kNumSamplerDescriptorAllocators =
            ConstexprLog2Ceil(kMaxSamplerDescriptorsPerBindGroup) + 1;
        static constexpr uint32_t kNumViewDescriptorAllocators =
            ConstexprLog2Ceil(kMaxViewDescriptorsPerBindGroup) + 1;

        // Index corresponds to Log2Ceil(descriptorCount) where descriptorCount is in
        // the range [0, kMaxSamplerDescriptorsPerBindGroup].
        std::array<std::unique_ptr<StagingDescriptorAllocator>, kNumViewDescriptorAllocators + 1>
            mViewAllocators;

        // Index corresponds to Log2Ceil(descriptorCount) where descriptorCount is in
        // the range [0, kMaxViewDescriptorsPerBindGroup].
        std::array<std::unique_ptr<StagingDescriptorAllocator>, kNumSamplerDescriptorAllocators + 1>
            mSamplerAllocators;

        std::unique_ptr<StagingDescriptorAllocator> mRenderTargetViewAllocator;

        std::unique_ptr<StagingDescriptorAllocator> mDepthStencilViewAllocator;

        std::unique_ptr<ShaderVisibleDescriptorAllocator> mViewShaderVisibleDescriptorAllocator;

        std::unique_ptr<ShaderVisibleDescriptorAllocator> mSamplerShaderVisibleDescriptorAllocator;

        // Sampler cache needs to be destroyed before the CPU sampler allocator to ensure the final
        // release is called.
        std::unique_ptr<SamplerHeapCache> mSamplerHeapCache;
    };

}}  // namespace dawn_native::d3d12

#endif  // DAWNNATIVE_D3D12_DEVICED3D12_H_
