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

#include "dawn_native/vulkan/RenderPipelineVk.h"

#include "dawn_native/vulkan/DeviceVk.h"
#include "dawn_native/vulkan/FencedDeleter.h"
#include "dawn_native/vulkan/PipelineLayoutVk.h"
#include "dawn_native/vulkan/RenderPassCache.h"
#include "dawn_native/vulkan/ShaderModuleVk.h"
#include "dawn_native/vulkan/TextureVk.h"
#include "dawn_native/vulkan/UtilsVulkan.h"
#include "dawn_native/vulkan/VulkanError.h"

namespace dawn_native { namespace vulkan {

    namespace {

        VkVertexInputRate VulkanInputRate(wgpu::InputStepMode stepMode) {
            switch (stepMode) {
                case wgpu::InputStepMode::Vertex:
                    return VK_VERTEX_INPUT_RATE_VERTEX;
                case wgpu::InputStepMode::Instance:
                    return VK_VERTEX_INPUT_RATE_INSTANCE;
                default:
                    UNREACHABLE();
            }
        }

        VkFormat VulkanVertexFormat(wgpu::VertexFormat format) {
            switch (format) {
                case wgpu::VertexFormat::UChar2:
                    return VK_FORMAT_R8G8_UINT;
                case wgpu::VertexFormat::UChar4:
                    return VK_FORMAT_R8G8B8A8_UINT;
                case wgpu::VertexFormat::Char2:
                    return VK_FORMAT_R8G8_SINT;
                case wgpu::VertexFormat::Char4:
                    return VK_FORMAT_R8G8B8A8_SINT;
                case wgpu::VertexFormat::UChar2Norm:
                    return VK_FORMAT_R8G8_UNORM;
                case wgpu::VertexFormat::UChar4Norm:
                    return VK_FORMAT_R8G8B8A8_UNORM;
                case wgpu::VertexFormat::Char2Norm:
                    return VK_FORMAT_R8G8_SNORM;
                case wgpu::VertexFormat::Char4Norm:
                    return VK_FORMAT_R8G8B8A8_SNORM;
                case wgpu::VertexFormat::UShort2:
                    return VK_FORMAT_R16G16_UINT;
                case wgpu::VertexFormat::UShort4:
                    return VK_FORMAT_R16G16B16A16_UINT;
                case wgpu::VertexFormat::Short2:
                    return VK_FORMAT_R16G16_SINT;
                case wgpu::VertexFormat::Short4:
                    return VK_FORMAT_R16G16B16A16_SINT;
                case wgpu::VertexFormat::UShort2Norm:
                    return VK_FORMAT_R16G16_UNORM;
                case wgpu::VertexFormat::UShort4Norm:
                    return VK_FORMAT_R16G16B16A16_UNORM;
                case wgpu::VertexFormat::Short2Norm:
                    return VK_FORMAT_R16G16_SNORM;
                case wgpu::VertexFormat::Short4Norm:
                    return VK_FORMAT_R16G16B16A16_SNORM;
                case wgpu::VertexFormat::Half2:
                    return VK_FORMAT_R16G16_SFLOAT;
                case wgpu::VertexFormat::Half4:
                    return VK_FORMAT_R16G16B16A16_SFLOAT;
                case wgpu::VertexFormat::Float:
                    return VK_FORMAT_R32_SFLOAT;
                case wgpu::VertexFormat::Float2:
                    return VK_FORMAT_R32G32_SFLOAT;
                case wgpu::VertexFormat::Float3:
                    return VK_FORMAT_R32G32B32_SFLOAT;
                case wgpu::VertexFormat::Float4:
                    return VK_FORMAT_R32G32B32A32_SFLOAT;
                case wgpu::VertexFormat::UInt:
                    return VK_FORMAT_R32_UINT;
                case wgpu::VertexFormat::UInt2:
                    return VK_FORMAT_R32G32_UINT;
                case wgpu::VertexFormat::UInt3:
                    return VK_FORMAT_R32G32B32_UINT;
                case wgpu::VertexFormat::UInt4:
                    return VK_FORMAT_R32G32B32A32_UINT;
                case wgpu::VertexFormat::Int:
                    return VK_FORMAT_R32_SINT;
                case wgpu::VertexFormat::Int2:
                    return VK_FORMAT_R32G32_SINT;
                case wgpu::VertexFormat::Int3:
                    return VK_FORMAT_R32G32B32_SINT;
                case wgpu::VertexFormat::Int4:
                    return VK_FORMAT_R32G32B32A32_SINT;
                default:
                    UNREACHABLE();
            }
        }

