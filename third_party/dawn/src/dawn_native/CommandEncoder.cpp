// Copyright 2019 The Dawn Authors
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

#include "dawn_native/CommandEncoder.h"

#include "common/BitSetIterator.h"
#include "dawn_native/BindGroup.h"
#include "dawn_native/Buffer.h"
#include "dawn_native/CommandBuffer.h"
#include "dawn_native/CommandBufferStateTracker.h"
#include "dawn_native/Commands.h"
#include "dawn_native/ComputePassEncoder.h"
#include "dawn_native/Device.h"
#include "dawn_native/ErrorData.h"
#include "dawn_native/RenderPassEncoder.h"
#include "dawn_native/RenderPipeline.h"

#include <map>

namespace dawn_native {

    namespace {

        MaybeError ValidateCopySizeFitsInTexture(const TextureCopy& textureCopy,
                                                 const Extent3D& copySize) {
            const TextureBase* texture = textureCopy.texture.Get();
            if (textureCopy.level >= texture->GetNumMipLevels()) {
                return DAWN_VALIDATION_ERROR("Copy mip-level out of range");
            }

            if (textureCopy.slice >= texture->GetArrayLayers()) {
                return DAWN_VALIDATION_ERROR("Copy array-layer out of range");
            }

            // All texture dimensions are in uint32_t so by doing checks in uint64_t we avoid
            // overflows.
            uint64_t level = textureCopy.level;
            if (uint64_t(textureCopy.origin.x) + uint64_t(copySize.width) >
                    (static_cast<uint64_t>(texture->GetSize().width) >> level) ||
                uint64_t(textureCopy.origin.y) + uint64_t(copySize.height) >
                    (static_cast<uint64_t>(texture->GetSize().height) >> level)) {
                return DAWN_VALIDATION_ERROR("Copy would touch outside of the texture");
            }

            // TODO(cwallez@chromium.org): Check the depth bound differently for 2D arrays and 3D
            // textures
            if (textureCopy.origin.z != 0 || copySize.depth > 1) {
                return DAWN_VALIDATION_ERROR("No support for z != 0 and depth > 1 for now");
            }

            return {};
        }

        MaybeError ValidateCopySizeFitsInBuffer(const Ref<BufferBase>& buffer,
                                                uint64_t offset,
                                                uint64_t size) {
            uint64_t bufferSize = buffer->GetSize();
            bool fitsInBuffer = offset <= bufferSize && (size <= (bufferSize - offset));
            if (!fitsInBuffer) {
                return DAWN_VALIDATION_ERROR("Copy would overflow the buffer");
            }

            return {};
        }

        MaybeError ValidateCopySizeFitsInBuffer(const BufferCopy& bufferCopy, uint64_t dataSize) {
            return ValidateCopySizeFitsInBuffer(bufferCopy.buffer, bufferCopy.offset, dataSize);
        }

        MaybeError ValidateB2BCopySizeAlignment(uint64_t dataSize,
                                                uint64_t srcOffset,
                                                uint64_t dstOffset) {
            // Copy size must be a multiple of 4 bytes on macOS.
            if (dataSize % 4 != 0) {
                return DAWN_VALIDATION_ERROR("Copy size must be a multiple of 4 bytes");
            }

            // SourceOffset and destinationOffset must be multiples of 4 bytes on macOS.
            if (srcOffset % 4 != 0 || dstOffset % 4 != 0) {
                return DAWN_VALIDATION_ERROR(
                    "Source offset and destination offset must be multiples of 4 bytes");
            }

            return {};
        }

        MaybeError ValidateTexelBufferOffset(TextureBase* texture, const BufferCopy& bufferCopy) {
            uint32_t texelSize =
                static_cast<uint32_t>(TextureFormatPixelSize(texture->GetFormat()));
            if (bufferCopy.offset % texelSize != 0) {
                return DAWN_VALIDATION_ERROR("Buffer offset must be a multiple of the texel size");
            }

            return {};
        }

        MaybeError ValidateImageHeight(uint32_t imageHeight, uint32_t copyHeight) {
            if (imageHeight < copyHeight) {
                return DAWN_VALIDATION_ERROR("Image height must not be less than the copy height.");
            }

            return {};
        }

        inline MaybeError PushDebugMarkerStack(unsigned int* counter) {
            *counter += 1;
            return {};
        }

        inline MaybeError PopDebugMarkerStack(unsigned int* counter) {
            if (*counter == 0) {
                return DAWN_VALIDATION_ERROR("Pop must be balanced by a corresponding Push.");
            } else {
                *counter -= 1;
            }

            return {};
        }

        inline MaybeError ValidateDebugGroups(const unsigned int counter) {
            if (counter != 0) {
                return DAWN_VALIDATION_ERROR("Each Push must be balanced by a corresponding Pop.");
            }

            return {};
        }

        MaybeError ValidateTextureSampleCountInCopyCommands(const TextureBase* texture) {
            if (texture->GetSampleCount() > 1) {
                return DAWN_VALIDATION_ERROR("The sample count of textures must be 1");
            }

            return {};
        }

        MaybeError ValidateEntireSubresourceCopied(const TextureCopy& src,
                                                   const TextureCopy& dst,
                                                   const Extent3D& copySize) {
            Extent3D srcSize = src.texture.Get()->GetSize();

            if (dst.origin.x != 0 || dst.origin.y != 0 || dst.origin.z != 0 ||
                srcSize.width != copySize.width || srcSize.height != copySize.height ||
                srcSize.depth != copySize.depth) {
                return DAWN_VALIDATION_ERROR(
                    "The entire subresource must be copied when using a depth/stencil texture or "
                    "when samples are greater than 1.");
            }

            return {};
        }

