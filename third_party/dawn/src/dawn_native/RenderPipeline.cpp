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

#include "dawn_native/RenderPipeline.h"

#include "common/BitSetIterator.h"
#include "common/HashUtils.h"
#include "dawn_native/Commands.h"
#include "dawn_native/Device.h"
#include "dawn_native/Texture.h"
#include "dawn_native/ValidationUtils_autogen.h"

namespace dawn_native {
    // Helper functions
    namespace {

        MaybeError ValidateVertexAttributeDescriptor(
            const VertexAttributeDescriptor* attribute,
            uint64_t vertexBufferStride,
            std::bitset<kMaxVertexAttributes>* attributesSetMask) {
            DAWN_TRY(ValidateVertexFormat(attribute->format));

            if (attribute->shaderLocation >= kMaxVertexAttributes) {
                return DAWN_VALIDATION_ERROR("Setting attribute out of bounds");
            }

            // No underflow is possible because the max vertex format size is smaller than
            // kMaxVertexAttributeEnd.
            ASSERT(kMaxVertexAttributeEnd >= VertexFormatSize(attribute->format));
            if (attribute->offset > kMaxVertexAttributeEnd - VertexFormatSize(attribute->format)) {
                return DAWN_VALIDATION_ERROR("Setting attribute offset out of bounds");
            }

            // No overflow is possible because the offset is already validated to be less
            // than kMaxVertexAttributeEnd.
            ASSERT(attribute->offset < kMaxVertexAttributeEnd);
            if (vertexBufferStride > 0 &&
                attribute->offset + VertexFormatSize(attribute->format) > vertexBufferStride) {
                return DAWN_VALIDATION_ERROR("Setting attribute offset out of bounds");
            }

            if ((*attributesSetMask)[attribute->shaderLocation]) {
                return DAWN_VALIDATION_ERROR("Setting already set attribute");
            }

            attributesSetMask->set(attribute->shaderLocation);
            return {};
        }

        MaybeError ValidateVertexBufferDescriptor(
            const VertexBufferDescriptor* buffer,
            std::bitset<kMaxVertexAttributes>* attributesSetMask) {
            DAWN_TRY(ValidateInputStepMode(buffer->stepMode));
            if (buffer->stride > kMaxVertexBufferStride) {
                return DAWN_VALIDATION_ERROR("Setting input stride out of bounds");
            }

            for (uint32_t i = 0; i < buffer->attributeCount; ++i) {
                DAWN_TRY(ValidateVertexAttributeDescriptor(&buffer->attributes[i], buffer->stride,
                                                           attributesSetMask));
            }

            return {};
        }

        MaybeError ValidateVertexInputDescriptor(
            const VertexInputDescriptor* descriptor,
            std::bitset<kMaxVertexAttributes>* attributesSetMask) {
            if (descriptor->nextInChain != nullptr) {
                return DAWN_VALIDATION_ERROR("nextInChain must be nullptr");
            }
            DAWN_TRY(ValidateIndexFormat(descriptor->indexFormat));

            if (descriptor->bufferCount > kMaxVertexBuffers) {
                return DAWN_VALIDATION_ERROR("Vertex Inputs number exceeds maximum");
            }

            uint32_t totalAttributesNum = 0;
            for (uint32_t i = 0; i < descriptor->bufferCount; ++i) {
                DAWN_TRY(
                    ValidateVertexBufferDescriptor(&descriptor->buffers[i], attributesSetMask));
                totalAttributesNum += descriptor->buffers[i].attributeCount;
            }

            // Every vertex attribute has a member called shaderLocation, and there are some
            // requirements for shaderLocation: 1) >=0, 2) values are different across different
            // attributes, 3) can't exceed kMaxVertexAttributes. So it can ensure that total
            // attribute number never exceed kMaxVertexAttributes.
            ASSERT(totalAttributesNum <= kMaxVertexAttributes);

            return {};
        }

        MaybeError ValidateRasterizationStateDescriptor(
            const RasterizationStateDescriptor* descriptor) {
            if (descriptor->nextInChain != nullptr) {
                return DAWN_VALIDATION_ERROR("nextInChain must be nullptr");
            }
            DAWN_TRY(ValidateFrontFace(descriptor->frontFace));
            DAWN_TRY(ValidateCullMode(descriptor->cullMode));
            return {};
        }

