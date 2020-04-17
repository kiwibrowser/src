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

#include "dawn_native/metal/RenderPipelineMTL.h"

#include "dawn_native/metal/DeviceMTL.h"
#include "dawn_native/metal/PipelineLayoutMTL.h"
#include "dawn_native/metal/ShaderModuleMTL.h"
#include "dawn_native/metal/TextureMTL.h"
#include "dawn_native/metal/UtilsMetal.h"

namespace dawn_native { namespace metal {

    namespace {
        MTLVertexFormat VertexFormatType(dawn::VertexFormat format) {
            switch (format) {
                case dawn::VertexFormat::UChar2:
                    return MTLVertexFormatUChar2;
                case dawn::VertexFormat::UChar4:
                    return MTLVertexFormatUChar4;
                case dawn::VertexFormat::Char2:
                    return MTLVertexFormatChar2;
                case dawn::VertexFormat::Char4:
                    return MTLVertexFormatChar4;
                case dawn::VertexFormat::UChar2Norm:
                    return MTLVertexFormatUChar2Normalized;
                case dawn::VertexFormat::UChar4Norm:
                    return MTLVertexFormatUChar4Normalized;
                case dawn::VertexFormat::Char2Norm:
                    return MTLVertexFormatChar2Normalized;
                case dawn::VertexFormat::Char4Norm:
                    return MTLVertexFormatChar4Normalized;
                case dawn::VertexFormat::UShort2:
                    return MTLVertexFormatUShort2;
                case dawn::VertexFormat::UShort4:
                    return MTLVertexFormatUShort4;
                case dawn::VertexFormat::Short2:
                    return MTLVertexFormatShort2;
                case dawn::VertexFormat::Short4:
                    return MTLVertexFormatShort4;
                case dawn::VertexFormat::UShort2Norm:
                    return MTLVertexFormatUShort2Normalized;
                case dawn::VertexFormat::UShort4Norm:
                    return MTLVertexFormatUShort4Normalized;
                case dawn::VertexFormat::Short2Norm:
                    return MTLVertexFormatShort2Normalized;
                case dawn::VertexFormat::Short4Norm:
                    return MTLVertexFormatShort4Normalized;
                case dawn::VertexFormat::Half2:
                    return MTLVertexFormatHalf2;
                case dawn::VertexFormat::Half4:
                    return MTLVertexFormatHalf4;
                case dawn::VertexFormat::Float:
                    return MTLVertexFormatFloat;
                case dawn::VertexFormat::Float2:
                    return MTLVertexFormatFloat2;
                case dawn::VertexFormat::Float3:
                    return MTLVertexFormatFloat3;
                case dawn::VertexFormat::Float4:
                    return MTLVertexFormatFloat4;
                case dawn::VertexFormat::UInt:
                    return MTLVertexFormatUInt;
                case dawn::VertexFormat::UInt2:
                    return MTLVertexFormatUInt2;
                case dawn::VertexFormat::UInt3:
                    return MTLVertexFormatUInt3;
                case dawn::VertexFormat::UInt4:
                    return MTLVertexFormatUInt4;
                case dawn::VertexFormat::Int:
                    return MTLVertexFormatInt;
                case dawn::VertexFormat::Int2:
                    return MTLVertexFormatInt2;
                case dawn::VertexFormat::Int3:
                    return MTLVertexFormatInt3;
                case dawn::VertexFormat::Int4:
                    return MTLVertexFormatInt4;
            }
        }

        MTLVertexStepFunction InputStepModeFunction(dawn::InputStepMode mode) {
            switch (mode) {
                case dawn::InputStepMode::Vertex:
                    return MTLVertexStepFunctionPerVertex;
                case dawn::InputStepMode::Instance:
                    return MTLVertexStepFunctionPerInstance;
            }
        }

