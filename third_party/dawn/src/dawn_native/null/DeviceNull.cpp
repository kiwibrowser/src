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
#include "dawn_native/ErrorData.h"
#include "dawn_native/Instance.h"
#include "dawn_native/Surface.h"

#include <spirv_cross.hpp>

namespace dawn_native { namespace null {

    // Implementation of pre-Device objects: the null adapter, null backend connection and Connect()

    Adapter::Adapter(InstanceBase* instance) : AdapterBase(instance, wgpu::BackendType::Null) {
        mPCIInfo.name = "Null backend";
        mAdapterType = wgpu::AdapterType::CPU;

        // Enable all extensions by default for the convenience of tests.
        mSupportedExtensions.extensionsBitSet.flip();
    }

    Adapter::~Adapter() = default;

    // Used for the tests that intend to use an adapter without all extensions enabled.
    void Adapter::SetSupportedExtensions(const std::vector<const char*>& requiredExtensions) {
        mSupportedExtensions = GetInstance()->ExtensionNamesToExtensionsSet(requiredExtensions);
    }

    ResultOrError<DeviceBase*> Adapter::CreateDeviceImpl(const DeviceDescriptor* descriptor) {
        return Device::Create(this, descriptor);
    }

    class Backend : public BackendConnection {
      public:
        Backend(InstanceBase* instance) : BackendConnection(instance, wgpu::BackendType::Null) {
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
        Ref<Buffer> destination;
        uint64_t sourceOffset;
        uint64_t destinationOffset;
        uint64_t size;
    };

    // Device

    // static
    ResultOrError<Device*> Device::Create(Adapter* adapter, const DeviceDescriptor* descriptor) {
        Ref<Device> device = AcquireRef(new Device(adapter, descriptor));
        DAWN_TRY(device->Initialize());
        return device.Detach();
    }

    Device::~Device() {
        ShutDownBase();
    }

    MaybeError Device::Initialize() {
        return DeviceBase::Initialize(new Queue(this));
    }

    ResultOrError<BindGroupBase*> Device::CreateBindGroupImpl(
        const BindGroupDescriptor* descriptor) {
        return new BindGroup(this, descriptor);
    }
    ResultOrError<BindGroupLayoutBase*> Device::CreateBindGroupLayoutImpl(
        const BindGroupLayoutDescriptor* descriptor) {
        return new BindGroupLayout(this, descriptor);
    }
    ResultOrError<Ref<BufferBase>> Device::CreateBufferImpl(const BufferDescriptor* descriptor) {
        DAWN_TRY(IncrementMemoryUsage(descriptor->size));
        return AcquireRef(new Buffer(this, descriptor));
    }
    CommandBufferBase* Device::CreateCommandBuffer(CommandEncoder* encoder,
                                                   const CommandBufferDescriptor* descriptor) {
        return new CommandBuffer(encoder, descriptor);
    }
    ResultOrError<ComputePipelineBase*> Device::CreateComputePipelineImpl(
        const ComputePipelineDescriptor* descriptor) {
        return new ComputePipeline(this, descriptor);
    }
    ResultOrError<PipelineLayoutBase*> Device::CreatePipelineLayoutImpl(
        const PipelineLayoutDescriptor* descriptor) {
        return new PipelineLayout(this, descriptor);
    }
    ResultOrError<QuerySetBase*> Device::CreateQuerySetImpl(const QuerySetDescriptor* descriptor) {
        return new QuerySet(this, descriptor);
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
        Ref<ShaderModule> module = AcquireRef(new ShaderModule(this, descriptor));

        if (IsToggleEnabled(Toggle::UseSpvc)) {
            shaderc_spvc::CompileOptions options;
            options.SetValidate(IsValidationEnabled());
            shaderc_spvc::Context* context = module->GetContext();
            shaderc_spvc_status status = context->InitializeForGlsl(
                module->GetSpirv().data(), module->GetSpirv().size(), options);
            if (status != shaderc_spvc_status_success) {
                return DAWN_VALIDATION_ERROR("Unable to initialize instance of spvc");
            }

            spirv_cross::Compiler* compiler;
            status = context->GetCompiler(reinterpret_cast<void**>(&compiler));
            if (status != shaderc_spvc_status_success) {
                return DAWN_VALIDATION_ERROR("Unable to get cross compiler");
            }
            DAWN_TRY(module->ExtractSpirvInfo(*compiler));
        } else {
            spirv_cross::Compiler compiler(module->GetSpirv());
            DAWN_TRY(module->ExtractSpirvInfo(compiler));
        }
        return module.Detach();
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
        return AcquireRef(new Texture(this, descriptor, TextureBase::TextureState::OwnedInternal));
    }
    ResultOrError<TextureViewBase*> Device::CreateTextureViewImpl(
        TextureBase* texture,
        const TextureViewDescriptor* descriptor) {
        return new TextureView(texture, descriptor);
    }