        MaybeError ValidateTextureToTextureCopyRestrictions(const TextureCopy& src,
                                                            const TextureCopy& dst,
                                                            const Extent3D& copySize) {
            const uint32_t srcSamples = src.texture.Get()->GetSampleCount();
            const uint32_t dstSamples = dst.texture.Get()->GetSampleCount();

            if (srcSamples != dstSamples) {
                return DAWN_VALIDATION_ERROR(
                    "Source and destination textures must have matching sample counts.");
            } else if (srcSamples > 1) {
                // D3D12 requires entire subresource to be copied when using CopyTextureRegion when
                // samples > 1.
                DAWN_TRY(ValidateEntireSubresourceCopied(src, dst, copySize));
            }

            if (src.texture.Get()->GetFormat() != dst.texture.Get()->GetFormat()) {
                // Metal requires texture-to-texture copies be the same format
                return DAWN_VALIDATION_ERROR("Source and destination texture formats must match.");
            } else if (TextureFormatHasDepthOrStencil(src.texture.Get()->GetFormat())) {
                // D3D12 requires entire subresource to be copied when using CopyTextureRegion is
                // used with depth/stencil.
                DAWN_TRY(ValidateEntireSubresourceCopied(src, dst, copySize));
            }

            return {};
        }

        MaybeError ComputeTextureCopyBufferSize(dawn::TextureFormat textureFormat,
                                                const Extent3D& copySize,
                                                uint32_t rowPitch,
                                                uint32_t imageHeight,
                                                uint32_t* bufferSize) {
            DAWN_TRY(ValidateImageHeight(imageHeight, copySize.height));

            // TODO(cwallez@chromium.org): check for overflows
            uint32_t slicePitch = rowPitch * imageHeight;
            uint32_t sliceSize = rowPitch * (copySize.height - 1) +
                                 copySize.width * TextureFormatPixelSize(textureFormat);
            *bufferSize = (slicePitch * (copySize.depth - 1)) + sliceSize;

            return {};
        }

        uint32_t ComputeDefaultRowPitch(TextureBase* texture, uint32_t width) {
            uint32_t texelSize = TextureFormatPixelSize(texture->GetFormat());
            return texelSize * width;
        }

        MaybeError ValidateRowPitch(dawn::TextureFormat format,
                                    const Extent3D& copySize,
                                    uint32_t rowPitch) {
            if (rowPitch % kTextureRowPitchAlignment != 0) {
                return DAWN_VALIDATION_ERROR("Row pitch must be a multiple of 256");
            }

            uint32_t texelSize = TextureFormatPixelSize(format);
            if (rowPitch < copySize.width * texelSize) {
                return DAWN_VALIDATION_ERROR(
                    "Row pitch must not be less than the number of bytes per row");
            }

            return {};
        }

        MaybeError ValidateCanUseAs(BufferBase* buffer, dawn::BufferUsageBit usage) {
            ASSERT(HasZeroOrOneBits(usage));
            if (!(buffer->GetUsage() & usage)) {
                return DAWN_VALIDATION_ERROR("buffer doesn't have the required usage.");
            }

            return {};
        }

        MaybeError ValidateCanUseAs(TextureBase* texture, dawn::TextureUsageBit usage) {
            ASSERT(HasZeroOrOneBits(usage));
            if (!(texture->GetUsage() & usage)) {
                return DAWN_VALIDATION_ERROR("texture doesn't have the required usage.");
            }

            return {};
        }

        MaybeError ValidateAttachmentArrayLayersAndLevelCount(const TextureViewBase* attachment) {
            // Currently we do not support layered rendering.
            if (attachment->GetLayerCount() > 1) {
                return DAWN_VALIDATION_ERROR(
                    "The layer count of the texture view used as attachment cannot be greater than "
                    "1");
            }

            if (attachment->GetLevelCount() > 1) {
                return DAWN_VALIDATION_ERROR(
                    "The mipmap level count of the texture view used as attachment cannot be "
                    "greater than 1");
            }

            return {};
        }

        MaybeError ValidateOrSetAttachmentSize(const TextureViewBase* attachment,
                                               uint32_t* width,
                                               uint32_t* height) {
            const Extent3D& textureSize = attachment->GetTexture()->GetSize();
            const uint32_t attachmentWidth = textureSize.width >> attachment->GetBaseMipLevel();
            const uint32_t attachmentHeight = textureSize.height >> attachment->GetBaseMipLevel();

            if (*width == 0) {
                DAWN_ASSERT(*height == 0);
                *width = attachmentWidth;
                *height = attachmentHeight;
                DAWN_ASSERT(*width != 0 && *height != 0);
            } else if (*width != attachmentWidth || *height != attachmentHeight) {
                return DAWN_VALIDATION_ERROR("Attachment size mismatch");
            }

            return {};
        }

        MaybeError ValidateOrSetColorAttachmentSampleCount(const TextureViewBase* colorAttachment,
                                                           uint32_t* sampleCount) {
            if (*sampleCount == 0) {
                *sampleCount = colorAttachment->GetTexture()->GetSampleCount();
                DAWN_ASSERT(*sampleCount != 0);
            } else if (*sampleCount != colorAttachment->GetTexture()->GetSampleCount()) {
                return DAWN_VALIDATION_ERROR("Color attachment sample counts mismatch");
            }

            return {};
        }