        MTLPrimitiveType MTLPrimitiveTopology(dawn::PrimitiveTopology primitiveTopology) {
            switch (primitiveTopology) {
                case dawn::PrimitiveTopology::PointList:
                    return MTLPrimitiveTypePoint;
                case dawn::PrimitiveTopology::LineList:
                    return MTLPrimitiveTypeLine;
                case dawn::PrimitiveTopology::LineStrip:
                    return MTLPrimitiveTypeLineStrip;
                case dawn::PrimitiveTopology::TriangleList:
                    return MTLPrimitiveTypeTriangle;
                case dawn::PrimitiveTopology::TriangleStrip:
                    return MTLPrimitiveTypeTriangleStrip;
            }
        }

        MTLPrimitiveTopologyClass MTLInputPrimitiveTopology(
            dawn::PrimitiveTopology primitiveTopology) {
            switch (primitiveTopology) {
                case dawn::PrimitiveTopology::PointList:
                    return MTLPrimitiveTopologyClassPoint;
                case dawn::PrimitiveTopology::LineList:
                case dawn::PrimitiveTopology::LineStrip:
                    return MTLPrimitiveTopologyClassLine;
                case dawn::PrimitiveTopology::TriangleList:
                case dawn::PrimitiveTopology::TriangleStrip:
                    return MTLPrimitiveTopologyClassTriangle;
            }
        }

        MTLIndexType MTLIndexFormat(dawn::IndexFormat format) {
            switch (format) {
                case dawn::IndexFormat::Uint16:
                    return MTLIndexTypeUInt16;
                case dawn::IndexFormat::Uint32:
                    return MTLIndexTypeUInt32;
            }
        }

        MTLBlendFactor MetalBlendFactor(dawn::BlendFactor factor, bool alpha) {
            switch (factor) {
                case dawn::BlendFactor::Zero:
                    return MTLBlendFactorZero;
                case dawn::BlendFactor::One:
                    return MTLBlendFactorOne;
                case dawn::BlendFactor::SrcColor:
                    return MTLBlendFactorSourceColor;
                case dawn::BlendFactor::OneMinusSrcColor:
                    return MTLBlendFactorOneMinusSourceColor;
                case dawn::BlendFactor::SrcAlpha:
                    return MTLBlendFactorSourceAlpha;
                case dawn::BlendFactor::OneMinusSrcAlpha:
                    return MTLBlendFactorOneMinusSourceAlpha;
                case dawn::BlendFactor::DstColor:
                    return MTLBlendFactorDestinationColor;
                case dawn::BlendFactor::OneMinusDstColor:
                    return MTLBlendFactorOneMinusDestinationColor;
                case dawn::BlendFactor::DstAlpha:
                    return MTLBlendFactorDestinationAlpha;
                case dawn::BlendFactor::OneMinusDstAlpha:
                    return MTLBlendFactorOneMinusDestinationAlpha;
                case dawn::BlendFactor::SrcAlphaSaturated:
                    return MTLBlendFactorSourceAlphaSaturated;
                case dawn::BlendFactor::BlendColor:
                    return alpha ? MTLBlendFactorBlendAlpha : MTLBlendFactorBlendColor;
                case dawn::BlendFactor::OneMinusBlendColor:
                    return alpha ? MTLBlendFactorOneMinusBlendAlpha
                                 : MTLBlendFactorOneMinusBlendColor;
            }
        }

        MTLBlendOperation MetalBlendOperation(dawn::BlendOperation operation) {
            switch (operation) {
                case dawn::BlendOperation::Add:
                    return MTLBlendOperationAdd;
                case dawn::BlendOperation::Subtract:
                    return MTLBlendOperationSubtract;
                case dawn::BlendOperation::ReverseSubtract:
                    return MTLBlendOperationReverseSubtract;
                case dawn::BlendOperation::Min:
                    return MTLBlendOperationMin;
                case dawn::BlendOperation::Max:
                    return MTLBlendOperationMax;
            }
        }

        MTLColorWriteMask MetalColorWriteMask(dawn::ColorWriteMask writeMask) {
            MTLColorWriteMask mask = MTLColorWriteMaskNone;

            if (writeMask & dawn::ColorWriteMask::Red) {
                mask |= MTLColorWriteMaskRed;
            }
            if (writeMask & dawn::ColorWriteMask::Green) {
                mask |= MTLColorWriteMaskGreen;
            }
            if (writeMask & dawn::ColorWriteMask::Blue) {
                mask |= MTLColorWriteMaskBlue;
            }
            if (writeMask & dawn::ColorWriteMask::Alpha) {
                mask |= MTLColorWriteMaskAlpha;
            }

            return mask;
        }

