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

#include "utils/ComboRenderPipelineDescriptor.h"

#include "utils/WGPUHelpers.h"

namespace utils {

    ComboVertexStateDescriptor::ComboVertexStateDescriptor() {
        wgpu::VertexStateDescriptor* descriptor = this;

        descriptor->indexFormat = wgpu::IndexFormat::Uint32;
        descriptor->vertexBufferCount = 0;

        // Fill the default values for vertexBuffers and vertexAttributes in buffers.
        wgpu::VertexAttributeDescriptor vertexAttribute;
        vertexAttribute.shaderLocation = 0;
        vertexAttribute.offset = 0;
        vertexAttribute.format = wgpu::VertexFormat::Float;
        for (uint32_t i = 0; i < kMaxVertexAttributes; ++i) {
            cAttributes[i] = vertexAttribute;
        }
        for (uint32_t i = 0; i < kMaxVertexBuffers; ++i) {
            cVertexBuffers[i].arrayStride = 0;
            cVertexBuffers[i].stepMode = wgpu::InputStepMode::Vertex;
            cVertexBuffers[i].attributeCount = 0;
            cVertexBuffers[i].attributes = nullptr;
        }
        // cVertexBuffers[i].attributes points to somewhere in cAttributes.
        // cVertexBuffers[0].attributes points to &cAttributes[0] by default. Assuming
        // cVertexBuffers[0] has two attributes, then cVertexBuffers[1].attributes should point to
        // &cAttributes[2]. Likewise, if cVertexBuffers[1] has 3 attributes, then
        // cVertexBuffers[2].attributes should point to &cAttributes[5].
        cVertexBuffers[0].attributes = &cAttributes[0];
        descriptor->vertexBuffers = &cVertexBuffers[0];
    }

    ComboRenderPipelineDescriptor::ComboRenderPipelineDescriptor(const wgpu::Device& device) {
        wgpu::RenderPipelineDescriptor* descriptor = this;

        descriptor->primitiveTopology = wgpu::PrimitiveTopology::TriangleList;
        descriptor->sampleCount = 1;

        // Set defaults for the vertex stage descriptor.
        { vertexStage.entryPoint = "main"; }

        // Set defaults for the fragment stage desriptor.
        {
            descriptor->fragmentStage = &cFragmentStage;
            cFragmentStage.entryPoint = "main";
        }

        // Set defaults for the input state descriptors.
        descriptor->vertexState = &cVertexState;

        // Set defaults for the rasterization state descriptor.
        {
            cRasterizationState.frontFace = wgpu::FrontFace::CCW;
            cRasterizationState.cullMode = wgpu::CullMode::None;

            cRasterizationState.depthBias = 0;
            cRasterizationState.depthBiasSlopeScale = 0.0;
            cRasterizationState.depthBiasClamp = 0.0;
            descriptor->rasterizationState = &cRasterizationState;
        }

        // Set defaults for the color state descriptors.
        {
            descriptor->colorStateCount = 1;
            descriptor->colorStates = cColorStates.data();

            wgpu::BlendDescriptor blend;
            blend.operation = wgpu::BlendOperation::Add;
            blend.srcFactor = wgpu::BlendFactor::One;
            blend.dstFactor = wgpu::BlendFactor::Zero;
            wgpu::ColorStateDescriptor colorStateDescriptor;
            colorStateDescriptor.format = wgpu::TextureFormat::RGBA8Unorm;
            colorStateDescriptor.alphaBlend = blend;
            colorStateDescriptor.colorBlend = blend;
            colorStateDescriptor.writeMask = wgpu::ColorWriteMask::All;
            for (uint32_t i = 0; i < kMaxColorAttachments; ++i) {
                cColorStates[i] = colorStateDescriptor;
            }
        }

        // Set defaults for the depth stencil state descriptors.
        {
            wgpu::StencilStateFaceDescriptor stencilFace;
            stencilFace.compare = wgpu::CompareFunction::Always;
            stencilFace.failOp = wgpu::StencilOperation::Keep;
            stencilFace.depthFailOp = wgpu::StencilOperation::Keep;
            stencilFace.passOp = wgpu::StencilOperation::Keep;

            cDepthStencilState.format = wgpu::TextureFormat::Depth24PlusStencil8;
            cDepthStencilState.depthWriteEnabled = false;
            cDepthStencilState.depthCompare = wgpu::CompareFunction::Always;
            cDepthStencilState.stencilBack = stencilFace;
            cDepthStencilState.stencilFront = stencilFace;
            cDepthStencilState.stencilReadMask = 0xff;
            cDepthStencilState.stencilWriteMask = 0xff;
            descriptor->depthStencilState = nullptr;
        }
    }

}  // namespace utils
