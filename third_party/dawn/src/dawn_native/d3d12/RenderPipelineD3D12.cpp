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

#include "dawn_native/d3d12/RenderPipelineD3D12.h"

#include "common/Assert.h"
#include "dawn_native/d3d12/DeviceD3D12.h"
#include "dawn_native/d3d12/PipelineLayoutD3D12.h"
#include "dawn_native/d3d12/PlatformFunctions.h"
#include "dawn_native/d3d12/ShaderModuleD3D12.h"
#include "dawn_native/d3d12/TextureD3D12.h"
#include "dawn_native/d3d12/UtilsD3D12.h"

#include <d3dcompiler.h>

namespace dawn_native { namespace d3d12 {

    namespace {
        DXGI_FORMAT VertexFormatType(dawn::VertexFormat format) {
            switch (format) {
                case dawn::VertexFormat::UChar2:
                    return DXGI_FORMAT_R8G8_UINT;
                case dawn::VertexFormat::UChar4:
                    return DXGI_FORMAT_R8G8B8A8_UINT;
                case dawn::VertexFormat::Char2:
                    return DXGI_FORMAT_R8G8_SINT;
                case dawn::VertexFormat::Char4:
                    return DXGI_FORMAT_R8G8B8A8_SINT;
                case dawn::VertexFormat::UChar2Norm:
                    return DXGI_FORMAT_R8G8_UNORM;
                case dawn::VertexFormat::UChar4Norm:
                    return DXGI_FORMAT_R8G8B8A8_UNORM;
                case dawn::VertexFormat::Char2Norm:
                    return DXGI_FORMAT_R8G8_SNORM;
                case dawn::VertexFormat::Char4Norm:
                    return DXGI_FORMAT_R8G8B8A8_SNORM;
                case dawn::VertexFormat::UShort2:
                    return DXGI_FORMAT_R16G16_UINT;
                case dawn::VertexFormat::UShort4:
                    return DXGI_FORMAT_R16G16B16A16_UINT;
                case dawn::VertexFormat::Short2:
                    return DXGI_FORMAT_R16G16_SINT;
                case dawn::VertexFormat::Short4:
                    return DXGI_FORMAT_R16G16B16A16_SINT;
                case dawn::VertexFormat::UShort2Norm:
                    return DXGI_FORMAT_R16G16_UNORM;
                case dawn::VertexFormat::UShort4Norm:
                    return DXGI_FORMAT_R16G16B16A16_UNORM;
                case dawn::VertexFormat::Short2Norm:
                    return DXGI_FORMAT_R16G16_SNORM;
                case dawn::VertexFormat::Short4Norm:
                    return DXGI_FORMAT_R16G16B16A16_SNORM;
                case dawn::VertexFormat::Half2:
                    return DXGI_FORMAT_R16G16_FLOAT;
                case dawn::VertexFormat::Half4:
                    return DXGI_FORMAT_R16G16B16A16_FLOAT;
                case dawn::VertexFormat::Float:
                    return DXGI_FORMAT_R32_FLOAT;
                case dawn::VertexFormat::Float2:
                    return DXGI_FORMAT_R32G32_FLOAT;
                case dawn::VertexFormat::Float3:
                    return DXGI_FORMAT_R32G32B32_FLOAT;
                case dawn::VertexFormat::Float4:
                    return DXGI_FORMAT_R32G32B32A32_FLOAT;
                case dawn::VertexFormat::UInt:
                    return DXGI_FORMAT_R32_UINT;
                case dawn::VertexFormat::UInt2:
                    return DXGI_FORMAT_R32G32_UINT;
                case dawn::VertexFormat::UInt3:
                    return DXGI_FORMAT_R32G32B32_UINT;
                case dawn::VertexFormat::UInt4:
                    return DXGI_FORMAT_R32G32B32A32_UINT;
                case dawn::VertexFormat::Int:
                    return DXGI_FORMAT_R32_SINT;
                case dawn::VertexFormat::Int2:
                    return DXGI_FORMAT_R32G32_SINT;
                case dawn::VertexFormat::Int3:
                    return DXGI_FORMAT_R32G32B32_SINT;
                case dawn::VertexFormat::Int4:
                    return DXGI_FORMAT_R32G32B32A32_SINT;
                default:
                    UNREACHABLE();
            }
        }