        void ComputeBlendDesc(MTLRenderPipelineColorAttachmentDescriptor* attachment,
                              const ColorStateDescriptor* descriptor) {
            attachment.blendingEnabled = BlendEnabled(descriptor);
            attachment.sourceRGBBlendFactor =
                MetalBlendFactor(descriptor->colorBlend.srcFactor, false);
            attachment.destinationRGBBlendFactor =
                MetalBlendFactor(descriptor->colorBlend.dstFactor, false);
            attachment.rgbBlendOperation = MetalBlendOperation(descriptor->colorBlend.operation);
            attachment.sourceAlphaBlendFactor =
                MetalBlendFactor(descriptor->alphaBlend.srcFactor, true);
            attachment.destinationAlphaBlendFactor =
                MetalBlendFactor(descriptor->alphaBlend.dstFactor, true);
            attachment.alphaBlendOperation = MetalBlendOperation(descriptor->alphaBlend.operation);
            attachment.writeMask = MetalColorWriteMask(descriptor->writeMask);
        }

        MTLStencilOperation MetalStencilOperation(dawn::StencilOperation stencilOperation) {
            switch (stencilOperation) {
                case dawn::StencilOperation::Keep:
                    return MTLStencilOperationKeep;
                case dawn::StencilOperation::Zero:
                    return MTLStencilOperationZero;
                case dawn::StencilOperation::Replace:
                    return MTLStencilOperationReplace;
                case dawn::StencilOperation::Invert:
                    return MTLStencilOperationInvert;
                case dawn::StencilOperation::IncrementClamp:
                    return MTLStencilOperationIncrementClamp;
                case dawn::StencilOperation::DecrementClamp:
                    return MTLStencilOperationDecrementClamp;
                case dawn::StencilOperation::IncrementWrap:
                    return MTLStencilOperationIncrementWrap;
                case dawn::StencilOperation::DecrementWrap:
                    return MTLStencilOperationDecrementWrap;
            }
        }

        MTLDepthStencilDescriptor* MakeDepthStencilDesc(
            const DepthStencilStateDescriptor* descriptor) {
            MTLDepthStencilDescriptor* mtlDepthStencilDescriptor = [MTLDepthStencilDescriptor new];

            mtlDepthStencilDescriptor.depthCompareFunction =
                ToMetalCompareFunction(descriptor->depthCompare);
            mtlDepthStencilDescriptor.depthWriteEnabled = descriptor->depthWriteEnabled;

            if (StencilTestEnabled(descriptor)) {
                MTLStencilDescriptor* backFaceStencil = [MTLStencilDescriptor new];
                MTLStencilDescriptor* frontFaceStencil = [MTLStencilDescriptor new];

                backFaceStencil.stencilCompareFunction =
                    ToMetalCompareFunction(descriptor->stencilBack.compare);
                backFaceStencil.stencilFailureOperation =
                    MetalStencilOperation(descriptor->stencilBack.failOp);
                backFaceStencil.depthFailureOperation =
                    MetalStencilOperation(descriptor->stencilBack.depthFailOp);
                backFaceStencil.depthStencilPassOperation =
                    MetalStencilOperation(descriptor->stencilBack.passOp);
                backFaceStencil.readMask = descriptor->stencilReadMask;
                backFaceStencil.writeMask = descriptor->stencilWriteMask;

                frontFaceStencil.stencilCompareFunction =
                    ToMetalCompareFunction(descriptor->stencilFront.compare);
                frontFaceStencil.stencilFailureOperation =
                    MetalStencilOperation(descriptor->stencilFront.failOp);
                frontFaceStencil.depthFailureOperation =
                    MetalStencilOperation(descriptor->stencilFront.depthFailOp);
                frontFaceStencil.depthStencilPassOperation =
                    MetalStencilOperation(descriptor->stencilFront.passOp);
                frontFaceStencil.readMask = descriptor->stencilReadMask;
                frontFaceStencil.writeMask = descriptor->stencilWriteMask;

                mtlDepthStencilDescriptor.backFaceStencil = backFaceStencil;
                mtlDepthStencilDescriptor.frontFaceStencil = frontFaceStencil;

                [backFaceStencil release];
                [frontFaceStencil release];
            }

            return mtlDepthStencilDescriptor;
        }