        MaybeError ValidateResolveTarget(
            const DeviceBase* device,
            const RenderPassColorAttachmentDescriptor* colorAttachment) {
            if (colorAttachment->resolveTarget == nullptr) {
                return {};
            }

            DAWN_TRY(device->ValidateObject(colorAttachment->resolveTarget));

            if (!colorAttachment->attachment->GetTexture()->IsMultisampledTexture()) {
                return DAWN_VALIDATION_ERROR(
                    "Cannot set resolve target when the sample count of the color attachment is 1");
            }

            if (colorAttachment->resolveTarget->GetTexture()->IsMultisampledTexture()) {
                return DAWN_VALIDATION_ERROR("Cannot use multisampled texture as resolve target");
            }

            if (colorAttachment->resolveTarget->GetLayerCount() > 1) {
                return DAWN_VALIDATION_ERROR(
                    "The array layer count of the resolve target must be 1");
            }

            if (colorAttachment->resolveTarget->GetLevelCount() > 1) {
                return DAWN_VALIDATION_ERROR("The mip level count of the resolve target must be 1");
            }

            uint32_t colorAttachmentBaseMipLevel = colorAttachment->attachment->GetBaseMipLevel();
            const Extent3D& colorTextureSize = colorAttachment->attachment->GetTexture()->GetSize();
            uint32_t colorAttachmentWidth = colorTextureSize.width >> colorAttachmentBaseMipLevel;
            uint32_t colorAttachmentHeight = colorTextureSize.height >> colorAttachmentBaseMipLevel;

            uint32_t resolveTargetBaseMipLevel = colorAttachment->resolveTarget->GetBaseMipLevel();
            const Extent3D& resolveTextureSize =
                colorAttachment->resolveTarget->GetTexture()->GetSize();
            uint32_t resolveTargetWidth = resolveTextureSize.width >> resolveTargetBaseMipLevel;
            uint32_t resolveTargetHeight = resolveTextureSize.height >> resolveTargetBaseMipLevel;
            if (colorAttachmentWidth != resolveTargetWidth ||
                colorAttachmentHeight != resolveTargetHeight) {
                return DAWN_VALIDATION_ERROR(
                    "The size of the resolve target must be the same as the color attachment");
            }

            dawn::TextureFormat resolveTargetFormat = colorAttachment->resolveTarget->GetFormat();
            if (resolveTargetFormat != colorAttachment->attachment->GetFormat()) {
                return DAWN_VALIDATION_ERROR(
                    "The format of the resolve target must be the same as the color attachment");
            }

            return {};
        }

        MaybeError ValidateRenderPassColorAttachment(
            const DeviceBase* device,
            const RenderPassColorAttachmentDescriptor* colorAttachment,
            uint32_t* width,
            uint32_t* height,
            uint32_t* sampleCount) {
            DAWN_ASSERT(colorAttachment != nullptr);

            DAWN_TRY(device->ValidateObject(colorAttachment->attachment));

            const TextureViewBase* attachment = colorAttachment->attachment;
            if (!IsColorRenderableTextureFormat(attachment->GetFormat())) {
                return DAWN_VALIDATION_ERROR(
                    "The format of the texture view used as color attachment is not color "
                    "renderable");
            }

            DAWN_TRY(ValidateOrSetColorAttachmentSampleCount(attachment, sampleCount));

            DAWN_TRY(ValidateResolveTarget(device, colorAttachment));

            DAWN_TRY(ValidateAttachmentArrayLayersAndLevelCount(attachment));
            DAWN_TRY(ValidateOrSetAttachmentSize(attachment, width, height));

            return {};
        }

        MaybeError ValidateRenderPassDepthStencilAttachment(
            const DeviceBase* device,
            const RenderPassDepthStencilAttachmentDescriptor* depthStencilAttachment,
            uint32_t* width,
            uint32_t* height,
            uint32_t* sampleCount) {
            DAWN_ASSERT(depthStencilAttachment != nullptr);

            DAWN_TRY(device->ValidateObject(depthStencilAttachment->attachment));

            const TextureViewBase* attachment = depthStencilAttachment->attachment;
            if (!TextureFormatHasDepthOrStencil(attachment->GetFormat())) {
                return DAWN_VALIDATION_ERROR(
                    "The format of the texture view used as depth stencil attachment is not a "
                    "depth stencil format");
            }

            // *sampleCount == 0 must only happen when there is no color attachment. In that case we
            // do not need to validate the sample count of the depth stencil attachment.
            const uint32_t depthStencilSampleCount = attachment->GetTexture()->GetSampleCount();
            if (*sampleCount != 0) {
                if (depthStencilSampleCount != *sampleCount) {
                    return DAWN_VALIDATION_ERROR("Depth stencil attachment sample counts mismatch");
                }
            } else {
                *sampleCount = depthStencilSampleCount;
            }

            DAWN_TRY(ValidateAttachmentArrayLayersAndLevelCount(attachment));
            DAWN_TRY(ValidateOrSetAttachmentSize(attachment, width, height));

            return {};
        }

        MaybeError ValidateRenderPassDescriptor(const DeviceBase* device,
                                                const RenderPassDescriptor* renderPass,
                                                uint32_t* width,
                                                uint32_t* height,
                                                uint32_t* sampleCount) {
            if (renderPass->colorAttachmentCount > kMaxColorAttachments) {
                return DAWN_VALIDATION_ERROR("Setting color attachments out of bounds");
            }

            for (uint32_t i = 0; i < renderPass->colorAttachmentCount; ++i) {
                DAWN_TRY(ValidateRenderPassColorAttachment(device, renderPass->colorAttachments[i],
                                                           width, height, sampleCount));
            }

            if (renderPass->depthStencilAttachment != nullptr) {
                DAWN_TRY(ValidateRenderPassDepthStencilAttachment(
                    device, renderPass->depthStencilAttachment, width, height, sampleCount));
            }

            if (renderPass->colorAttachmentCount == 0 &&
                renderPass->depthStencilAttachment == nullptr) {
                return DAWN_VALIDATION_ERROR("Cannot use render pass with no attachments.");
            }

            return {};
        }

        enum class PassType {
            Render,
            Compute,
        };

        // Helper class to encapsulate the logic of tracking per-resource usage during the
        // validation of command buffer passes. It is used both to know if there are validation
        // errors, and to get a list of resources used per pass for backends that need the
        // information.
        class PassResourceUsageTracker {
          public:
            void BufferUsedAs(BufferBase* buffer, dawn::BufferUsageBit usage) {
                // std::map's operator[] will create the key and return 0 if the key didn't exist
                // before.
                dawn::BufferUsageBit& storedUsage = mBufferUsages[buffer];

                if (usage == dawn::BufferUsageBit::Storage &&
                    storedUsage & dawn::BufferUsageBit::Storage) {
                    mStorageUsedMultipleTimes = true;
                }

                storedUsage |= usage;
            }

