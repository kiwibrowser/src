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

#include "dawn_native/opengl/DeviceGL.h"

#include "dawn_native/BackendConnection.h"
#include "dawn_native/BindGroupLayout.h"
#include "dawn_native/ErrorData.h"
#include "dawn_native/StagingBuffer.h"
#include "dawn_native/opengl/BindGroupGL.h"
#include "dawn_native/opengl/BindGroupLayoutGL.h"
#include "dawn_native/opengl/BufferGL.h"
#include "dawn_native/opengl/CommandBufferGL.h"
#include "dawn_native/opengl/ComputePipelineGL.h"
#include "dawn_native/opengl/PipelineLayoutGL.h"
#include "dawn_native/opengl/QuerySetGL.h"
#include "dawn_native/opengl/QueueGL.h"
#include "dawn_native/opengl/RenderPipelineGL.h"
#include "dawn_native/opengl/SamplerGL.h"
#include "dawn_native/opengl/ShaderModuleGL.h"
#include "dawn_native/opengl/SwapChainGL.h"
#include "dawn_native/opengl/TextureGL.h"

namespace dawn_native { namespace opengl {

    // static
    ResultOrError<Device*> Device::Create(AdapterBase* adapter,
                                          const DeviceDescriptor* descriptor,
                                          const OpenGLFunctions& functions) {
        Ref<Device> device = AcquireRef(new Device(adapter, descriptor, functions));
        DAWN_TRY(device->Initialize());
        return device.Detach();
    }

    Device::Device(AdapterBase* adapter,
                   const DeviceDescriptor* descriptor,
                   const OpenGLFunctions& functions)
        : DeviceBase(adapter, descriptor), gl(functions) {
    }

    Device::~Device() {
        ShutDownBase();
    }

    MaybeError Device::Initialize() {
        InitTogglesFromDriver();
        mFormatTable = BuildGLFormatTable();

        return DeviceBase::Initialize(new Queue(this));
    }

    void Device::InitTogglesFromDriver() {
        bool supportsBaseVertex = gl.IsAtLeastGLES(3, 2) || gl.IsAtLeastGL(3, 2);

        bool supportsBaseInstance = gl.IsAtLeastGLES(3, 2) || gl.IsAtLeastGL(4, 2);

        // TODO(crbug.com/dawn/343): We can support the extension variants, but need to load the EXT
        // procs without the extension suffix.
        // We'll also need emulation of shader builtins gl_BaseVertex and gl_BaseInstance.

        // supportsBaseVertex |=
        //     (gl.IsAtLeastGLES(2, 0) &&
        //      (gl.IsGLExtensionSupported("OES_draw_elements_base_vertex") ||
        //       gl.IsGLExtensionSupported("EXT_draw_elements_base_vertex"))) ||
        //     (gl.IsAtLeastGL(3, 1) && gl.IsGLExtensionSupported("ARB_draw_elements_base_vertex"));

        // supportsBaseInstance |=
        //     (gl.IsAtLeastGLES(3, 1) && gl.IsGLExtensionSupported("EXT_base_instance")) ||
        //     (gl.IsAtLeastGL(3, 1) && gl.IsGLExtensionSupported("ARB_base_instance"));

        // TODO(crbug.com/dawn/343): Investigate emulation.
        SetToggle(Toggle::DisableBaseVertex, !supportsBaseVertex);
        SetToggle(Toggle::DisableBaseInstance, !supportsBaseInstance);
    }

    const GLFormat& Device::GetGLFormat(const Format& format) {
        ASSERT(format.isSupported);
        ASSERT(format.GetIndex() < mFormatTable.size());

        const GLFormat& result = mFormatTable[format.GetIndex()];
        ASSERT(result.isSupportedOnBackend);
        return result;
    }

    ResultOrError<BindGroupBase*> Device::CreateBindGroupImpl(
        const BindGroupDescriptor* descriptor) {
        DAWN_TRY(ValidateGLBindGroupDescriptor(descriptor));
        return BindGroup::Create(this, descriptor);
    }
    ResultOrError<BindGroupLayoutBase*> Device::CreateBindGroupLayoutImpl(
        const BindGroupLayoutDescriptor* descriptor) {
        return new BindGroupLayout(this, descriptor);
    }
    ResultOrError<Ref<BufferBase>> Device::CreateBufferImpl(const BufferDescriptor* descriptor) {
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
        return ShaderModule::Create(this, descriptor);
    }
    ResultOrError<SwapChainBase*> Device::CreateSwapChainImpl(
        const SwapChainDescriptor* descriptor) {
        return new SwapChain(this, descriptor);
    }
    ResultOrError<NewSwapChainBase*> Device::CreateSwapChainImpl(
        Surface* surface,
        NewSwapChainBase* previousSwapChain,
        const SwapChainDescriptor* descriptor) {
        return DAWN_VALIDATION_ERROR("New swapchains not implemented.");
    }
    ResultOrError<Ref<TextureBase>> Device::CreateTextureImpl(const TextureDescriptor* descriptor) {
        return AcquireRef(new Texture(this, descriptor));
    }
    ResultOrError<TextureViewBase*> Device::CreateTextureViewImpl(
        TextureBase* texture,
        const TextureViewDescriptor* descriptor) {
        return new TextureView(texture, descriptor);
    }

    void Device::SubmitFenceSync() {
        GLsync sync = gl.FenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        IncrementLastSubmittedCommandSerial();
        mFencesInFlight.emplace(sync, GetLastSubmittedCommandSerial());
    }

    MaybeError Device::TickImpl() {
        return {};
    }

    Serial Device::CheckAndUpdateCompletedSerials() {
        Serial fenceSerial = 0;
        while (!mFencesInFlight.empty()) {
            GLsync sync = mFencesInFlight.front().first;
            Serial tentativeSerial = mFencesInFlight.front().second;

            // Fence are added in order, so we can stop searching as soon
            // as we see one that's not ready.
            GLenum result = gl.ClientWaitSync(sync, GL_SYNC_FLUSH_COMMANDS_BIT, 0);
            if (result == GL_TIMEOUT_EXPIRED) {
                return fenceSerial;
            }
            // Update fenceSerial since fence is ready.
            fenceSerial = tentativeSerial;

            gl.DeleteSync(sync);

            mFencesInFlight.pop();

            ASSERT(fenceSerial > GetCompletedCommandSerial());
        }
        return fenceSerial;
    }

    ResultOrError<std::unique_ptr<StagingBufferBase>> Device::CreateStagingBuffer(size_t size) {
        return DAWN_UNIMPLEMENTED_ERROR("Device unable to create staging buffer.");
    }

    MaybeError Device::CopyFromStagingToBuffer(StagingBufferBase* source,
                                               uint64_t sourceOffset,
                                               BufferBase* destination,
                                               uint64_t destinationOffset,
                                               uint64_t size) {
        return DAWN_UNIMPLEMENTED_ERROR("Device unable to copy from staging buffer.");
    }

    void Device::ShutDownImpl() {
        ASSERT(GetState() == State::Disconnected);
    }

    MaybeError Device::WaitForIdleForDestruction() {
        gl.Finish();
        CheckPassedSerials();
        ASSERT(mFencesInFlight.empty());

        return {};
    }

}}  // namespace dawn_native::opengl