        D3D12_INPUT_CLASSIFICATION InputStepModeFunction(dawn::InputStepMode mode) {
            switch (mode) {
                case dawn::InputStepMode::Vertex:
                    return D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
                case dawn::InputStepMode::Instance:
                    return D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
                default:
                    UNREACHABLE();
            }
        }

        D3D12_PRIMITIVE_TOPOLOGY D3D12PrimitiveTopology(dawn::PrimitiveTopology primitiveTopology) {
            switch (primitiveTopology) {
                case dawn::PrimitiveTopology::PointList:
                    return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
                case dawn::PrimitiveTopology::LineList:
                    return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
                case dawn::PrimitiveTopology::LineStrip:
                    return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
                case dawn::PrimitiveTopology::TriangleList:
                    return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
                case dawn::PrimitiveTopology::TriangleStrip:
                    return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
                default:
                    UNREACHABLE();
            }
        }

        D3D12_PRIMITIVE_TOPOLOGY_TYPE D3D12PrimitiveTopologyType(
            dawn::PrimitiveTopology primitiveTopology) {
            switch (primitiveTopology) {
                case dawn::PrimitiveTopology::PointList:
                    return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
                case dawn::PrimitiveTopology::LineList:
                case dawn::PrimitiveTopology::LineStrip:
                    return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
                case dawn::PrimitiveTopology::TriangleList:
                case dawn::PrimitiveTopology::TriangleStrip:
                    return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
                default:
                    UNREACHABLE();
            }
        }

        D3D12_BLEND D3D12Blend(dawn::BlendFactor factor) {
            switch (factor) {
                case dawn::BlendFactor::Zero:
                    return D3D12_BLEND_ZERO;
                case dawn::BlendFactor::One:
                    return D3D12_BLEND_ONE;
                case dawn::BlendFactor::SrcColor:
                    return D3D12_BLEND_SRC_COLOR;
                case dawn::BlendFactor::OneMinusSrcColor:
                    return D3D12_BLEND_INV_SRC_COLOR;
                case dawn::BlendFactor::SrcAlpha:
                    return D3D12_BLEND_SRC_ALPHA;
                case dawn::BlendFactor::OneMinusSrcAlpha:
                    return D3D12_BLEND_INV_SRC_ALPHA;
                case dawn::BlendFactor::DstColor:
                    return D3D12_BLEND_DEST_COLOR;
                case dawn::BlendFactor::OneMinusDstColor:
                    return D3D12_BLEND_INV_DEST_COLOR;
                case dawn::BlendFactor::DstAlpha:
                    return D3D12_BLEND_DEST_ALPHA;
                case dawn::BlendFactor::OneMinusDstAlpha:
                    return D3D12_BLEND_INV_DEST_ALPHA;
                case dawn::BlendFactor::SrcAlphaSaturated:
                    return D3D12_BLEND_SRC_ALPHA_SAT;
                case dawn::BlendFactor::BlendColor:
                    return D3D12_BLEND_BLEND_FACTOR;
                case dawn::BlendFactor::OneMinusBlendColor:
                    return D3D12_BLEND_INV_BLEND_FACTOR;
                default:
                    UNREACHABLE();
            }
        }

        D3D12_BLEND_OP D3D12BlendOperation(dawn::BlendOperation operation) {
            switch (operation) {
                case dawn::BlendOperation::Add:
                    return D3D12_BLEND_OP_ADD;
                case dawn::BlendOperation::Subtract:
                    return D3D12_BLEND_OP_SUBTRACT;
                case dawn::BlendOperation::ReverseSubtract:
                    return D3D12_BLEND_OP_REV_SUBTRACT;
                case dawn::BlendOperation::Min:
                    return D3D12_BLEND_OP_MIN;
                case dawn::BlendOperation::Max:
                    return D3D12_BLEND_OP_MAX;
                default:
                    UNREACHABLE();
            }
        }