            void TextureUsedAs(TextureBase* texture, dawn::TextureUsageBit usage) {
                // std::map's operator[] will create the key and return 0 if the key didn't exist
                // before.
                dawn::TextureUsageBit& storedUsage = mTextureUsages[texture];

                if (usage == dawn::TextureUsageBit::Storage &&
                    storedUsage & dawn::TextureUsageBit::Storage) {
                    mStorageUsedMultipleTimes = true;
                }

                storedUsage |= usage;
            }

            // Performs the per-pass usage validation checks
            MaybeError ValidateUsages(PassType pass) const {
                // Storage resources cannot be used twice in the same compute pass
                if (pass == PassType::Compute && mStorageUsedMultipleTimes) {
                    return DAWN_VALIDATION_ERROR(
                        "Storage resource used multiple times in compute pass");
                }

                // Buffers can only be used as single-write or multiple read.
                for (auto& it : mBufferUsages) {
                    BufferBase* buffer = it.first;
                    dawn::BufferUsageBit usage = it.second;

                    if (usage & ~buffer->GetUsage()) {
                        return DAWN_VALIDATION_ERROR("Buffer missing usage for the pass");
                    }

                    bool readOnly = (usage & kReadOnlyBufferUsages) == usage;
                    bool singleUse = dawn::HasZeroOrOneBits(usage);

                    if (!readOnly && !singleUse) {
                        return DAWN_VALIDATION_ERROR(
                            "Buffer used as writable usage and another usage in pass");
                    }
                }

                // Textures can only be used as single-write or multiple read.
                // TODO(cwallez@chromium.org): implement per-subresource tracking
                for (auto& it : mTextureUsages) {
                    TextureBase* texture = it.first;
                    dawn::TextureUsageBit usage = it.second;

                    if (usage & ~texture->GetUsage()) {
                        return DAWN_VALIDATION_ERROR("Texture missing usage for the pass");
                    }

                    // For textures the only read-only usage in a pass is Sampled, so checking the
                    // usage constraint simplifies to checking a single usage bit is set.
                    if (!dawn::HasZeroOrOneBits(it.second)) {
                        return DAWN_VALIDATION_ERROR(
                            "Texture used with more than one usage in pass");
                    }
                }

                return {};
            }

            // Returns the per-pass usage for use by backends for APIs with explicit barriers.
            PassResourceUsage AcquireResourceUsage() {
                PassResourceUsage result;
                result.buffers.reserve(mBufferUsages.size());
                result.bufferUsages.reserve(mBufferUsages.size());
                result.textures.reserve(mTextureUsages.size());
                result.textureUsages.reserve(mTextureUsages.size());

                for (auto& it : mBufferUsages) {
                    result.buffers.push_back(it.first);
                    result.bufferUsages.push_back(it.second);
                }

                for (auto& it : mTextureUsages) {
                    result.textures.push_back(it.first);
                    result.textureUsages.push_back(it.second);
                }

                return result;
            }

          private:
            std::map<BufferBase*, dawn::BufferUsageBit> mBufferUsages;
            std::map<TextureBase*, dawn::TextureUsageBit> mTextureUsages;
            bool mStorageUsedMultipleTimes = false;
        };

        void TrackBindGroupResourceUsage(BindGroupBase* group, PassResourceUsageTracker* tracker) {
            const auto& layoutInfo = group->GetLayout()->GetBindingInfo();

            for (uint32_t i : IterateBitSet(layoutInfo.mask)) {
                dawn::BindingType type = layoutInfo.types[i];

                switch (type) {
                    case dawn::BindingType::UniformBuffer:
                    case dawn::BindingType::DynamicUniformBuffer: {
                        BufferBase* buffer = group->GetBindingAsBufferBinding(i).buffer;
                        tracker->BufferUsedAs(buffer, dawn::BufferUsageBit::Uniform);
                    } break;

                    case dawn::BindingType::StorageBuffer:
                    case dawn::BindingType::DynamicStorageBuffer: {
                        BufferBase* buffer = group->GetBindingAsBufferBinding(i).buffer;
                        tracker->BufferUsedAs(buffer, dawn::BufferUsageBit::Storage);
                    } break;

                    case dawn::BindingType::SampledTexture: {
                        TextureBase* texture = group->GetBindingAsTextureView(i)->GetTexture();
                        tracker->TextureUsedAs(texture, dawn::TextureUsageBit::Sampled);
                    } break;

                    case dawn::BindingType::Sampler:
                        break;
                }
            }
        }

    }  // namespace

    enum class CommandEncoderBase::EncodingState : uint8_t {
        TopLevel,
        ComputePass,
        RenderPass,
        Finished
    };

    CommandEncoderBase::CommandEncoderBase(DeviceBase* device)
        : ObjectBase(device), mEncodingState(EncodingState::TopLevel) {
    }

    CommandEncoderBase::~CommandEncoderBase() {
        if (!mWereCommandsAcquired) {
            MoveToIterator();
            FreeCommands(&mIterator);
        }
    }

    CommandIterator CommandEncoderBase::AcquireCommands() {
        ASSERT(!mWereCommandsAcquired);
        mWereCommandsAcquired = true;
        return std::move(mIterator);
    }

    CommandBufferResourceUsage CommandEncoderBase::AcquireResourceUsages() {
        ASSERT(!mWereResourceUsagesAcquired);
        mWereResourceUsagesAcquired = true;
        return std::move(mResourceUsages);
    }

    void CommandEncoderBase::MoveToIterator() {
        if (!mWasMovedToIterator) {
            mIterator = std::move(mAllocator);
            mWasMovedToIterator = true;
        }
    }

    // Implementation of the API's command recording methods

    ComputePassEncoderBase* CommandEncoderBase::BeginComputePass() {
        DeviceBase* device = GetDevice();
        if (ConsumedError(ValidateCanRecordTopLevelCommands())) {
            return ComputePassEncoderBase::MakeError(device, this);
        }

        mAllocator.Allocate<BeginComputePassCmd>(Command::BeginComputePass);

        mEncodingState = EncodingState::ComputePass;
        return new ComputePassEncoderBase(device, this, &mAllocator);
    }

