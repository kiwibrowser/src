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

#ifndef DAWNNATIVE_FORWARD_H_
#define DAWNNATIVE_FORWARD_H_

#include <cstdint>

namespace dawn_native {

    class AdapterBase;
    class BindGroupBase;
    class BindGroupLayoutBase;
    class BufferBase;
    class ComputePipelineBase;
    class CommandBufferBase;
    class CommandEncoderBase;
    class ComputePassEncoderBase;
    class FenceBase;
    class InstanceBase;
    class PipelineBase;
    class PipelineLayoutBase;
    class QueueBase;
    class RenderPassEncoderBase;
    class RenderPipelineBase;
    class SamplerBase;
    class ShaderModuleBase;
    class StagingBufferBase;
    class SwapChainBase;
    class TextureBase;
    class TextureViewBase;

    class DeviceBase;

    template <typename T>
    class Ref;

    template <typename T>
    class PerStage;
}  // namespace dawn_native

#endif  // DAWNNATIVE_FORWARD_H_
