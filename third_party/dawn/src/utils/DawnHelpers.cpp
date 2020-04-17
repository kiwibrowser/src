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

#include "utils/DawnHelpers.h"

#include "common/Assert.h"
#include "common/Constants.h"

#include <shaderc/shaderc.hpp>

#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace utils {

    namespace {

        shaderc_shader_kind ShadercShaderKind(dawn::ShaderStage stage) {
            switch (stage) {
                case dawn::ShaderStage::Vertex:
                    return shaderc_glsl_vertex_shader;
                case dawn::ShaderStage::Fragment:
                    return shaderc_glsl_fragment_shader;
                case dawn::ShaderStage::Compute:
                    return shaderc_glsl_compute_shader;
                default:
                    UNREACHABLE();
            }
        }

        dawn::ShaderModule CreateShaderModuleFromResult(
            const dawn::Device& device,
            const shaderc::SpvCompilationResult& result) {
            // result.cend and result.cbegin return pointers to uint32_t.
            const uint32_t* resultBegin = result.cbegin();
            const uint32_t* resultEnd = result.cend();
            // So this size is in units of sizeof(uint32_t).
            ptrdiff_t resultSize = resultEnd - resultBegin;
            // SetSource takes data as uint32_t*.

            dawn::ShaderModuleDescriptor descriptor;
            descriptor.codeSize = static_cast<uint32_t>(resultSize);
            descriptor.code = result.cbegin();
            return device.CreateShaderModule(&descriptor);
        }

    }  // anonymous namespace

    dawn::ShaderModule CreateShaderModule(const dawn::Device& device,
                                          dawn::ShaderStage stage,
                                          const char* source) {
        shaderc_shader_kind kind = ShadercShaderKind(stage);

        shaderc::Compiler compiler;
        auto result = compiler.CompileGlslToSpv(source, strlen(source), kind, "myshader?");
        if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
            std::cerr << result.GetErrorMessage();
            return {};
        }
#ifdef DUMP_SPIRV_ASSEMBLY
        {
            shaderc::CompileOptions options;
            auto resultAsm = compiler.CompileGlslToSpvAssembly(source, strlen(source), kind,
                                                               "myshader?", options);
            size_t sizeAsm = (resultAsm.cend() - resultAsm.cbegin());

            char* buffer = reinterpret_cast<char*>(malloc(sizeAsm + 1));
            memcpy(buffer, resultAsm.cbegin(), sizeAsm);
            buffer[sizeAsm] = '\0';
            printf("SPIRV ASSEMBLY DUMP START\n%s\nSPIRV ASSEMBLY DUMP END\n", buffer);
            free(buffer);
        }
#endif

#ifdef DUMP_SPIRV_JS_ARRAY
        printf("SPIRV JS ARRAY DUMP START\n");
        for (size_t i = 0; i < size; i++) {
            printf("%#010x", result.cbegin()[i]);
            if ((i + 1) % 4 == 0) {
                printf(",\n");
            } else {
                printf(", ");
            }
        }
        printf("\n");
        printf("SPIRV JS ARRAY DUMP END\n");
