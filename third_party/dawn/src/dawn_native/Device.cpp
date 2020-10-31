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

#include "common/Log.h"
#include "dawn_native/Adapter.h"
#include "dawn_native/AttachmentState.h"
#include "dawn_native/BindGroup.h"
#include "dawn_native/BindGroupLayout.h"
#include "dawn_native/Buffer.h"
#include "dawn_native/CommandBuffer.h"
#include "dawn_native/CommandEncoder.h"
#include "dawn_native/ComputePipeline.h"
#include "dawn_native/DynamicUploader.h"
#include "dawn_native/ErrorData.h"
#include "dawn_native/ErrorScope.h"
#include "dawn_native/ErrorScopeTracker.h"
#include "dawn_native/Fence.h"
#include "dawn_native/FenceSignalTracker.h"
#include "dawn_native/Instance.h"
#include "dawn_native/MapRequestTracker.h"
#include "dawn_native/PipelineLayout.h"
#include "dawn_native/QuerySet.h"
#include "dawn_native/Queue.h"
#include "dawn_native/RenderBundleEncoder.h"
#include "dawn_native/RenderPipeline.h"
#include "dawn_native/Sampler.h"
#include "dawn_native/ShaderModule.h"
#include "dawn_native/Surface.h"
#include "dawn_native/SwapChain.h"
#include "dawn_native/Texture.h"
#include "dawn_native/ValidationUtils_autogen.h"

#include <unordered_set>

namespace dawn_native {

    // DeviceBase sub-structures

    // The caches are unordered_sets of pointers with special hash and compare functions
    // to compare the value of the objects, instead of the pointers.
    template <typename Object>
    using ContentLessObjectCache =
        std::unordered_set<Object*, typename Object::HashFunc, typename Object::EqualityFunc>;

    struct DeviceBase::Caches {
        ~Caches() {
            ASSERT(attachmentStates.empty());
            ASSERT(bindGroupLayouts.empty());
            ASSERT(computePipelines.empty());
            ASSERT(pipelineLayouts.empty());
            ASSERT(renderPipelines.empty());
            ASSERT(samplers.empty());
            ASSERT(shaderModules.empty());
        }

        ContentLessObjectCache<AttachmentStateBlueprint> attachmentStates;
        ContentLessObjectCache<BindGroupLayoutBase> bindGroupLayouts;
        ContentLessObjectCache<ComputePipelineBase> computePipelines;
        ContentLessObjectCache<PipelineLayoutBase> pipelineLayouts;
        ContentLessObjectCache<RenderPipelineBase> renderPipelines;
        ContentLessObjectCache<SamplerBase> samplers;
        ContentLessObjectCache<ShaderModuleBase> shaderModules;
    };

    struct DeviceBase::DeprecationWarnings {
        std::unordered_set<std::string> emitted;
        size_t count = 0;
    };

    // DeviceBase

    DeviceBase::DeviceBase(AdapterBase* adapter, const DeviceDescriptor* descriptor)
        : mAdapter(adapter) {
        if (descriptor != nullptr) {
            ApplyToggleOverrides(descriptor);
            ApplyExtensions(descriptor);
        }

        mFormatTable = BuildFormatTable(this);
        SetDefaultToggles();
    }

    DeviceBase::~DeviceBase() {
    }

    MaybeError DeviceBase::Initialize(QueueBase* defaultQueue) {
        mDefaultQueue = AcquireRef(defaultQueue);
        mRootErrorScope = AcquireRef(new ErrorScope());
        mCurrentErrorScope = mRootErrorScope.Get();

        mCaches = std::make_unique<DeviceBase::Caches>();
        mErrorScopeTracker = std::make_unique<ErrorScopeTracker>(this);
        mFenceSignalTracker = std::make_unique<FenceSignalTracker>(this);
        mMapRequestTracker = std::make_unique<MapRequestTracker>(this);
        mDynamicUploader = std::make_unique<DynamicUploader>(this);
        mDeprecationWarnings = std::make_unique<DeprecationWarnings>();

        // Starting from now the backend can start doing reentrant calls so the device is marked as
        // alive.
        mState = State::Alive;

        DAWN_TRY_ASSIGN(mEmptyBindGroupLayout, CreateEmptyBindGroupLayout());

        return {};
    }