        MaybeError ValidateColorStateDescriptor(const ColorStateDescriptor* descriptor) {
            if (descriptor->nextInChain != nullptr) {
                return DAWN_VALIDATION_ERROR("nextInChain must be nullptr");
            }
            DAWN_TRY(ValidateBlendOperation(descriptor->alphaBlend.operation));
            DAWN_TRY(ValidateBlendFactor(descriptor->alphaBlend.srcFactor));
            DAWN_TRY(ValidateBlendFactor(descriptor->alphaBlend.dstFactor));
            DAWN_TRY(ValidateBlendOperation(descriptor->colorBlend.operation));
            DAWN_TRY(ValidateBlendFactor(descriptor->colorBlend.srcFactor));
            DAWN_TRY(ValidateBlendFactor(descriptor->colorBlend.dstFactor));
            DAWN_TRY(ValidateColorWriteMask(descriptor->writeMask));

            dawn::TextureFormat format = descriptor->format;
            DAWN_TRY(ValidateTextureFormat(format));
            if (!IsColorRenderableTextureFormat(format)) {
                return DAWN_VALIDATION_ERROR("Color format must be color renderable");
            }

            return {};
        }

        MaybeError ValidateDepthStencilStateDescriptor(
            const DepthStencilStateDescriptor* descriptor) {
            if (descriptor->nextInChain != nullptr) {
                return DAWN_VALIDATION_ERROR("nextInChain must be nullptr");
            }
            DAWN_TRY(ValidateCompareFunction(descriptor->depthCompare));
            DAWN_TRY(ValidateCompareFunction(descriptor->stencilFront.compare));
            DAWN_TRY(ValidateStencilOperation(descriptor->stencilFront.failOp));
            DAWN_TRY(ValidateStencilOperation(descriptor->stencilFront.depthFailOp));
            DAWN_TRY(ValidateStencilOperation(descriptor->stencilFront.passOp));
            DAWN_TRY(ValidateCompareFunction(descriptor->stencilBack.compare));
            DAWN_TRY(ValidateStencilOperation(descriptor->stencilBack.failOp));
            DAWN_TRY(ValidateStencilOperation(descriptor->stencilBack.depthFailOp));
            DAWN_TRY(ValidateStencilOperation(descriptor->stencilBack.passOp));

            dawn::TextureFormat format = descriptor->format;
            DAWN_TRY(ValidateTextureFormat(format));
            if (!IsDepthStencilRenderableTextureFormat(format)) {
                return DAWN_VALIDATION_ERROR(
                    "Depth stencil format must be depth-stencil renderable");
            }

            return {};
        }

    }  // anonymous namespace

    // Helper functions
    size_t IndexFormatSize(dawn::IndexFormat format) {
        switch (format) {
            case dawn::IndexFormat::Uint16:
                return sizeof(uint16_t);
            case dawn::IndexFormat::Uint32:
                return sizeof(uint32_t);
            default:
                UNREACHABLE();
        }
    }

    uint32_t VertexFormatNumComponents(dawn::VertexFormat format) {
        switch (format) {
            case dawn::VertexFormat::UChar4:
            case dawn::VertexFormat::Char4:
            case dawn::VertexFormat::UChar4Norm:
            case dawn::VertexFormat::Char4Norm:
            case dawn::VertexFormat::UShort4:
            case dawn::VertexFormat::Short4:
            case dawn::VertexFormat::UShort4Norm:
            case dawn::VertexFormat::Short4Norm:
            case dawn::VertexFormat::Half4:
            case dawn::VertexFormat::Float4:
            case dawn::VertexFormat::UInt4:
            case dawn::VertexFormat::Int4:
                return 4;
            case dawn::VertexFormat::Float3:
            case dawn::VertexFormat::UInt3:
            case dawn::VertexFormat::Int3:
                return 3;
            case dawn::VertexFormat::UChar2:
            case dawn::VertexFormat::Char2:
            case dawn::VertexFormat::UChar2Norm:
            case dawn::VertexFormat::Char2Norm:
            case dawn::VertexFormat::UShort2:
            case dawn::VertexFormat::Short2:
            case dawn::VertexFormat::UShort2Norm:
            case dawn::VertexFormat::Short2Norm:
            case dawn::VertexFormat::Half2:
            case dawn::VertexFormat::Float2:
            case dawn::VertexFormat::UInt2:
            case dawn::VertexFormat::Int2:
                return 2;
            case dawn::VertexFormat::Float:
            case dawn::VertexFormat::UInt:
            case dawn::VertexFormat::Int:
                return 1;
            default:
                UNREACHABLE();
        }
    }