        uint8_t D3D12RenderTargetWriteMask(dawn::ColorWriteMask writeMask) {
            static_assert(static_cast<D3D12_COLOR_WRITE_ENABLE>(dawn::ColorWriteMask::Red) ==
                              D3D12_COLOR_WRITE_ENABLE_RED,
                          "ColorWriteMask values must match");
            static_assert(static_cast<D3D12_COLOR_WRITE_ENABLE>(dawn::ColorWriteMask::Green) ==
                              D3D12_COLOR_WRITE_ENABLE_GREEN,
                          "ColorWriteMask values must match");
            static_assert(static_cast<D3D12_COLOR_WRITE_ENABLE>(dawn::ColorWriteMask::Blue) ==
                              D3D12_COLOR_WRITE_ENABLE_BLUE,
                          "ColorWriteMask values must match");
            static_assert(static_cast<D3D12_COLOR_WRITE_ENABLE>(dawn::ColorWriteMask::Alpha) ==
                              D3D12_COLOR_WRITE_ENABLE_ALPHA,
                          "ColorWriteMask values must match");
            return static_cast<uint8_t>(writeMask);
        }

        D3D12_RENDER_TARGET_BLEND_DESC ComputeColorDesc(const ColorStateDescriptor* descriptor) {
            D3D12_RENDER_TARGET_BLEND_DESC blendDesc;
            blendDesc.BlendEnable = BlendEnabled(descriptor);
            blendDesc.SrcBlend = D3D12Blend(descriptor->colorBlend.srcFactor);
            blendDesc.DestBlend = D3D12Blend(descriptor->colorBlend.dstFactor);
            blendDesc.BlendOp = D3D12BlendOperation(descriptor->colorBlend.operation);
            blendDesc.SrcBlendAlpha = D3D12Blend(descriptor->alphaBlend.srcFactor);
            blendDesc.DestBlendAlpha = D3D12Blend(descriptor->alphaBlend.dstFactor);
            blendDesc.BlendOpAlpha = D3D12BlendOperation(descriptor->alphaBlend.operation);
            blendDesc.RenderTargetWriteMask = D3D12RenderTargetWriteMask(descriptor->writeMask);
            blendDesc.LogicOpEnable = false;
            blendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
            return blendDesc;
        }

        D3D12_STENCIL_OP StencilOp(dawn::StencilOperation op) {
            switch (op) {
                case dawn::StencilOperation::Keep:
                    return D3D12_STENCIL_OP_KEEP;
                case dawn::StencilOperation::Zero:
                    return D3D12_STENCIL_OP_ZERO;
                case dawn::StencilOperation::Replace:
                    return D3D12_STENCIL_OP_REPLACE;
                case dawn::StencilOperation::IncrementClamp:
                    return D3D12_STENCIL_OP_INCR_SAT;
                case dawn::StencilOperation::DecrementClamp:
                    return D3D12_STENCIL_OP_DECR_SAT;
                case dawn::StencilOperation::Invert:
                    return D3D12_STENCIL_OP_INVERT;
                case dawn::StencilOperation::IncrementWrap:
                    return D3D12_STENCIL_OP_INCR;
                case dawn::StencilOperation::DecrementWrap:
                    return D3D12_STENCIL_OP_DECR;
                default:
                    UNREACHABLE();
            }
        }

        D3D12_DEPTH_STENCILOP_DESC StencilOpDesc(const StencilStateFaceDescriptor descriptor) {
            D3D12_DEPTH_STENCILOP_DESC desc;

            desc.StencilFailOp = StencilOp(descriptor.failOp);
            desc.StencilDepthFailOp = StencilOp(descriptor.depthFailOp);
            desc.StencilPassOp = StencilOp(descriptor.passOp);
            desc.StencilFunc = ToD3D12ComparisonFunc(descriptor.compare);

            return desc;
        }