        MTLWinding MTLFrontFace(dawn::FrontFace face) {
            // Note that these are inverted because we flip the Y coordinate in the vertex shader
            switch (face) {
                case dawn::FrontFace::CW:
                    return MTLWindingCounterClockwise;
                case dawn::FrontFace::CCW:
                    return MTLWindingClockwise;
            }
        }

        MTLCullMode ToMTLCullMode(dawn::CullMode mode) {
            switch (mode) {
                case dawn::CullMode::None:
                    return MTLCullModeNone;
                case dawn::CullMode::Front:
                    return MTLCullModeFront;
                case dawn::CullMode::Back:
                    return MTLCullModeBack;
            }
        }

    }  // anonymous namespace

    RenderPipeline::RenderPipeline(Device* device, const RenderPipelineDescriptor* descriptor)
        : RenderPipelineBase(device, descriptor),
          mMtlIndexType(MTLIndexFormat(GetVertexInputDescriptor()->indexFormat)),
          mMtlPrimitiveTopology(MTLPrimitiveTopology(GetPrimitiveTopology())),
          mMtlFrontFace(MTLFrontFace(GetFrontFace())),
          mMtlCullMode(ToMTLCullMode(GetCullMode())) {
        auto mtlDevice = device->GetMTLDevice();

        MTLRenderPipelineDescriptor* descriptorMTL = [MTLRenderPipelineDescriptor new];

        const ShaderModule* vertexModule = ToBackend(descriptor->vertexStage->module);
        const char* vertexEntryPoint = descriptor->vertexStage->entryPoint;
        ShaderModule::MetalFunctionData vertexData = vertexModule->GetFunction(
            vertexEntryPoint, dawn::ShaderStage::Vertex, ToBackend(GetLayout()));
        descriptorMTL.vertexFunction = vertexData.function;

        const ShaderModule* fragmentModule = ToBackend(descriptor->fragmentStage->module);
        const char* fragmentEntryPoint = descriptor->fragmentStage->entryPoint;
        ShaderModule::MetalFunctionData fragmentData = fragmentModule->GetFunction(
            fragmentEntryPoint, dawn::ShaderStage::Fragment, ToBackend(GetLayout()));
        descriptorMTL.fragmentFunction = fragmentData.function;

        if (HasDepthStencilAttachment()) {
            // TODO(kainino@chromium.org): Handle depth-only and stencil-only formats.
            dawn::TextureFormat depthStencilFormat = GetDepthStencilFormat();
            descriptorMTL.depthAttachmentPixelFormat = MetalPixelFormat(depthStencilFormat);
            descriptorMTL.stencilAttachmentPixelFormat = MetalPixelFormat(depthStencilFormat);
        }

        for (uint32_t i : IterateBitSet(GetColorAttachmentsMask())) {
            descriptorMTL.colorAttachments[i].pixelFormat =
                MetalPixelFormat(GetColorAttachmentFormat(i));
            const ColorStateDescriptor* descriptor = GetColorStateDescriptor(i);
            ComputeBlendDesc(descriptorMTL.colorAttachments[i], descriptor);
        }

        descriptorMTL.inputPrimitiveTopology = MTLInputPrimitiveTopology(GetPrimitiveTopology());

        MTLVertexDescriptor* vertexDesc = MakeVertexDesc();
        descriptorMTL.vertexDescriptor = vertexDesc;
        [vertexDesc release];

        descriptorMTL.sampleCount = GetSampleCount();

        // TODO(kainino@chromium.org): push constants, textures, samplers

        {
            NSError* error = nil;
            mMtlRenderPipelineState = [mtlDevice newRenderPipelineStateWithDescriptor:descriptorMTL
                                                                                error:&error];
            [descriptorMTL release];
            if (error != nil) {
                NSLog(@" error => %@", error);
                device->HandleError("Error creating rendering pipeline state");
                return;
            }
        }

        // Create depth stencil state and cache it, fetch the cached depth stencil state when we
        // call setDepthStencilState() for a given render pipeline in CommandEncoder, in order to
        // improve performance.
        MTLDepthStencilDescriptor* depthStencilDesc =
            MakeDepthStencilDesc(GetDepthStencilStateDescriptor());
        mMtlDepthStencilState = [mtlDevice newDepthStencilStateWithDescriptor:depthStencilDesc];
        [depthStencilDesc release];
    }

