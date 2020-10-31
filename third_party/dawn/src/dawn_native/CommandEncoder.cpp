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
#include "dawn_native/CommandValidation.h"
#include "dawn_native/Commands.h"
#include "dawn_native/ComputePassEncoder.h"
#include "dawn_native/Device.h"
#include "dawn_native/ErrorData.h"
#include "dawn_native/QuerySet.h"
#include "dawn_native/RenderPassEncoder.h"
#include "dawn_native/RenderPipeline.h"
#include "dawn_native/ValidationUtils_autogen.h"
#include "dawn_platform/DawnPlatform.h"
#include "dawn_platform/tracing/TraceEvent.h"

#include <cmath>
#include <map>

namespace dawn_native {

    namespace {

        MaybeError ValidateB2BCopyAlignment(uint64_t dataSize,
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

        MaybeError ValidateTextureSampleCountInCopyCommands(const TextureBase* texture) {
            if (texture->GetSampleCount() > 1) {
                return DAWN_VALIDATION_ERROR("The sample count of textures must be 1");
            }

            return {};
        }

        MaybeError ValidateEntireSubresourceCopied(const TextureCopyView& src,
                                                   const TextureCopyView& dst,
                                                   const Extent3D& copySize) {
            Extent3D srcSize = src.texture->GetSize();

            ASSERT(src.texture->GetDimension() == wgpu::TextureDimension::e2D &&
                   dst.texture->GetDimension() == wgpu::TextureDimension::e2D);
            if (dst.origin.x != 0 || dst.origin.y != 0 || srcSize.width != copySize.width ||
                srcSize.height != copySize.height) {
                return DAWN_VALIDATION_ERROR(
                    "The entire subresource must be copied when using a depth/stencil texture or "
                    "when samples are greater than 1.");
            }

            return {};
        }

        MaybeError ValidateTextureToTextureCopyRestrictions(const TextureCopyView& src,
                                                            const TextureCopyView& dst,
                                                            const Extent3D& copySize) {
            const uint32_t srcSamples = src.texture->GetSampleCount();
            const uint32_t dstSamples = dst.texture->GetSampleCount();

            if (srcSamples != dstSamples) {
                return DAWN_VALIDATION_ERROR(
                    "Source and destination textures must have matching sample counts.");
            } else if (srcSamples > 1) {
                // D3D12 requires entire subresource to be copied when using CopyTextureRegion when
                // samples > 1.
                DAWN_TRY(ValidateEntireSubresourceCopied(src, dst, copySize));
            }

            if (src.texture->GetFormat().format != dst.texture->GetFormat().format) {
                // Metal requires texture-to-texture copies be the same format
                return DAWN_VALIDATION_ERROR("Source and destination texture formats must match.");
            }

            if (src.texture->GetFormat().HasDepthOrStencil()) {
                // D3D12 requires entire subresource to be copied when using CopyTextureRegion is
                // used with depth/stencil.
                DAWN_TRY(ValidateEntireSubresourceCopied(src, dst, copySize));
            }

            if (src.texture == dst.texture && src.mipLevel == dst.mipLevel) {
                ASSERT(src.texture->GetDimension() == wgpu::TextureDimension::e2D &&
                       dst.texture->GetDimension() == wgpu::TextureDimension::e2D);
                if (IsRangeOverlapped(src.origin.z, dst.origin.z, copySize.depth)) {
                    return DAWN_VALIDATION_ERROR(
                        "Copy subresources cannot be overlapped when copying within the same "
                        "texture.");
                }
            }

            return {};
        }

        MaybeError ValidateCanUseAs(const BufferBase* buffer, wgpu::BufferUsage usage) {
            ASSERT(wgpu::HasZeroOrOneBits(usage));
            if (!(buffer->GetUsage() & usage)) {
                return DAWN_VALIDATION_ERROR("buffer doesn't have the required usage.");
            }

            return {};
        }

        MaybeError ValidateCanUseAs(const TextureBase* texture, wgpu::TextureUsage usage) {
            ASSERT(wgpu::HasZeroOrOneBits(usage));
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
            const RenderPassColorAttachmentDescriptor& colorAttachment) {
            if (colorAttachment.resolveTarget == nullptr) {
                return {};
            }

            const TextureViewBase* resolveTarget = colorAttachment.resolveTarget;
            const TextureViewBase* attachment = colorAttachment.attachment;
            DAWN_TRY(device->ValidateObject(colorAttachment.resolveTarget));

            if (!attachment->GetTexture()->IsMultisampledTexture()) {
                return DAWN_VALIDATION_ERROR(
                    "Cannot set resolve target when the sample count of the color attachment is 1");
            }

            if (resolveTarget->GetTexture()->IsMultisampledTexture()) {
                return DAWN_VALIDATION_ERROR("Cannot use multisampled texture as resolve target");
            }

            if (resolveTarget->GetLayerCount() > 1) {
                return DAWN_VALIDATION_ERROR(
                    "The array layer count of the resolve target must be 1");
            }

            if (resolveTarget->GetLevelCount() > 1) {
                return DAWN_VALIDATION_ERROR("The mip level count of the resolve target must be 1");
            }

            uint32_t colorAttachmentBaseMipLevel = attachment->GetBaseMipLevel();
            const Extent3D& colorTextureSize = attachment->GetTexture()->GetSize();
            uint32_t colorAttachmentWidth = colorTextureSize.width >> colorAttachmentBaseMipLevel;
            uint32_t colorAttachmentHeight = colorTextureSize.height >> colorAttachmentBaseMipLevel;

            uint32_t resolveTargetBaseMipLevel = resolveTarget->GetBaseMipLevel();
            const Extent3D& resolveTextureSize = resolveTarget->GetTexture()->GetSize();
            uint32_t resolveTargetWidth = resolveTextureSize.width >> resolveTargetBaseMipLevel;
            uint32_t resolveTargetHeight = resolveTextureSize.height >> resolveTargetBaseMipLevel;
            if (colorAttachmentWidth != resolveTargetWidth ||
                colorAttachmentHeight != resolveTargetHeight) {
                return DAWN_VALIDATION_ERROR(
                    "The size of the resolve target must be the same as the color attachment");
            }

            wgpu::TextureFormat resolveTargetFormat = resolveTarget->GetFormat().format;
            if (resolveTargetFormat != attachment->GetFormat().format) {
                return DAWN_VALIDATION_ERROR(
                    "The format of the resolve target must be the same as the color attachment");
            }

            return {};
        }

        MaybeError ValidateRenderPassColorAttachment(
            const DeviceBase* device,
            const RenderPassColorAttachmentDescriptor& colorAttachment,
            uint32_t* width,
            uint32_t* height,
            uint32_t* sampleCount) {
            DAWN_TRY(device->ValidateObject(colorAttachment.attachment));

            const TextureViewBase* attachment = colorAttachment.attachment;
            if (!attachment->GetFormat().IsColor() || !attachment->GetFormat().isRenderable) {
                return DAWN_VALIDATION_ERROR(
                    "The format of the texture view used as color attachment is not color "
                    "renderable");
            }

            DAWN_TRY(ValidateLoadOp(colorAttachment.loadOp));
            DAWN_TRY(ValidateStoreOp(colorAttachment.storeOp));

            if (colorAttachment.loadOp == wgpu::LoadOp::Clear) {
                if (std::isnan(colorAttachment.clearColor.r) ||
                    std::isnan(colorAttachment.clearColor.g) ||
                    std::isnan(colorAttachment.clearColor.b) ||
                    std::isnan(colorAttachment.clearColor.a)) {
                    return DAWN_VALIDATION_ERROR("Color clear value cannot contain NaN");
                }
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
            if (!attachment->GetFormat().HasDepthOrStencil() ||
                !attachment->GetFormat().isRenderable) {
                return DAWN_VALIDATION_ERROR(
                    "The format of the texture view used as depth stencil attachment is not a "
                    "depth stencil format");
            }

            DAWN_TRY(ValidateLoadOp(depthStencilAttachment->depthLoadOp));
            DAWN_TRY(ValidateLoadOp(depthStencilAttachment->stencilLoadOp));
            DAWN_TRY(ValidateStoreOp(depthStencilAttachment->depthStoreOp));
            DAWN_TRY(ValidateStoreOp(depthStencilAttachment->stencilStoreOp));

            if (attachment->GetAspect() == wgpu::TextureAspect::All &&
                attachment->GetFormat().HasStencil() &&
                depthStencilAttachment->depthReadOnly != depthStencilAttachment->stencilReadOnly) {
                return DAWN_VALIDATION_ERROR(
                    "depthReadOnly and stencilReadOnly must be the same when texture aspect is "
                    "'all'");
            }

            if (depthStencilAttachment->depthReadOnly &&
                (depthStencilAttachment->depthLoadOp != wgpu::LoadOp::Load ||
                 depthStencilAttachment->depthStoreOp != wgpu::StoreOp::Store)) {
                return DAWN_VALIDATION_ERROR(
                    "depthLoadOp must be load and depthStoreOp must be store when depthReadOnly "
                    "is true.");
            }

            if (depthStencilAttachment->stencilReadOnly &&
                (depthStencilAttachment->stencilLoadOp != wgpu::LoadOp::Load ||
                 depthStencilAttachment->stencilStoreOp != wgpu::StoreOp::Store)) {
                return DAWN_VALIDATION_ERROR(
                    "stencilLoadOp must be load and stencilStoreOp must be store when "
                    "stencilReadOnly "
                    "is true.");
            }

            if (depthStencilAttachment->depthLoadOp == wgpu::LoadOp::Clear &&
                std::isnan(depthStencilAttachment->clearDepth)) {
                return DAWN_VALIDATION_ERROR("Depth clear value cannot be NaN");
            }

            // This validates that the depth storeOp and stencil storeOps are the same
            if (depthStencilAttachment->depthStoreOp != depthStencilAttachment->stencilStoreOp) {
                return DAWN_VALIDATION_ERROR(
                    "The depth storeOp and stencil storeOp are not the same");
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
                                                const RenderPassDescriptor* descriptor,
                                                uint32_t* width,
                                                uint32_t* height,
                                                uint32_t* sampleCount) {
            if (descriptor->colorAttachmentCount > kMaxColorAttachments) {
                return DAWN_VALIDATION_ERROR("Setting color attachments out of bounds");
            }

            for (uint32_t i = 0; i < descriptor->colorAttachmentCount; ++i) {
                DAWN_TRY(ValidateRenderPassColorAttachment(device, descriptor->colorAttachments[i],
                                                           width, height, sampleCount));
            }

            if (descriptor->depthStencilAttachment != nullptr) {
                DAWN_TRY(ValidateRenderPassDepthStencilAttachment(
                    device, descriptor->depthStencilAttachment, width, height, sampleCount));
            }

            if (descriptor->occlusionQuerySet != nullptr) {
                return DAWN_VALIDATION_ERROR("occlusionQuerySet not implemented");
            }

            if (descriptor->colorAttachmentCount == 0 &&
                descriptor->depthStencilAttachment == nullptr) {
                return DAWN_VALIDATION_ERROR("Cannot use render pass with no attachments.");
            }

            return {};
        }

        MaybeError ValidateComputePassDescriptor(const DeviceBase* device,
                                                 const ComputePassDescriptor* descriptor) {
            return {};
        }

        ResultOrError<TextureCopyView> FixTextureCopyView(DeviceBase* device,
                                                          const TextureCopyView* view) {
            TextureCopyView fixedView = *view;

            if (view->arrayLayer != 0) {
                if (view->origin.z != 0) {
                    return DAWN_VALIDATION_ERROR("arrayLayer and origin.z cannot both be != 0");
                } else {
                    fixedView.origin.z = fixedView.arrayLayer;
                    fixedView.arrayLayer = 1;
                    device->EmitDeprecationWarning(
                        "wgpu::TextureCopyView::arrayLayer is deprecated in favor of "
                        "::origin::z");
                }
            }

            return fixedView;
        }

        ResultOrError<BufferCopyView> FixBufferCopyView(DeviceBase* device,
                                                        const BufferCopyView* view) {
            BufferCopyView fixedView = *view;

            TextureDataLayout& layout = fixedView.layout;
            if (layout.offset != 0 || layout.bytesPerRow != 0 || layout.rowsPerImage != 0) {
                // Using non-deprecated path
                if (fixedView.offset != 0 || fixedView.bytesPerRow != 0 ||
                    fixedView.rowsPerImage != 0) {
                    return DAWN_VALIDATION_ERROR(
                        "WGPUBufferCopyView.offset/bytesPerRow/rowsPerImage is deprecated; use "
                        "only WGPUBufferCopyView.layout");
                }
            } else if (fixedView.offset != 0 || fixedView.bytesPerRow != 0 ||
                       fixedView.rowsPerImage != 0) {
                device->EmitDeprecationWarning(
                    "WGPUBufferCopyView.offset/bytesPerRow/rowsPerImage is deprecated; use "
                    "WGPUBufferCopyView.layout");

                layout.offset = fixedView.offset;
                layout.bytesPerRow = fixedView.bytesPerRow;
                layout.rowsPerImage = fixedView.rowsPerImage;
                fixedView.offset = 0;
                fixedView.bytesPerRow = 0;
                fixedView.rowsPerImage = 0;
            }
            return fixedView;
        }

        MaybeError ValidateQuerySetResolve(const QuerySetBase* querySet,
                                           uint32_t firstQuery,
                                           uint32_t queryCount,
                                           const BufferBase* destination,
                                           uint64_t destinationOffset) {
            if (firstQuery >= querySet->GetQueryCount()) {
                return DAWN_VALIDATION_ERROR("Query index out of bounds");
            }

            if (queryCount > querySet->GetQueryCount() - firstQuery) {
                return DAWN_VALIDATION_ERROR(
                    "The sum of firstQuery and queryCount exceeds the number of queries in query "
                    "set");
            }

            // TODO(hao.x.li@intel.com): Validate that the queries between [firstQuery, firstQuery +
            // queryCount - 1] must be available(written by query operations).

            // The destinationOffset must be a multiple of 8 bytes on D3D12 and Vulkan
            if (destinationOffset % 8 != 0) {
                return DAWN_VALIDATION_ERROR(
                    "The alignment offset into the destination buffer must be a multiple of 8 "
                    "bytes");
            }

            uint64_t bufferSize = destination->GetSize();
            // The destination buffer must have enough storage, from destination offset, to contain
            // the result of resolved queries
            bool fitsInBuffer = destinationOffset <= bufferSize &&
                                (static_cast<uint64_t>(queryCount) * sizeof(uint64_t) <=
                                 (bufferSize - destinationOffset));
            if (!fitsInBuffer) {
                return DAWN_VALIDATION_ERROR("The resolved query data would overflow the buffer");
            }

            return {};
        }

    }  // namespace

    CommandEncoder::CommandEncoder(DeviceBase* device, const CommandEncoderDescriptor*)
        : ObjectBase(device), mEncodingContext(device, this) {
    }

    CommandBufferResourceUsage CommandEncoder::AcquireResourceUsages() {
        return CommandBufferResourceUsage{mEncodingContext.AcquirePassUsages(),
                                          std::move(mTopLevelBuffers), std::move(mTopLevelTextures),
                                          std::move(mUsedQuerySets)};
    }

    CommandIterator CommandEncoder::AcquireCommands() {
        return mEncodingContext.AcquireCommands();
    }

    void CommandEncoder::TrackUsedQuerySet(QuerySetBase* querySet) {
        mUsedQuerySets.insert(querySet);
    }

    // Implementation of the API's command recording methods

    ComputePassEncoder* CommandEncoder::BeginComputePass(const ComputePassDescriptor* descriptor) {
        DeviceBase* device = GetDevice();

        bool success =
            mEncodingContext.TryEncode(this, [&](CommandAllocator* allocator) -> MaybeError {
                DAWN_TRY(ValidateComputePassDescriptor(device, descriptor));

                allocator->Allocate<BeginComputePassCmd>(Command::BeginComputePass);

                return {};
            });

        if (success) {
            ComputePassEncoder* passEncoder =
                new ComputePassEncoder(device, this, &mEncodingContext);
            mEncodingContext.EnterPass(passEncoder);
            return passEncoder;
        }

        return ComputePassEncoder::MakeError(device, this, &mEncodingContext);
    }

    RenderPassEncoder* CommandEncoder::BeginRenderPass(const RenderPassDescriptor* descriptor) {
        DeviceBase* device = GetDevice();

        PassResourceUsageTracker usageTracker(PassType::Render);
        bool success =
            mEncodingContext.TryEncode(this, [&](CommandAllocator* allocator) -> MaybeError {
                uint32_t width = 0;
                uint32_t height = 0;
                uint32_t sampleCount = 0;

                DAWN_TRY(ValidateRenderPassDescriptor(device, descriptor, &width, &height,
                                                      &sampleCount));

                ASSERT(width > 0 && height > 0 && sampleCount > 0);

                BeginRenderPassCmd* cmd =
                    allocator->Allocate<BeginRenderPassCmd>(Command::BeginRenderPass);

                cmd->attachmentState = device->GetOrCreateAttachmentState(descriptor);

                for (uint32_t i : IterateBitSet(cmd->attachmentState->GetColorAttachmentsMask())) {
                    TextureViewBase* view = descriptor->colorAttachments[i].attachment;
                    TextureViewBase* resolveTarget = descriptor->colorAttachments[i].resolveTarget;

                    cmd->colorAttachments[i].view = view;
                    cmd->colorAttachments[i].resolveTarget = resolveTarget;
                    cmd->colorAttachments[i].loadOp = descriptor->colorAttachments[i].loadOp;
                    cmd->colorAttachments[i].storeOp = descriptor->colorAttachments[i].storeOp;
                    cmd->colorAttachments[i].clearColor =
                        descriptor->colorAttachments[i].clearColor;

                    usageTracker.TextureViewUsedAs(view, wgpu::TextureUsage::OutputAttachment);

                    if (resolveTarget != nullptr) {
                        usageTracker.TextureViewUsedAs(resolveTarget,
                                                       wgpu::TextureUsage::OutputAttachment);
                    }
                }

                if (cmd->attachmentState->HasDepthStencilAttachment()) {
                    TextureViewBase* view = descriptor->depthStencilAttachment->attachment;

                    cmd->depthStencilAttachment.view = view;
                    cmd->depthStencilAttachment.clearDepth =
                        descriptor->depthStencilAttachment->clearDepth;
                    cmd->depthStencilAttachment.clearStencil =
                        descriptor->depthStencilAttachment->clearStencil;
                    cmd->depthStencilAttachment.depthLoadOp =
                        descriptor->depthStencilAttachment->depthLoadOp;
                    cmd->depthStencilAttachment.depthStoreOp =
                        descriptor->depthStencilAttachment->depthStoreOp;
                    cmd->depthStencilAttachment.stencilLoadOp =
                        descriptor->depthStencilAttachment->stencilLoadOp;
                    cmd->depthStencilAttachment.stencilStoreOp =
                        descriptor->depthStencilAttachment->stencilStoreOp;

                    usageTracker.TextureViewUsedAs(view, wgpu::TextureUsage::OutputAttachment);
                }

                cmd->width = width;
                cmd->height = height;

                return {};
            });

        if (success) {
            RenderPassEncoder* passEncoder =
                new RenderPassEncoder(device, this, &mEncodingContext, std::move(usageTracker));
            mEncodingContext.EnterPass(passEncoder);
            return passEncoder;
        }

        return RenderPassEncoder::MakeError(device, this, &mEncodingContext);
    }

    void CommandEncoder::CopyBufferToBuffer(BufferBase* source,
                                            uint64_t sourceOffset,
                                            BufferBase* destination,
                                            uint64_t destinationOffset,
                                            uint64_t size) {
        mEncodingContext.TryEncode(this, [&](CommandAllocator* allocator) -> MaybeError {
            if (GetDevice()->IsValidationEnabled()) {
                DAWN_TRY(GetDevice()->ValidateObject(source));
                DAWN_TRY(GetDevice()->ValidateObject(destination));

                if (source == destination) {
                    return DAWN_VALIDATION_ERROR(
                        "Source and destination cannot be the same buffer.");
                }

                DAWN_TRY(ValidateCopySizeFitsInBuffer(source, sourceOffset, size));
                DAWN_TRY(ValidateCopySizeFitsInBuffer(destination, destinationOffset, size));
                DAWN_TRY(ValidateB2BCopyAlignment(size, sourceOffset, destinationOffset));

                DAWN_TRY(ValidateCanUseAs(source, wgpu::BufferUsage::CopySrc));
                DAWN_TRY(ValidateCanUseAs(destination, wgpu::BufferUsage::CopyDst));

                mTopLevelBuffers.insert(source);
                mTopLevelBuffers.insert(destination);
            }

            // Skip noop copies. Some backends validation rules disallow them.
            if (size != 0) {
                CopyBufferToBufferCmd* copy =
                    allocator->Allocate<CopyBufferToBufferCmd>(Command::CopyBufferToBuffer);
                copy->source = source;
                copy->sourceOffset = sourceOffset;
                copy->destination = destination;
                copy->destinationOffset = destinationOffset;
                copy->size = size;
            }

            return {};
        });
    }

    void CommandEncoder::CopyBufferToTexture(const BufferCopyView* source,
                                             const TextureCopyView* destination,
                                             const Extent3D* copySize) {
        mEncodingContext.TryEncode(this, [&](CommandAllocator* allocator) -> MaybeError {
            // TODO(crbug.com/dawn/22): Remove once migration from GPUTextureCopyView.arrayLayer to
            // GPUTextureCopyView.origin.z is done.
            TextureCopyView fixedDest;
            DAWN_TRY_ASSIGN(fixedDest, FixTextureCopyView(GetDevice(), destination));
            destination = &fixedDest;

            // TODO(crbug.com/dawn/22): Remove once migration to .layout is done.
            BufferCopyView fixedSource;
            DAWN_TRY_ASSIGN(fixedSource, FixBufferCopyView(GetDevice(), source));
            source = &fixedSource;

            if (GetDevice()->IsValidationEnabled()) {
                DAWN_TRY(ValidateBufferCopyView(GetDevice(), *source));
                DAWN_TRY(ValidateCanUseAs(source->buffer, wgpu::BufferUsage::CopySrc));

                DAWN_TRY(ValidateTextureCopyView(GetDevice(), *destination));
                DAWN_TRY(ValidateCanUseAs(destination->texture, wgpu::TextureUsage::CopyDst));
                DAWN_TRY(ValidateTextureSampleCountInCopyCommands(destination->texture));

                // We validate texture copy range before validating linear texture data,
                // because in the latter we divide copyExtent.width by blockWidth and
                // copyExtent.height by blockHeight while the divisibility conditions are
                // checked in validating texture copy range.
                DAWN_TRY(ValidateTextureCopyRange(*destination, *copySize));
                DAWN_TRY(ValidateLinearTextureData(source->layout, source->buffer->GetSize(),
                                                   destination->texture->GetFormat(), *copySize));

                mTopLevelBuffers.insert(source->buffer);
                mTopLevelTextures.insert(destination->texture);
            }

            // Compute default value for rowsPerImage
            uint32_t defaultedRowsPerImage = source->layout.rowsPerImage;
            if (defaultedRowsPerImage == 0) {
                defaultedRowsPerImage = copySize->height;
            }

            // Record the copy command.
            CopyBufferToTextureCmd* copy =
                allocator->Allocate<CopyBufferToTextureCmd>(Command::CopyBufferToTexture);
            copy->source.buffer = source->buffer;
            copy->source.offset = source->layout.offset;
            copy->source.bytesPerRow = source->layout.bytesPerRow;
            copy->source.rowsPerImage = defaultedRowsPerImage;
            copy->destination.texture = destination->texture;
            copy->destination.origin = destination->origin;
            copy->destination.mipLevel = destination->mipLevel;
            copy->copySize = *copySize;

            return {};
        });
    }

    void CommandEncoder::CopyTextureToBuffer(const TextureCopyView* source,
                                             const BufferCopyView* destination,
                                             const Extent3D* copySize) {
        mEncodingContext.TryEncode(this, [&](CommandAllocator* allocator) -> MaybeError {
            // TODO(crbug.com/dawn/22): Remove once migration from GPUTextureCopyView.arrayLayer to
            // GPUTextureCopyView.origin.z is done.
            TextureCopyView fixedSrc;
            DAWN_TRY_ASSIGN(fixedSrc, FixTextureCopyView(GetDevice(), source));
            source = &fixedSrc;

            // TODO(crbug.com/dawn/22): Remove once migration to .layout is done.
            BufferCopyView fixedDst;
            DAWN_TRY_ASSIGN(fixedDst, FixBufferCopyView(GetDevice(), destination));
            destination = &fixedDst;

            if (GetDevice()->IsValidationEnabled()) {
                DAWN_TRY(ValidateTextureCopyView(GetDevice(), *source));
                DAWN_TRY(ValidateCanUseAs(source->texture, wgpu::TextureUsage::CopySrc));
                DAWN_TRY(ValidateTextureSampleCountInCopyCommands(source->texture));

                DAWN_TRY(ValidateBufferCopyView(GetDevice(), *destination));
                DAWN_TRY(ValidateCanUseAs(destination->buffer, wgpu::BufferUsage::CopyDst));

                // We validate texture copy range before validating linear texture data,
                // because in the latter we divide copyExtent.width by blockWidth and
                // copyExtent.height by blockHeight while the divisibility conditions are
                // checked in validating texture copy range.
                DAWN_TRY(ValidateTextureCopyRange(*source, *copySize));
                DAWN_TRY(ValidateLinearTextureData(destination->layout,
                                                   destination->buffer->GetSize(),
                                                   source->texture->GetFormat(), *copySize));

                mTopLevelTextures.insert(source->texture);
                mTopLevelBuffers.insert(destination->buffer);
            }

            // Compute default value for rowsPerImage
            uint32_t defaultedRowsPerImage = destination->layout.rowsPerImage;
            if (defaultedRowsPerImage == 0) {
                defaultedRowsPerImage = copySize->height;
            }

            // Record the copy command.
            CopyTextureToBufferCmd* copy =
                allocator->Allocate<CopyTextureToBufferCmd>(Command::CopyTextureToBuffer);
            copy->source.texture = source->texture;
            copy->source.origin = source->origin;
            copy->source.mipLevel = source->mipLevel;
            copy->destination.buffer = destination->buffer;
            copy->destination.offset = destination->layout.offset;
            copy->destination.bytesPerRow = destination->layout.bytesPerRow;
            copy->destination.rowsPerImage = defaultedRowsPerImage;
            copy->copySize = *copySize;

            return {};
        });
    }

    void CommandEncoder::CopyTextureToTexture(const TextureCopyView* source,
                                              const TextureCopyView* destination,
                                              const Extent3D* copySize) {
        mEncodingContext.TryEncode(this, [&](CommandAllocator* allocator) -> MaybeError {
            // TODO(crbug.com/dawn/22): Remove once migration from GPUTextureCopyView.arrayLayer to
            // GPUTextureCopyView.origin.z is done.
            TextureCopyView fixedSrc;
            DAWN_TRY_ASSIGN(fixedSrc, FixTextureCopyView(GetDevice(), source));
            source = &fixedSrc;
            TextureCopyView fixedDest;
            DAWN_TRY_ASSIGN(fixedDest, FixTextureCopyView(GetDevice(), destination));
            destination = &fixedDest;

            if (GetDevice()->IsValidationEnabled()) {
                DAWN_TRY(GetDevice()->ValidateObject(source->texture));
                DAWN_TRY(GetDevice()->ValidateObject(destination->texture));

                DAWN_TRY(
                    ValidateTextureToTextureCopyRestrictions(*source, *destination, *copySize));

                DAWN_TRY(ValidateTextureCopyRange(*source, *copySize));
                DAWN_TRY(ValidateTextureCopyRange(*destination, *copySize));

                DAWN_TRY(ValidateTextureCopyView(GetDevice(), *source));
                DAWN_TRY(ValidateTextureCopyView(GetDevice(), *destination));

                DAWN_TRY(ValidateCanUseAs(source->texture, wgpu::TextureUsage::CopySrc));
                DAWN_TRY(ValidateCanUseAs(destination->texture, wgpu::TextureUsage::CopyDst));

                mTopLevelTextures.insert(source->texture);
                mTopLevelTextures.insert(destination->texture);
            }

            CopyTextureToTextureCmd* copy =
                allocator->Allocate<CopyTextureToTextureCmd>(Command::CopyTextureToTexture);
            copy->source.texture = source->texture;
            copy->source.origin = source->origin;
            copy->source.mipLevel = source->mipLevel;
            copy->destination.texture = destination->texture;
            copy->destination.origin = destination->origin;
            copy->destination.mipLevel = destination->mipLevel;
            copy->copySize = *copySize;

            return {};
        });
    }

    void CommandEncoder::InsertDebugMarker(const char* groupLabel) {
        mEncodingContext.TryEncode(this, [&](CommandAllocator* allocator) -> MaybeError {
            InsertDebugMarkerCmd* cmd =
                allocator->Allocate<InsertDebugMarkerCmd>(Command::InsertDebugMarker);
            cmd->length = strlen(groupLabel);

            char* label = allocator->AllocateData<char>(cmd->length + 1);
            memcpy(label, groupLabel, cmd->length + 1);

            return {};
        });
    }

    void CommandEncoder::PopDebugGroup() {
        mEncodingContext.TryEncode(this, [&](CommandAllocator* allocator) -> MaybeError {
            allocator->Allocate<PopDebugGroupCmd>(Command::PopDebugGroup);

            return {};
        });
    }

    void CommandEncoder::PushDebugGroup(const char* groupLabel) {
        mEncodingContext.TryEncode(this, [&](CommandAllocator* allocator) -> MaybeError {
            PushDebugGroupCmd* cmd =
                allocator->Allocate<PushDebugGroupCmd>(Command::PushDebugGroup);
            cmd->length = strlen(groupLabel);

            char* label = allocator->AllocateData<char>(cmd->length + 1);
            memcpy(label, groupLabel, cmd->length + 1);

            return {};
        });
    }

    void CommandEncoder::ResolveQuerySet(QuerySetBase* querySet,
                                         uint32_t firstQuery,
                                         uint32_t queryCount,
                                         BufferBase* destination,
                                         uint64_t destinationOffset) {
        mEncodingContext.TryEncode(this, [&](CommandAllocator* allocator) -> MaybeError {
            if (GetDevice()->IsValidationEnabled()) {
                DAWN_TRY(GetDevice()->ValidateObject(querySet));
                DAWN_TRY(GetDevice()->ValidateObject(destination));

                DAWN_TRY(ValidateQuerySetResolve(querySet, firstQuery, queryCount, destination,
                                                 destinationOffset));

                DAWN_TRY(ValidateCanUseAs(destination, wgpu::BufferUsage::QueryResolve));

                TrackUsedQuerySet(querySet);
                mTopLevelBuffers.insert(destination);
            }

            ResolveQuerySetCmd* cmd =
                allocator->Allocate<ResolveQuerySetCmd>(Command::ResolveQuerySet);
            cmd->querySet = querySet;
            cmd->firstQuery = firstQuery;
            cmd->queryCount = queryCount;
            cmd->destination = destination;
            cmd->destinationOffset = destinationOffset;

            return {};
        });
    }

    void CommandEncoder::WriteTimestamp(QuerySetBase* querySet, uint32_t queryIndex) {
        mEncodingContext.TryEncode(this, [&](CommandAllocator* allocator) -> MaybeError {
            if (GetDevice()->IsValidationEnabled()) {
                DAWN_TRY(GetDevice()->ValidateObject(querySet));
                DAWN_TRY(ValidateTimestampQuery(querySet, queryIndex));
                TrackUsedQuerySet(querySet);
            }

            WriteTimestampCmd* cmd =
                allocator->Allocate<WriteTimestampCmd>(Command::WriteTimestamp);
            cmd->querySet = querySet;
            cmd->queryIndex = queryIndex;

            return {};
        });
    }

    CommandBufferBase* CommandEncoder::Finish(const CommandBufferDescriptor* descriptor) {
        DeviceBase* device = GetDevice();
        // Even if mEncodingContext.Finish() validation fails, calling it will mutate the internal
        // state of the encoding context. The internal state is set to finished, and subsequent
        // calls to encode commands will generate errors.
        if (device->ConsumedError(mEncodingContext.Finish()) ||
            device->ConsumedError(device->ValidateIsAlive()) ||
            (device->IsValidationEnabled() &&
             device->ConsumedError(ValidateFinish(mEncodingContext.GetIterator(),
                                                  mEncodingContext.GetPassUsages())))) {
            return CommandBufferBase::MakeError(device);
        }
        ASSERT(!IsError());
        return device->CreateCommandBuffer(this, descriptor);
    }

    // Implementation of the command buffer validation that can be precomputed before submit
    MaybeError CommandEncoder::ValidateFinish(CommandIterator* commands,
                                              const PerPassUsages& perPassUsages) const {
        TRACE_EVENT0(GetDevice()->GetPlatform(), Validation, "CommandEncoder::ValidateFinish");
        DAWN_TRY(GetDevice()->ValidateObject(this));

        for (const PassResourceUsage& passUsage : perPassUsages) {
            DAWN_TRY(ValidatePassResourceUsage(passUsage));
        }

        uint64_t debugGroupStackSize = 0;

        commands->Reset();
        Command type;
        while (commands->NextCommandId(&type)) {
            switch (type) {
                case Command::BeginComputePass: {
                    commands->NextCommand<BeginComputePassCmd>();
                    DAWN_TRY(ValidateComputePass(commands));
                    break;
                }

                case Command::BeginRenderPass: {
                    const BeginRenderPassCmd* cmd = commands->NextCommand<BeginRenderPassCmd>();
                    DAWN_TRY(ValidateRenderPass(commands, cmd));
                    break;
                }

                case Command::CopyBufferToBuffer: {
                    commands->NextCommand<CopyBufferToBufferCmd>();
                    break;
                }

                case Command::CopyBufferToTexture: {
                    commands->NextCommand<CopyBufferToTextureCmd>();
                    break;
                }

                case Command::CopyTextureToBuffer: {
                    commands->NextCommand<CopyTextureToBufferCmd>();
                    break;
                }

                case Command::CopyTextureToTexture: {
                    commands->NextCommand<CopyTextureToTextureCmd>();
                    break;
                }

                case Command::InsertDebugMarker: {
                    const InsertDebugMarkerCmd* cmd = commands->NextCommand<InsertDebugMarkerCmd>();
                    commands->NextData<char>(cmd->length + 1);
                    break;
                }

                case Command::PopDebugGroup: {
                    commands->NextCommand<PopDebugGroupCmd>();
                    DAWN_TRY(ValidateCanPopDebugGroup(debugGroupStackSize));
                    debugGroupStackSize--;
                    break;
                }

                case Command::PushDebugGroup: {
                    const PushDebugGroupCmd* cmd = commands->NextCommand<PushDebugGroupCmd>();
                    commands->NextData<char>(cmd->length + 1);
                    debugGroupStackSize++;
                    break;
                }

                case Command::ResolveQuerySet: {
                    commands->NextCommand<ResolveQuerySetCmd>();
                    break;
                }

                case Command::WriteTimestamp: {
                    commands->NextCommand<WriteTimestampCmd>();
                    break;
                }
                default:
                    return DAWN_VALIDATION_ERROR("Command disallowed outside of a pass");
            }
        }

        DAWN_TRY(ValidateFinalDebugGroupStackSize(debugGroupStackSize));

        return {};
    }

}  // namespace dawn_native
