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

#ifndef DAWNNATIVE_METAL_DEVICEMTL_H_
#define DAWNNATIVE_METAL_DEVICEMTL_H_

#include "dawn_native/dawn_platform.h"

#include "common/Serial.h"
#include "dawn_native/Commands.h"
#include "dawn_native/Device.h"
#include "dawn_native/metal/CommandRecordingContext.h"
#include "dawn_native/metal/Forward.h"

#import <IOSurface/IOSurfaceRef.h>
#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>

#include <atomic>
#include <memory>
#include <mutex>

namespace dawn_native { namespace metal {

    class Device : public DeviceBase {
      public:
        static ResultOrError<Device*> Create(AdapterBase* adapter,
                                             id<MTLDevice> mtlDevice,
                                             const DeviceDescriptor* descriptor);
        ~Device() override;

        MaybeError Initialize();

        CommandBufferBase* CreateCommandBuffer(CommandEncoder* encoder,
                                               const CommandBufferDescriptor* descriptor) override;

        MaybeError TickImpl() override;

        id<MTLDevice> GetMTLDevice();
        id<MTLCommandQueue> GetMTLQueue();

        CommandRecordingContext* GetPendingCommandContext();
        void SubmitPendingCommandBuffer();

        TextureBase* CreateTextureWrappingIOSurface(const ExternalImageDescriptor* descriptor,
                                                    IOSurfaceRef ioSurface,
                                                    uint32_t plane);
        void WaitForCommandsToBeScheduled();

        ResultOrError<std::unique_ptr<StagingBufferBase>> CreateStagingBuffer(size_t size) override;
        MaybeError CopyFromStagingToBuffer(StagingBufferBase* source,
                                           uint64_t sourceOffset,
                                           BufferBase* destination,
                                           uint64_t destinationOffset,
                                           uint64_t size) override;
        MaybeError CopyFromStagingToTexture(StagingBufferBase* source,
                                            const TextureDataLayout& dataLayout,
                                            TextureCopy* dst,
                                            const Extent3D copySize);

      private:
        Device(AdapterBase* adapter, id<MTLDevice> mtlDevice, const DeviceDescriptor* descriptor);

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

        void InitTogglesFromDriver();
        void ShutDownImpl() override;
        MaybeError WaitForIdleForDestruction() override;
        Serial CheckAndUpdateCompletedSerials() override;

        id<MTLDevice> mMtlDevice = nil;
        id<MTLCommandQueue> mCommandQueue = nil;

        CommandRecordingContext mCommandContext;

        // The completed serial is updated in a Metal completion handler that can be fired on a
        // different thread, so it needs to be atomic.
        std::atomic<uint64_t> mCompletedSerial;

        // mLastSubmittedCommands will be accessed in a Metal schedule handler that can be fired on
        // a different thread so we guard access to it with a mutex.
        std::mutex mLastSubmittedCommandsMutex;
        id<MTLCommandBuffer> mLastSubmittedCommands = nil;
    };

}}  // namespace dawn_native::metal

#endif  // DAWNNATIVE_METAL_DEVICEMTL_H_
