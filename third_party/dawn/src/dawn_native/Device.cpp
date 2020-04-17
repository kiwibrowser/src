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

#include "dawn_native/Device.h"

#include "dawn_native/Adapter.h"
#include "dawn_native/BindGroup.h"
#include "dawn_native/BindGroupLayout.h"
#include "dawn_native/Buffer.h"
#include "dawn_native/CommandBuffer.h"
#include "dawn_native/CommandEncoder.h"
#include "dawn_native/ComputePipeline.h"
#include "dawn_native/DynamicUploader.h"
#include "dawn_native/ErrorData.h"
#include "dawn_native/Fence.h"
#include "dawn_native/FenceSignalTracker.h"
#include "dawn_native/Instance.h"
#include "dawn_native/PipelineLayout.h"
#include "dawn_native/Queue.h"
#include "dawn_native/RenderPipeline.h"
#include "dawn_native/Sampler.h"
#include "dawn_native/ShaderModule.h"
#include "dawn_native/SwapChain.h"
#include "dawn_native/Texture.h"

#include <unordered_set>

namespace dawn_native {

    // DeviceBase::Caches

    // The caches are unordered_sets of pointers with special hash and compare functions
    // to compare the value of the objects, instead of the pointers.
    template <typename Object>
    using ContentLessObjectCache =
        std::unordered_set<Object*, typename Object::HashFunc, typename Object::EqualityFunc>;

    struct DeviceBase::Caches {
        ContentLessObjectCache<BindGroupLayoutBase> bindGroupLayouts;
        ContentLessObjectCache<ComputePipelineBase> computePipelines;
        ContentLessObjectCache<PipelineLayoutBase> pipelineLayouts;
        ContentLessObjectCache<RenderPipelineBase> renderPipelines;
        ContentLessObjectCache<SamplerBase> samplers;
        ContentLessObjectCache<ShaderModuleBase> shaderModules;
    };

    // DeviceBase

    DeviceBase::DeviceBase(AdapterBase* adapter, const DeviceDescriptor* descriptor)
        : mAdapter(adapter) {
        mCaches = std::make_unique<DeviceBase::Caches>();
        mFenceSignalTracker = std::make_unique<FenceSignalTracker>(this);
        mDynamicUploader = std::make_unique<DynamicUploader>(this);
        SetDefaultToggles();
    }

    DeviceBase::~DeviceBase() {
        // Devices must explicitly free the uploader
        ASSERT(mDynamicUploader == nullptr);
    }

    void DeviceBase::HandleError(const char* message) {
        if (mErrorCallback) {
            mErrorCallback(message, mErrorUserdata);
        }
    }

    void DeviceBase::SetErrorCallback(dawn::DeviceErrorCallback callback, void* userdata) {
        mErrorCallback = callback;
        mErrorUserdata = userdata;
    }

    MaybeError DeviceBase::ValidateObject(const ObjectBase* object) const {
        if (DAWN_UNLIKELY(object->GetDevice() != this)) {
            return DAWN_VALIDATION_ERROR("Object from a different device.");
        }
        if (DAWN_UNLIKELY(object->IsError())) {
            return DAWN_VALIDATION_ERROR("Object is an error.");
        }
        return {};
    }

    AdapterBase* DeviceBase::GetAdapter() const {
        return mAdapter;
    }

    DeviceBase* DeviceBase::GetDevice() {
        return this;
    }

    FenceSignalTracker* DeviceBase::GetFenceSignalTracker() const {
        return mFenceSignalTracker.get();
    }

    ResultOrError<BindGroupLayoutBase*> DeviceBase::GetOrCreateBindGroupLayout(
        const BindGroupLayoutDescriptor* descriptor) {
        BindGroupLayoutBase blueprint(this, descriptor, true);

        auto iter = mCaches->bindGroupLayouts.find(&blueprint);
        if (iter != mCaches->bindGroupLayouts.end()) {
            (*iter)->Reference();
            return *iter;
        }

        BindGroupLayoutBase* backendObj;
        DAWN_TRY_ASSIGN(backendObj, CreateBindGroupLayoutImpl(descriptor));
        mCaches->bindGroupLayouts.insert(backendObj);
        return backendObj;
    }

