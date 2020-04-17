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

#ifndef DAWNNATIVE_NULL_DEVICENULL_H_
#define DAWNNATIVE_NULL_DEVICENULL_H_

#include "dawn_native/BindGroup.h"
#include "dawn_native/BindGroupLayout.h"
#include "dawn_native/Buffer.h"
#include "dawn_native/CommandBuffer.h"
#include "dawn_native/CommandEncoder.h"
#include "dawn_native/ComputePipeline.h"
#include "dawn_native/Device.h"
#include "dawn_native/PipelineLayout.h"
#include "dawn_native/Queue.h"
#include "dawn_native/RenderPipeline.h"
#include "dawn_native/RingBuffer.h"
#include "dawn_native/Sampler.h"
#include "dawn_native/ShaderModule.h"
#include "dawn_native/StagingBuffer.h"
#include "dawn_native/SwapChain.h"
#include "dawn_native/Texture.h"
#include "dawn_native/ToBackend.h"
#include "dawn_native/dawn_platform.h"

namespace dawn_native { namespace null {

    class Adapter;
    using BindGroup = BindGroupBase;
    using BindGroupLayout = BindGroupLayoutBase;
    class Buffer;
    class CommandBuffer;
    using ComputePipeline = ComputePipelineBase;
    class Device;
    using PipelineLayout = PipelineLayoutBase;
    class Queue;
    using RenderPipeline = RenderPipelineBase;
    using Sampler = SamplerBase;
    using ShaderModule = ShaderModuleBase;
    class SwapChain;
    using Texture = TextureBase;
    using TextureView = TextureViewBase;

    struct NullBackendTraits {
        using AdapterType = Adapter;
        using BindGroupType = BindGroup;
        using BindGroupLayoutType = BindGroupLayout;
        using BufferType = Buffer;
        using CommandBufferType = CommandBuffer;
        using ComputePipelineType = ComputePipeline;
        using DeviceType = Device;
        using PipelineLayoutType = PipelineLayout;
        using QueueType = Queue;
        using RenderPipelineType = RenderPipeline;
        using SamplerType = Sampler;
        using ShaderModuleType = ShaderModule;
        using SwapChainType = SwapChain;
        using TextureType = Texture;
        using TextureViewType = TextureView;
    };

    template <typename T>
    auto ToBackend(T&& common) -> decltype(ToBackendBase<NullBackendTraits>(common)) {
        return ToBackendBase<NullBackendTraits>(common);
    }

    struct PendingOperation {
        virtual ~PendingOperation() = default;
        virtual void Execute() = 0;
    };

    class Device : public DeviceBase {
      public:
        Device(Adapter* adapter, const DeviceDescriptor* descriptor);
        ~Device();

        CommandBufferBase* CreateCommandBuffer(CommandEncoderBase* encoder) override;

        Serial GetCompletedCommandSerial() const final override;
        Serial GetLastSubmittedCommandSerial() const final override;
        Serial GetPendingCommandSerial() const override;
        void TickImpl() override;

        void AddPendingOperation(std::unique_ptr<PendingOperation> operation);
        void SubmitPendingOperations();

        ResultOrError<std::unique_ptr<StagingBufferBase>> CreateStagingBuffer(size_t size) override;
        MaybeError CopyFromStagingToBuffer(StagingBufferBase* source,
                                           uint64_t sourceOffset,
                                           BufferBase* destination,
                                           uint64_t destinationOffset,
                                           uint64_t size) override;

        MaybeError IncrementMemoryUsage(size_t bytes);
        void DecrementMemoryUsage(size_t bytes);

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
        std::vector<std::unique_ptr<PendingOperation>> mPendingOperations;

        static constexpr size_t kMaxMemoryUsage = 256 * 1024 * 1024;
        size_t mMemoryUsage = 0;
    };

    class Buffer : public BufferBase {
      public:
        Buffer(Device* device, const BufferDescriptor* descriptor);
        ~Buffer();

        void MapReadOperationCompleted(uint32_t serial, void* ptr, bool isWrite);
        void CopyFromStaging(StagingBufferBase* staging,
                             uint64_t sourceOffset,
                             uint64_t destinationOffset,
                             uint64_t size);

      private:
        // Dawn API
        MaybeError SetSubDataImpl(uint32_t start, uint32_t count, const void* data) override;
        void MapReadAsyncImpl(uint32_t serial) override;
        void MapWriteAsyncImpl(uint32_t serial) override;
        void UnmapImpl() override;
        void DestroyImpl() override;

        bool IsMapWritable() const override;
        MaybeError MapAtCreationImpl(uint8_t** mappedPointer) override;
        void MapAsyncImplCommon(uint32_t serial, bool isWrite);

        std::unique_ptr<uint8_t[]> mBackingData;
    };

    class CommandBuffer : public CommandBufferBase {
      public:
        CommandBuffer(Device* device, CommandEncoderBase* encoder);
        ~CommandBuffer();

      private:
        CommandIterator mCommands;
    };

    class Queue : public QueueBase {
      public:
        Queue(Device* device);
        ~Queue();

      private:
        void SubmitImpl(uint32_t commandCount, CommandBufferBase* const* commands) override;
    };

    class SwapChain : public SwapChainBase {
      public:
        SwapChain(Device* device, const SwapChainDescriptor* descriptor);
        ~SwapChain();

      protected:
        TextureBase* GetNextTextureImpl(const TextureDescriptor* descriptor) override;
        void OnBeforePresent(TextureBase*) override;
    };

    class NativeSwapChainImpl {
      public:
        using WSIContext = struct {};
        void Init(WSIContext* context);
        DawnSwapChainError Configure(DawnTextureFormat format,
                                     DawnTextureUsageBit,
                                     uint32_t width,
                                     uint32_t height);
        DawnSwapChainError GetNextTexture(DawnSwapChainNextTexture* nextTexture);
        DawnSwapChainError Present();
        dawn::TextureFormat GetPreferredFormat() const;
    };

    class StagingBuffer : public StagingBufferBase {
      public:
        StagingBuffer(size_t size, Device* device);
        ~StagingBuffer() override;
        MaybeError Initialize() override;

      private:
        Device* mDevice;
        std::unique_ptr<uint8_t[]> mBuffer;
    };

}}  // namespace dawn_native::null

#endif  // DAWNNATIVE_NULL_DEVICENULL_H_