#endif

        return CreateShaderModuleFromResult(device, result);
    }

    dawn::ShaderModule CreateShaderModuleFromASM(const dawn::Device& device, const char* source) {
        shaderc::Compiler compiler;
        shaderc::SpvCompilationResult result = compiler.AssembleToSpv(source, strlen(source));
        if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
            std::cerr << result.GetErrorMessage();
            return {};
        }

        return CreateShaderModuleFromResult(device, result);
    }

    dawn::Buffer CreateBufferFromData(const dawn::Device& device,
                                      const void* data,
                                      uint64_t size,
                                      dawn::BufferUsageBit usage) {
        dawn::BufferDescriptor descriptor;
        descriptor.size = size;
        descriptor.usage = usage | dawn::BufferUsageBit::TransferDst;

        dawn::Buffer buffer = device.CreateBuffer(&descriptor);
        buffer.SetSubData(0, size, data);
        return buffer;
    }

    ComboRenderPassDescriptor::ComboRenderPassDescriptor(
        std::initializer_list<dawn::TextureView> colorAttachmentInfo,
        dawn::TextureView depthStencil)
        : cColorAttachmentsInfoPtr() {
        for (uint32_t i = 0; i < kMaxColorAttachments; ++i) {
            mColorAttachmentsInfo[i].loadOp = dawn::LoadOp::Clear;
            mColorAttachmentsInfo[i].storeOp = dawn::StoreOp::Store;
            mColorAttachmentsInfo[i].clearColor = {0.0f, 0.0f, 0.0f, 0.0f};
            cColorAttachmentsInfoPtr[i] = nullptr;
        }

        cDepthStencilAttachmentInfo.clearDepth = 1.0f;
        cDepthStencilAttachmentInfo.clearStencil = 0;
        cDepthStencilAttachmentInfo.depthLoadOp = dawn::LoadOp::Clear;
        cDepthStencilAttachmentInfo.depthStoreOp = dawn::StoreOp::Store;
        cDepthStencilAttachmentInfo.stencilLoadOp = dawn::LoadOp::Clear;
        cDepthStencilAttachmentInfo.stencilStoreOp = dawn::StoreOp::Store;

        colorAttachmentCount = static_cast<uint32_t>(colorAttachmentInfo.size());
        uint32_t colorAttachmentIndex = 0;
        for (const dawn::TextureView& colorAttachment : colorAttachmentInfo) {
            if (colorAttachment.Get() != nullptr) {
                mColorAttachmentsInfo[colorAttachmentIndex].attachment = colorAttachment;
                cColorAttachmentsInfoPtr[colorAttachmentIndex] =
                    &mColorAttachmentsInfo[colorAttachmentIndex];
            }
            ++colorAttachmentIndex;
        }
        colorAttachments = cColorAttachmentsInfoPtr;

        if (depthStencil.Get() != nullptr) {
            cDepthStencilAttachmentInfo.attachment = depthStencil;
            depthStencilAttachment = &cDepthStencilAttachmentInfo;
        } else {
            depthStencilAttachment = nullptr;
        }
    }

    const ComboRenderPassDescriptor& ComboRenderPassDescriptor::operator=(
        const ComboRenderPassDescriptor& otherRenderPass) {
        cDepthStencilAttachmentInfo = otherRenderPass.cDepthStencilAttachmentInfo;
        mColorAttachmentsInfo = otherRenderPass.mColorAttachmentsInfo;

        colorAttachmentCount = otherRenderPass.colorAttachmentCount;

        // Assign the pointers in colorAttachmentsInfoPtr to items in this->mColorAttachmentsInfo
        for (uint32_t i = 0; i < colorAttachmentCount; ++i) {
            if (otherRenderPass.cColorAttachmentsInfoPtr[i] != nullptr) {
                cColorAttachmentsInfoPtr[i] = &mColorAttachmentsInfo[i];
            } else {
                cColorAttachmentsInfoPtr[i] = nullptr;
            }
        }
        colorAttachments = cColorAttachmentsInfoPtr;

        if (otherRenderPass.depthStencilAttachment != nullptr) {
            // Assign desc.depthStencilAttachment to this->depthStencilAttachmentInfo;
            depthStencilAttachment = &cDepthStencilAttachmentInfo;
        } else {
            depthStencilAttachment = nullptr;
        }

        return *this;
    }

    BasicRenderPass::BasicRenderPass()
        : width(0),
          height(0),
          color(nullptr),
          colorFormat(dawn::TextureFormat::R8G8B8A8Unorm),
          renderPassInfo({}) {
    }

    BasicRenderPass::BasicRenderPass(uint32_t texWidth,
                                     uint32_t texHeight,
                                     dawn::Texture colorAttachment,
                                     dawn::TextureFormat textureFormat)
        : width(texWidth),
          height(texHeight),
          color(colorAttachment),
          colorFormat(textureFormat),
          renderPassInfo({colorAttachment.CreateDefaultView()}) {
    }

    BasicRenderPass CreateBasicRenderPass(const dawn::Device& device,
                                          uint32_t width,
                                          uint32_t height) {
        DAWN_ASSERT(width > 0 && height > 0);

        dawn::TextureFormat kColorFormat = dawn::TextureFormat::R8G8B8A8Unorm;

        dawn::TextureDescriptor descriptor;
        descriptor.dimension = dawn::TextureDimension::e2D;
        descriptor.size.width = width;
        descriptor.size.height = height;
        descriptor.size.depth = 1;
        descriptor.arrayLayerCount = 1;
        descriptor.sampleCount = 1;
        descriptor.format = kColorFormat;
        descriptor.mipLevelCount = 1;
        descriptor.usage =
            dawn::TextureUsageBit::OutputAttachment | dawn::TextureUsageBit::TransferSrc;
        dawn::Texture color = device.CreateTexture(&descriptor);

        return BasicRenderPass(width, height, color, kColorFormat);
    }

    dawn::BufferCopyView CreateBufferCopyView(dawn::Buffer buffer,
                                              uint64_t offset,
                                              uint32_t rowPitch,
                                              uint32_t imageHeight) {
        dawn::BufferCopyView bufferCopyView;
        bufferCopyView.buffer = buffer;
        bufferCopyView.offset = offset;
        bufferCopyView.rowPitch = rowPitch;
        bufferCopyView.imageHeight = imageHeight;

        return bufferCopyView;
    }

    dawn::TextureCopyView CreateTextureCopyView(dawn::Texture texture,
                                                uint32_t level,
                                                uint32_t slice,
                                                dawn::Origin3D origin) {
        dawn::TextureCopyView textureCopyView;
        textureCopyView.texture = texture;
        textureCopyView.level = level;
        textureCopyView.slice = slice;
        textureCopyView.origin = origin;

        return textureCopyView;
    }

    dawn::SamplerDescriptor GetDefaultSamplerDescriptor() {
        dawn::SamplerDescriptor desc;

        desc.minFilter = dawn::FilterMode::Linear;
        desc.magFilter = dawn::FilterMode::Linear;
        desc.mipmapFilter = dawn::FilterMode::Linear;
        desc.addressModeU = dawn::AddressMode::Repeat;
        desc.addressModeV = dawn::AddressMode::Repeat;
        desc.addressModeW = dawn::AddressMode::Repeat;
        desc.lodMinClamp = kLodMin;
        desc.lodMaxClamp = kLodMax;
        desc.compareFunction = dawn::CompareFunction::Never;

        return desc;
    }

    dawn::PipelineLayout MakeBasicPipelineLayout(const dawn::Device& device,
                                                 const dawn::BindGroupLayout* bindGroupLayout) {
        dawn::PipelineLayoutDescriptor descriptor;
        if (bindGroupLayout != nullptr) {
            descriptor.bindGroupLayoutCount = 1;
            descriptor.bindGroupLayouts = bindGroupLayout;
        } else {
            descriptor.bindGroupLayoutCount = 0;
            descriptor.bindGroupLayouts = nullptr;
        }
        return device.CreatePipelineLayout(&descriptor);
    }

    dawn::BindGroupLayout MakeBindGroupLayout(
        const dawn::Device& device,
        std::initializer_list<dawn::BindGroupLayoutBinding> bindingsInitializer) {
        constexpr dawn::ShaderStageBit kNoStages{};

        std::vector<dawn::BindGroupLayoutBinding> bindings;
        for (const dawn::BindGroupLayoutBinding& binding : bindingsInitializer) {
            if (binding.visibility != kNoStages) {
                bindings.push_back(binding);
            }
        }

        dawn::BindGroupLayoutDescriptor descriptor;
        descriptor.bindingCount = static_cast<uint32_t>(bindings.size());
        descriptor.bindings = bindings.data();
        return device.CreateBindGroupLayout(&descriptor);
    }

    BindingInitializationHelper::BindingInitializationHelper(uint32_t binding,
                                                             const dawn::Sampler& sampler)
        : binding(binding), sampler(sampler) {
    }

    BindingInitializationHelper::BindingInitializationHelper(uint32_t binding,
                                                             const dawn::TextureView& textureView)
        : binding(binding), textureView(textureView) {
    }

    BindingInitializationHelper::BindingInitializationHelper(uint32_t binding,
                                                             const dawn::Buffer& buffer,
                                                             uint64_t offset,
                                                             uint64_t size)
        : binding(binding), buffer(buffer), offset(offset), size(size) {
    }

    dawn::BindGroupBinding BindingInitializationHelper::GetAsBinding() const {
        dawn::BindGroupBinding result;

        result.binding = binding;
        result.sampler = sampler;
        result.textureView = textureView;
        result.buffer = buffer;
        result.offset = offset;
        result.size = size;

        return result;
    }

    dawn::BindGroup MakeBindGroup(
        const dawn::Device& device,
        const dawn::BindGroupLayout& layout,
        std::initializer_list<BindingInitializationHelper> bindingsInitializer) {
        std::vector<dawn::BindGroupBinding> bindings;
        for (const BindingInitializationHelper& helper : bindingsInitializer) {
            bindings.push_back(helper.GetAsBinding());
        }

        dawn::BindGroupDescriptor descriptor;
        descriptor.layout = layout;
        descriptor.bindingCount = bindings.size();
        descriptor.bindings = bindings.data();

        return device.CreateBindGroup(&descriptor);
    }

}  // namespace utils