        D3D12_DEPTH_STENCIL_DESC ComputeDepthStencilDesc(
            const DepthStencilStateDescriptor* descriptor) {
            D3D12_DEPTH_STENCIL_DESC mDepthStencilDescriptor;
            mDepthStencilDescriptor.DepthEnable = TRUE;
            mDepthStencilDescriptor.DepthWriteMask = descriptor->depthWriteEnabled
                                                         ? D3D12_DEPTH_WRITE_MASK_ALL
                                                         : D3D12_DEPTH_WRITE_MASK_ZERO;
            mDepthStencilDescriptor.DepthFunc = ToD3D12ComparisonFunc(descriptor->depthCompare);

            mDepthStencilDescriptor.StencilEnable = StencilTestEnabled(descriptor) ? TRUE : FALSE;
            mDepthStencilDescriptor.StencilReadMask =
                static_cast<UINT8>(descriptor->stencilReadMask);
            mDepthStencilDescriptor.StencilWriteMask =
                static_cast<UINT8>(descriptor->stencilWriteMask);

            mDepthStencilDescriptor.FrontFace = StencilOpDesc(descriptor->stencilFront);
            mDepthStencilDescriptor.BackFace = StencilOpDesc(descriptor->stencilBack);
            return mDepthStencilDescriptor;
        }

    }  // anonymous namespace

    RenderPipeline::RenderPipeline(Device* device, const RenderPipelineDescriptor* descriptor)
        : RenderPipelineBase(device, descriptor),
          mD3d12PrimitiveTopology(D3D12PrimitiveTopology(GetPrimitiveTopology())) {
        uint32_t compileFlags = 0;
#if defined(_DEBUG)
        // Enable better shader debugging with the graphics debugging tools.
        compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
        // SPRIV-cross does matrix multiplication expecting row major matrices
        compileFlags |= D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC descriptorD3D12 = {};

        PerStage<ComPtr<ID3DBlob>> compiledShader;
        ComPtr<ID3DBlob> errors;

        dawn::ShaderStageBit renderStages =
            dawn::ShaderStageBit::Vertex | dawn::ShaderStageBit::Fragment;
        for (auto stage : IterateStages(renderStages)) {
            const ShaderModule* module = nullptr;
            const char* entryPoint = nullptr;
            const char* compileTarget = nullptr;
            D3D12_SHADER_BYTECODE* shader = nullptr;
            switch (stage) {
                case dawn::ShaderStage::Vertex:
                    module = ToBackend(descriptor->vertexStage->module);
                    entryPoint = descriptor->vertexStage->entryPoint;
                    shader = &descriptorD3D12.VS;
                    compileTarget = "vs_5_1";
                    break;
                case dawn::ShaderStage::Fragment:
                    module = ToBackend(descriptor->fragmentStage->module);
                    entryPoint = descriptor->fragmentStage->entryPoint;
                    shader = &descriptorD3D12.PS;
                    compileTarget = "ps_5_1";
                    break;
                default:
                    UNREACHABLE();
                    break;
            }

            const std::string hlslSource = module->GetHLSLSource(ToBackend(GetLayout()));

            const PlatformFunctions* functions = device->GetFunctions();
            if (FAILED(functions->d3dCompile(hlslSource.c_str(), hlslSource.length(), nullptr,
                                             nullptr, nullptr, entryPoint, compileTarget,
                                             compileFlags, 0, &compiledShader[stage], &errors))) {
                printf("%s\n", reinterpret_cast<char*>(errors->GetBufferPointer()));
                ASSERT(false);
            }

            if (shader != nullptr) {
                shader->pShaderBytecode = compiledShader[stage]->GetBufferPointer();
                shader->BytecodeLength = compiledShader[stage]->GetBufferSize();
            }
        }

        PipelineLayout* layout = ToBackend(GetLayout());

        descriptorD3D12.pRootSignature = layout->GetRootSignature().Get();

        // D3D12 logs warnings if any empty input state is used
        std::array<D3D12_INPUT_ELEMENT_DESC, kMaxVertexAttributes> inputElementDescriptors;
        if (GetAttributesSetMask().any()) {
            descriptorD3D12.InputLayout = ComputeInputLayout(&inputElementDescriptors);
        }

        descriptorD3D12.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        descriptorD3D12.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        descriptorD3D12.RasterizerState.FrontCounterClockwise = FALSE;
        descriptorD3D12.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
        descriptorD3D12.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        descriptorD3D12.RasterizerState.SlopeScaledDepthBias =
            D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        descriptorD3D12.RasterizerState.DepthClipEnable = TRUE;
        descriptorD3D12.RasterizerState.MultisampleEnable = (GetSampleCount() > 1) ? TRUE : FALSE;
        descriptorD3D12.RasterizerState.AntialiasedLineEnable = FALSE;
        descriptorD3D12.RasterizerState.ForcedSampleCount = 0;
        descriptorD3D12.RasterizerState.ConservativeRaster =
            D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

        if (HasDepthStencilAttachment()) {
            descriptorD3D12.DSVFormat = D3D12TextureFormat(GetDepthStencilFormat());
        }

        for (uint32_t i : IterateBitSet(GetColorAttachmentsMask())) {
            descriptorD3D12.RTVFormats[i] = D3D12TextureFormat(GetColorAttachmentFormat(i));
            descriptorD3D12.BlendState.RenderTarget[i] =
                ComputeColorDesc(GetColorStateDescriptor(i));
        }
        descriptorD3D12.NumRenderTargets = static_cast<uint32_t>(GetColorAttachmentsMask().count());

        descriptorD3D12.BlendState.AlphaToCoverageEnable = FALSE;
        descriptorD3D12.BlendState.IndependentBlendEnable = TRUE;

        descriptorD3D12.DepthStencilState =
            ComputeDepthStencilDesc(GetDepthStencilStateDescriptor());

        descriptorD3D12.SampleMask = UINT_MAX;
        descriptorD3D12.PrimitiveTopologyType = D3D12PrimitiveTopologyType(GetPrimitiveTopology());
        descriptorD3D12.SampleDesc.Count = GetSampleCount();
        descriptorD3D12.SampleDesc.Quality = 0;

        ASSERT_SUCCESS(device->GetD3D12Device()->CreateGraphicsPipelineState(
            &descriptorD3D12, IID_PPV_ARGS(&mPipelineState)));
    }