        VkPrimitiveTopology VulkanPrimitiveTopology(wgpu::PrimitiveTopology topology) {
            switch (topology) {
                case wgpu::PrimitiveTopology::PointList:
                    return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
                case wgpu::PrimitiveTopology::LineList:
                    return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
                case wgpu::PrimitiveTopology::LineStrip:
                    return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
                case wgpu::PrimitiveTopology::TriangleList:
                    return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
                case wgpu::PrimitiveTopology::TriangleStrip:
                    return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
                default:
                    UNREACHABLE();
            }
        }

        bool ShouldEnablePrimitiveRestart(wgpu::PrimitiveTopology topology) {
            // Primitive restart is always enabled in WebGPU but Vulkan validation rules ask that
            // primitive restart be only enabled on primitive topologies that support restarting.
            switch (topology) {
                case wgpu::PrimitiveTopology::PointList:
                case wgpu::PrimitiveTopology::LineList:
                case wgpu::PrimitiveTopology::TriangleList:
                    return false;
                case wgpu::PrimitiveTopology::LineStrip:
                case wgpu::PrimitiveTopology::TriangleStrip:
                    return true;
                default:
                    UNREACHABLE();
            }
        }

        VkFrontFace VulkanFrontFace(wgpu::FrontFace face) {
            switch (face) {
                case wgpu::FrontFace::CCW:
                    return VK_FRONT_FACE_COUNTER_CLOCKWISE;
                case wgpu::FrontFace::CW:
                    return VK_FRONT_FACE_CLOCKWISE;
                default:
                    UNREACHABLE();
            }
        }

        VkCullModeFlagBits VulkanCullMode(wgpu::CullMode mode) {
            switch (mode) {
                case wgpu::CullMode::None:
                    return VK_CULL_MODE_NONE;
                case wgpu::CullMode::Front:
                    return VK_CULL_MODE_FRONT_BIT;
                case wgpu::CullMode::Back:
                    return VK_CULL_MODE_BACK_BIT;
                default:
                    UNREACHABLE();
            }
        }

        VkBlendFactor VulkanBlendFactor(wgpu::BlendFactor factor) {
            switch (factor) {
                case wgpu::BlendFactor::Zero:
                    return VK_BLEND_FACTOR_ZERO;
                case wgpu::BlendFactor::One:
                    return VK_BLEND_FACTOR_ONE;
                case wgpu::BlendFactor::SrcColor:
                    return VK_BLEND_FACTOR_SRC_COLOR;
                case wgpu::BlendFactor::OneMinusSrcColor:
                    return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
                case wgpu::BlendFactor::SrcAlpha:
                    return VK_BLEND_FACTOR_SRC_ALPHA;
                case wgpu::BlendFactor::OneMinusSrcAlpha:
                    return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                case wgpu::BlendFactor::DstColor:
                    return VK_BLEND_FACTOR_DST_COLOR;
                case wgpu::BlendFactor::OneMinusDstColor:
                    return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
                case wgpu::BlendFactor::DstAlpha:
                    return VK_BLEND_FACTOR_DST_ALPHA;
                case wgpu::BlendFactor::OneMinusDstAlpha:
                    return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
                case wgpu::BlendFactor::SrcAlphaSaturated:
                    return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
                case wgpu::BlendFactor::BlendColor:
                    return VK_BLEND_FACTOR_CONSTANT_COLOR;
                case wgpu::BlendFactor::OneMinusBlendColor:
                    return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
                default:
                    UNREACHABLE();
            }
        }

        VkBlendOp VulkanBlendOperation(wgpu::BlendOperation operation) {
            switch (operation) {
                case wgpu::BlendOperation::Add:
                    return VK_BLEND_OP_ADD;
                case wgpu::BlendOperation::Subtract:
                    return VK_BLEND_OP_SUBTRACT;
                case wgpu::BlendOperation::ReverseSubtract:
                    return VK_BLEND_OP_REVERSE_SUBTRACT;
                case wgpu::BlendOperation::Min:
                    return VK_BLEND_OP_MIN;
                case wgpu::BlendOperation::Max:
                    return VK_BLEND_OP_MAX;
                default:
                    UNREACHABLE();
            }
        }