    RenderPassEncoderBase* CommandEncoderBase::BeginRenderPass(const RenderPassDescriptor* info) {
        DeviceBase* device = GetDevice();

        if (ConsumedError(ValidateCanRecordTopLevelCommands())) {
            return RenderPassEncoderBase::MakeError(device, this);
        }

        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t sampleCount = 0;
        if (ConsumedError(
                ValidateRenderPassDescriptor(device, info, &width, &height, &sampleCount))) {
            return RenderPassEncoderBase::MakeError(device, this);
        }

        ASSERT(width > 0 && height > 0 && sampleCount > 0);

        mEncodingState = EncodingState::RenderPass;

        BeginRenderPassCmd* cmd = mAllocator.Allocate<BeginRenderPassCmd>(Command::BeginRenderPass);

        for (uint32_t i = 0; i < info->colorAttachmentCount; ++i) {
            if (info->colorAttachments[i] != nullptr) {
                cmd->colorAttachmentsSet.set(i);
                cmd->colorAttachments[i].view = info->colorAttachments[i]->attachment;
                cmd->colorAttachments[i].resolveTarget = info->colorAttachments[i]->resolveTarget;
                cmd->colorAttachments[i].loadOp = info->colorAttachments[i]->loadOp;
                cmd->colorAttachments[i].storeOp = info->colorAttachments[i]->storeOp;
                cmd->colorAttachments[i].clearColor = info->colorAttachments[i]->clearColor;
            }
        }

        cmd->hasDepthStencilAttachment = info->depthStencilAttachment != nullptr;
        if (cmd->hasDepthStencilAttachment) {
            cmd->hasDepthStencilAttachment = true;
            cmd->depthStencilAttachment.view = info->depthStencilAttachment->attachment;
            cmd->depthStencilAttachment.clearDepth = info->depthStencilAttachment->clearDepth;
            cmd->depthStencilAttachment.clearStencil = info->depthStencilAttachment->clearStencil;
            cmd->depthStencilAttachment.depthLoadOp = info->depthStencilAttachment->depthLoadOp;
            cmd->depthStencilAttachment.depthStoreOp = info->depthStencilAttachment->depthStoreOp;
            cmd->depthStencilAttachment.stencilLoadOp = info->depthStencilAttachment->stencilLoadOp;
            cmd->depthStencilAttachment.stencilStoreOp =
                info->depthStencilAttachment->stencilStoreOp;
        }

        cmd->width = width;
        cmd->height = height;
        cmd->sampleCount = sampleCount;

        return new RenderPassEncoderBase(device, this, &mAllocator);
    }

    void CommandEncoderBase::CopyBufferToBuffer(BufferBase* source,
                                                uint64_t sourceOffset,
                                                BufferBase* destination,
                                                uint64_t destinationOffset,
                                                uint64_t size) {
        if (ConsumedError(ValidateCanRecordTopLevelCommands())) {
            return;
        }

        if (ConsumedError(GetDevice()->ValidateObject(source))) {
            return;
        }

        if (ConsumedError(GetDevice()->ValidateObject(destination))) {
            return;
        }

        CopyBufferToBufferCmd* copy =
            mAllocator.Allocate<CopyBufferToBufferCmd>(Command::CopyBufferToBuffer);
        copy->source = source;
        copy->sourceOffset = sourceOffset;
        copy->destination = destination;
        copy->destinationOffset = destinationOffset;
        copy->size = size;
    }

    void CommandEncoderBase::CopyBufferToTexture(const BufferCopyView* source,
                                                 const TextureCopyView* destination,
                                                 const Extent3D* copySize) {
        if (ConsumedError(ValidateCanRecordTopLevelCommands())) {
            return;
        }

        if (ConsumedError(GetDevice()->ValidateObject(source->buffer))) {
            return;
        }

        if (ConsumedError(GetDevice()->ValidateObject(destination->texture))) {
            return;
        }

        CopyBufferToTextureCmd* copy =
            mAllocator.Allocate<CopyBufferToTextureCmd>(Command::CopyBufferToTexture);
        copy->source.buffer = source->buffer;
        copy->source.offset = source->offset;
        copy->destination.texture = destination->texture;
        copy->destination.origin = destination->origin;
        copy->copySize = *copySize;
        copy->destination.level = destination->level;
        copy->destination.slice = destination->slice;
        if (source->rowPitch == 0) {
            copy->source.rowPitch = ComputeDefaultRowPitch(destination->texture, copySize->width);
        } else {
            copy->source.rowPitch = source->rowPitch;
        }
        if (source->imageHeight == 0) {
            copy->source.imageHeight = copySize->height;
        } else {
            copy->source.imageHeight = source->imageHeight;
        }
    }

    void CommandEncoderBase::CopyTextureToBuffer(const TextureCopyView* source,
                                                 const BufferCopyView* destination,
                                                 const Extent3D* copySize) {
        if (ConsumedError(ValidateCanRecordTopLevelCommands())) {
            return;
        }

        if (ConsumedError(GetDevice()->ValidateObject(source->texture))) {
            return;
        }

        if (ConsumedError(GetDevice()->ValidateObject(destination->buffer))) {
            return;
        }

        CopyTextureToBufferCmd* copy =
            mAllocator.Allocate<CopyTextureToBufferCmd>(Command::CopyTextureToBuffer);
        copy->source.texture = source->texture;
        copy->source.origin = source->origin;
        copy->copySize = *copySize;
        copy->source.level = source->level;
        copy->source.slice = source->slice;
        copy->destination.buffer = destination->buffer;
        copy->destination.offset = destination->offset;
        if (destination->rowPitch == 0) {
            copy->destination.rowPitch = ComputeDefaultRowPitch(source->texture, copySize->width);
        } else {
            copy->destination.rowPitch = destination->rowPitch;
        }
        if (destination->imageHeight == 0) {
            copy->destination.imageHeight = copySize->height;
        } else {
            copy->destination.imageHeight = destination->imageHeight;
        }
    }