    RenderPipeline::~RenderPipeline() {
        [mMtlRenderPipelineState release];
        [mMtlDepthStencilState release];
    }

    MTLIndexType RenderPipeline::GetMTLIndexType() const {
        return mMtlIndexType;
    }

    MTLPrimitiveType RenderPipeline::GetMTLPrimitiveTopology() const {
        return mMtlPrimitiveTopology;
    }

    MTLWinding RenderPipeline::GetMTLFrontFace() const {
        return mMtlFrontFace;
    }

    MTLCullMode RenderPipeline::GetMTLCullMode() const {
        return mMtlCullMode;
    }

    void RenderPipeline::Encode(id<MTLRenderCommandEncoder> encoder) {
        [encoder setRenderPipelineState:mMtlRenderPipelineState];
    }

    id<MTLDepthStencilState> RenderPipeline::GetMTLDepthStencilState() {
        return mMtlDepthStencilState;
    }

    MTLVertexDescriptor* RenderPipeline::MakeVertexDesc() {
        MTLVertexDescriptor* mtlVertexDescriptor = [MTLVertexDescriptor new];

        for (uint32_t i : IterateBitSet(GetAttributesSetMask())) {
            const VertexAttributeInfo& info = GetAttribute(i);

            auto attribDesc = [MTLVertexAttributeDescriptor new];
            attribDesc.format = VertexFormatType(info.format);
            attribDesc.offset = info.offset;
            attribDesc.bufferIndex = kMaxBindingsPerGroup + info.inputSlot;
            mtlVertexDescriptor.attributes[i] = attribDesc;
            [attribDesc release];
        }

        for (uint32_t vbInputSlot : IterateBitSet(GetInputsSetMask())) {
            const VertexBufferInfo& info = GetInput(vbInputSlot);

            auto layoutDesc = [MTLVertexBufferLayoutDescriptor new];
            if (info.stride == 0) {
                // For MTLVertexStepFunctionConstant, the stepRate must be 0,
                // but the stride must NOT be 0, so we made up it with
                // max(attrib.offset + sizeof(attrib) for each attrib)
                size_t max_stride = 0;
                for (uint32_t attribIndex : IterateBitSet(GetAttributesSetMask())) {
                    const VertexAttributeInfo& attrib = GetAttribute(attribIndex);
                    // Only use the attributes that use the current input
                    if (attrib.inputSlot != vbInputSlot) {
                        continue;
                    }
                    max_stride = std::max(max_stride,
                                          VertexFormatSize(attrib.format) + size_t(attrib.offset));
                }
                layoutDesc.stepFunction = MTLVertexStepFunctionConstant;
                layoutDesc.stepRate = 0;
                // Metal requires the stride must be a multiple of 4 bytes, align it with next
                // multiple of 4 if it's not.
                layoutDesc.stride = Align(max_stride, 4);
            } else {
                layoutDesc.stepFunction = InputStepModeFunction(info.stepMode);
                layoutDesc.stepRate = 1;
                layoutDesc.stride = info.stride;
            }
            // TODO(cwallez@chromium.org): make the offset depend on the pipeline layout
            mtlVertexDescriptor.layouts[kMaxBindingsPerGroup + vbInputSlot] = layoutDesc;
            [layoutDesc release];
        }
        return mtlVertexDescriptor;
    }

}}  // namespace dawn_native::metal