    void DeviceBase::UncacheBindGroupLayout(BindGroupLayoutBase* obj) {
        size_t removedCount = mCaches->bindGroupLayouts.erase(obj);
        ASSERT(removedCount == 1);
    }

    ResultOrError<ComputePipelineBase*> DeviceBase::GetOrCreateComputePipeline(
        const ComputePipelineDescriptor* descriptor) {
        ComputePipelineBase blueprint(this, descriptor, true);

        auto iter = mCaches->computePipelines.find(&blueprint);
        if (iter != mCaches->computePipelines.end()) {
            (*iter)->Reference();
            return *iter;
        }

        ComputePipelineBase* backendObj;
        DAWN_TRY_ASSIGN(backendObj, CreateComputePipelineImpl(descriptor));
        mCaches->computePipelines.insert(backendObj);
        return backendObj;
    }

    void DeviceBase::UncacheComputePipeline(ComputePipelineBase* obj) {
        size_t removedCount = mCaches->computePipelines.erase(obj);
        ASSERT(removedCount == 1);
    }

    ResultOrError<PipelineLayoutBase*> DeviceBase::GetOrCreatePipelineLayout(
        const PipelineLayoutDescriptor* descriptor) {
        PipelineLayoutBase blueprint(this, descriptor, true);

        auto iter = mCaches->pipelineLayouts.find(&blueprint);
        if (iter != mCaches->pipelineLayouts.end()) {
            (*iter)->Reference();
            return *iter;
        }

        PipelineLayoutBase* backendObj;
        DAWN_TRY_ASSIGN(backendObj, CreatePipelineLayoutImpl(descriptor));
        mCaches->pipelineLayouts.insert(backendObj);
        return backendObj;
    }

    void DeviceBase::UncachePipelineLayout(PipelineLayoutBase* obj) {
        size_t removedCount = mCaches->pipelineLayouts.erase(obj);
        ASSERT(removedCount == 1);
    }

    ResultOrError<RenderPipelineBase*> DeviceBase::GetOrCreateRenderPipeline(
        const RenderPipelineDescriptor* descriptor) {
        RenderPipelineBase blueprint(this, descriptor, true);

        auto iter = mCaches->renderPipelines.find(&blueprint);
        if (iter != mCaches->renderPipelines.end()) {
            (*iter)->Reference();
            return *iter;
        }

        RenderPipelineBase* backendObj;
        DAWN_TRY_ASSIGN(backendObj, CreateRenderPipelineImpl(descriptor));
        mCaches->renderPipelines.insert(backendObj);
        return backendObj;
    }

    void DeviceBase::UncacheRenderPipeline(RenderPipelineBase* obj) {
        size_t removedCount = mCaches->renderPipelines.erase(obj);
        ASSERT(removedCount == 1);
    }

    ResultOrError<SamplerBase*> DeviceBase::GetOrCreateSampler(
        const SamplerDescriptor* descriptor) {
        SamplerBase blueprint(this, descriptor, true);

        auto iter = mCaches->samplers.find(&blueprint);
        if (iter != mCaches->samplers.end()) {
            (*iter)->Reference();
            return *iter;
        }

        SamplerBase* backendObj;
        DAWN_TRY_ASSIGN(backendObj, CreateSamplerImpl(descriptor));
        mCaches->samplers.insert(backendObj);
        return backendObj;
    }

    void DeviceBase::UncacheSampler(SamplerBase* obj) {
        size_t removedCount = mCaches->samplers.erase(obj);
        ASSERT(removedCount == 1);
    }

    ResultOrError<ShaderModuleBase*> DeviceBase::GetOrCreateShaderModule(
        const ShaderModuleDescriptor* descriptor) {
        ShaderModuleBase blueprint(this, descriptor, true);

        auto iter = mCaches->shaderModules.find(&blueprint);
        if (iter != mCaches->shaderModules.end()) {
            (*iter)->Reference();
            return *iter;
        }

        ShaderModuleBase* backendObj;
        DAWN_TRY_ASSIGN(backendObj, CreateShaderModuleImpl(descriptor));
        mCaches->shaderModules.insert(backendObj);
        return backendObj;
    }