    void CommandEncoderBase::CopyTextureToTexture(const TextureCopyView* source,
                                                  const TextureCopyView* destination,
                                                  const Extent3D* copySize) {
        if (ConsumedError(ValidateCanRecordTopLevelCommands())) {
            return;
        }

        if (ConsumedError(GetDevice()->ValidateObject(source->texture))) {
            return;
        }

        if (ConsumedError(GetDevice()->ValidateObject(destination->texture))) {
            return;
        }

        CopyTextureToTextureCmd* copy =
            mAllocator.Allocate<CopyTextureToTextureCmd>(Command::CopyTextureToTexture);
        copy->source.texture = source->texture;
        copy->source.origin = source->origin;
        copy->source.level = source->level;
        copy->source.slice = source->slice;
        copy->destination.texture = destination->texture;
        copy->destination.origin = destination->origin;
        copy->destination.level = destination->level;
        copy->destination.slice = destination->slice;
        copy->copySize = *copySize;
    }

    CommandBufferBase* CommandEncoderBase::Finish() {
        if (GetDevice()->ConsumedError(ValidateFinish())) {
            // Even if finish validation fails, it is now invalid to call any encoding commands on
            // this object, so we set its state to finished.
            mEncodingState = EncodingState::Finished;
            return CommandBufferBase::MakeError(GetDevice());
        }
        ASSERT(!IsError());

        mEncodingState = EncodingState::Finished;

        MoveToIterator();
        return GetDevice()->CreateCommandBuffer(this);
    }

    // Implementation of functions to interact with sub-encoders

    void CommandEncoderBase::HandleError(const char* message) {
        if (mEncodingState != EncodingState::Finished) {
            if (!mGotError) {
                mGotError = true;
                mErrorMessage = message;
            }
        } else {
            GetDevice()->HandleError(message);
        }
    }

    void CommandEncoderBase::ConsumeError(ErrorData* error) {
        HandleError(error->GetMessage().c_str());
        delete error;
    }

    void CommandEncoderBase::PassEnded() {
        // This function may still be called when the command encoder is finished, just do nothing.
        if (mEncodingState == EncodingState::Finished) {
            return;
        }

        if (mEncodingState == EncodingState::ComputePass) {
            mAllocator.Allocate<EndComputePassCmd>(Command::EndComputePass);
        } else {
            ASSERT(mEncodingState == EncodingState::RenderPass);
            mAllocator.Allocate<EndRenderPassCmd>(Command::EndRenderPass);
        }
        mEncodingState = EncodingState::TopLevel;
    }

    // Implementation of the command buffer validation that can be precomputed before submit

    MaybeError CommandEncoderBase::ValidateFinish() {
        DAWN_TRY(GetDevice()->ValidateObject(this));

        if (mGotError) {
            return DAWN_VALIDATION_ERROR(mErrorMessage);
        }

        if (mEncodingState != EncodingState::TopLevel) {
            return DAWN_VALIDATION_ERROR("Command buffer recording ended mid-pass");
        }

        MoveToIterator();
        mIterator.Reset();

        Command type;
        while (mIterator.NextCommandId(&type)) {
            switch (type) {
                case Command::BeginComputePass: {
                    mIterator.NextCommand<BeginComputePassCmd>();
                    DAWN_TRY(ValidateComputePass());
                } break;

                case Command::BeginRenderPass: {
                    BeginRenderPassCmd* cmd = mIterator.NextCommand<BeginRenderPassCmd>();
                    DAWN_TRY(ValidateRenderPass(cmd));
                } break;

                case Command::CopyBufferToBuffer: {
                    CopyBufferToBufferCmd* copy = mIterator.NextCommand<CopyBufferToBufferCmd>();

                    DAWN_TRY(
                        ValidateCopySizeFitsInBuffer(copy->source, copy->sourceOffset, copy->size));
                    DAWN_TRY(ValidateCopySizeFitsInBuffer(copy->destination,
                                                          copy->destinationOffset, copy->size));
                    DAWN_TRY(ValidateB2BCopySizeAlignment(copy->size, copy->sourceOffset,
                                                          copy->destinationOffset));

                    DAWN_TRY(
                        ValidateCanUseAs(copy->source.Get(), dawn::BufferUsageBit::TransferSrc));
                    DAWN_TRY(ValidateCanUseAs(copy->destination.Get(),
                                              dawn::BufferUsageBit::TransferDst));

                    mResourceUsages.topLevelBuffers.insert(copy->source.Get());
                    mResourceUsages.topLevelBuffers.insert(copy->destination.Get());
                } break;

                case Command::CopyBufferToTexture: {
                    CopyBufferToTextureCmd* copy = mIterator.NextCommand<CopyBufferToTextureCmd>();

                    DAWN_TRY(
                        ValidateTextureSampleCountInCopyCommands(copy->destination.texture.Get()));

                    uint32_t bufferCopySize = 0;
                    DAWN_TRY(ValidateRowPitch(copy->destination.texture->GetFormat(),
                                              copy->copySize, copy->source.rowPitch));

                    DAWN_TRY(ComputeTextureCopyBufferSize(
                        copy->destination.texture->GetFormat(), copy->copySize,
                        copy->source.rowPitch, copy->source.imageHeight, &bufferCopySize));

                    DAWN_TRY(ValidateCopySizeFitsInTexture(copy->destination, copy->copySize));
                    DAWN_TRY(ValidateCopySizeFitsInBuffer(copy->source, bufferCopySize));
                    DAWN_TRY(
                        ValidateTexelBufferOffset(copy->destination.texture.Get(), copy->source));

                    DAWN_TRY(ValidateCanUseAs(copy->source.buffer.Get(),
                                              dawn::BufferUsageBit::TransferSrc));
                    DAWN_TRY(ValidateCanUseAs(copy->destination.texture.Get(),
                                              dawn::TextureUsageBit::TransferDst));

                    mResourceUsages.topLevelBuffers.insert(copy->source.buffer.Get());
                    mResourceUsages.topLevelTextures.insert(copy->destination.texture.Get());
                } break;

                case Command::CopyTextureToBuffer: {
                    CopyTextureToBufferCmd* copy = mIterator.NextCommand<CopyTextureToBufferCmd>();

                    DAWN_TRY(ValidateTextureSampleCountInCopyCommands(copy->source.texture.Get()));

                    uint32_t bufferCopySize = 0;
                    DAWN_TRY(ValidateRowPitch(copy->source.texture->GetFormat(), copy->copySize,
                                              copy->destination.rowPitch));
                    DAWN_TRY(ComputeTextureCopyBufferSize(
                        copy->source.texture->GetFormat(), copy->copySize,
                        copy->destination.rowPitch, copy->destination.imageHeight,
                        &bufferCopySize));

                    DAWN_TRY(ValidateCopySizeFitsInTexture(copy->source, copy->copySize));
                    DAWN_TRY(ValidateCopySizeFitsInBuffer(copy->destination, bufferCopySize));
                    DAWN_TRY(
                        ValidateTexelBufferOffset(copy->source.texture.Get(), copy->destination));

                    DAWN_TRY(ValidateCanUseAs(copy->source.texture.Get(),
                                              dawn::TextureUsageBit::TransferSrc));
                    DAWN_TRY(ValidateCanUseAs(copy->destination.buffer.Get(),
                                              dawn::BufferUsageBit::TransferDst));

                    mResourceUsages.topLevelTextures.insert(copy->source.texture.Get());
                    mResourceUsages.topLevelBuffers.insert(copy->destination.buffer.Get());
                } break;

                case Command::CopyTextureToTexture: {
                    CopyTextureToTextureCmd* copy =
                        mIterator.NextCommand<CopyTextureToTextureCmd>();

                    DAWN_TRY(ValidateTextureToTextureCopyRestrictions(
                        copy->source, copy->destination, copy->copySize));

                    DAWN_TRY(ValidateCopySizeFitsInTexture(copy->source, copy->copySize));
                    DAWN_TRY(ValidateCopySizeFitsInTexture(copy->destination, copy->copySize));

                    DAWN_TRY(ValidateCanUseAs(copy->source.texture.Get(),
                                              dawn::TextureUsageBit::TransferSrc));
                    DAWN_TRY(ValidateCanUseAs(copy->destination.texture.Get(),
                                              dawn::TextureUsageBit::TransferDst));

                    mResourceUsages.topLevelTextures.insert(copy->source.texture.Get());
                    mResourceUsages.topLevelTextures.insert(copy->destination.texture.Get());
                } break;

                default:
                    return DAWN_VALIDATION_ERROR("Command disallowed outside of a pass");
            }
        }

        return {};
    }