        VkColorComponentFlags VulkanColorWriteMask(wgpu::ColorWriteMask mask,
                                                   bool isDeclaredInFragmentShader) {
            // Vulkan and Dawn color write masks match, static assert it and return the mask
            static_assert(static_cast<VkColorComponentFlagBits>(wgpu::ColorWriteMask::Red) ==
                              VK_COLOR_COMPONENT_R_BIT,
                          "");
            static_assert(static_cast<VkColorComponentFlagBits>(wgpu::ColorWriteMask::Green) ==
                              VK_COLOR_COMPONENT_G_BIT,
                          "");
            static_assert(static_cast<VkColorComponentFlagBits>(wgpu::ColorWriteMask::Blue) ==
                              VK_COLOR_COMPONENT_B_BIT,
                          "");
            static_assert(static_cast<VkColorComponentFlagBits>(wgpu::ColorWriteMask::Alpha) ==
                              VK_COLOR_COMPONENT_A_BIT,
                          "");

            // According to Vulkan SPEC (Chapter 14.3): "The input values to blending or color
            // attachment writes are undefined for components which do not correspond to a fragment
            // shader outputs", we set the color write mask to 0 to prevent such undefined values
            // being written into the color attachments.
            return isDeclaredInFragmentShader ? static_cast<VkColorComponentFlags>(mask)
                                              : static_cast<VkColorComponentFlags>(0);
        }

        VkPipelineColorBlendAttachmentState ComputeColorDesc(const ColorStateDescriptor* descriptor,
                                                             bool isDeclaredInFragmentShader) {
            VkPipelineColorBlendAttachmentState attachment;
            attachment.blendEnable = BlendEnabled(descriptor) ? VK_TRUE : VK_FALSE;
            attachment.srcColorBlendFactor = VulkanBlendFactor(descriptor->colorBlend.srcFactor);
            attachment.dstColorBlendFactor = VulkanBlendFactor(descriptor->colorBlend.dstFactor);
            attachment.colorBlendOp = VulkanBlendOperation(descriptor->colorBlend.operation);
            attachment.srcAlphaBlendFactor = VulkanBlendFactor(descriptor->alphaBlend.srcFactor);
            attachment.dstAlphaBlendFactor = VulkanBlendFactor(descriptor->alphaBlend.dstFactor);
            attachment.alphaBlendOp = VulkanBlendOperation(descriptor->alphaBlend.operation);
            attachment.colorWriteMask =
                VulkanColorWriteMask(descriptor->writeMask, isDeclaredInFragmentShader);
            return attachment;
        }

        VkStencilOp VulkanStencilOp(wgpu::StencilOperation op) {
            switch (op) {
                case wgpu::StencilOperation::Keep:
                    return VK_STENCIL_OP_KEEP;
                case wgpu::StencilOperation::Zero:
                    return VK_STENCIL_OP_ZERO;
                case wgpu::StencilOperation::Replace:
                    return VK_STENCIL_OP_REPLACE;
                case wgpu::StencilOperation::IncrementClamp:
                    return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
                case wgpu::StencilOperation::DecrementClamp:
                    return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
                case wgpu::StencilOperation::Invert:
                    return VK_STENCIL_OP_INVERT;
                case wgpu::StencilOperation::IncrementWrap:
                    return VK_STENCIL_OP_INCREMENT_AND_WRAP;
                case wgpu::StencilOperation::DecrementWrap:
                    return VK_STENCIL_OP_DECREMENT_AND_WRAP;
                default:
                    UNREACHABLE();
            }
        }