    void DeviceBase::ShutDownBase() {
        // Disconnect the device, depending on which state we are currently in.
        switch (mState) {
            case State::BeingCreated:
                // The GPU timeline was never started so we don't have to wait.
                break;

            case State::Alive:
                // Alive is the only state which can have GPU work happening. Wait for all of it to
                // complete before proceeding with destruction.
                // Ignore errors so that we can continue with destruction
                IgnoreErrors(WaitForIdleForDestruction());
                AssumeCommandsComplete();
                break;

            case State::BeingDisconnected:
                // Getting disconnected is a transient state happening in a single API call so there
                // is always an external reference keeping the Device alive, which means the
                // destructor cannot run while BeingDisconnected.
                UNREACHABLE();
                break;

            case State::Disconnected:
                break;
        }
        ASSERT(mCompletedSerial == mLastSubmittedSerial);
        ASSERT(mFutureCallbackSerial <= mCompletedSerial);

        // Skip handling device facilities if they haven't even been created (or failed doing so)
        if (mState != State::BeingCreated) {
            // The GPU timeline is finished so all services can be freed immediately. They need to
            // be freed before ShutDownImpl() because they might relinquish resources that will be
            // freed by backends in the ShutDownImpl() call. Still tick the ones that might have
            // pending callbacks.
            mErrorScopeTracker->Tick(GetCompletedCommandSerial());
            mFenceSignalTracker->Tick(GetCompletedCommandSerial());
            mMapRequestTracker->Tick(GetCompletedCommandSerial());
            // call TickImpl once last time to clean up resources
            // Ignore errors so that we can continue with destruction
            IgnoreErrors(TickImpl());
        }

        // At this point GPU operations are always finished, so we are in the disconnected state.
        mState = State::Disconnected;

        // mCurrentErrorScope can be null if we failed device initialization.
        if (mCurrentErrorScope.Get() != nullptr) {
            mCurrentErrorScope->UnlinkForShutdown();
        }
        mErrorScopeTracker = nullptr;
        mFenceSignalTracker = nullptr;
        mDynamicUploader = nullptr;
        mMapRequestTracker = nullptr;

        mEmptyBindGroupLayout = nullptr;

        AssumeCommandsComplete();
        // Tell the backend that it can free all the objects now that the GPU timeline is empty.
        ShutDownImpl();

        mCaches = nullptr;
    }

    void DeviceBase::HandleError(InternalErrorType type, const char* message) {
        // If we receive an internal error, assume the backend can't recover and proceed with
        // device destruction. We first wait for all previous commands to be completed so that
        // backend objects can be freed immediately, before handling the loss.
        if (type == InternalErrorType::Internal) {
            // Move away from the Alive state so that the application cannot use this device
            // anymore.
            // TODO(cwallez@chromium.org): Do we need atomics for this to become visible to other
            // threads in a multithreaded scenario?
            mState = State::BeingDisconnected;

            // Ignore errors so that we can continue with destruction
            // Assume all commands are complete after WaitForIdleForDestruction (because they were)
            IgnoreErrors(WaitForIdleForDestruction());
            IgnoreErrors(TickImpl());
            AssumeCommandsComplete();
            ASSERT(mFutureCallbackSerial <= mCompletedSerial);
            mState = State::Disconnected;

            // Now everything is as if the device was lost.
            type = InternalErrorType::DeviceLost;
        }

        // The device was lost, call the application callback.
        if (type == InternalErrorType::DeviceLost && mDeviceLostCallback != nullptr) {
            mDeviceLostCallback(message, mDeviceLostUserdata);
            mDeviceLostCallback = nullptr;
        }

        // Still forward device loss and internal errors to the error scopes so they all reject.
        mCurrentErrorScope->HandleError(ToWGPUErrorType(type), message);
    }

    void DeviceBase::InjectError(wgpu::ErrorType type, const char* message) {
        if (ConsumedError(ValidateErrorType(type))) {
            return;
        }

        // This method should only be used to make error scope reject. For DeviceLost there is the
        // LoseForTesting function that can be used instead.
        if (type != wgpu::ErrorType::Validation && type != wgpu::ErrorType::OutOfMemory) {
            HandleError(InternalErrorType::Validation,
                        "Invalid injected error, must be Validation or OutOfMemory");
            return;
        }

        HandleError(FromWGPUErrorType(type), message);
    }

    void DeviceBase::ConsumeError(std::unique_ptr<ErrorData> error) {
        ASSERT(error != nullptr);
        std::ostringstream ss;
        ss << error->GetMessage();
        for (const auto& callsite : error->GetBacktrace()) {
            ss << "\n    at " << callsite.function << " (" << callsite.file << ":" << callsite.line
               << ")";
        }
        HandleError(error->GetType(), ss.str().c_str());
    }

    void DeviceBase::SetUncapturedErrorCallback(wgpu::ErrorCallback callback, void* userdata) {
        mRootErrorScope->SetCallback(callback, userdata);
    }

    void DeviceBase::SetDeviceLostCallback(wgpu::DeviceLostCallback callback, void* userdata) {
        mDeviceLostCallback = callback;
        mDeviceLostUserdata = userdata;
    }

    void DeviceBase::PushErrorScope(wgpu::ErrorFilter filter) {
        if (ConsumedError(ValidateErrorFilter(filter))) {
            return;
        }
        mCurrentErrorScope = AcquireRef(new ErrorScope(filter, mCurrentErrorScope.Get()));
    }

    bool DeviceBase::PopErrorScope(wgpu::ErrorCallback callback, void* userdata) {
        if (DAWN_UNLIKELY(mCurrentErrorScope.Get() == mRootErrorScope.Get())) {
            return false;
        }
        mCurrentErrorScope->SetCallback(callback, userdata);
        mCurrentErrorScope = Ref<ErrorScope>(mCurrentErrorScope->GetParent());

        return true;
    }

