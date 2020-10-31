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
#include "dawn_native/BindGroupLayout.h"
#include "dawn_native/Commands.h"
#include "dawn_native/ErrorData.h"
#include "dawn_native/metal/BindGroupLayoutMTL.h"
#include "dawn_native/metal/BindGroupMTL.h"
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
#include "dawn_native/metal/UtilsMetal.h"
#include "dawn_platform/DawnPlatform.h"
#include "dawn_platform/tracing/TraceEvent.h"

#include <type_traits>

namespace dawn_native { namespace metal {

    // static
    ResultOrError<Device*> Device::Create(AdapterBase* adapter,
                                          id<MTLDevice> mtlDevice,
                                          const DeviceDescriptor* descriptor) {
        Ref<Device> device = AcquireRef(new Device(adapter, mtlDevice, descriptor));
        DAWN_TRY(device->Initialize());
        return device.Detach();
    }

    Device::Device(AdapterBase* adapter,
                   id<MTLDevice> mtlDevice,
                   const DeviceDescriptor* descriptor)
        : DeviceBase(adapter, descriptor), mMtlDevice([mtlDevice retain]), mCompletedSerial(0) {
        [mMtlDevice retain];
    }

    Device::~Device() {
        ShutDownBase();
    }

    MaybeError Device::Initialize() {
        InitTogglesFromDriver();
        mCommandQueue = [mMtlDevice newCommandQueue];

        return DeviceBase::Initialize(new Queue(this));
    }

    void Device::InitTogglesFromDriver() {
        {
            bool haveStoreAndMSAAResolve = false;
#if defined(DAWN_PLATFORM_MACOS)
            haveStoreAndMSAAResolve =
                [mMtlDevice supportsFeatureSet:MTLFeatureSet_macOS_GPUFamily1_v2];
#elif defined(DAWN_PLATFORM_IOS)
            haveStoreAndMSAAResolve =
                [mMtlDevice supportsFeatureSet:MTLFeatureSet_iOS_GPUFamily3_v2];
#endif
            // On tvOS, we would need MTLFeatureSet_tvOS_GPUFamily2_v1.
            SetToggle(Toggle::EmulateStoreAndMSAAResolve, !haveStoreAndMSAAResolve);

            bool haveSamplerCompare = true;
#if defined(DAWN_PLATFORM_IOS)
            haveSamplerCompare = [mMtlDevice supportsFeatureSet:MTLFeatureSet_iOS_GPUFamily3_v1];
#endif
            // TODO(crbug.com/dawn/342): Investigate emulation -- possibly expensive.
            SetToggle(Toggle::MetalDisableSamplerCompare, !haveSamplerCompare);

            bool haveBaseVertexBaseInstance = true;
#if defined(DAWN_PLATFORM_IOS)
            haveBaseVertexBaseInstance =
                [mMtlDevice supportsFeatureSet:MTLFeatureSet_iOS_GPUFamily3_v1];
#endif
            // TODO(crbug.com/dawn/343): Investigate emulation.
            SetToggle(Toggle::DisableBaseVertex, !haveBaseVertexBaseInstance);
            SetToggle(Toggle::DisableBaseInstance, !haveBaseVertexBaseInstance);
        }

        // TODO(jiawei.shao@intel.com): tighten this workaround when the driver bug is fixed.
        SetToggle(Toggle::AlwaysResolveIntoZeroLevelAndLayer, true);
    }

    ResultOrError<BindGroupBase*> Device::CreateBindGroupImpl(
        const BindGroupDescriptor* descriptor) {
        return BindGroup::Create(this, descriptor);
    }
    ResultOrError<BindGroupLayoutBase*> Device::CreateBindGroupLayoutImpl(
        const BindGroupLayoutDescriptor* descriptor) {
        return new BindGroupLayout(this, descriptor);
    }
    ResultOrError<Ref<BufferBase>> Device::CreateBufferImpl(const BufferDescriptor* descriptor) {
        return Buffer::Create(this, descriptor);
    }
    CommandBufferBase* Device::CreateCommandBuffer(CommandEncoder* encoder,
                                                   const CommandBufferDescriptor* descriptor) {
        return new CommandBuffer(encoder, descriptor);
    }
    ResultOrError<ComputePipelineBase*> Device::CreateComputePipelineImpl(
        const ComputePipelineDescriptor* descriptor) {
        return ComputePipeline::Create(this, descriptor);
    }
    ResultOrError<PipelineLayoutBase*> Device::CreatePipelineLayoutImpl(
        const PipelineLayoutDescriptor* descriptor) {
        return new PipelineLayout(this, descriptor);
    }
    ResultOrError<QuerySetBase*> Device::CreateQuerySetImpl(const QuerySetDescriptor* descriptor) {
        return DAWN_UNIMPLEMENTED_ERROR("Waiting for implementation");
    }
    ResultOrError<RenderPipelineBase*> Device::CreateRenderPipelineImpl(
        const RenderPipelineDescriptor* descriptor) {
        return RenderPipeline::Create(this, descriptor);
    }
    ResultOrError<SamplerBase*> Device::CreateSamplerImpl(const SamplerDescriptor* descriptor) {
        return Sampler::Create(this, descriptor);
    }
    ResultOrError<ShaderModuleBase*> Device::CreateShaderModuleImpl(
        const ShaderModuleDescriptor* descriptor) {
        return ShaderModule::Create(this, descriptor);
    }
    ResultOrError<SwapChainBase*> Device::CreateSwapChainImpl(
        const SwapChainDescriptor* descriptor) {
        return new OldSwapChain(this, descriptor);
    }
    ResultOrError<NewSwapChainBase*> Device::CreateSwapChainImpl(
        Surface* surface,
        NewSwapChainBase* previousSwapChain,
        const SwapChainDescriptor* descriptor) {
        return new SwapChain(this, surface, previousSwapChain, descriptor);
    }
    ResultOrError<Ref<TextureBase>> Device::CreateTextureImpl(const TextureDescriptor* descriptor) {
        return AcquireRef(new Texture(this, descriptor));
    }
    ResultOrError<TextureViewBase*> Device::CreateTextureViewImpl(
        TextureBase* texture,
        const TextureViewDescriptor* descriptor) {
        return new TextureView(texture, descriptor);
    }