        VkPipelineDepthStencilStateCreateInfo ComputeDepthStencilDesc(
            const DepthStencilStateDescriptor* descriptor) {
            VkPipelineDepthStencilStateCreateInfo depthStencilState;
            depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            depthStencilState.pNext = nullptr;
            depthStencilState.flags = 0;

            // Depth writes only occur if depth is enabled
            depthStencilState.depthTestEnable =
                (descriptor->depthCompare == wgpu::CompareFunction::Always &&
                 !descriptor->depthWriteEnabled)
                    ? VK_FALSE
                    : VK_TRUE;
            depthStencilState.depthWriteEnable = descriptor->depthWriteEnabled ? VK_TRUE : VK_FALSE;
            depthStencilState.depthCompareOp = ToVulkanCompareOp(descriptor->depthCompare);
            depthStencilState.depthBoundsTestEnable = false;
            depthStencilState.minDepthBounds = 0.0f;
            depthStencilState.maxDepthBounds = 1.0f;

            depthStencilState.stencilTestEnable =
                StencilTestEnabled(descriptor) ? VK_TRUE : VK_FALSE;

            depthStencilState.front.failOp = VulkanStencilOp(descriptor->stencilFront.failOp);
            depthStencilState.front.passOp = VulkanStencilOp(descriptor->stencilFront.passOp);
            depthStencilState.front.depthFailOp =
                VulkanStencilOp(descriptor->stencilFront.depthFailOp);
            depthStencilState.front.compareOp = ToVulkanCompareOp(descriptor->stencilFront.compare);

            depthStencilState.back.failOp = VulkanStencilOp(descriptor->stencilBack.failOp);
            depthStencilState.back.passOp = VulkanStencilOp(descriptor->stencilBack.passOp);
            depthStencilState.back.depthFailOp =
                VulkanStencilOp(descriptor->stencilBack.depthFailOp);
            depthStencilState.back.compareOp = ToVulkanCompareOp(descriptor->stencilBack.compare);

            // Dawn doesn't have separate front and back stencil masks.
            depthStencilState.front.compareMask = descriptor->stencilReadMask;
            depthStencilState.back.compareMask = descriptor->stencilReadMask;
            depthStencilState.front.writeMask = descriptor->stencilWriteMask;
            depthStencilState.back.writeMask = descriptor->stencilWriteMask;

            // The stencil reference is always dynamic
            depthStencilState.front.reference = 0;
            depthStencilState.back.reference = 0;

            return depthStencilState;
        }

    }  // anonymous namespace

    // static
    ResultOrError<RenderPipeline*> RenderPipeline::Create(
        Device* device,
        const RenderPipelineDescriptor* descriptor) {
        Ref<RenderPipeline> pipeline = AcquireRef(new RenderPipeline(device, descriptor));
        DAWN_TRY(pipeline->Initialize(descriptor));
        return pipeline.Detach();
    }