    ErrorScope* DeviceBase::GetCurrentErrorScope() {
        ASSERT(mCurrentErrorScope.Get() != nullptr);
        return mCurrentErrorScope.Get();
    }

    MaybeError DeviceBase::ValidateObject(const ObjectBase* object) const {
        ASSERT(object != nullptr);
        if (DAWN_UNLIKELY(object->GetDevice() != this)) {
            return DAWN_VALIDATION_ERROR("Object from a different device.");
        }
        if (DAWN_UNLIKELY(object->IsError())) {
            return DAWN_VALIDATION_ERROR("Object is an error.");
        }
        return {};
    }

    MaybeError DeviceBase::ValidateIsAlive() const {
        if (DAWN_LIKELY(mState == State::Alive)) {
            return {};
        }
        return DAWN_DEVICE_LOST_ERROR("Device is lost");
    }

    void DeviceBase::LoseForTesting() {
        if (mState != State::Alive) {
            return;
        }

        HandleError(InternalErrorType::Internal, "Device lost for testing");
    }

    DeviceBase::State DeviceBase::GetState() const {
        return mState;
    }

    bool DeviceBase::IsLost() const {
        ASSERT(mState != State::BeingCreated);
        return mState != State::Alive;
    }

    AdapterBase* DeviceBase::GetAdapter() const {
        return mAdapter;
    }

    dawn_platform::Platform* DeviceBase::GetPlatform() const {
        return GetAdapter()->GetInstance()->GetPlatform();
    }

    ErrorScopeTracker* DeviceBase::GetErrorScopeTracker() const {
        return mErrorScopeTracker.get();
    }

    FenceSignalTracker* DeviceBase::GetFenceSignalTracker() const {
        return mFenceSignalTracker.get();
    }

    MapRequestTracker* DeviceBase::GetMapRequestTracker() const {
        return mMapRequestTracker.get();
    }

    Serial DeviceBase::GetCompletedCommandSerial() const {
        return mCompletedSerial;
    }

    Serial DeviceBase::GetLastSubmittedCommandSerial() const {
        return mLastSubmittedSerial;
    }

    Serial DeviceBase::GetFutureCallbackSerial() const {
        return mFutureCallbackSerial;
    }

    void DeviceBase::IncrementLastSubmittedCommandSerial() {
        mLastSubmittedSerial++;
    }

    void DeviceBase::AssumeCommandsComplete() {
        Serial maxSerial = std::max(mLastSubmittedSerial + 1, mFutureCallbackSerial);
        mLastSubmittedSerial = maxSerial;
        mCompletedSerial = maxSerial;
    }

    Serial DeviceBase::GetPendingCommandSerial() const {
        return mLastSubmittedSerial + 1;
    }

    void DeviceBase::AddFutureCallbackSerial(Serial serial) {
        if (serial > mFutureCallbackSerial) {
            mFutureCallbackSerial = serial;
        }
    }

    void DeviceBase::CheckPassedSerials() {
        Serial completedSerial = CheckAndUpdateCompletedSerials();

        ASSERT(completedSerial <= mLastSubmittedSerial);
        // completedSerial should not be less than mCompletedSerial unless it is 0.
        // It can be 0 when there's no fences to check.
        ASSERT(completedSerial >= mCompletedSerial || completedSerial == 0);

        if (completedSerial > mCompletedSerial) {
            mCompletedSerial = completedSerial;
        }
    }

    ResultOrError<const Format*> DeviceBase::GetInternalFormat(wgpu::TextureFormat format) const {
        size_t index = ComputeFormatIndex(format);
        if (index >= mFormatTable.size()) {
            return DAWN_VALIDATION_ERROR("Unknown texture format");
        }

        const Format* internalFormat = &mFormatTable[index];
        if (!internalFormat->isSupported) {
            return DAWN_VALIDATION_ERROR("Unsupported texture format");
        }

        return internalFormat;
    }

    const Format& DeviceBase::GetValidInternalFormat(wgpu::TextureFormat format) const {
        size_t index = ComputeFormatIndex(format);
        ASSERT(index < mFormatTable.size());
        ASSERT(mFormatTable[index].isSupported);
        return mFormatTable[index];
    }

    ResultOrError<Ref<BindGroupLayoutBase>> DeviceBase::GetOrCreateBindGroupLayout(
        const BindGroupLayoutDescriptor* descriptor) {
        BindGroupLayoutBase blueprint(this, descriptor);

        Ref<BindGroupLayoutBase> result = nullptr;
        auto iter = mCaches->bindGroupLayouts.find(&blueprint);
        if (iter != mCaches->bindGroupLayouts.end()) {
            result = *iter;
        } else {
            BindGroupLayoutBase* backendObj;
            DAWN_TRY_ASSIGN(backendObj, CreateBindGroupLayoutImpl(descriptor));
            backendObj->SetIsCachedReference();
            mCaches->bindGroupLayouts.insert(backendObj);
            result = AcquireRef(backendObj);
        }
        return std::move(result);
    }