    size_t VertexFormatComponentSize(dawn::VertexFormat format) {
        switch (format) {
            case dawn::VertexFormat::UChar2:
            case dawn::VertexFormat::UChar4:
            case dawn::VertexFormat::Char2:
            case dawn::VertexFormat::Char4:
            case dawn::VertexFormat::UChar2Norm:
            case dawn::VertexFormat::UChar4Norm:
            case dawn::VertexFormat::Char2Norm:
            case dawn::VertexFormat::Char4Norm:
                return sizeof(char);
            case dawn::VertexFormat::UShort2:
            case dawn::VertexFormat::UShort4:
            case dawn::VertexFormat::UShort2Norm:
            case dawn::VertexFormat::UShort4Norm:
            case dawn::VertexFormat::Short2:
            case dawn::VertexFormat::Short4:
            case dawn::VertexFormat::Short2Norm:
            case dawn::VertexFormat::Short4Norm:
            case dawn::VertexFormat::Half2:
            case dawn::VertexFormat::Half4:
                return sizeof(uint16_t);
            case dawn::VertexFormat::Float:
            case dawn::VertexFormat::Float2:
            case dawn::VertexFormat::Float3:
            case dawn::VertexFormat::Float4:
                return sizeof(float);
            case dawn::VertexFormat::UInt:
            case dawn::VertexFormat::UInt2:
            case dawn::VertexFormat::UInt3:
            case dawn::VertexFormat::UInt4:
            case dawn::VertexFormat::Int:
            case dawn::VertexFormat::Int2:
            case dawn::VertexFormat::Int3:
            case dawn::VertexFormat::Int4:
                return sizeof(int32_t);
            default:
                UNREACHABLE();
        }
    }

    size_t VertexFormatSize(dawn::VertexFormat format) {
        return VertexFormatNumComponents(format) * VertexFormatComponentSize(format);
    }

    MaybeError ValidateRenderPipelineDescriptor(DeviceBase* device,
                                                const RenderPipelineDescriptor* descriptor) {
        if (descriptor->nextInChain != nullptr) {
            return DAWN_VALIDATION_ERROR("nextInChain must be nullptr");
        }

        DAWN_TRY(device->ValidateObject(descriptor->layout));

        if (descriptor->vertexInput == nullptr) {
            return DAWN_VALIDATION_ERROR("Input state must not be null");
        }

        std::bitset<kMaxVertexAttributes> attributesSetMask;
        DAWN_TRY(ValidateVertexInputDescriptor(descriptor->vertexInput, &attributesSetMask));
        DAWN_TRY(ValidatePrimitiveTopology(descriptor->primitiveTopology));
        DAWN_TRY(ValidatePipelineStageDescriptor(device, descriptor->vertexStage,
                                                 descriptor->layout, dawn::ShaderStage::Vertex));
        DAWN_TRY(ValidatePipelineStageDescriptor(device, descriptor->fragmentStage,
                                                 descriptor->layout, dawn::ShaderStage::Fragment));
        DAWN_TRY(ValidateRasterizationStateDescriptor(descriptor->rasterizationState));

        if ((descriptor->vertexStage->module->GetUsedVertexAttributes() & ~attributesSetMask)
                .any()) {
            return DAWN_VALIDATION_ERROR(
                "Pipeline vertex stage uses inputs not in the input state");
        }

        if (!IsValidSampleCount(descriptor->sampleCount)) {
            return DAWN_VALIDATION_ERROR("Sample count is not supported");
        }

        if (descriptor->colorStateCount > kMaxColorAttachments) {
            return DAWN_VALIDATION_ERROR("Color States number exceeds maximum");
        }

        if (descriptor->colorStateCount == 0 && !descriptor->depthStencilState) {
            return DAWN_VALIDATION_ERROR("Should have at least one attachment");
        }

        for (uint32_t i = 0; i < descriptor->colorStateCount; ++i) {
            DAWN_TRY(ValidateColorStateDescriptor(descriptor->colorStates[i]));
        }

        if (descriptor->depthStencilState) {
            DAWN_TRY(ValidateDepthStencilStateDescriptor(descriptor->depthStencilState));
        }

        return {};
    }