    MaybeError RenderPipeline::Initialize(const RenderPipelineDescriptor* descriptor) {
        Device* device = ToBackend(GetDevice());

        VkPipelineShaderStageCreateInfo shaderStages[2];
        {
            shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStages[0].pNext = nullptr;
            shaderStages[0].flags = 0;
            shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
            shaderStages[0].pSpecializationInfo = nullptr;
            shaderStages[0].module = ToBackend(descriptor->vertexStage.module)->GetHandle();
            shaderStages[0].pName = descriptor->vertexStage.entryPoint;

            shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStages[1].pNext = nullptr;
            shaderStages[1].flags = 0;
            shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            shaderStages[1].pSpecializationInfo = nullptr;
            shaderStages[1].module = ToBackend(descriptor->fragmentStage->module)->GetHandle();
            shaderStages[1].pName = descriptor->fragmentStage->entryPoint;
        }

        PipelineVertexInputStateCreateInfoTemporaryAllocations tempAllocations;
        VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo =
            ComputeVertexInputDesc(&tempAllocations);

        VkPipelineInputAssemblyStateCreateInfo inputAssembly;
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.pNext = nullptr;
        inputAssembly.flags = 0;
        inputAssembly.topology = VulkanPrimitiveTopology(GetPrimitiveTopology());
        inputAssembly.primitiveRestartEnable = ShouldEnablePrimitiveRestart(GetPrimitiveTopology());

        // A dummy viewport/scissor info. The validation layers force use to provide at least one
        // scissor and one viewport here, even if we choose to make them dynamic.
        VkViewport viewportDesc;
        viewportDesc.x = 0.0f;
        viewportDesc.y = 0.0f;
        viewportDesc.width = 1.0f;
        viewportDesc.height = 1.0f;
        viewportDesc.minDepth = 0.0f;
        viewportDesc.maxDepth = 1.0f;
        VkRect2D scissorRect;
        scissorRect.offset.x = 0;
        scissorRect.offset.y = 0;
        scissorRect.extent.width = 1;
        scissorRect.extent.height = 1;
        VkPipelineViewportStateCreateInfo viewport;
        viewport.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport.pNext = nullptr;
        viewport.flags = 0;
        viewport.viewportCount = 1;
        viewport.pViewports = &viewportDesc;
        viewport.scissorCount = 1;
        viewport.pScissors = &scissorRect;

        VkPipelineRasterizationStateCreateInfo rasterization;
        rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterization.pNext = nullptr;
        rasterization.flags = 0;
        rasterization.depthClampEnable = VK_FALSE;
        rasterization.rasterizerDiscardEnable = VK_FALSE;
        rasterization.polygonMode = VK_POLYGON_MODE_FILL;
        rasterization.cullMode = VulkanCullMode(GetCullMode());
        rasterization.frontFace = VulkanFrontFace(GetFrontFace());
        rasterization.depthBiasEnable = VK_FALSE;
        rasterization.depthBiasConstantFactor = 0.0f;
        rasterization.depthBiasClamp = 0.0f;
        rasterization.depthBiasSlopeFactor = 0.0f;
        rasterization.lineWidth = 1.0f;

        VkPipelineMultisampleStateCreateInfo multisample;
        multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisample.pNext = nullptr;
        multisample.flags = 0;
        multisample.rasterizationSamples = VulkanSampleCount(GetSampleCount());
        multisample.sampleShadingEnable = VK_FALSE;
        multisample.minSampleShading = 0.0f;
        // VkPipelineMultisampleStateCreateInfo.pSampleMask is an array of length
        // ceil(rasterizationSamples / 32) and since we're passing a single uint32_t
        // we have to assert that this length is indeed 1.
        ASSERT(multisample.rasterizationSamples <= 32);
        VkSampleMask sampleMask = GetSampleMask();
        multisample.pSampleMask = &sampleMask;
        multisample.alphaToCoverageEnable = VK_FALSE;
        multisample.alphaToOneEnable = VK_FALSE;

        VkPipelineDepthStencilStateCreateInfo depthStencilState =
            ComputeDepthStencilDesc(GetDepthStencilStateDescriptor());

        // Initialize the "blend state info" that will be chained in the "create info" from the data
        // pre-computed in the ColorState
        std::array<VkPipelineColorBlendAttachmentState, kMaxColorAttachments> colorBlendAttachments;
        const ShaderModuleBase::FragmentOutputBaseTypes& fragmentOutputBaseTypes =
            descriptor->fragmentStage->module->GetFragmentOutputBaseTypes();
        for (uint32_t i : IterateBitSet(GetColorAttachmentsMask())) {
            const ColorStateDescriptor* colorStateDescriptor = GetColorStateDescriptor(i);
            bool isDeclaredInFragmentShader = fragmentOutputBaseTypes[i] != Format::Other;
            colorBlendAttachments[i] =
                ComputeColorDesc(colorStateDescriptor, isDeclaredInFragmentShader);
        }
        VkPipelineColorBlendStateCreateInfo colorBlend;
        colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlend.pNext = nullptr;
        colorBlend.flags = 0;
        // LogicOp isn't supported so we disable it.
        colorBlend.logicOpEnable = VK_FALSE;
        colorBlend.logicOp = VK_LOGIC_OP_CLEAR;
        // TODO(cwallez@chromium.org): Do we allow holes in the color attachments?
        colorBlend.attachmentCount = static_cast<uint32_t>(GetColorAttachmentsMask().count());
        colorBlend.pAttachments = colorBlendAttachments.data();
        // The blend constant is always dynamic so we fill in a dummy value
        colorBlend.blendConstants[0] = 0.0f;
        colorBlend.blendConstants[1] = 0.0f;
        colorBlend.blendConstants[2] = 0.0f;
        colorBlend.blendConstants[3] = 0.0f;

        // Tag all state as dynamic but stencil masks.
        VkDynamicState dynamicStates[] = {
            VK_DYNAMIC_STATE_VIEWPORT,          VK_DYNAMIC_STATE_SCISSOR,
            VK_DYNAMIC_STATE_LINE_WIDTH,        VK_DYNAMIC_STATE_DEPTH_BIAS,
            VK_DYNAMIC_STATE_BLEND_CONSTANTS,   VK_DYNAMIC_STATE_DEPTH_BOUNDS,
            VK_DYNAMIC_STATE_STENCIL_REFERENCE,
        };
        VkPipelineDynamicStateCreateInfo dynamic;
        dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic.pNext = nullptr;
        dynamic.flags = 0;
        dynamic.dynamicStateCount = sizeof(dynamicStates) / sizeof(dynamicStates[0]);
        dynamic.pDynamicStates = dynamicStates;

        // Get a VkRenderPass that matches the attachment formats for this pipeline, load ops don't
        // matter so set them all to LoadOp::Load
        VkRenderPass renderPass = VK_NULL_HANDLE;
        {
            RenderPassCacheQuery query;

            for (uint32_t i : IterateBitSet(GetColorAttachmentsMask())) {
                query.SetColor(i, GetColorAttachmentFormat(i), wgpu::LoadOp::Load, false);
            }

            if (HasDepthStencilAttachment()) {
                query.SetDepthStencil(GetDepthStencilFormat(), wgpu::LoadOp::Load,
                                      wgpu::LoadOp::Load);
            }

            query.SetSampleCount(GetSampleCount());

            DAWN_TRY_ASSIGN(renderPass, device->GetRenderPassCache()->GetRenderPass(query));
        }

        // The create info chains in a bunch of things created on the stack here or inside state
        // objects.
        VkGraphicsPipelineCreateInfo createInfo;
        createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.stageCount = 2;
        createInfo.pStages = shaderStages;
        createInfo.pVertexInputState = &vertexInputCreateInfo;
        createInfo.pInputAssemblyState = &inputAssembly;
        createInfo.pTessellationState = nullptr;
        createInfo.pViewportState = &viewport;
        createInfo.pRasterizationState = &rasterization;
        createInfo.pMultisampleState = &multisample;
        createInfo.pDepthStencilState = &depthStencilState;
        createInfo.pColorBlendState = &colorBlend;
        createInfo.pDynamicState = &dynamic;
        createInfo.layout = ToBackend(GetLayout())->GetHandle();
        createInfo.renderPass = renderPass;
        createInfo.subpass = 0;
        createInfo.basePipelineHandle = VkPipeline{};
        createInfo.basePipelineIndex = -1;

        return CheckVkSuccess(
            device->fn.CreateGraphicsPipelines(device->GetVkDevice(), VkPipelineCache{}, 1,
                                               &createInfo, nullptr, &*mHandle),
            "CreateGraphicsPipeline");
    }

