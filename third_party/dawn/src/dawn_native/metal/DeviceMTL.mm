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

#include "dawn_native/metal/DeviceMTL.h"

#include "dawn_native/BackendConnection.h"
#include "dawn_native/BindGroup.h"
#include "dawn_native/BindGroupLayout.h"
#include "dawn_native/DynamicUploader.h"
#include "dawn_native/metal/BufferMTL.h"
#include "dawn_native/metal/CommandBufferMTL.h"
#include "dawn_native/metal/ComputePipelineMTL.h"
#include "dawn_native/metal/PipelineLayoutMTL.h"
#include "dawn_native/metal/QueueMTL.h"
#include "dawn_native/metal/RenderPipelineMTL.h"
#include "dawn_native/metal/SamplerMTL.h"
#include "dawn_native/metal/ShaderModuleMTL.h"
#include "dawn_native/metal/StagingBufferMTL.h"
#include "dawn_native/metal/SwapChainMTL.h"
#include "dawn_native/metal/TextureMTL.h"

#include <type_traits>

namespace dawn_native { namespace metal {

    Device::Device(AdapterBase* adapter,
                   id<MTLDevice> mtlDevice,
                   const DeviceDescriptor* descriptor)
        : DeviceBase(adapter, descriptor),
          mMtlDevice([mtlDevice retain]),
          mMapTracker(new MapRequestTracker(this)),
          mCompletedSerial(0) {
        [mMtlDevice retain];
        mCommandQueue = [mMtlDevice newCommandQueue];

        InitTogglesFromDriver();
        if (descriptor != nil) {
            ApplyToggleOverrides(descriptor);
        }
    }

    Device::~Device() {
        // Wait for all commands to be finished so we can free resources SubmitPendingCommandBuffer
        // may not increment the pendingCommandSerial if there are no pending commands, so we can't
        // store the pendingSerial before SubmitPendingCommandBuffer then wait for it to be passed.
        // Instead we submit and wait for the serial before the next pendingCommandSerial.
        SubmitPendingCommandBuffer();
        while (GetCompletedCommandSerial() != mLastSubmittedSerial) {
            usleep(100);
        }
        Tick();

        [mPendingCommands release];
        mPendingCommands = nil;

        mMapTracker = nullptr;
        mDynamicUploader = nullptr;

        [mCommandQueue release];
        mCommandQueue = nil;

        [mMtlDevice release];
        mMtlDevice = nil;
    }

    void Device::InitTogglesFromDriver() {
        // TODO(jiawei.shao@intel.com): check iOS feature sets
        bool emulateStoreAndMSAAResolve =
            ![mMtlDevice supportsFeatureSet:MTLFeatureSet_macOS_GPUFamily1_v2];
        SetToggle(Toggle::EmulateStoreAndMSAAResolve, emulateStoreAndMSAAResolve);

        // TODO(jiawei.shao@intel.com): tighten this workaround when the driver bug is fixed.
        SetToggle(Toggle::AlwaysResolveIntoZeroLevelAndLayer, true);
    }

    ResultOrError<BindGroupBase*> Device::CreateBindGroupImpl(
        const BindGroupDescriptor* descriptor) {
        return new BindGroup(this, descriptor);
    }
    ResultOrError<BindGroupLayoutBase*> Device::CreateBindGroupLayoutImpl(
        const BindGroupLayoutDescriptor* descriptor) {
        return new BindGroupLayout(this, descriptor);
    }
    ResultOrError<BufferBase*> Device::CreateBufferImpl(const BufferDescriptor* descriptor) {
        return new Buffer(this, descriptor);
    }
    CommandBufferBase* Device::CreateCommandBuffer(CommandEncoderBase* encoder) {
        return new CommandBuffer(this, encoder);
    }
    ResultOrError<ComputePipelineBase*> Device::CreateComputePipelineImpl(
        const ComputePipelineDescriptor* descriptor) {
        return new ComputePipeline(this, descriptor);
    }
    ResultOrError<PipelineLayoutBase*> Device::CreatePipelineLayoutImpl(
        const PipelineLayoutDescriptor* descriptor) {
        return new PipelineLayout(this, descriptor);
    }
    ResultOrError<QueueBase*> Device::CreateQueueImpl() {
        return new Queue(this);
    }
    ResultOrError<RenderPipelineBase*> Device::CreateRenderPipelineImpl(
        const RenderPipelineDescriptor* descriptor) {
        return new RenderPipeline(this, descriptor);
    }
    ResultOrError<SamplerBase*> Device::CreateSamplerImpl(const SamplerDescriptor* descriptor) {
        return new Sampler(this, descriptor);
    }
    ResultOrError<ShaderModuleBase*> Device::CreateShaderModuleImpl(
        const ShaderModuleDescriptor* descriptor) {
        return new ShaderModule(this, descriptor);
    }
    ResultOrError<SwapChainBase*> Device::CreateSwapChainImpl(
        const SwapChainDescriptor* descriptor) {
        return new SwapChain(this, descriptor);
    }
    ResultOrError<TextureBase*> Device::CreateTextureImpl(const TextureDescriptor* descriptor) {
        return new Texture(this, descriptor);
    }
    ResultOrError<TextureViewBase*> Device::CreateTextureViewImpl(
        TextureBase* texture,
        const TextureViewDescriptor* descriptor) {
        return new TextureView(texture, descriptor);
    }