    bool StencilTestEnabled(const DepthStencilStateDescriptor* mDepthStencilState) {
        return mDepthStencilState->stencilBack.compare != dawn::CompareFunction::Always ||
               mDepthStencilState->stencilBack.failOp != dawn::StencilOperation::Keep ||
               mDepthStencilState->stencilBack.depthFailOp != dawn::StencilOperation::Keep ||
               mDepthStencilState->stencilBack.passOp != dawn::StencilOperation::Keep ||
               mDepthStencilState->stencilFront.compare != dawn::CompareFunction::Always ||
               mDepthStencilState->stencilFront.failOp != dawn::StencilOperation::Keep ||
               mDepthStencilState->stencilFront.depthFailOp != dawn::StencilOperation::Keep ||
               mDepthStencilState->stencilFront.passOp != dawn::StencilOperation::Keep;
    }

    bool BlendEnabled(const ColorStateDescriptor* mColorState) {
        return mColorState->alphaBlend.operation != dawn::BlendOperation::Add ||
               mColorState->alphaBlend.srcFactor != dawn::BlendFactor::One ||
               mColorState->alphaBlend.dstFactor != dawn::BlendFactor::Zero ||
               mColorState->colorBlend.operation != dawn::BlendOperation::Add ||
               mColorState->colorBlend.srcFactor != dawn::BlendFactor::One ||
               mColorState->colorBlend.dstFactor != dawn::BlendFactor::Zero;
    }

    // RenderPipelineBase

    RenderPipelineBase::RenderPipelineBase(DeviceBase* device,
                                           const RenderPipelineDescriptor* descriptor,
                                           bool blueprint)
        : PipelineBase(device,
                       descriptor->layout,
                       dawn::ShaderStageBit::Vertex | dawn::ShaderStageBit::Fragment),
          mVertexInput(*descriptor->vertexInput),
          mHasDepthStencilAttachment(descriptor->depthStencilState != nullptr),
          mPrimitiveTopology(descriptor->primitiveTopology),
          mRasterizationState(*descriptor->rasterizationState),
          mSampleCount(descriptor->sampleCount),
          mVertexModule(descriptor->vertexStage->module),
          mVertexEntryPoint(descriptor->vertexStage->entryPoint),
          mFragmentModule(descriptor->fragmentStage->module),
          mFragmentEntryPoint(descriptor->fragmentStage->entryPoint),
          mIsBlueprint(blueprint) {
        for (uint32_t slot = 0; slot < mVertexInput.bufferCount; ++slot) {
            if (mVertexInput.buffers[slot].attributeCount == 0) {
                continue;
            }

            mInputsSetMask.set(slot);
            mInputInfos[slot].stride = mVertexInput.buffers[slot].stride;
            mInputInfos[slot].stepMode = mVertexInput.buffers[slot].stepMode;

            uint32_t location = 0;
            for (uint32_t i = 0; i < mVertexInput.buffers[slot].attributeCount; ++i) {
                location = mVertexInput.buffers[slot].attributes[i].shaderLocation;
                mAttributesSetMask.set(location);
                mAttributeInfos[location].shaderLocation = location;
                mAttributeInfos[location].inputSlot = slot;
                mAttributeInfos[location].offset = mVertexInput.buffers[slot].attributes[i].offset;
                mAttributeInfos[location].format = mVertexInput.buffers[slot].attributes[i].format;
            }
        }

        if (mHasDepthStencilAttachment) {
            mDepthStencilState = *descriptor->depthStencilState;
        } else {
            // These default values below are useful for backends to fill information.
            // The values indicate that depth and stencil test are disabled when backends
            // set their own depth stencil states/descriptors according to the values in
            // mDepthStencilState.
            mDepthStencilState.depthCompare = dawn::CompareFunction::Always;
            mDepthStencilState.depthWriteEnabled = false;
            mDepthStencilState.stencilBack.compare = dawn::CompareFunction::Always;
            mDepthStencilState.stencilBack.failOp = dawn::StencilOperation::Keep;
            mDepthStencilState.stencilBack.depthFailOp = dawn::StencilOperation::Keep;
            mDepthStencilState.stencilBack.passOp = dawn::StencilOperation::Keep;
            mDepthStencilState.stencilFront.compare = dawn::CompareFunction::Always;
            mDepthStencilState.stencilFront.failOp = dawn::StencilOperation::Keep;
            mDepthStencilState.stencilFront.depthFailOp = dawn::StencilOperation::Keep;
            mDepthStencilState.stencilFront.passOp = dawn::StencilOperation::Keep;
            mDepthStencilState.stencilReadMask = 0xff;
            mDepthStencilState.stencilWriteMask = 0xff;
        }

        for (uint32_t i = 0; i < descriptor->colorStateCount; ++i) {
            mColorAttachmentsSet.set(i);
            mColorStates[i] = *descriptor->colorStates[i];
        }

        // TODO(cwallez@chromium.org): Check against the shader module that the correct color
        // attachment are set?
    }