    RenderPipeline::~RenderPipeline() {
        ToBackend(GetDevice())->ReferenceUntilUnused(mPipelineState);
    }

    D3D12_PRIMITIVE_TOPOLOGY RenderPipeline::GetD3D12PrimitiveTopology() const {
        return mD3d12PrimitiveTopology;
    }

    ComPtr<ID3D12PipelineState> RenderPipeline::GetPipelineState() {
        return mPipelineState;
    }

    D3D12_INPUT_LAYOUT_DESC RenderPipeline::ComputeInputLayout(
        std::array<D3D12_INPUT_ELEMENT_DESC, kMaxVertexAttributes>* inputElementDescriptors) {
        unsigned int count = 0;
        for (auto i : IterateBitSet(GetAttributesSetMask())) {
            D3D12_INPUT_ELEMENT_DESC& inputElementDescriptor = (*inputElementDescriptors)[count++];

            const VertexAttributeInfo& attribute = GetAttribute(i);

            // If the HLSL semantic is TEXCOORDN the SemanticName should be "TEXCOORD" and the
            // SemanticIndex N
            inputElementDescriptor.SemanticName = "TEXCOORD";
            inputElementDescriptor.SemanticIndex = static_cast<uint32_t>(i);
            inputElementDescriptor.Format = VertexFormatType(attribute.format);
            inputElementDescriptor.InputSlot = attribute.inputSlot;

            const VertexBufferInfo& input = GetInput(attribute.inputSlot);

            inputElementDescriptor.AlignedByteOffset = attribute.offset;
            inputElementDescriptor.InputSlotClass = InputStepModeFunction(input.stepMode);
            if (inputElementDescriptor.InputSlotClass ==
                D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA) {
                inputElementDescriptor.InstanceDataStepRate = 0;
            } else {
                inputElementDescriptor.InstanceDataStepRate = 1;
            }
        }

        D3D12_INPUT_LAYOUT_DESC inputLayoutDescriptor;
        inputLayoutDescriptor.pInputElementDescs = &(*inputElementDescriptors)[0];
        inputLayoutDescriptor.NumElements = count;
        return inputLayoutDescriptor;
    }

}}  // namespace dawn_native::d3d12