    ResultOrError<std::unique_ptr<StagingBufferBase>> Device::CreateStagingBuffer(size_t size) {
        std::unique_ptr<StagingBufferBase> stagingBuffer =
            std::make_unique<StagingBuffer>(size, this);
        DAWN_TRY(stagingBuffer->Initialize());
        return std::move(stagingBuffer);
    }

    void Device::ShutDownImpl() {
        ASSERT(GetState() == State::Disconnected);

        // Clear pending operations before checking mMemoryUsage because some operations keep a
        // reference to Buffers.
        mPendingOperations.clear();
        ASSERT(mMemoryUsage == 0);
    }

    MaybeError Device::WaitForIdleForDestruction() {
        mPendingOperations.clear();
        return {};
    }

    MaybeError Device::CopyFromStagingToBuffer(StagingBufferBase* source,
                                               uint64_t sourceOffset,
                                               BufferBase* destination,
                                               uint64_t destinationOffset,
                                               uint64_t size) {
        if (IsToggleEnabled(Toggle::LazyClearBufferOnFirstUse)) {
            destination->SetIsDataInitialized();
        }

        auto operation = std::make_unique<CopyFromStagingToBufferOperation>();
        operation->staging = source;
        operation->destination = ToBackend(destination);
        operation->sourceOffset = sourceOffset;
        operation->destinationOffset = destinationOffset;
        operation->size = size;

        AddPendingOperation(std::move(operation));

        return {};
    }

    MaybeError Device::IncrementMemoryUsage(uint64_t bytes) {
        static_assert(kMaxMemoryUsage <= std::numeric_limits<size_t>::max(), "");
        if (bytes > kMaxMemoryUsage || mMemoryUsage + bytes > kMaxMemoryUsage) {
            return DAWN_OUT_OF_MEMORY_ERROR("Out of memory.");
        }
        mMemoryUsage += bytes;
        return {};
    }

    void Device::DecrementMemoryUsage(uint64_t bytes) {
        ASSERT(mMemoryUsage >= bytes);
        mMemoryUsage -= bytes;
    }

    MaybeError Device::TickImpl() {
        SubmitPendingOperations();
        return {};
    }

    Serial Device::CheckAndUpdateCompletedSerials() {
        return GetLastSubmittedCommandSerial();
    }

    void Device::AddPendingOperation(std::unique_ptr<PendingOperation> operation) {
        mPendingOperations.emplace_back(std::move(operation));
    }
    void Device::SubmitPendingOperations() {
        for (auto& operation : mPendingOperations) {
            operation->Execute();
        }
        mPendingOperations.clear();

        CheckPassedSerials();
        IncrementLastSubmittedCommandSerial();
    }

    // BindGroupDataHolder

    BindGroupDataHolder::BindGroupDataHolder(size_t size)
        : mBindingDataAllocation(malloc(size))  // malloc is guaranteed to return a
                                                // pointer aligned enough for the allocation
    {
    }

    BindGroupDataHolder::~BindGroupDataHolder() {
        free(mBindingDataAllocation);
    }

    // BindGroup

    BindGroup::BindGroup(DeviceBase* device, const BindGroupDescriptor* descriptor)
        : BindGroupDataHolder(descriptor->layout->GetBindingDataSize()),
          BindGroupBase(device, descriptor, mBindingDataAllocation) {
    }

    // Buffer

    Buffer::Buffer(Device* device, const BufferDescriptor* descriptor)
        : BufferBase(device, descriptor) {
        mBackingData = std::unique_ptr<uint8_t[]>(new uint8_t[GetSize()]);
    }

    Buffer::~Buffer() {
        DestroyInternal();
        ToBackend(GetDevice())->DecrementMemoryUsage(GetSize());
    }

    bool Buffer::IsMappableAtCreation() const {
        // Only return true for mappable buffers so we can test cases that need / don't need a
        // staging buffer.
        return (GetUsage() & (wgpu::BufferUsage::MapRead | wgpu::BufferUsage::MapWrite)) != 0;
    }

    MaybeError Buffer::MapAtCreationImpl() {
        return {};
    }

    void Buffer::CopyFromStaging(StagingBufferBase* staging,
                                 uint64_t sourceOffset,
                                 uint64_t destinationOffset,
                                 uint64_t size) {
        uint8_t* ptr = reinterpret_cast<uint8_t*>(staging->GetMappedPointer());
        memcpy(mBackingData.get() + destinationOffset, ptr + sourceOffset, size);
    }

