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

#ifndef UTILS_COMBORENDERPIPELINEDESCRIPTOR_H_
#define UTILS_COMBORENDERPIPELINEDESCRIPTOR_H_

#include <dawn/webgpu_cpp.h>

#include "common/Constants.h"

#include <array>

namespace utils {

    class ComboVertexStateDescriptor : public wgpu::VertexStateDescriptor {
      public:
        ComboVertexStateDescriptor();

        std::array<wgpu::VertexBufferLayoutDescriptor, kMaxVertexBuffers> cVertexBuffers;
        std::array<wgpu::VertexAttributeDescriptor, kMaxVertexAttributes> cAttributes;
    };

    class ComboRenderPipelineDescriptor : public wgpu::RenderPipelineDescriptor {
      public:
        ComboRenderPipelineDescriptor(const wgpu::Device& device);

        ComboRenderPipelineDescriptor(const ComboRenderPipelineDescriptor&) = delete;
        ComboRenderPipelineDescriptor& operator=(const ComboRenderPipelineDescriptor&) = delete;
        ComboRenderPipelineDescriptor(ComboRenderPipelineDescriptor&&) = delete;
        ComboRenderPipelineDescriptor& operator=(ComboRenderPipelineDescriptor&&) = delete;

        wgpu::ProgrammableStageDescriptor cFragmentStage;

        ComboVertexStateDescriptor cVertexState;
        wgpu::RasterizationStateDescriptor cRasterizationState;
        std::array<wgpu::ColorStateDescriptor, kMaxColorAttachments> cColorStates;
        wgpu::DepthStencilStateDescriptor cDepthStencilState;
    };

}  // namespace utils

#endif  // UTILS_COMBORENDERPIPELINEDESCRIPTOR_H_
