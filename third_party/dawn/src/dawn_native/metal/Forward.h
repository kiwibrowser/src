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

#ifndef DAWNNATIVE_METAL_FORWARD_H_
#define DAWNNATIVE_METAL_FORWARD_H_

#include "dawn_native/ToBackend.h"

namespace {
    class BindGroupBase;
    class BindGroup;
}  // namespace

namespace dawn_native { namespace metal {

    class Adapter;
    using BindGroup = BindGroupBase;
    using BindGroupLayout = BindGroupLayoutBase;
    class Buffer;
    class CommandBuffer;
    class ComputePipeline;
    class Device;
    class Framebuffer;
    class PipelineLayout;
    class Queue;
    class RenderPipeline;
    class Sampler;
    class ShaderModule;
    class StagingBuffer;
    class SwapChain;
    class Texture;
    class TextureView;

    struct MetalBackendTraits {
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
        using StagingBufferType = StagingBuffer;
        using SwapChainType = SwapChain;
        using TextureType = Texture;
        using TextureViewType = TextureView;
    };

    template <typename T>
    auto ToBackend(T&& common) -> decltype(ToBackendBase<MetalBackendTraits>(common)) {
        return ToBackendBase<MetalBackendTraits>(common);
    }

}}  // namespace dawn_native::metal

#endif  // DAWNNATIVE_METAL_FORWARD_H_