    Serial Device::CheckAndUpdateCompletedSerials() {
        if (GetCompletedCommandSerial() > mCompletedSerial) {
            // sometimes we increase the serials, in which case the completed serial in
            // the device base will surpass the completed serial we have in the metal backend, so we
            // must update ours when we see that the completed serial from device base has
            // increased.
            mCompletedSerial = GetCompletedCommandSerial();
        }
        static_assert(std::is_same<Serial, uint64_t>::value, "");
        return mCompletedSerial.load();
    }

    MaybeError Device::TickImpl() {
        if (mCommandContext.GetCommands() != nil) {
            SubmitPendingCommandBuffer();
        }

        return {};
    }

    id<MTLDevice> Device::GetMTLDevice() {
        return mMtlDevice;
    }

    id<MTLCommandQueue> Device::GetMTLQueue() {
        return mCommandQueue;
    }

    CommandRecordingContext* Device::GetPendingCommandContext() {
        if (mCommandContext.GetCommands() == nil) {
            TRACE_EVENT0(GetPlatform(), General, "[MTLCommandQueue commandBuffer]");
            // The MTLCommandBuffer will be autoreleased by default.
            // The autorelease pool may drain before the command buffer is submitted. Retain so it
            // stays alive.
            mCommandContext = CommandRecordingContext([[mCommandQueue commandBuffer] retain]);
        }
        return &mCommandContext;
    }

    void Device::SubmitPendingCommandBuffer() {
        if (mCommandContext.GetCommands() == nil) {
            return;
        }

        IncrementLastSubmittedCommandSerial();

        // Acquire the pending command buffer, which is retained. It must be released later.
        id<MTLCommandBuffer> pendingCommands = mCommandContext.AcquireCommands();

        // Replace mLastSubmittedCommands with the mutex held so we avoid races between the
        // schedule handler and this code.
        {
            std::lock_guard<std::mutex> lock(mLastSubmittedCommandsMutex);
            mLastSubmittedCommands = pendingCommands;
        }

        [pendingCommands addScheduledHandler:^(id<MTLCommandBuffer>) {
            // This is DRF because we hold the mutex for mLastSubmittedCommands and pendingCommands
            // is a local value (and not the member itself).
            std::lock_guard<std::mutex> lock(mLastSubmittedCommandsMutex);
            if (this->mLastSubmittedCommands == pendingCommands) {
                this->mLastSubmittedCommands = nil;
            }
        }];

        // Update the completed serial once the completed handler is fired. Make a local copy of
        // mLastSubmittedSerial so it is captured by value.
        Serial pendingSerial = GetLastSubmittedCommandSerial();
        // this ObjC block runs on a different thread
        [pendingCommands addCompletedHandler:^(id<MTLCommandBuffer>) {
            TRACE_EVENT_ASYNC_END0(GetPlatform(), GPUWork, "DeviceMTL::SubmitPendingCommandBuffer",
                                   pendingSerial);
            ASSERT(pendingSerial > mCompletedSerial.load());
            this->mCompletedSerial = pendingSerial;
        }];

        TRACE_EVENT_ASYNC_BEGIN0(GetPlatform(), GPUWork, "DeviceMTL::SubmitPendingCommandBuffer",
                                 pendingSerial);
        [pendingCommands commit];
        [pendingCommands release];
    }

    ResultOrError<std::unique_ptr<StagingBufferBase>> Device::CreateStagingBuffer(size_t size) {
        std::unique_ptr<StagingBufferBase> stagingBuffer =
            std::make_unique<StagingBuffer>(size, this);
        DAWN_TRY(stagingBuffer->Initialize());
        return std::move(stagingBuffer);
    }