    RenderPipelineBase::RenderPipelineBase(DeviceBase* device, ObjectBase::ErrorTag tag)
        : PipelineBase(device, tag) {
    }

    // static
    RenderPipelineBase* RenderPipelineBase::MakeError(DeviceBase* device) {
        return new RenderPipelineBase(device, ObjectBase::kError);
    }

    RenderPipelineBase::~RenderPipelineBase() {
        // Do not uncache the actual cached object if we are a blueprint
        if (!mIsBlueprint && !IsError()) {
            GetDevice()->UncacheRenderPipeline(this);
        }
    }

    const VertexInputDescriptor* RenderPipelineBase::GetVertexInputDescriptor() const {
        ASSERT(!IsError());
        return &mVertexInput;
    }

    const std::bitset<kMaxVertexAttributes>& RenderPipelineBase::GetAttributesSetMask() const {
        ASSERT(!IsError());
        return mAttributesSetMask;
    }

    const VertexAttributeInfo& RenderPipelineBase::GetAttribute(uint32_t location) const {
        ASSERT(!IsError());
        ASSERT(mAttributesSetMask[location]);
        return mAttributeInfos[location];
    }

    const std::bitset<kMaxVertexBuffers>& RenderPipelineBase::GetInputsSetMask() const {
        ASSERT(!IsError());
        return mInputsSetMask;
    }

    const VertexBufferInfo& RenderPipelineBase::GetInput(uint32_t slot) const {
        ASSERT(!IsError());
        ASSERT(mInputsSetMask[slot]);
        return mInputInfos[slot];
    }

    const ColorStateDescriptor* RenderPipelineBase::GetColorStateDescriptor(
        uint32_t attachmentSlot) const {
        ASSERT(!IsError());
        ASSERT(attachmentSlot < mColorStates.size());
        return &mColorStates[attachmentSlot];
    }

    const DepthStencilStateDescriptor* RenderPipelineBase::GetDepthStencilStateDescriptor() const {
        ASSERT(!IsError());
        return &mDepthStencilState;
    }

    dawn::PrimitiveTopology RenderPipelineBase::GetPrimitiveTopology() const {
        ASSERT(!IsError());
        return mPrimitiveTopology;
    }

    dawn::CullMode RenderPipelineBase::GetCullMode() const {
        ASSERT(!IsError());
        return mRasterizationState.cullMode;
    }

    dawn::FrontFace RenderPipelineBase::GetFrontFace() const {
        ASSERT(!IsError());
        return mRasterizationState.frontFace;
    }

    std::bitset<kMaxColorAttachments> RenderPipelineBase::GetColorAttachmentsMask() const {
        ASSERT(!IsError());
        return mColorAttachmentsSet;
    }

    bool RenderPipelineBase::HasDepthStencilAttachment() const {
        ASSERT(!IsError());
        return mHasDepthStencilAttachment;
    }

    dawn::TextureFormat RenderPipelineBase::GetColorAttachmentFormat(uint32_t attachment) const {
        ASSERT(!IsError());
        return mColorStates[attachment].format;
    }

    dawn::TextureFormat RenderPipelineBase::GetDepthStencilFormat() const {
        ASSERT(!IsError());
        ASSERT(mHasDepthStencilAttachment);
        return mDepthStencilState.format;
    }

    uint32_t RenderPipelineBase::GetSampleCount() const {
        ASSERT(!IsError());
        return mSampleCount;
    }