    void DeviceBase::UncacheShaderModule(ShaderModuleBase* obj) {
        size_t removedCount = mCaches->shaderModules.erase(obj);
        ASSERT(removedCount == 1);
    }

    // Object creation API methods

    BindGroupBase* DeviceBase::CreateBindGroup(const BindGroupDescriptor* descriptor) {
        BindGroupBase* result = nullptr;

        if (ConsumedError(CreateBindGroupInternal(&result, descriptor))) {
            return BindGroupBase::MakeError(this);
        }

        return result;
    }
    BindGroupLayoutBase* DeviceBase::CreateBindGroupLayout(
        const BindGroupLayoutDescriptor* descriptor) {
        BindGroupLayoutBase* result = nullptr;

        if (ConsumedError(CreateBindGroupLayoutInternal(&result, descriptor))) {
            return BindGroupLayoutBase::MakeError(this);
        }

        return result;
    }
    BufferBase* DeviceBase::CreateBuffer(const BufferDescriptor* descriptor) {
        BufferBase* result = nullptr;

        if (ConsumedError(CreateBufferInternal(&result, descriptor))) {
            return BufferBase::MakeError(this);
        }

        return result;
    }
    DawnCreateBufferMappedResult DeviceBase::CreateBufferMapped(
        const BufferDescriptor* descriptor) {
        BufferBase* buffer = nullptr;
        uint8_t* data = nullptr;

        uint64_t size = descriptor->size;
        if (ConsumedError(CreateBufferInternal(&buffer, descriptor)) ||
            ConsumedError(buffer->MapAtCreation(&data))) {
            // Map failed. Replace the buffer with an error buffer.
            if (buffer != nullptr) {
                delete buffer;
            }
            buffer = BufferBase::MakeErrorMapped(this, size, &data);
        }

        ASSERT(buffer != nullptr);
        if (data == nullptr) {
            // |data| may be nullptr if there was an OOM in MakeErrorMapped.
            // Non-zero dataLength and nullptr data is used to indicate there should be
            // mapped data but the allocation failed.
            ASSERT(buffer->IsError());
        } else {
            memset(data, 0, size);
        }

        DawnCreateBufferMappedResult result = {};
        result.buffer = reinterpret_cast<DawnBuffer>(buffer);
        result.data = data;
        result.dataLength = size;

        return result;
    }
    CommandEncoderBase* DeviceBase::CreateCommandEncoder() {
        return new CommandEncoderBase(this);
    }
    ComputePipelineBase* DeviceBase::CreateComputePipeline(
        const ComputePipelineDescriptor* descriptor) {
        ComputePipelineBase* result = nullptr;

        if (ConsumedError(CreateComputePipelineInternal(&result, descriptor))) {
            return ComputePipelineBase::MakeError(this);
        }

        return result;
    }
    PipelineLayoutBase* DeviceBase::CreatePipelineLayout(
        const PipelineLayoutDescriptor* descriptor) {
        PipelineLayoutBase* result = nullptr;

        if (ConsumedError(CreatePipelineLayoutInternal(&result, descriptor))) {
            return PipelineLayoutBase::MakeError(this);
        }

        return result;
    }
    QueueBase* DeviceBase::CreateQueue() {
        QueueBase* result = nullptr;

        if (ConsumedError(CreateQueueInternal(&result))) {
            // If queue creation failure ever becomes possible, we should implement MakeError and
            // friends for them.
            UNREACHABLE();
            return nullptr;
        }

        return result;
    }
    SamplerBase* DeviceBase::CreateSampler(const SamplerDescriptor* descriptor) {
        SamplerBase* result = nullptr;

        if (ConsumedError(CreateSamplerInternal(&result, descriptor))) {
            return SamplerBase::MakeError(this);
        }

        return result;
    }
    RenderPipelineBase* DeviceBase::CreateRenderPipeline(
        const RenderPipelineDescriptor* descriptor) {
        RenderPipelineBase* result = nullptr;

        if (ConsumedError(CreateRenderPipelineInternal(&result, descriptor))) {
            return RenderPipelineBase::MakeError(this);
        }

        return result;
    }
    ShaderModuleBase* DeviceBase::CreateShaderModule(const ShaderModuleDescriptor* descriptor) {
        ShaderModuleBase* result = nullptr;

        if (ConsumedError(CreateShaderModuleInternal(&result, descriptor))) {
            return ShaderModuleBase::MakeError(this);
        }

        return result;
    }
    SwapChainBase* DeviceBase::CreateSwapChain(const SwapChainDescriptor* descriptor) {
        SwapChainBase* result = nullptr;

        if (ConsumedError(CreateSwapChainInternal(&result, descriptor))) {
            return SwapChainBase::MakeError(this);
        }

        return result;
    }
    TextureBase* DeviceBase::CreateTexture(const TextureDescriptor* descriptor) {
        TextureBase* result = nullptr;

        if (ConsumedError(CreateTextureInternal(&result, descriptor))) {
            return TextureBase::MakeError(this);
        }

        return result;
    }
    TextureViewBase* DeviceBase::CreateTextureView(TextureBase* texture,
                                                   const TextureViewDescriptor* descriptor) {
        TextureViewBase* result = nullptr;

        if (ConsumedError(CreateTextureViewInternal(&result, texture, descriptor))) {
            return TextureViewBase::MakeError(this);
        }

        return result;
    }