    MaybeError Device::CopyFromStagingToBuffer(StagingBufferBase* source,
                                               uint64_t sourceOffset,
                                               BufferBase* destination,
                                               uint64_t destinationOffset,
                                               uint64_t size) {
        // Metal validation layers forbid  0-sized copies, assert it is skipped prior to calling
        // this function.
        ASSERT(size != 0);

        ToBackend(destination)
            ->EnsureDataInitializedAsDestination(GetPendingCommandContext(), destinationOffset,
                                                 size);

        id<MTLBuffer> uploadBuffer = ToBackend(source)->GetBufferHandle();
        id<MTLBuffer> buffer = ToBackend(destination)->GetMTLBuffer();
        [GetPendingCommandContext()->EnsureBlit() copyFromBuffer:uploadBuffer
                                                    sourceOffset:sourceOffset
                                                        toBuffer:buffer
                                               destinationOffset:destinationOffset
                                                            size:size];
        return {};
    }

    MaybeError Device::CopyFromStagingToTexture(StagingBufferBase* source,
                                                const TextureDataLayout& dataLayout,
                                                TextureCopy* dst,
                                                const Extent3D copySize) {
        Texture* texture = ToBackend(dst->texture.Get());

        // This function assumes data is perfectly aligned. Otherwise, it might be necessary
        // to split copying to several stages: see ComputeTextureBufferCopySplit.
        uint32_t blockSize = dst->texture->GetFormat().blockByteSize;
        uint32_t blockWidth = dst->texture->GetFormat().blockWidth;
        uint32_t blockHeight = dst->texture->GetFormat().blockHeight;
        ASSERT(dataLayout.rowsPerImage == (copySize.height));
        ASSERT(dataLayout.bytesPerRow == (copySize.width) / blockWidth * blockSize);

        // TODO(tommek@google.com): Add tests for this in TextureZeroInitTests.
        EnsureDestinationTextureInitialized(texture, *dst, copySize);

        // Metal validation layer requires that if the texture's pixel format is a compressed
        // format, the sourceSize must be a multiple of the pixel format's block size or be
        // clamped to the edge of the texture if the block extends outside the bounds of a
        // texture.
        const Extent3D clampedSize =
            texture->ClampToMipLevelVirtualSize(dst->mipLevel, dst->origin, copySize);
        const uint32_t copyBaseLayer = dst->origin.z;
        const uint32_t copyLayerCount = copySize.depth;
        const uint64_t bytesPerImage =
            dataLayout.rowsPerImage * dataLayout.bytesPerRow / blockHeight;

        uint64_t bufferOffset = dataLayout.offset;
        for (uint32_t copyLayer = copyBaseLayer; copyLayer < copyBaseLayer + copyLayerCount;
             ++copyLayer) {
            [GetPendingCommandContext()->EnsureBlit()
                     copyFromBuffer:ToBackend(source)->GetBufferHandle()
                       sourceOffset:bufferOffset
                  sourceBytesPerRow:dataLayout.bytesPerRow
                sourceBytesPerImage:bytesPerImage
                         sourceSize:MTLSizeMake(clampedSize.width, clampedSize.height, 1)
                          toTexture:texture->GetMTLTexture()
                   destinationSlice:copyLayer
                   destinationLevel:dst->mipLevel
                  destinationOrigin:MTLOriginMake(dst->origin.x, dst->origin.y, 0)];

            bufferOffset += bytesPerImage;
        }

        return {};
    }

    TextureBase* Device::CreateTextureWrappingIOSurface(const ExternalImageDescriptor* descriptor,
                                                        IOSurfaceRef ioSurface,
                                                        uint32_t plane) {
        const TextureDescriptor* textureDescriptor =
            reinterpret_cast<const TextureDescriptor*>(descriptor->cTextureDescriptor);

        // TODO(dawn:22): Remove once migration from GPUTextureDescriptor.arrayLayerCount to
        // GPUTextureDescriptor.size.depth is done.
        TextureDescriptor fixedDescriptor;
        if (ConsumedError(FixTextureDescriptor(this, textureDescriptor), &fixedDescriptor)) {
            return nullptr;
        }
        textureDescriptor = &fixedDescriptor;

        if (ConsumedError(ValidateTextureDescriptor(this, textureDescriptor))) {
            return nullptr;
        }
        if (ConsumedError(
                ValidateIOSurfaceCanBeWrapped(this, textureDescriptor, ioSurface, plane))) {
            return nullptr;
        }

        return new Texture(this, descriptor, ioSurface, plane);
    }

    void Device::WaitForCommandsToBeScheduled() {
        SubmitPendingCommandBuffer();
        [mLastSubmittedCommands waitUntilScheduled];
    }

    MaybeError Device::WaitForIdleForDestruction() {
        [mCommandContext.AcquireCommands() release];
        CheckPassedSerials();

        // Wait for all commands to be finished so we can free resources
        while (GetCompletedCommandSerial() != GetLastSubmittedCommandSerial()) {
            usleep(100);
            CheckPassedSerials();
        }

        return {};
    }

    void Device::ShutDownImpl() {
        ASSERT(GetState() == State::Disconnected);

        [mCommandContext.AcquireCommands() release];

        [mCommandQueue release];
        mCommandQueue = nil;

        [mMtlDevice release];
        mMtlDevice = nil;
    }

}}  // namespace dawn_native::metal