    Serial Device::GetCompletedCommandSerial() const {
        static_assert(std::is_same<Serial, uint64_t>::value, "");
        return mCompletedSerial.load();
    }

    Serial Device::GetLastSubmittedCommandSerial() const {
        return mLastSubmittedSerial;
    }

    Serial Device::GetPendingCommandSerial() const {
        return mLastSubmittedSerial + 1;
    }

    void Device::TickImpl() {
        Serial completedSerial = GetCompletedCommandSerial();

        mDynamicUploader->Tick(completedSerial);
        mMapTracker->Tick(completedSerial);

        if (mPendingCommands != nil) {
            SubmitPendingCommandBuffer();
        } else if (completedSerial == mLastSubmittedSerial) {
            // If there's no GPU work in flight we still need to artificially increment the serial
            // so that CPU operations waiting on GPU completion can know they don't have to wait.
            mCompletedSerial++;
            mLastSubmittedSerial++;
        }
    }

    id<MTLDevice> Device::GetMTLDevice() {
        return mMtlDevice;
    }

    id<MTLCommandBuffer> Device::GetPendingCommandBuffer() {
        if (mPendingCommands == nil) {
            mPendingCommands = [mCommandQueue commandBuffer];
            [mPendingCommands retain];
        }
        return mPendingCommands;
    }

    void Device::SubmitPendingCommandBuffer() {
        if (mPendingCommands == nil) {
            return;
        }

        mLastSubmittedSerial++;

        // Replace mLastSubmittedCommands with the mutex held so we avoid races between the
        // schedule handler and this code.
        {
            std::lock_guard<std::mutex> lock(mLastSubmittedCommandsMutex);
            [mLastSubmittedCommands release];
            mLastSubmittedCommands = mPendingCommands;
        }

        // Ok, ObjC blocks are weird. My understanding is that local variables are captured by
        // value so this-> works as expected. However it is unclear how members are captured, (are
        // they captured using this-> or by value?). To be safe we copy members to local variables
        // to ensure they are captured "by value".

        // Free mLastSubmittedCommands as soon as it is scheduled so that it doesn't hold
        // references to its resources. Make a local copy of pendingCommands first so it is
        // captured "by-value" by the block.
        id<MTLCommandBuffer> pendingCommands = mPendingCommands;

        [mPendingCommands addScheduledHandler:^(id<MTLCommandBuffer>) {
            // This is DRF because we hold the mutex for mLastSubmittedCommands and pendingCommands
            // is a local value (and not the member itself).
            std::lock_guard<std::mutex> lock(mLastSubmittedCommandsMutex);
            if (this->mLastSubmittedCommands == pendingCommands) {
                [this->mLastSubmittedCommands release];
                this->mLastSubmittedCommands = nil;
            }
        }];

        // Update the completed serial once the completed handler is fired. Make a local copy of
        // mLastSubmittedSerial so it is captured by value.
        Serial pendingSerial = mLastSubmittedSerial;
        [mPendingCommands addCompletedHandler:^(id<MTLCommandBuffer>) {
            ASSERT(pendingSerial > mCompletedSerial.load());
            this->mCompletedSerial = pendingSerial;
        }];

        [mPendingCommands commit];
        mPendingCommands = nil;
    }

    MapRequestTracker* Device::GetMapTracker() const {
        return mMapTracker.get();
    }

    ResultOrError<std::unique_ptr<StagingBufferBase>> Device::CreateStagingBuffer(size_t size) {
        std::unique_ptr<StagingBufferBase> stagingBuffer =
            std::make_unique<StagingBuffer>(size, this);
        return std::move(stagingBuffer);
    }

    MaybeError Device::CopyFromStagingToBuffer(StagingBufferBase* source,
                                               uint64_t sourceOffset,
                                               BufferBase* destination,
                                               uint64_t destinationOffset,
                                               uint64_t size) {
        id<MTLBuffer> uploadBuffer = ToBackend(source)->GetBufferHandle();
        id<MTLBuffer> buffer = ToBackend(destination)->GetMTLBuffer();
        id<MTLCommandBuffer> commandBuffer = GetPendingCommandBuffer();
        id<MTLBlitCommandEncoder> encoder = [commandBuffer blitCommandEncoder];
        [encoder copyFromBuffer:uploadBuffer
                   sourceOffset:sourceOffset
                       toBuffer:buffer
              destinationOffset:destinationOffset
                           size:size];
        [encoder endEncoding];

        return {};
    }

    TextureBase* Device::CreateTextureWrappingIOSurface(const TextureDescriptor* descriptor,
                                                        IOSurfaceRef ioSurface,
                                                        uint32_t plane) {
        if (ConsumedError(ValidateTextureDescriptor(this, descriptor))) {
            return nullptr;
        }
        if (ConsumedError(ValidateIOSurfaceCanBeWrapped(this, descriptor, ioSurface, plane))) {
            return nullptr;
        }

        return new Texture(this, descriptor, ioSurface, plane);
    }

    void Device::WaitForCommandsToBeScheduled() {
        SubmitPendingCommandBuffer();
        [mLastSubmittedCommands waitUntilScheduled];
    }

}}  // namespace dawn_native::metal