    void DeviceBase::UncacheBindGroupLayout(BindGroupLayoutBase* obj) {
        ASSERT(obj->IsCachedReference());
        size_t removedCount = mCaches->bindGroupLayouts.erase(obj);
        ASSERT(removedCount == 1);
    }

    // Private function used at initialization
    ResultOrError<Ref<BindGroupLayoutBase>> DeviceBase::CreateEmptyBindGroupLayout() {
        BindGroupLayoutDescriptor desc = {};
        desc.entryCount = 0;
        desc.entries = nullptr;

        return GetOrCreateBindGroupLayout(&desc);
    }

    BindGroupLayoutBase* DeviceBase::GetEmptyBindGroupLayout() {
        ASSERT(mEmptyBindGroupLayout);
        return mEmptyBindGroupLayout.Get();
    }

    ResultOrError<ComputePipelineBase*> DeviceBase::GetOrCreateComputePipeline(
        const ComputePipelineDescriptor* descriptor) {
        ComputePipelineBase blueprint(this, descriptor);

        auto iter = mCaches->computePipelines.find(&blueprint);
        if (iter != mCaches->computePipelines.end()) {
            (*iter)->Reference();
            return *iter;
        }

        ComputePipelineBase* backendObj;
        DAWN_TRY_ASSIGN(backendObj, CreateComputePipelineImpl(descriptor));
        backendObj->SetIsCachedReference();
        mCaches->computePipelines.insert(backendObj);
        return backendObj;
    }

    void DeviceBase::UncacheComputePipeline(ComputePipelineBase* obj) {
        ASSERT(obj->IsCachedReference());
        size_t removedCount = mCaches->computePipelines.erase(obj);
        ASSERT(removedCount == 1);
    }

    ResultOrError<PipelineLayoutBase*> DeviceBase::GetOrCreatePipelineLayout(
        const PipelineLayoutDescriptor* descriptor) {
        PipelineLayoutBase blueprint(this, descriptor);

        auto iter = mCaches->pipelineLayouts.find(&blueprint);
        if (iter != mCaches->pipelineLayouts.end()) {
            (*iter)->Reference();
            return *iter;
        }

        PipelineLayoutBase* backendObj;
        DAWN_TRY_ASSIGN(backendObj, CreatePipelineLayoutImpl(descriptor));
        backendObj->SetIsCachedReference();
        mCaches->pipelineLayouts.insert(backendObj);
        return backendObj;
    }

    void DeviceBase::UncachePipelineLayout(PipelineLayoutBase* obj) {
        ASSERT(obj->IsCachedReference());
        size_t removedCount = mCaches->pipelineLayouts.erase(obj);
        ASSERT(removedCount == 1);
    }

    ResultOrError<RenderPipelineBase*> DeviceBase::GetOrCreateRenderPipeline(
        const RenderPipelineDescriptor* descriptor) {
        RenderPipelineBase blueprint(this, descriptor);

        auto iter = mCaches->renderPipelines.find(&blueprint);
        if (iter != mCaches->renderPipelines.end()) {
            (*iter)->Reference();
            return *iter;
        }

        RenderPipelineBase* backendObj;
        DAWN_TRY_ASSIGN(backendObj, CreateRenderPipelineImpl(descriptor));
        backendObj->SetIsCachedReference();
        mCaches->renderPipelines.insert(backendObj);
        return backendObj;
    }

    void DeviceBase::UncacheRenderPipeline(RenderPipelineBase* obj) {
        ASSERT(obj->IsCachedReference());
        size_t removedCount = mCaches->renderPipelines.erase(obj);
        ASSERT(removedCount == 1);
    }

    ResultOrError<SamplerBase*> DeviceBase::GetOrCreateSampler(
        const SamplerDescriptor* descriptor) {
        SamplerBase blueprint(this, descriptor);

        auto iter = mCaches->samplers.find(&blueprint);
        if (iter != mCaches->samplers.end()) {
            (*iter)->Reference();
            return *iter;
        }

        SamplerBase* backendObj;
        DAWN_TRY_ASSIGN(backendObj, CreateSamplerImpl(descriptor));
        backendObj->SetIsCachedReference();
        mCaches->samplers.insert(backendObj);
        return backendObj;
    }

    void DeviceBase::UncacheSampler(SamplerBase* obj) {
        ASSERT(obj->IsCachedReference());
        size_t removedCount = mCaches->samplers.erase(obj);
        ASSERT(removedCount == 1);
    }

    ResultOrError<ShaderModuleBase*> DeviceBase::GetOrCreateShaderModule(
        const ShaderModuleDescriptor* descriptor) {
        ShaderModuleBase blueprint(this, descriptor);

        auto iter = mCaches->shaderModules.find(&blueprint);
        if (iter != mCaches->shaderModules.end()) {
            (*iter)->Reference();
            return *iter;
        }

        ShaderModuleBase* backendObj;
        DAWN_TRY_ASSIGN(backendObj, CreateShaderModuleImpl(descriptor));
        backendObj->SetIsCachedReference();
        mCaches->shaderModules.insert(backendObj);
        return backendObj;
    }