    // Other Device API methods

    void DeviceBase::Tick() {
        TickImpl();
        mFenceSignalTracker->Tick(GetCompletedCommandSerial());
    }

    void DeviceBase::Reference() {
        ASSERT(mRefCount != 0);
        mRefCount++;
    }

    void DeviceBase::Release() {
        ASSERT(mRefCount != 0);
        mRefCount--;
        if (mRefCount == 0) {
            delete this;
        }
    }

    void DeviceBase::ApplyToggleOverrides(const DeviceDescriptor* deviceDescriptor) {
        ASSERT(deviceDescriptor);

        for (const char* toggleName : deviceDescriptor->forceEnabledToggles) {
            Toggle toggle = GetAdapter()->GetInstance()->ToggleNameToEnum(toggleName);
            if (toggle != Toggle::InvalidEnum) {
                mTogglesSet.SetToggle(toggle, true);
            }
        }
        for (const char* toggleName : deviceDescriptor->forceDisabledToggles) {
            Toggle toggle = GetAdapter()->GetInstance()->ToggleNameToEnum(toggleName);
            if (toggle != Toggle::InvalidEnum) {
                mTogglesSet.SetToggle(toggle, false);
            }
        }
    }

    std::vector<const char*> DeviceBase::GetTogglesUsed() const {
        std::vector<const char*> togglesNameInUse(mTogglesSet.toggleBitset.count());

        uint32_t index = 0;
        for (uint32_t i : IterateBitSet(mTogglesSet.toggleBitset)) {
            const char* toggleName =
                GetAdapter()->GetInstance()->ToggleEnumToName(static_cast<Toggle>(i));
            togglesNameInUse[index] = toggleName;
            ++index;
        }

        return togglesNameInUse;
    }

    bool DeviceBase::IsToggleEnabled(Toggle toggle) const {
        return mTogglesSet.IsEnabled(toggle);
    }

    void DeviceBase::SetDefaultToggles() {
        // Sets the default-enabled toggles
        mTogglesSet.SetToggle(Toggle::LazyClearResourceOnFirstUse, true);
    }

    // Implementation details of object creation

    MaybeError DeviceBase::CreateBindGroupInternal(BindGroupBase** result,
                                                   const BindGroupDescriptor* descriptor) {
        DAWN_TRY(ValidateBindGroupDescriptor(this, descriptor));
        DAWN_TRY_ASSIGN(*result, CreateBindGroupImpl(descriptor));
        return {};
    }

    MaybeError DeviceBase::CreateBindGroupLayoutInternal(
        BindGroupLayoutBase** result,
        const BindGroupLayoutDescriptor* descriptor) {
        DAWN_TRY(ValidateBindGroupLayoutDescriptor(this, descriptor));
        DAWN_TRY_ASSIGN(*result, GetOrCreateBindGroupLayout(descriptor));
        return {};
    }

