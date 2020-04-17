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

#include "dawn_native/null/DeviceNull.h"

#include "dawn_native/BackendConnection.h"
#include "dawn_native/Commands.h"
#include "dawn_native/DynamicUploader.h"

#include <spirv-cross/spirv_cross.hpp>

namespace dawn_native { namespace null {

    // Implementation of pre-Device objects: the null adapter, null backend connection and Connect()

    class Adapter : public AdapterBase {
      public:
        Adapter(InstanceBase* instance) : AdapterBase(instance, BackendType::Null) {
            mPCIInfo.name = "Null backend";
            mDeviceType = DeviceType::CPU;
        }
        virtual ~Adapter() = default;

      private:
        ResultOrError<DeviceBase*> CreateDeviceImpl(const DeviceDescriptor* descriptor) override {
            return {new Device(this, descriptor)};
        }
    };

    class Backend : public BackendConnection {
      public:
        Backend(InstanceBase* instance) : BackendConnection(instance, BackendType::Null) {
        }

        std::vector<std::unique_ptr<AdapterBase>> DiscoverDefaultAdapters() override {
            // There is always a single Null adapter because it is purely CPU based and doesn't
            // depend on the system.
            std::vector<std::unique_ptr<AdapterBase>> adapters;
            adapters.push_back(std::make_unique<Adapter>(GetInstance()));
            return adapters;
        }
    };

    BackendConnection* Connect(InstanceBase* instance) {
        return new Backend(instance);
    }

    struct CopyFromStagingToBufferOperation : PendingOperation {
        virtual void Execute() {
            destination->CopyFromStaging(staging, sourceOffset, destinationOffset, size);
        }

        StagingBufferBase* staging;
        Buffer* destination;
        uint64_t sourceOffset;
        uint64_t destinationOffset;
        uint64_t size;
    };

    // Device

    Device::Device(Adapter* adapter, const DeviceDescriptor* descriptor)
        : DeviceBase(adapter, descriptor) {
        // Apply toggle overrides if necessary for test
        if (descriptor != nullptr) {
            ApplyToggleOverrides(descriptor);
        }
    }

    Device::~Device() {
        mDynamicUploader = nullptr;

        mPendingOperations.clear();
        ASSERT(mMemoryUsage == 0);
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
        DAWN_TRY(IncrementMemoryUsage(descriptor->size));
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
        auto module = new ShaderModule(this, descriptor);

        spirv_cross::Compiler compiler(descriptor->code, descriptor->codeSize);
        module->ExtractSpirvInfo(compiler);

        return module;
    }
    ResultOrError<SwapChainBase*> Device::CreateSwapChainImpl(
        const SwapChainDescriptor* descriptor) {
        return new SwapChain(this, descriptor);
    }
    ResultOrError<TextureBase*> Device::CreateTextureImpl(const TextureDescriptor* descriptor) {
        return new Texture(this, descriptor, TextureBase::TextureState::OwnedInternal);
    }
    ResultOrError<TextureViewBase*> Device::CreateTextureViewImpl(
        TextureBase* texture,
        const TextureViewDescriptor* descriptor) {
        return new TextureView(texture, descriptor);
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
        auto operation = std::make_unique<CopyFromStagingToBufferOperation>();
        operation->staging = source;
        operation->destination = reinterpret_cast<Buffer*>(destination);
        operation->sourceOffset = sourceOffset;
        operation->destinationOffset = destinationOffset;
        operation->size = size;

        ToBackend(GetDevice())->AddPendingOperation(std::move(operation));

        return {};
    }

    MaybeError Device::IncrementMemoryUsage(size_t bytes) {
        static_assert(kMaxMemoryUsage <= std::numeric_limits<size_t>::max() / 2, "");
        if (bytes > kMaxMemoryUsage || mMemoryUsage + bytes > kMaxMemoryUsage) {
            return DAWN_CONTEXT_LOST_ERROR("Out of memory.");
        }
        mMemoryUsage += bytes;
        return {};
    }

    void Device::DecrementMemoryUsage(size_t bytes) {
        ASSERT(mMemoryUsage >= bytes);
        mMemoryUsage -= bytes;
    }

    Serial Device::GetCompletedCommandSerial() const {
        return mCompletedSerial;
    }

    Serial Device::GetLastSubmittedCommandSerial() const {
        return mLastSubmittedSerial;
    }

    Serial Device::GetPendingCommandSerial() const {
        return mLastSubmittedSerial + 1;
    }

    void Device::TickImpl() {
        SubmitPendingOperations();
    }

    void Device::AddPendingOperation(std::unique_ptr<PendingOperation> operation) {
        mPendingOperations.emplace_back(std::move(operation));
    }
    void Device::SubmitPendingOperations() {
        for (auto& operation : mPendingOperations) {
            operation->Execute();
        }
        mPendingOperations.clear();

        mCompletedSerial = mLastSubmittedSerial;
        mLastSubmittedSerial++;
    }

    // Buffer

    struct BufferMapReadOperation : PendingOperation {
        virtual void Execute() {
            buffer->MapReadOperationCompleted(serial, ptr, isWrite);
        }

        Ref<Buffer> buffer;
        void* ptr;
        uint32_t serial;
        bool isWrite;
    };

