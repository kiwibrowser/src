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

#ifndef DAWNNATIVE_RENDERPIPELINE_H_
#define DAWNNATIVE_RENDERPIPELINE_H_

#include "dawn_native/AttachmentState.h"
#include "dawn_native/Pipeline.h"

#include "dawn_native/dawn_platform.h"

#include <array>
#include <bitset>

namespace dawn_native {

    struct BeginRenderPassCmd;

    class DeviceBase;
    class RenderBundleEncoder;

    MaybeError ValidateRenderPipelineDescriptor(const DeviceBase* device,
                                                const RenderPipelineDescriptor* descriptor);
    size_t IndexFormatSize(wgpu::IndexFormat format);
    uint32_t VertexFormatNumComponents(wgpu::VertexFormat format);
    size_t VertexFormatComponentSize(wgpu::VertexFormat format);
    size_t VertexFormatSize(wgpu::VertexFormat format);

    bool StencilTestEnabled(const DepthStencilStateDescriptor* mDepthStencilState);
    bool BlendEnabled(const ColorStateDescriptor* mColorState);

    struct VertexAttributeInfo {
        wgpu::VertexFormat format;
        uint64_t offset;
        uint32_t shaderLocation;
        uint32_t vertexBufferSlot;
    };

    struct VertexBufferInfo {
        uint64_t arrayStride;
        wgpu::InputStepMode stepMode;
    };

    class RenderPipelineBase : public PipelineBase {
      public:
        RenderPipelineBase(DeviceBase* device, const RenderPipelineDescriptor* descriptor);
        ~RenderPipelineBase() override;

        static RenderPipelineBase* MakeError(DeviceBase* device);

        const VertexStateDescriptor* GetVertexStateDescriptor() const;
        const std::bitset<kMaxVertexAttributes>& GetAttributeLocationsUsed() const;
        const VertexAttributeInfo& GetAttribute(uint32_t location) const;
        const std::bitset<kMaxVertexBuffers>& GetVertexBufferSlotsUsed() const;
        const VertexBufferInfo& GetVertexBuffer(uint32_t slot) const;

        const ColorStateDescriptor* GetColorStateDescriptor(uint32_t attachmentSlot) const;
        const DepthStencilStateDescriptor* GetDepthStencilStateDescriptor() const;
        wgpu::PrimitiveTopology GetPrimitiveTopology() const;
        wgpu::CullMode GetCullMode() const;
        wgpu::FrontFace GetFrontFace() const;

        std::bitset<kMaxColorAttachments> GetColorAttachmentsMask() const;
        bool HasDepthStencilAttachment() const;
        wgpu::TextureFormat GetColorAttachmentFormat(uint32_t attachment) const;
        wgpu::TextureFormat GetDepthStencilFormat() const;
        uint32_t GetSampleCount() const;
        uint32_t GetSampleMask() const;

        const AttachmentState* GetAttachmentState() const;

        std::bitset<kMaxVertexAttributes> GetAttributesUsingVertexBuffer(uint32_t slot) const;
        std::array<std::bitset<kMaxVertexAttributes>, kMaxVertexBuffers>
            attributesUsingVertexBuffer;

        // Functors necessary for the unordered_set<RenderPipelineBase*>-based cache.
        struct HashFunc {
            size_t operator()(const RenderPipelineBase* pipeline) const;
        };
        struct EqualityFunc {
            bool operator()(const RenderPipelineBase* a, const RenderPipelineBase* b) const;
        };

      private:
        RenderPipelineBase(DeviceBase* device, ObjectBase::ErrorTag tag);

        // Vertex state
        VertexStateDescriptor mVertexState;
        std::bitset<kMaxVertexAttributes> mAttributeLocationsUsed;
        std::array<VertexAttributeInfo, kMaxVertexAttributes> mAttributeInfos;
        std::bitset<kMaxVertexBuffers> mVertexBufferSlotsUsed;
        std::array<VertexBufferInfo, kMaxVertexBuffers> mVertexBufferInfos;

        // Attachments
        Ref<AttachmentState> mAttachmentState;
        DepthStencilStateDescriptor mDepthStencilState;
        std::array<ColorStateDescriptor, kMaxColorAttachments> mColorStates;

        // Other state
        wgpu::PrimitiveTopology mPrimitiveTopology;
        RasterizationStateDescriptor mRasterizationState;
        uint32_t mSampleMask;
        bool mAlphaToCoverageEnabled;

        // Stage information
        // TODO(cwallez@chromium.org): Store a crypto hash of the modules instead.
        Ref<ShaderModuleBase> mVertexModule;
        std::string mVertexEntryPoint;
        Ref<ShaderModuleBase> mFragmentModule;
        std::string mFragmentEntryPoint;
    };

}  // namespace dawn_native

#endif  // DAWNNATIVE_RENDERPIPELINE_H_