    MaybeError DeviceBase::CreateBufferInternal(BufferBase** result,
                                                const BufferDescriptor* descriptor) {
        DAWN_TRY(ValidateBufferDescriptor(this, descriptor));
        DAWN_TRY_ASSIGN(*result, CreateBufferImpl(descriptor));
        return {};
    }

    MaybeError DeviceBase::CreateComputePipelineInternal(
        ComputePipelineBase** result,
        const ComputePipelineDescriptor* descriptor) {
        DAWN_TRY(ValidateComputePipelineDescriptor(this, descriptor));
        DAWN_TRY_ASSIGN(*result, GetOrCreateComputePipeline(descriptor));
        return {};
    }

    MaybeError DeviceBase::CreatePipelineLayoutInternal(
        PipelineLayoutBase** result,
        const PipelineLayoutDescriptor* descriptor) {
        DAWN_TRY(ValidatePipelineLayoutDescriptor(this, descriptor));
        DAWN_TRY_ASSIGN(*result, GetOrCreatePipelineLayout(descriptor));
        return {};
    }

    MaybeError DeviceBase::CreateQueueInternal(QueueBase** result) {
        DAWN_TRY_ASSIGN(*result, CreateQueueImpl());
        return {};
    }

    MaybeError DeviceBase::CreateRenderPipelineInternal(
        RenderPipelineBase** result,
        const RenderPipelineDescriptor* descriptor) {
        DAWN_TRY(ValidateRenderPipelineDescriptor(this, descriptor));
        DAWN_TRY_ASSIGN(*result, GetOrCreateRenderPipeline(descriptor));
        return {};
    }

    MaybeError DeviceBase::CreateSamplerInternal(SamplerBase** result,
                                                 const SamplerDescriptor* descriptor) {
        DAWN_TRY(ValidateSamplerDescriptor(this, descriptor));
        DAWN_TRY_ASSIGN(*result, GetOrCreateSampler(descriptor));
        return {};
    }

    MaybeError DeviceBase::CreateShaderModuleInternal(ShaderModuleBase** result,
                                                      const ShaderModuleDescriptor* descriptor) {
        DAWN_TRY(ValidateShaderModuleDescriptor(this, descriptor));
        DAWN_TRY_ASSIGN(*result, GetOrCreateShaderModule(descriptor));
        return {};
    }

    MaybeError DeviceBase::CreateSwapChainInternal(SwapChainBase** result,
                                                   const SwapChainDescriptor* descriptor) {
        DAWN_TRY(ValidateSwapChainDescriptor(this, descriptor));
        DAWN_TRY_ASSIGN(*result, CreateSwapChainImpl(descriptor));
        return {};
    }

    MaybeError DeviceBase::CreateTextureInternal(TextureBase** result,
                                                 const TextureDescriptor* descriptor) {
        DAWN_TRY(ValidateTextureDescriptor(this, descriptor));
        DAWN_TRY_ASSIGN(*result, CreateTextureImpl(descriptor));
        return {};
    }

    MaybeError DeviceBase::CreateTextureViewInternal(TextureViewBase** result,
                                                     TextureBase* texture,
                                                     const TextureViewDescriptor* descriptor) {
        DAWN_TRY(ValidateTextureViewDescriptor(this, texture, descriptor));
        DAWN_TRY_ASSIGN(*result, CreateTextureViewImpl(texture, descriptor));
        return {};
    }

    // Other implementation details

    void DeviceBase::ConsumeError(ErrorData* error) {
        ASSERT(error != nullptr);
        HandleError(error->GetMessage().c_str());
        delete error;
    }

    ResultOrError<DynamicUploader*> DeviceBase::GetDynamicUploader() const {
        if (mDynamicUploader->IsEmpty()) {
            DAWN_TRY(mDynamicUploader->CreateAndAppendBuffer());
        }
        return mDynamicUploader.get();
    }

    void DeviceBase::SetToggle(Toggle toggle, bool isEnabled) {
        mTogglesSet.SetToggle(toggle, isEnabled);
    }

}  // namespace dawn_native
