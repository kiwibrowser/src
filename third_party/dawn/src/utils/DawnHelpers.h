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

#ifndef UTILS_DAWNHELPERS_H_
#define UTILS_DAWNHELPERS_H_

#include <dawn/dawncpp.h>

#include <array>
#include <initializer_list>

#include "common/Constants.h"

namespace utils {

    enum Expectation { Success, Failure };

    dawn::ShaderModule CreateShaderModule(const dawn::Device& device,
                                          dawn::ShaderStage stage,
                                          const char* source);
    dawn::ShaderModule CreateShaderModuleFromASM(const dawn::Device& device, const char* source);

    dawn::Buffer CreateBufferFromData(const dawn::Device& device,
                                      const void* data,
                                      uint64_t size,
                                      dawn::BufferUsageBit usage);

    template <typename T>
    dawn::Buffer CreateBufferFromData(const dawn::Device& device,
                                      dawn::BufferUsageBit usage,
                                      std::initializer_list<T> data) {
        return CreateBufferFromData(device, data.begin(), uint32_t(sizeof(T) * data.size()), usage);
    }

    dawn::BufferCopyView CreateBufferCopyView(dawn::Buffer buffer,
                                              uint64_t offset,
                                              uint32_t rowPitch,
                                              uint32_t imageHeight);
    dawn::TextureCopyView CreateTextureCopyView(dawn::Texture texture,
                                                uint32_t level,
                                                uint32_t slice,
                                                dawn::Origin3D origin);

    struct ComboRenderPassDescriptor : public dawn::RenderPassDescriptor {
      public:
        ComboRenderPassDescriptor(std::initializer_list<dawn::TextureView> colorAttachmentInfo,
                                  dawn::TextureView depthStencil = dawn::TextureView());
        const ComboRenderPassDescriptor& operator=(
            const ComboRenderPassDescriptor& otherRenderPass);

        dawn::RenderPassColorAttachmentDescriptor* cColorAttachmentsInfoPtr[kMaxColorAttachments];
        dawn::RenderPassDepthStencilAttachmentDescriptor cDepthStencilAttachmentInfo;

      private:
        std::array<dawn::RenderPassColorAttachmentDescriptor, kMaxColorAttachments>
            mColorAttachmentsInfo;
    };

    struct BasicRenderPass {
      public:
        BasicRenderPass();
        BasicRenderPass(uint32_t width,
                        uint32_t height,
                        dawn::Texture color,
                        dawn::TextureFormat texture);

        uint32_t width;
        uint32_t height;
        dawn::Texture color;
        dawn::TextureFormat colorFormat;
        utils::ComboRenderPassDescriptor renderPassInfo;
    };
    BasicRenderPass CreateBasicRenderPass(const dawn::Device& device,
                                          uint32_t width,
                                          uint32_t height);

    dawn::SamplerDescriptor GetDefaultSamplerDescriptor();
    dawn::PipelineLayout MakeBasicPipelineLayout(const dawn::Device& device,
                                                 const dawn::BindGroupLayout* bindGroupLayout);
    dawn::BindGroupLayout MakeBindGroupLayout(
        const dawn::Device& device,
        std::initializer_list<dawn::BindGroupLayoutBinding> bindingsInitializer);

    // Helpers to make creating bind groups look nicer:
    //
    //   utils::MakeBindGroup(device, layout, {
    //       {0, mySampler},
    //       {1, myBuffer, offset, size},
    //       {3, myTexture}
    //   });

    // Structure with one constructor per-type of bindings, so that the initializer_list accepts
    // bindings with the right type and no extra information.
    struct BindingInitializationHelper {
        BindingInitializationHelper(uint32_t binding, const dawn::Sampler& sampler);
        BindingInitializationHelper(uint32_t binding, const dawn::TextureView& textureView);
        BindingInitializationHelper(uint32_t binding,
                                    const dawn::Buffer& buffer,
                                    uint64_t offset,
                                    uint64_t size);

        dawn::BindGroupBinding GetAsBinding() const;

        uint32_t binding;
        dawn::Sampler sampler;
        dawn::TextureView textureView;
        dawn::Buffer buffer;
        uint64_t offset = 0;
        uint64_t size = 0;
    };

    dawn::BindGroup MakeBindGroup(
        const dawn::Device& device,
        const dawn::BindGroupLayout& layout,
        std::initializer_list<BindingInitializationHelper> bindingsInitializer);

}  // namespace utils

#endif  // UTILS_DAWNHELPERS_H_
