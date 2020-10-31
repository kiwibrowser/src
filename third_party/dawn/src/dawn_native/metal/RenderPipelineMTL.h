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

#ifndef DAWNNATIVE_METAL_RENDERPIPELINEMTL_H_
#define DAWNNATIVE_METAL_RENDERPIPELINEMTL_H_

#include "dawn_native/RenderPipeline.h"

#import <Metal/Metal.h>

namespace dawn_native { namespace metal {

    class Device;

    class RenderPipeline final : public RenderPipelineBase {
      public:
        static ResultOrError<RenderPipeline*> Create(Device* device,
                                                     const RenderPipelineDescriptor* descriptor);

        MTLIndexType GetMTLIndexType() const;
        MTLPrimitiveType GetMTLPrimitiveTopology() const;
        MTLWinding GetMTLFrontFace() const;
        MTLCullMode GetMTLCullMode() const;

        void Encode(id<MTLRenderCommandEncoder> encoder);

        id<MTLDepthStencilState> GetMTLDepthStencilState();

        // For each Dawn vertex buffer, give the index in which it will be positioned in the Metal
        // vertex buffer table.
        uint32_t GetMtlVertexBufferIndex(uint32_t dawnIndex) const;

        wgpu::ShaderStage GetStagesRequiringStorageBufferLength() const;

      private:
        ~RenderPipeline() override;
        using RenderPipelineBase::RenderPipelineBase;
        MaybeError Initialize(const RenderPipelineDescriptor* descriptor);

        MTLVertexDescriptor* MakeVertexDesc();

        MTLIndexType mMtlIndexType;
        MTLPrimitiveType mMtlPrimitiveTopology;
        MTLWinding mMtlFrontFace;
        MTLCullMode mMtlCullMode;
        id<MTLRenderPipelineState> mMtlRenderPipelineState = nil;
        id<MTLDepthStencilState> mMtlDepthStencilState = nil;
        std::array<uint32_t, kMaxVertexBuffers> mMtlVertexBufferIndices;

        wgpu::ShaderStage mStagesRequiringStorageBufferLength = wgpu::ShaderStage::None;
    };

}}  // namespace dawn_native::metal

#endif  // DAWNNATIVE_METAL_RENDERPIPELINEMTL_H_