    bool RenderPipelineBase::IsCompatibleWith(const BeginRenderPassCmd* renderPass) const {
        ASSERT(!IsError());
        // TODO(cwallez@chromium.org): This is called on every SetPipeline command. Optimize it for
        // example by caching some "attachment compatibility" object that would make the
        // compatibility check a single pointer comparison.

        if (renderPass->colorAttachmentsSet != mColorAttachmentsSet) {
            return false;
        }

        for (uint32_t i : IterateBitSet(mColorAttachmentsSet)) {
            if (renderPass->colorAttachments[i].view->GetFormat() != mColorStates[i].format) {
                return false;
            }
        }

        if (renderPass->hasDepthStencilAttachment != mHasDepthStencilAttachment) {
            return false;
        }

        if (mHasDepthStencilAttachment &&
            (renderPass->depthStencilAttachment.view->GetFormat() != mDepthStencilState.format)) {
            return false;
        }

        if (renderPass->sampleCount != mSampleCount) {
            return false;
        }

        return true;
    }

    std::bitset<kMaxVertexAttributes> RenderPipelineBase::GetAttributesUsingInput(
        uint32_t slot) const {
        ASSERT(!IsError());
        return attributesUsingInput[slot];
    }

    size_t RenderPipelineBase::HashFunc::operator()(const RenderPipelineBase* pipeline) const {
        size_t hash = 0;

        // Hash modules and layout
        HashCombine(&hash, pipeline->GetLayout());
        HashCombine(&hash, pipeline->mVertexModule.Get(), pipeline->mFragmentEntryPoint);
        HashCombine(&hash, pipeline->mFragmentModule.Get(), pipeline->mFragmentEntryPoint);

        // Hash attachments
        HashCombine(&hash, pipeline->mColorAttachmentsSet);
        for (uint32_t i : IterateBitSet(pipeline->mColorAttachmentsSet)) {
            const ColorStateDescriptor& desc = *pipeline->GetColorStateDescriptor(i);
            HashCombine(&hash, desc.format, desc.writeMask);
            HashCombine(&hash, desc.colorBlend.operation, desc.colorBlend.srcFactor,
                        desc.colorBlend.dstFactor);
            HashCombine(&hash, desc.alphaBlend.operation, desc.alphaBlend.srcFactor,
                        desc.alphaBlend.dstFactor);
        }

        if (pipeline->mHasDepthStencilAttachment) {
            const DepthStencilStateDescriptor& desc = pipeline->mDepthStencilState;
            HashCombine(&hash, desc.format, desc.depthWriteEnabled, desc.depthCompare);
            HashCombine(&hash, desc.stencilReadMask, desc.stencilWriteMask);
            HashCombine(&hash, desc.stencilFront.compare, desc.stencilFront.failOp,
                        desc.stencilFront.depthFailOp, desc.stencilFront.passOp);
            HashCombine(&hash, desc.stencilBack.compare, desc.stencilBack.failOp,
                        desc.stencilBack.depthFailOp, desc.stencilBack.passOp);
        }

        // Hash vertex input state
        HashCombine(&hash, pipeline->mAttributesSetMask);
        for (uint32_t i : IterateBitSet(pipeline->mAttributesSetMask)) {
            const VertexAttributeInfo& desc = pipeline->GetAttribute(i);
            HashCombine(&hash, desc.shaderLocation, desc.inputSlot, desc.offset, desc.format);
        }

        HashCombine(&hash, pipeline->mInputsSetMask);
        for (uint32_t i : IterateBitSet(pipeline->mInputsSetMask)) {
            const VertexBufferInfo& desc = pipeline->GetInput(i);
            HashCombine(&hash, desc.stride, desc.stepMode);
        }

        HashCombine(&hash, pipeline->mVertexInput.indexFormat);

        // Hash rasterization state
        {
            const RasterizationStateDescriptor& desc = pipeline->mRasterizationState;
            HashCombine(&hash, desc.frontFace, desc.cullMode);
            HashCombine(&hash, desc.depthBias, desc.depthBiasSlopeScale, desc.depthBiasClamp);
        }

        // Hash other state
        HashCombine(&hash, pipeline->mSampleCount, pipeline->mPrimitiveTopology);

        return hash;
    }