    MaybeError CommandEncoderBase::ValidateComputePass() {
        PassResourceUsageTracker usageTracker;
        CommandBufferStateTracker persistentState;

        Command type;
        while (mIterator.NextCommandId(&type)) {
            switch (type) {
                case Command::EndComputePass: {
                    mIterator.NextCommand<EndComputePassCmd>();

                    DAWN_TRY(ValidateDebugGroups(mDebugGroupStackSize));

                    DAWN_TRY(usageTracker.ValidateUsages(PassType::Compute));
                    mResourceUsages.perPass.push_back(usageTracker.AcquireResourceUsage());
                    return {};
                } break;

                case Command::Dispatch: {
                    mIterator.NextCommand<DispatchCmd>();
                    DAWN_TRY(persistentState.ValidateCanDispatch());
                } break;

                case Command::DispatchIndirect: {
                    DispatchIndirectCmd* cmd = mIterator.NextCommand<DispatchIndirectCmd>();
                    DAWN_TRY(persistentState.ValidateCanDispatch());
                    usageTracker.BufferUsedAs(cmd->indirectBuffer.Get(),
                                              dawn::BufferUsageBit::Indirect);
                } break;

                case Command::InsertDebugMarker: {
                    InsertDebugMarkerCmd* cmd = mIterator.NextCommand<InsertDebugMarkerCmd>();
                    mIterator.NextData<char>(cmd->length + 1);
                } break;

                case Command::PopDebugGroup: {
                    mIterator.NextCommand<PopDebugGroupCmd>();
                    DAWN_TRY(PopDebugMarkerStack(&mDebugGroupStackSize));
                } break;

                case Command::PushDebugGroup: {
                    PushDebugGroupCmd* cmd = mIterator.NextCommand<PushDebugGroupCmd>();
                    mIterator.NextData<char>(cmd->length + 1);
                    DAWN_TRY(PushDebugMarkerStack(&mDebugGroupStackSize));
                } break;

                case Command::SetComputePipeline: {
                    SetComputePipelineCmd* cmd = mIterator.NextCommand<SetComputePipelineCmd>();
                    ComputePipelineBase* pipeline = cmd->pipeline.Get();
                    persistentState.SetComputePipeline(pipeline);
                } break;

                case Command::SetBindGroup: {
                    SetBindGroupCmd* cmd = mIterator.NextCommand<SetBindGroupCmd>();
                    if (cmd->dynamicOffsetCount > 0) {
                        mIterator.NextData<uint64_t>(cmd->dynamicOffsetCount);
                    }

                    TrackBindGroupResourceUsage(cmd->group.Get(), &usageTracker);
                    persistentState.SetBindGroup(cmd->index, cmd->group.Get());
                } break;

                default:
                    return DAWN_VALIDATION_ERROR("Command disallowed inside a compute pass");
            }
        }

        UNREACHABLE();
        return DAWN_VALIDATION_ERROR("Unfinished compute pass");
    }