    void DeviceBase::UncacheShaderModule(ShaderModuleBase* obj) {
        ASSERT(obj->IsCachedReference());
        size_t removedCount = mCaches->shaderModules.erase(obj);
        ASSERT(removedCount == 1);
    }

    Ref<AttachmentState> DeviceBase::GetOrCreateAttachmentState(
        AttachmentStateBlueprint* blueprint) {
        auto iter = mCaches->attachmentStates.find(blueprint);
        if (iter != mCaches->attachmentStates.end()) {
            return static_cast<AttachmentState*>(*iter);
        }

        Ref<AttachmentState> attachmentState = AcquireRef(new AttachmentState(this, *blueprint));
        attachmentState->SetIsCachedReference();
        mCaches->attachmentStates.insert(attachmentState.Get());
        return attachmentState;
    }

    Ref<AttachmentState> DeviceBase::GetOrCreateAttachmentState(
        const RenderBundleEncoderDescriptor* descriptor) {
        AttachmentStateBlueprint blueprint(descriptor);
        return GetOrCreateAttachmentState(&blueprint);
    }

    Ref<AttachmentState> DeviceBase::GetOrCreateAttachmentState(
        const RenderPipelineDescriptor* descriptor) {
        AttachmentStateBlueprint blueprint(descriptor);
        return GetOrCreateAttachmentState(&blueprint);
    }

    Ref<AttachmentState> DeviceBase::GetOrCreateAttachmentState(
        const RenderPassDescriptor* descriptor) {
        AttachmentStateBlueprint blueprint(descriptor);
        return GetOrCreateAttachmentState(&blueprint);
    }