    void Buffer::DoWriteBuffer(uint64_t bufferOffset, const void* data, size_t size) {
        ASSERT(bufferOffset + size <= GetSize());
        ASSERT(mBackingData);
        memcpy(mBackingData.get() + bufferOffset, data, size);
    }

    MaybeError Buffer::MapReadAsyncImpl() {
        return {};
    }

    MaybeError Buffer::MapWriteAsyncImpl() {
        return {};
    }

    MaybeError Buffer::MapAsyncImpl(wgpu::MapMode mode, size_t offset, size_t size) {
        return {};
    }

    void* Buffer::GetMappedPointerImpl() {
        return mBackingData.get();
    }

    void Buffer::UnmapImpl() {
    }

    void Buffer::DestroyImpl() {
    }

    // CommandBuffer

    CommandBuffer::CommandBuffer(CommandEncoder* encoder, const CommandBufferDescriptor* descriptor)
        : CommandBufferBase(encoder, descriptor), mCommands(encoder->AcquireCommands()) {
    }

    CommandBuffer::~CommandBuffer() {
        FreeCommands(&mCommands);
    }

    // QuerySet

    QuerySet::QuerySet(Device* device, const QuerySetDescriptor* descriptor)
        : QuerySetBase(device, descriptor) {
    }

    QuerySet::~QuerySet() {
        DestroyInternal();
    }

    void QuerySet::DestroyImpl() {
    }

    // Queue

    Queue::Queue(Device* device) : QueueBase(device) {
    }

    Queue::~Queue() {
    }

    MaybeError Queue::SubmitImpl(uint32_t, CommandBufferBase* const*) {
        ToBackend(GetDevice())->SubmitPendingOperations();
        return {};
    }

    MaybeError Queue::WriteBufferImpl(BufferBase* buffer,
                                      uint64_t bufferOffset,
                                      const void* data,
                                      size_t size) {
        ToBackend(buffer)->DoWriteBuffer(bufferOffset, data, size);
        return {};
    }

    // SwapChain

    SwapChain::SwapChain(Device* device,
                         Surface* surface,
                         NewSwapChainBase* previousSwapChain,
                         const SwapChainDescriptor* descriptor)
        : NewSwapChainBase(device, surface, descriptor) {
        if (previousSwapChain != nullptr) {
            // TODO(cwallez@chromium.org): figure out what should happen when surfaces are used by
            // multiple backends one after the other. It probably needs to block until the backend
            // and GPU are completely finished with the previous swapchain.
            ASSERT(previousSwapChain->GetBackendType() == wgpu::BackendType::Null);
            previousSwapChain->DetachFromSurface();
        }
    }

    SwapChain::~SwapChain() {
        DetachFromSurface();
    }

    MaybeError SwapChain::PresentImpl() {
        mTexture->Destroy();
        mTexture = nullptr;
        return {};
    }

    ResultOrError<TextureViewBase*> SwapChain::GetCurrentTextureViewImpl() {
        TextureDescriptor textureDesc = GetSwapChainBaseTextureDescriptor(this);
        mTexture = AcquireRef(
            new Texture(GetDevice(), &textureDesc, TextureBase::TextureState::OwnedInternal));
        return mTexture->CreateView(nullptr);
    }

    void SwapChain::DetachFromSurfaceImpl() {
        if (mTexture.Get() != nullptr) {
            mTexture->Destroy();
            mTexture = nullptr;
        }
    }

    // OldSwapChain

    OldSwapChain::OldSwapChain(Device* device, const SwapChainDescriptor* descriptor)
        : OldSwapChainBase(device, descriptor) {
        const auto& im = GetImplementation();
        im.Init(im.userData, nullptr);
    }

    OldSwapChain::~OldSwapChain() {
    }

    TextureBase* OldSwapChain::GetNextTextureImpl(const TextureDescriptor* descriptor) {
        return GetDevice()->CreateTexture(descriptor);
    }

    MaybeError OldSwapChain::OnBeforePresent(TextureViewBase*) {
        return {};
    }

    // NativeSwapChainImpl

    void NativeSwapChainImpl::Init(WSIContext* context) {
    }

    DawnSwapChainError NativeSwapChainImpl::Configure(WGPUTextureFormat format,
                                                      WGPUTextureUsage,
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

    wgpu::TextureFormat NativeSwapChainImpl::GetPreferredFormat() const {
        return wgpu::TextureFormat::RGBA8Unorm;
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