    VkPipelineVertexInputStateCreateInfo RenderPipeline::ComputeVertexInputDesc(
        PipelineVertexInputStateCreateInfoTemporaryAllocations* tempAllocations) {
        // Fill in the "binding info" that will be chained in the create info
        uint32_t bindingCount = 0;
        for (uint32_t i : IterateBitSet(GetVertexBufferSlotsUsed())) {
            const VertexBufferInfo& bindingInfo = GetVertexBuffer(i);

            VkVertexInputBindingDescription* bindingDesc = &tempAllocations->bindings[bindingCount];
            bindingDesc->binding = i;
            bindingDesc->stride = bindingInfo.arrayStride;
            bindingDesc->inputRate = VulkanInputRate(bindingInfo.stepMode);

            bindingCount++;
        }

        // Fill in the "attribute info" that will be chained in the create info
        uint32_t attributeCount = 0;
        for (uint32_t i : IterateBitSet(GetAttributeLocationsUsed())) {
            const VertexAttributeInfo& attributeInfo = GetAttribute(i);

            VkVertexInputAttributeDescription* attributeDesc =
                &tempAllocations->attributes[attributeCount];
            attributeDesc->location = i;
            attributeDesc->binding = attributeInfo.vertexBufferSlot;
            attributeDesc->format = VulkanVertexFormat(attributeInfo.format);
            attributeDesc->offset = attributeInfo.offset;

            attributeCount++;
        }

        // Build the create info
        VkPipelineVertexInputStateCreateInfo mCreateInfo;
        mCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        mCreateInfo.pNext = nullptr;
        mCreateInfo.flags = 0;
        mCreateInfo.vertexBindingDescriptionCount = bindingCount;
        mCreateInfo.pVertexBindingDescriptions = tempAllocations->bindings.data();
        mCreateInfo.vertexAttributeDescriptionCount = attributeCount;
        mCreateInfo.pVertexAttributeDescriptions = tempAllocations->attributes.data();
        return mCreateInfo;
    }

    RenderPipeline::~RenderPipeline() {
        if (mHandle != VK_NULL_HANDLE) {
            ToBackend(GetDevice())->GetFencedDeleter()->DeleteWhenUnused(mHandle);
            mHandle = VK_NULL_HANDLE;
        }
    }

    VkPipeline RenderPipeline::GetHandle() const {
        return mHandle;
    }

}}  // namespace dawn_native::vulkan