    void DeviceBase::UncacheAttachmentState(AttachmentState* obj) {
        ASSERT(obj->IsCachedReference());
        size_t removedCount = mCaches->attachmentStates.erase(obj);
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
        Ref<BufferBase> result = nullptr;
        if (ConsumedError(CreateBufferInternal(descriptor), &result)) {
            ASSERT(result.Get() == nullptr);
            return BufferBase::MakeError(this, descriptor);
        }

        return result.Detach();
    }
    WGPUCreateBufferMappedResult DeviceBase::CreateBufferMapped(
        const BufferDescriptor* descriptor) {
        EmitDeprecationWarning(
            "CreateBufferMapped is deprecated, use wgpu::BufferDescriptor::mappedAtCreation and "
            "wgpu::Buffer::GetMappedRange instead");

        BufferDescriptor fixedDesc = *descriptor;
        fixedDesc.mappedAtCreation = true;
        BufferBase* buffer = CreateBuffer(&fixedDesc);

        WGPUCreateBufferMappedResult result = {};
        result.buffer = reinterpret_cast<WGPUBuffer>(buffer);
        result.data = buffer->GetMappedRange(0, descriptor->size);
        result.dataLength = descriptor->size;

        if (result.data != nullptr) {
            memset(result.data, 0, result.dataLength);
        }

        return result;
    }
    CommandEncoder* DeviceBase::CreateCommandEncoder(const CommandEncoderDescriptor* descriptor) {
        return new CommandEncoder(this, descriptor);
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
    QuerySetBase* DeviceBase::CreateQuerySet(const QuerySetDescriptor* descriptor) {
        QuerySetBase* result = nullptr;

        if (ConsumedError(CreateQuerySetInternal(&result, descriptor))) {
            return QuerySetBase::MakeError(this);
        }

        return result;
    }
    QueueBase* DeviceBase::CreateQueue() {
        // TODO(dawn:22): Remove this once users use GetDefaultQueue
        EmitDeprecationWarning(
            "Device::CreateQueue is deprecated, use Device::GetDefaultQueue instead");
        return GetDefaultQueue();
    }
    SamplerBase* DeviceBase::CreateSampler(const SamplerDescriptor* descriptor) {
        SamplerBase* result = nullptr;

        if (ConsumedError(CreateSamplerInternal(&result, descriptor))) {
            return SamplerBase::MakeError(this);
        }

        return result;
    }
    RenderBundleEncoder* DeviceBase::CreateRenderBundleEncoder(
        const RenderBundleEncoderDescriptor* descriptor) {
        RenderBundleEncoder* result = nullptr;

        if (ConsumedError(CreateRenderBundleEncoderInternal(&result, descriptor))) {
            return RenderBundleEncoder::MakeError(this);
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
    SwapChainBase* DeviceBase::CreateSwapChain(Surface* surface,
                                               const SwapChainDescriptor* descriptor) {
        SwapChainBase* result = nullptr;

        if (ConsumedError(CreateSwapChainInternal(&result, surface, descriptor))) {
            return SwapChainBase::MakeError(this);
        }

        return result;
    }
    TextureBase* DeviceBase::CreateTexture(const TextureDescriptor* descriptor) {
        Ref<TextureBase> result;

        if (ConsumedError(CreateTextureInternal(descriptor), &result)) {
            return TextureBase::MakeError(this);
        }

        return result.Detach();
    }
    TextureViewBase* DeviceBase::CreateTextureView(TextureBase* texture,
                                                   const TextureViewDescriptor* descriptor) {
        TextureViewBase* result = nullptr;

        if (ConsumedError(CreateTextureViewInternal(&result, texture, descriptor))) {
            return TextureViewBase::MakeError(this);
        }

        return result;
    }

    // For Dawn Wire

    BufferBase* DeviceBase::CreateErrorBuffer() {
        BufferDescriptor desc = {};
        return BufferBase::MakeError(this, &desc);
    }

    // Other Device API methods

    void DeviceBase::Tick() {
        if (ConsumedError(ValidateIsAlive())) {
            return;
        }
        // to avoid overly ticking, we only want to tick when:
        // 1. the last submitted serial has moved beyond the completed serial
        // 2. or the completed serial has not reached the future serial set by the trackers
        if (mLastSubmittedSerial > mCompletedSerial || mCompletedSerial < mFutureCallbackSerial) {
            CheckPassedSerials();

            if (ConsumedError(TickImpl())) {
                return;
            }

            // There is no GPU work in flight, we need to move the serials forward so that
            // so that CPU operations waiting on GPU completion can know they don't have to wait.
            // AssumeCommandsComplete will assign the max serial we must tick to in order to
            // fire the awaiting callbacks.
            if (mCompletedSerial == mLastSubmittedSerial) {
                AssumeCommandsComplete();
            }

            // TODO(cwallez@chromium.org): decouple TickImpl from updating the serial so that we can
            // tick the dynamic uploader before the backend resource allocators. This would allow
            // reclaiming resources one tick earlier.
            mDynamicUploader->Deallocate(mCompletedSerial);
            mErrorScopeTracker->Tick(mCompletedSerial);
            mFenceSignalTracker->Tick(mCompletedSerial);
            mMapRequestTracker->Tick(mCompletedSerial);
        }
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

    QueueBase* DeviceBase::GetDefaultQueue() {
        // Backends gave the default queue during initialization.
        ASSERT(mDefaultQueue.Get() != nullptr);

        // Returns a new reference to the queue.
        mDefaultQueue->Reference();
        return mDefaultQueue.Get();
    }

    void DeviceBase::ApplyExtensions(const DeviceDescriptor* deviceDescriptor) {
        ASSERT(deviceDescriptor);
        ASSERT(GetAdapter()->SupportsAllRequestedExtensions(deviceDescriptor->requiredExtensions));

        mEnabledExtensions = GetAdapter()->GetInstance()->ExtensionNamesToExtensionsSet(
            deviceDescriptor->requiredExtensions);
    }

    std::vector<const char*> DeviceBase::GetEnabledExtensions() const {
        return mEnabledExtensions.GetEnabledExtensionNames();
    }

    bool DeviceBase::IsExtensionEnabled(Extension extension) const {
        return mEnabledExtensions.IsEnabled(extension);
    }

    bool DeviceBase::IsValidationEnabled() const {
        return !IsToggleEnabled(Toggle::SkipValidation);
    }

    bool DeviceBase::IsRobustnessEnabled() const {
        return !IsToggleEnabled(Toggle::DisableRobustness);
    }

    size_t DeviceBase::GetLazyClearCountForTesting() {
        return mLazyClearCountForTesting;
    }

    void DeviceBase::IncrementLazyClearCountForTesting() {
        ++mLazyClearCountForTesting;
    }

    size_t DeviceBase::GetDeprecationWarningCountForTesting() {
        return mDeprecationWarnings->count;
    }

    void DeviceBase::EmitDeprecationWarning(const char* warning) {
        mDeprecationWarnings->count++;
        if (mDeprecationWarnings->emitted.insert(warning).second) {
            dawn::WarningLog() << warning;
        }
    }

    // Implementation details of object creation

    MaybeError DeviceBase::CreateBindGroupInternal(BindGroupBase** result,
                                                   const BindGroupDescriptor* descriptor) {
        DAWN_TRY(ValidateIsAlive());
        if (IsValidationEnabled()) {
            DAWN_TRY(ValidateBindGroupDescriptor(this, descriptor));
        }
        DAWN_TRY_ASSIGN(*result, CreateBindGroupImpl(descriptor));
        return {};
    }

    MaybeError DeviceBase::CreateBindGroupLayoutInternal(
        BindGroupLayoutBase** result,
        const BindGroupLayoutDescriptor* descriptor) {
        DAWN_TRY(ValidateIsAlive());
        if (IsValidationEnabled()) {
            DAWN_TRY(ValidateBindGroupLayoutDescriptor(this, descriptor));
        }
        Ref<BindGroupLayoutBase> bgl;
        DAWN_TRY_ASSIGN(bgl, GetOrCreateBindGroupLayout(descriptor));
        *result = bgl.Detach();
        return {};
    }

    ResultOrError<Ref<BufferBase>> DeviceBase::CreateBufferInternal(
        const BufferDescriptor* descriptor) {
        DAWN_TRY(ValidateIsAlive());
        if (IsValidationEnabled()) {
            DAWN_TRY(ValidateBufferDescriptor(this, descriptor));
        }

        Ref<BufferBase> buffer;
        DAWN_TRY_ASSIGN(buffer, CreateBufferImpl(descriptor));

        if (descriptor->mappedAtCreation) {
            DAWN_TRY(buffer->MapAtCreation());
        }

        return std::move(buffer);
    }

    MaybeError DeviceBase::CreateComputePipelineInternal(
        ComputePipelineBase** result,
        const ComputePipelineDescriptor* descriptor) {
        DAWN_TRY(ValidateIsAlive());
        if (IsValidationEnabled()) {
            DAWN_TRY(ValidateComputePipelineDescriptor(this, descriptor));
        }

        if (descriptor->layout == nullptr) {
            ComputePipelineDescriptor descriptorWithDefaultLayout = *descriptor;

            DAWN_TRY_ASSIGN(
                descriptorWithDefaultLayout.layout,
                PipelineLayoutBase::CreateDefault(this, &descriptor->computeStage.module, 1));
            // Ref will keep the pipeline layout alive until the end of the function where
            // the pipeline will take another reference.
            Ref<PipelineLayoutBase> layoutRef = AcquireRef(descriptorWithDefaultLayout.layout);

            DAWN_TRY_ASSIGN(*result, GetOrCreateComputePipeline(&descriptorWithDefaultLayout));
        } else {
            DAWN_TRY_ASSIGN(*result, GetOrCreateComputePipeline(descriptor));
        }
        return {};
    }

    MaybeError DeviceBase::CreatePipelineLayoutInternal(
        PipelineLayoutBase** result,
        const PipelineLayoutDescriptor* descriptor) {
        DAWN_TRY(ValidateIsAlive());
        if (IsValidationEnabled()) {
            DAWN_TRY(ValidatePipelineLayoutDescriptor(this, descriptor));
        }
        DAWN_TRY_ASSIGN(*result, GetOrCreatePipelineLayout(descriptor));
        return {};
    }

    MaybeError DeviceBase::CreateQuerySetInternal(QuerySetBase** result,
                                                  const QuerySetDescriptor* descriptor) {
        DAWN_TRY(ValidateIsAlive());
        if (IsValidationEnabled()) {
            DAWN_TRY(ValidateQuerySetDescriptor(this, descriptor));
        }
        DAWN_TRY_ASSIGN(*result, CreateQuerySetImpl(descriptor));
        return {};
    }

    MaybeError DeviceBase::CreateRenderBundleEncoderInternal(
        RenderBundleEncoder** result,
        const RenderBundleEncoderDescriptor* descriptor) {
        DAWN_TRY(ValidateIsAlive());
        if (IsValidationEnabled()) {
            DAWN_TRY(ValidateRenderBundleEncoderDescriptor(this, descriptor));
        }
        *result = new RenderBundleEncoder(this, descriptor);
        return {};
    }

    MaybeError DeviceBase::CreateRenderPipelineInternal(
        RenderPipelineBase** result,
        const RenderPipelineDescriptor* descriptor) {
        DAWN_TRY(ValidateIsAlive());
        if (IsValidationEnabled()) {
            DAWN_TRY(ValidateRenderPipelineDescriptor(this, descriptor));
        }

        if (descriptor->layout == nullptr) {
            RenderPipelineDescriptor descriptorWithDefaultLayout = *descriptor;

            const ShaderModuleBase* modules[2];
            modules[0] = descriptor->vertexStage.module;
            uint32_t count;
            if (descriptor->fragmentStage == nullptr) {
                count = 1;
            } else {
                modules[1] = descriptor->fragmentStage->module;
                count = 2;
            }

            DAWN_TRY_ASSIGN(descriptorWithDefaultLayout.layout,
                            PipelineLayoutBase::CreateDefault(this, modules, count));
            // Ref will keep the pipeline layout alive until the end of the function where
            // the pipeline will take another reference.
            Ref<PipelineLayoutBase> layoutRef = AcquireRef(descriptorWithDefaultLayout.layout);

            DAWN_TRY_ASSIGN(*result, GetOrCreateRenderPipeline(&descriptorWithDefaultLayout));
        } else {
            DAWN_TRY_ASSIGN(*result, GetOrCreateRenderPipeline(descriptor));
        }
        return {};
    }

    MaybeError DeviceBase::CreateSamplerInternal(SamplerBase** result,
                                                 const SamplerDescriptor* descriptor) {
        DAWN_TRY(ValidateIsAlive());
        if (IsValidationEnabled()) {
            DAWN_TRY(ValidateSamplerDescriptor(this, descriptor));
        }
        DAWN_TRY_ASSIGN(*result, GetOrCreateSampler(descriptor));
        return {};
    }

    MaybeError DeviceBase::CreateShaderModuleInternal(ShaderModuleBase** result,
                                                      const ShaderModuleDescriptor* descriptor) {
        DAWN_TRY(ValidateIsAlive());
        if (IsValidationEnabled()) {
            DAWN_TRY(ValidateShaderModuleDescriptor(this, descriptor));
        }
        DAWN_TRY_ASSIGN(*result, GetOrCreateShaderModule(descriptor));
        return {};
    }

    MaybeError DeviceBase::CreateSwapChainInternal(SwapChainBase** result,
                                                   Surface* surface,
                                                   const SwapChainDescriptor* descriptor) {
        DAWN_TRY(ValidateIsAlive());
        if (IsValidationEnabled()) {
            DAWN_TRY(ValidateSwapChainDescriptor(this, surface, descriptor));
        }

        if (surface == nullptr) {
            DAWN_TRY_ASSIGN(*result, CreateSwapChainImpl(descriptor));
        } else {
            ASSERT(descriptor->implementation == 0);

            NewSwapChainBase* previousSwapChain = surface->GetAttachedSwapChain();
            NewSwapChainBase* newSwapChain;
            DAWN_TRY_ASSIGN(newSwapChain,
                            CreateSwapChainImpl(surface, previousSwapChain, descriptor));

            if (previousSwapChain != nullptr) {
                ASSERT(!previousSwapChain->IsAttached());
            }
            ASSERT(newSwapChain->IsAttached());

            surface->SetAttachedSwapChain(newSwapChain);
            *result = newSwapChain;
        }
        return {};
    }

    ResultOrError<Ref<TextureBase>> DeviceBase::CreateTextureInternal(
        const TextureDescriptor* descriptor) {
        DAWN_TRY(ValidateIsAlive());

        // TODO(dawn:22): Remove once migration from GPUTextureDescriptor.arrayLayerCount to
        // GPUTextureDescriptor.size.depth is done.
        TextureDescriptor fixedDescriptor;
        DAWN_TRY_ASSIGN(fixedDescriptor, FixTextureDescriptor(this, descriptor));
        descriptor = &fixedDescriptor;

        if (IsValidationEnabled()) {
            DAWN_TRY(ValidateTextureDescriptor(this, descriptor));
        }
        return CreateTextureImpl(descriptor);
    }

    MaybeError DeviceBase::CreateTextureViewInternal(TextureViewBase** result,
                                                     TextureBase* texture,
                                                     const TextureViewDescriptor* descriptor) {
        DAWN_TRY(ValidateIsAlive());
        DAWN_TRY(ValidateObject(texture));
        TextureViewDescriptor desc = GetTextureViewDescriptorWithDefaults(texture, descriptor);
        if (IsValidationEnabled()) {
            DAWN_TRY(ValidateTextureViewDescriptor(texture, &desc));
        }
        DAWN_TRY_ASSIGN(*result, CreateTextureViewImpl(texture, &desc));
        return {};
    }

    // Other implementation details

    DynamicUploader* DeviceBase::GetDynamicUploader() const {
        return mDynamicUploader.get();
    }

    // The Toggle device facility

    std::vector<const char*> DeviceBase::GetTogglesUsed() const {
        return mEnabledToggles.GetContainedToggleNames();
    }

    bool DeviceBase::IsToggleEnabled(Toggle toggle) const {
        return mEnabledToggles.Has(toggle);
    }

    void DeviceBase::SetToggle(Toggle toggle, bool isEnabled) {
        if (!mOverridenToggles.Has(toggle)) {
            mEnabledToggles.Set(toggle, isEnabled);
        }
    }

    void DeviceBase::ForceSetToggle(Toggle toggle, bool isEnabled) {
        if (!mOverridenToggles.Has(toggle) && mEnabledToggles.Has(toggle) != isEnabled) {
            dawn::WarningLog() << "Forcing toggle \"" << ToggleEnumToName(toggle) << "\" to "
                               << isEnabled << "when it was overriden to be " << !isEnabled;
        }
        mEnabledToggles.Set(toggle, isEnabled);
    }

    void DeviceBase::SetDefaultToggles() {
        SetToggle(Toggle::LazyClearResourceOnFirstUse, true);
        SetToggle(Toggle::UseSpvc, false);
    }

    void DeviceBase::ApplyToggleOverrides(const DeviceDescriptor* deviceDescriptor) {
        ASSERT(deviceDescriptor);

        for (const char* toggleName : deviceDescriptor->forceEnabledToggles) {
            Toggle toggle = GetAdapter()->GetInstance()->ToggleNameToEnum(toggleName);
            if (toggle != Toggle::InvalidEnum) {
                mEnabledToggles.Set(toggle, true);
                mOverridenToggles.Set(toggle, true);
            }
        }
        for (const char* toggleName : deviceDescriptor->forceDisabledToggles) {
            Toggle toggle = GetAdapter()->GetInstance()->ToggleNameToEnum(toggleName);
            if (toggle != Toggle::InvalidEnum) {
                mEnabledToggles.Set(toggle, false);
                mOverridenToggles.Set(toggle, true);
            }
        }
    }

}  // namespace dawn_native