    MaybeError CommandEncoderBase::ValidateRenderPass(BeginRenderPassCmd* renderPass) {
        PassResourceUsageTracker usageTracker;
        CommandBufferStateTracker persistentState;

        // Track usage of the render pass attachments
        for (uint32_t i : IterateBitSet(renderPass->colorAttachmentsSet)) {
            RenderPassColorAttachmentInfo* colorAttachment = &renderPass->colorAttachments[i];
            TextureBase* texture = colorAttachment->view->GetTexture();
            usageTracker.TextureUsedAs(texture, dawn::TextureUsageBit::OutputAttachment);

            TextureViewBase* resolveTarget = colorAttachment->resolveTarget.Get();
            if (resolveTarget != nullptr) {
                usageTracker.TextureUsedAs(resolveTarget->GetTexture(),
                                           dawn::TextureUsageBit::OutputAttachment);
            }
        }

        if (renderPass->hasDepthStencilAttachment) {
            TextureBase* texture = renderPass->depthStencilAttachment.view->GetTexture();
            usageTracker.TextureUsedAs(texture, dawn::TextureUsageBit::OutputAttachment);
        }

        Command type;
        while (mIterator.NextCommandId(&type)) {
            switch (type) {
                case Command::EndRenderPass: {
                    mIterator.NextCommand<EndRenderPassCmd>();

                    DAWN_TRY(ValidateDebugGroups(mDebugGroupStackSize));

                    DAWN_TRY(usageTracker.ValidateUsages(PassType::Render));
                    mResourceUsages.perPass.push_back(usageTracker.AcquireResourceUsage());
                    return {};
                } break;

                case Command::Draw: {
                    mIterator.NextCommand<DrawCmd>();
                    DAWN_TRY(persistentState.ValidateCanDraw());
                } break;

                case Command::DrawIndexed: {
                    mIterator.NextCommand<DrawIndexedCmd>();
                    DAWN_TRY(persistentState.ValidateCanDrawIndexed());
                } break;

                case Command::DrawIndirect: {
                    DrawIndirectCmd* cmd = mIterator.NextCommand<DrawIndirectCmd>();
                    DAWN_TRY(persistentState.ValidateCanDraw());
                    usageTracker.BufferUsedAs(cmd->indirectBuffer.Get(),
                                              dawn::BufferUsageBit::Indirect);
                } break;

                case Command::DrawIndexedIndirect: {
                    DrawIndexedIndirectCmd* cmd = mIterator.NextCommand<DrawIndexedIndirectCmd>();
                    DAWN_TRY(persistentState.ValidateCanDrawIndexed());
                    usageTracker.BufferUsedAs(cmd->indirectBuffer.Get(),
                                              dawn::BufferUsageBit::Indirect);
                } break;

                case Command::InsertDebugMarker: {
                    InsertDebugMarkerCmd* cmd = mIterator.NextCommand<InsertDebugMarkerCmd>();
                    mIterator.NextData<char>(cmd->length + 1);
                } break;

                case Command::PopDebugGroup: {
                    mIterator.NextCommand<PopDebugGroupCmd>();
                    DAWN_TRY(PopDebugMarkerStack(&mDebugGroupStackSize));
                } break;

                case Command::PushDebugGroup: {
                    PushDebugGroupCmd* cmd = mIterator.NextCommand<PushDebugGroupCmd>();
                    mIterator.NextData<char>(cmd->length + 1);
                    DAWN_TRY(PushDebugMarkerStack(&mDebugGroupStackSize));
                } break;

                case Command::SetRenderPipeline: {
                    SetRenderPipelineCmd* cmd = mIterator.NextCommand<SetRenderPipelineCmd>();
                    RenderPipelineBase* pipeline = cmd->pipeline.Get();

                    if (!pipeline->IsCompatibleWith(renderPass)) {
                        return DAWN_VALIDATION_ERROR(
                            "Pipeline is incompatible with this render pass");
                    }

                    persistentState.SetRenderPipeline(pipeline);
                } break;

                case Command::SetStencilReference: {
                    mIterator.NextCommand<SetStencilReferenceCmd>();
                } break;

                case Command::SetBlendColor: {
                    mIterator.NextCommand<SetBlendColorCmd>();
                } break;

                case Command::SetScissorRect: {
                    mIterator.NextCommand<SetScissorRectCmd>();
                } break;

                case Command::SetBindGroup: {
                    SetBindGroupCmd* cmd = mIterator.NextCommand<SetBindGroupCmd>();
                    if (cmd->dynamicOffsetCount > 0) {
                        mIterator.NextData<uint64_t>(cmd->dynamicOffsetCount);
                    }

                    TrackBindGroupResourceUsage(cmd->group.Get(), &usageTracker);
                    persistentState.SetBindGroup(cmd->index, cmd->group.Get());
                } break;

                case Command::SetIndexBuffer: {
                    SetIndexBufferCmd* cmd = mIterator.NextCommand<SetIndexBufferCmd>();

                    usageTracker.BufferUsedAs(cmd->buffer.Get(), dawn::BufferUsageBit::Index);
                    persistentState.SetIndexBuffer();
                } break;

                case Command::SetVertexBuffers: {
                    SetVertexBuffersCmd* cmd = mIterator.NextCommand<SetVertexBuffersCmd>();
                    auto buffers = mIterator.NextData<Ref<BufferBase>>(cmd->count);
                    mIterator.NextData<uint64_t>(cmd->count);

                    for (uint32_t i = 0; i < cmd->count; ++i) {
                        usageTracker.BufferUsedAs(buffers[i].Get(), dawn::BufferUsageBit::Vertex);
                    }
                    persistentState.SetVertexBuffer(cmd->startSlot, cmd->count);
                } break;

                default:
                    return DAWN_VALIDATION_ERROR("Command disallowed inside a render pass");
            }
        }

        UNREACHABLE();
        return DAWN_VALIDATION_ERROR("Unfinished render pass");
    }

    MaybeError CommandEncoderBase::ValidateCanRecordTopLevelCommands() const {
        if (mEncodingState != EncodingState::TopLevel) {
            return DAWN_VALIDATION_ERROR("Command cannot be recorded inside a pass");
        }
        return {};
    }

}  // namespace dawn_native