    bool RenderPipelineBase::EqualityFunc::operator()(const RenderPipelineBase* a,
                                                      const RenderPipelineBase* b) const {
        // Check modules and layout
        if (a->GetLayout() != b->GetLayout() || a->mVertexModule.Get() != b->mVertexModule.Get() ||
            a->mVertexEntryPoint != b->mVertexEntryPoint ||
            a->mFragmentModule.Get() != b->mFragmentModule.Get() ||
            a->mFragmentEntryPoint != b->mFragmentEntryPoint) {
            return false;
        }

        // Check attachments
        if (a->mColorAttachmentsSet != b->mColorAttachmentsSet ||
            a->mHasDepthStencilAttachment != b->mHasDepthStencilAttachment) {
            return false;
        }

        for (uint32_t i : IterateBitSet(a->mColorAttachmentsSet)) {
            const ColorStateDescriptor& descA = *a->GetColorStateDescriptor(i);
            const ColorStateDescriptor& descB = *b->GetColorStateDescriptor(i);
            if (descA.format != descB.format || descA.writeMask != descB.writeMask) {
                return false;
            }
            if (descA.colorBlend.operation != descB.colorBlend.operation ||
                descA.colorBlend.srcFactor != descB.colorBlend.srcFactor ||
                descA.colorBlend.dstFactor != descB.colorBlend.dstFactor) {
                return false;
            }
            if (descA.alphaBlend.operation != descB.alphaBlend.operation ||
                descA.alphaBlend.srcFactor != descB.alphaBlend.srcFactor ||
                descA.alphaBlend.dstFactor != descB.alphaBlend.dstFactor) {
                return false;
            }
        }

        if (a->mHasDepthStencilAttachment) {
            const DepthStencilStateDescriptor& descA = a->mDepthStencilState;
            const DepthStencilStateDescriptor& descB = b->mDepthStencilState;
            if (descA.format != descB.format ||
                descA.depthWriteEnabled != descB.depthWriteEnabled ||
                descA.depthCompare != descB.depthCompare) {
                return false;
            }
            if (descA.stencilReadMask != descB.stencilReadMask ||
                descA.stencilWriteMask != descB.stencilWriteMask) {
                return false;
            }
            if (descA.stencilFront.compare != descB.stencilFront.compare ||
                descA.stencilFront.failOp != descB.stencilFront.failOp ||
                descA.stencilFront.depthFailOp != descB.stencilFront.depthFailOp ||
                descA.stencilFront.passOp != descB.stencilFront.passOp) {
                return false;
            }
            if (descA.stencilBack.compare != descB.stencilBack.compare ||
                descA.stencilBack.failOp != descB.stencilBack.failOp ||
                descA.stencilBack.depthFailOp != descB.stencilBack.depthFailOp ||
                descA.stencilBack.passOp != descB.stencilBack.passOp) {
                return false;
            }
        }

        // Check vertex input state
        if (a->mAttributesSetMask != b->mAttributesSetMask) {
            return false;
        }

        for (uint32_t i : IterateBitSet(a->mAttributesSetMask)) {
            const VertexAttributeInfo& descA = a->GetAttribute(i);
            const VertexAttributeInfo& descB = b->GetAttribute(i);
            if (descA.shaderLocation != descB.shaderLocation ||
                descA.inputSlot != descB.inputSlot || descA.offset != descB.offset ||
                descA.format != descB.format) {
                return false;
            }
        }

        if (a->mInputsSetMask != b->mInputsSetMask) {
            return false;
        }

        for (uint32_t i : IterateBitSet(a->mInputsSetMask)) {
            const VertexBufferInfo& descA = a->GetInput(i);
            const VertexBufferInfo& descB = b->GetInput(i);
            if (descA.stride != descB.stride || descA.stepMode != descB.stepMode) {
                return false;
            }
        }

        if (a->mVertexInput.indexFormat != b->mVertexInput.indexFormat) {
            return false;
        }

        // Check rasterization state
        {
            const RasterizationStateDescriptor& descA = a->mRasterizationState;
            const RasterizationStateDescriptor& descB = b->mRasterizationState;
            if (descA.frontFace != descB.frontFace || descA.cullMode != descB.cullMode) {
                return false;
            }
            if (descA.depthBias != descB.depthBias ||
                descA.depthBiasSlopeScale != descB.depthBiasSlopeScale ||
                descA.depthBiasClamp != descB.depthBiasClamp) {
                return false;
            }
        }

        // Check other state
        if (a->mSampleCount != b->mSampleCount || a->mPrimitiveTopology != b->mPrimitiveTopology) {
            return false;
        }

        return true;
    }

}  // namespace dawn_native