    Buffer::Buffer(Device* device, const BufferDescriptor* descriptor)
        : BufferBase(device, descriptor) {
        mBackingData = std::unique_ptr<uint8_t[]>(new uint8_t[GetSize()]);
    }

    Buffer::~Buffer() {
        DestroyInternal();
        ToBackend(GetDevice())->DecrementMemoryUsage(GetSize());
    }

    bool Buffer::IsMapWritable() const {
        // Only return true for mappable buffers so we can test cases that need / don't need a
        // staging buffer.
        return (GetUsage() & (dawn::BufferUsageBit::MapRead | dawn::BufferUsageBit::MapWrite)) != 0;
    }

    MaybeError Buffer::MapAtCreationImpl(uint8_t** mappedPointer) {
        *mappedPointer = mBackingData.get();
        return {};
    }

    void Buffer::MapReadOperationCompleted(uint32_t serial, void* ptr, bool isWrite) {
        if (isWrite) {
            CallMapWriteCallback(serial, DAWN_BUFFER_MAP_ASYNC_STATUS_SUCCESS, ptr, GetSize());
        } else {
            CallMapReadCallback(serial, DAWN_BUFFER_MAP_ASYNC_STATUS_SUCCESS, ptr, GetSize());
        }
    }

    void Buffer::CopyFromStaging(StagingBufferBase* staging,
                                 uint64_t sourceOffset,
                                 uint64_t destinationOffset,
                                 uint64_t size) {
        uint8_t* ptr = reinterpret_cast<uint8_t*>(staging->GetMappedPointer());
        memcpy(mBackingData.get() + destinationOffset, ptr + sourceOffset, size);
    }

    MaybeError Buffer::SetSubDataImpl(uint32_t start, uint32_t count, const void* data) {
        ASSERT(start + count <= GetSize());
        ASSERT(mBackingData);
        memcpy(mBackingData.get() + start, data, count);
        return {};
    }

    void Buffer::MapReadAsyncImpl(uint32_t serial) {
        MapAsyncImplCommon(serial, false);
    }

    void Buffer::MapWriteAsyncImpl(uint32_t serial) {
        MapAsyncImplCommon(serial, true);
    }

    void Buffer::MapAsyncImplCommon(uint32_t serial, bool isWrite) {
        ASSERT(mBackingData);

        auto operation = std::make_unique<BufferMapReadOperation>();
        operation->buffer = this;
        operation->ptr = mBackingData.get();
        operation->serial = serial;
        operation->isWrite = isWrite;

        ToBackend(GetDevice())->AddPendingOperation(std::move(operation));
    }

    void Buffer::UnmapImpl() {
    }

    void Buffer::DestroyImpl() {
    }

    // CommandBuffer

    CommandBuffer::CommandBuffer(Device* device, CommandEncoderBase* encoder)
        : CommandBufferBase(device, encoder), mCommands(encoder->AcquireCommands()) {
    }

    CommandBuffer::~CommandBuffer() {
        FreeCommands(&mCommands);
    }

    // Queue

    Queue::Queue(Device* device) : QueueBase(device) {
    }

    Queue::~Queue() {
    }

    void Queue::SubmitImpl(uint32_t, CommandBufferBase* const*) {
        ToBackend(GetDevice())->SubmitPendingOperations();
    }

    // SwapChain

    SwapChain::SwapChain(Device* device, const SwapChainDescriptor* descriptor)
        : SwapChainBase(device, descriptor) {
        const auto& im = GetImplementation();
        im.Init(im.userData, nullptr);
    }

    SwapChain::~SwapChain() {
    }

    TextureBase* SwapChain::GetNextTextureImpl(const TextureDescriptor* descriptor) {
        return GetDevice()->CreateTexture(descriptor);
    }

    void SwapChain::OnBeforePresent(TextureBase*) {
    }

    // NativeSwapChainImpl

    void NativeSwapChainImpl::Init(WSIContext* context) {
    }

    DawnSwapChainError NativeSwapChainImpl::Configure(DawnTextureFormat format,
                                                      DawnTextureUsageBit,
                                                      uint32_t width,
                                                      uint32_t height) {
        return DAWN_SWAP_CHAIN_NO_ERROR;
    }

    DawnSwapChainError NativeSwapChainImpl::GetNextTexture(DawnSwapChainNextTexture* nextTexture) {
        return DAWN_SWAP_CHAIN_NO_ERROR;
    }

    DawnSwapChainError NativeSwapChainImpl::Present() {
        return DAWN_SWAP_CHAIN_NO_ERROR;
    }

    dawn::TextureFormat NativeSwapChainImpl::GetPreferredFormat() const {
        return dawn::TextureFormat::R8G8B8A8Unorm;
    }

    // StagingBuffer

    StagingBuffer::StagingBuffer(size_t size, Device* device)
        : StagingBufferBase(size), mDevice(device) {
    }

    StagingBuffer::~StagingBuffer() {
        if (mBuffer) {
            mDevice->DecrementMemoryUsage(GetSize());
        }
    }

    MaybeError StagingBuffer::Initialize() {
        DAWN_TRY(mDevice->IncrementMemoryUsage(GetSize()));
        mBuffer = std::make_unique<uint8_t[]>(GetSize());
        mMappedPointer = mBuffer.get();
        return {};
    }

}}  // namespace dawn_native::null
