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

#include "dawn_native/opengl/CommandBufferGL.h"

#include "dawn_native/BindGroup.h"
#include "dawn_native/BindGroupTracker.h"
#include "dawn_native/CommandEncoder.h"
#include "dawn_native/Commands.h"
#include "dawn_native/RenderBundle.h"
#include "dawn_native/opengl/BufferGL.h"
#include "dawn_native/opengl/ComputePipelineGL.h"
#include "dawn_native/opengl/DeviceGL.h"
#include "dawn_native/opengl/Forward.h"
#include "dawn_native/opengl/PersistentPipelineStateGL.h"
#include "dawn_native/opengl/PipelineLayoutGL.h"
#include "dawn_native/opengl/RenderPipelineGL.h"
#include "dawn_native/opengl/SamplerGL.h"
#include "dawn_native/opengl/TextureGL.h"
#include "dawn_native/opengl/UtilsGL.h"

#include <cstring>

namespace dawn_native { namespace opengl {

    namespace {

        GLenum IndexFormatType(wgpu::IndexFormat format) {
            switch (format) {
                case wgpu::IndexFormat::Uint16:
                    return GL_UNSIGNED_SHORT;
                case wgpu::IndexFormat::Uint32:
                    return GL_UNSIGNED_INT;
                default:
                    UNREACHABLE();
            }
        }

        GLenum VertexFormatType(wgpu::VertexFormat format) {
            switch (format) {
                case wgpu::VertexFormat::UChar2:
                case wgpu::VertexFormat::UChar4:
                case wgpu::VertexFormat::UChar2Norm:
                case wgpu::VertexFormat::UChar4Norm:
                    return GL_UNSIGNED_BYTE;
                case wgpu::VertexFormat::Char2:
                case wgpu::VertexFormat::Char4:
                case wgpu::VertexFormat::Char2Norm:
                case wgpu::VertexFormat::Char4Norm:
                    return GL_BYTE;
                case wgpu::VertexFormat::UShort2:
                case wgpu::VertexFormat::UShort4:
                case wgpu::VertexFormat::UShort2Norm:
                case wgpu::VertexFormat::UShort4Norm:
                    return GL_UNSIGNED_SHORT;
                case wgpu::VertexFormat::Short2:
                case wgpu::VertexFormat::Short4:
                case wgpu::VertexFormat::Short2Norm:
                case wgpu::VertexFormat::Short4Norm:
                    return GL_SHORT;
                case wgpu::VertexFormat::Half2:
                case wgpu::VertexFormat::Half4:
                    return GL_HALF_FLOAT;
                case wgpu::VertexFormat::Float:
                case wgpu::VertexFormat::Float2:
                case wgpu::VertexFormat::Float3:
                case wgpu::VertexFormat::Float4:
                    return GL_FLOAT;
                case wgpu::VertexFormat::UInt:
                case wgpu::VertexFormat::UInt2:
                case wgpu::VertexFormat::UInt3:
                case wgpu::VertexFormat::UInt4:
                    return GL_UNSIGNED_INT;
                case wgpu::VertexFormat::Int:
                case wgpu::VertexFormat::Int2:
                case wgpu::VertexFormat::Int3:
                case wgpu::VertexFormat::Int4:
                    return GL_INT;
                default:
                    UNREACHABLE();
            }
        }

        GLboolean VertexFormatIsNormalized(wgpu::VertexFormat format) {
            switch (format) {
                case wgpu::VertexFormat::UChar2Norm:
                case wgpu::VertexFormat::UChar4Norm:
                case wgpu::VertexFormat::Char2Norm:
                case wgpu::VertexFormat::Char4Norm:
                case wgpu::VertexFormat::UShort2Norm:
                case wgpu::VertexFormat::UShort4Norm:
                case wgpu::VertexFormat::Short2Norm:
                case wgpu::VertexFormat::Short4Norm:
                    return GL_TRUE;
                default:
                    return GL_FALSE;
            }
        }

        bool VertexFormatIsInt(wgpu::VertexFormat format) {
            switch (format) {
                case wgpu::VertexFormat::UChar2:
                case wgpu::VertexFormat::UChar4:
                case wgpu::VertexFormat::Char2:
                case wgpu::VertexFormat::Char4:
                case wgpu::VertexFormat::UShort2:
                case wgpu::VertexFormat::UShort4:
                case wgpu::VertexFormat::Short2:
                case wgpu::VertexFormat::Short4:
                case wgpu::VertexFormat::UInt:
                case wgpu::VertexFormat::UInt2:
                case wgpu::VertexFormat::UInt3:
                case wgpu::VertexFormat::UInt4:
                case wgpu::VertexFormat::Int:
                case wgpu::VertexFormat::Int2:
                case wgpu::VertexFormat::Int3:
                case wgpu::VertexFormat::Int4:
                    return true;
                default:
                    return false;
            }
        }

        // Vertex buffers and index buffers are implemented as part of an OpenGL VAO that
        // corresponds to a VertexState. On the contrary in Dawn they are part of the global state.
        // This means that we have to re-apply these buffers on a VertexState change.
        class VertexStateBufferBindingTracker {
          public:
            void OnSetIndexBuffer(BufferBase* buffer) {
                mIndexBufferDirty = true;
                mIndexBuffer = ToBackend(buffer);
            }

            void OnSetVertexBuffer(uint32_t slot, BufferBase* buffer, uint64_t offset) {
                mVertexBuffers[slot] = ToBackend(buffer);
                mVertexBufferOffsets[slot] = offset;

                // Use 64 bit masks and make sure there are no shift UB
                static_assert(kMaxVertexBuffers <= 8 * sizeof(unsigned long long) - 1, "");
                mDirtyVertexBuffers |= 1ull << slot;
            }

            void OnSetPipeline(RenderPipelineBase* pipeline) {
                if (mLastPipeline == pipeline) {
                    return;
                }

                mIndexBufferDirty = true;
                mDirtyVertexBuffers |= pipeline->GetVertexBufferSlotsUsed();

                mLastPipeline = pipeline;
            }

            void Apply(const OpenGLFunctions& gl) {
                if (mIndexBufferDirty && mIndexBuffer != nullptr) {
                    gl.BindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer->GetHandle());
                    mIndexBufferDirty = false;
                }

                for (uint32_t slot : IterateBitSet(mDirtyVertexBuffers &
                                                   mLastPipeline->GetVertexBufferSlotsUsed())) {
                    for (uint32_t location :
                         IterateBitSet(mLastPipeline->GetAttributesUsingVertexBuffer(slot))) {
                        const VertexAttributeInfo& attribute =
                            mLastPipeline->GetAttribute(location);

                        GLuint buffer = mVertexBuffers[slot]->GetHandle();
                        uint64_t offset = mVertexBufferOffsets[slot];

                        const VertexBufferInfo& vertexBuffer = mLastPipeline->GetVertexBuffer(slot);
                        uint32_t components = VertexFormatNumComponents(attribute.format);
                        GLenum formatType = VertexFormatType(attribute.format);

                        GLboolean normalized = VertexFormatIsNormalized(attribute.format);
                        gl.BindBuffer(GL_ARRAY_BUFFER, buffer);
                        if (VertexFormatIsInt(attribute.format)) {
                            gl.VertexAttribIPointer(
                                location, components, formatType, vertexBuffer.arrayStride,
                                reinterpret_cast<void*>(
                                    static_cast<intptr_t>(offset + attribute.offset)));
                        } else {
                            gl.VertexAttribPointer(location, components, formatType, normalized,
                                                   vertexBuffer.arrayStride,
                                                   reinterpret_cast<void*>(static_cast<intptr_t>(
                                                       offset + attribute.offset)));
                        }
                    }
                }

                mDirtyVertexBuffers.reset();
            }

          private:
            bool mIndexBufferDirty = false;
            Buffer* mIndexBuffer = nullptr;

            std::bitset<kMaxVertexBuffers> mDirtyVertexBuffers;
            std::array<Buffer*, kMaxVertexBuffers> mVertexBuffers;
            std::array<uint64_t, kMaxVertexBuffers> mVertexBufferOffsets;

            RenderPipelineBase* mLastPipeline = nullptr;
        };

        class BindGroupTracker : public BindGroupTrackerBase<false, uint64_t> {
          public:
            void OnSetPipeline(RenderPipeline* pipeline) {
                BindGroupTrackerBase::OnSetPipeline(pipeline);
                mPipeline = pipeline;
            }

            void OnSetPipeline(ComputePipeline* pipeline) {
                BindGroupTrackerBase::OnSetPipeline(pipeline);
                mPipeline = pipeline;
            }

            void Apply(const OpenGLFunctions& gl) {
                for (BindGroupIndex index :
                     IterateBitSet(mDirtyBindGroupsObjectChangedOrIsDynamic)) {
                    ApplyBindGroup(gl, index, mBindGroups[index], mDynamicOffsetCounts[index],
                                   mDynamicOffsets[index].data());
                }
                DidApply();
            }

          private:
            void ApplyBindGroup(const OpenGLFunctions& gl,
                                BindGroupIndex index,
                                BindGroupBase* group,
                                uint32_t dynamicOffsetCount,
                                uint64_t* dynamicOffsets) {
                const auto& indices = ToBackend(mPipelineLayout)->GetBindingIndexInfo()[index];
                uint32_t currentDynamicOffsetIndex = 0;

                for (BindingIndex bindingIndex{0};
                     bindingIndex < group->GetLayout()->GetBindingCount(); ++bindingIndex) {
                    const BindingInfo& bindingInfo =
                        group->GetLayout()->GetBindingInfo(bindingIndex);

                    switch (bindingInfo.type) {
                        case wgpu::BindingType::UniformBuffer: {
                            BufferBinding binding = group->GetBindingAsBufferBinding(bindingIndex);
                            GLuint buffer = ToBackend(binding.buffer)->GetHandle();
                            GLuint uboIndex = indices[bindingIndex];
                            GLuint offset = binding.offset;

                            if (bindingInfo.hasDynamicOffset) {
                                offset += dynamicOffsets[currentDynamicOffsetIndex];
                                ++currentDynamicOffsetIndex;
                            }

                            gl.BindBufferRange(GL_UNIFORM_BUFFER, uboIndex, buffer, offset,
                                               binding.size);
                            break;
                        }

                        case wgpu::BindingType::StorageBuffer:
                        case wgpu::BindingType::ReadonlyStorageBuffer: {
                            BufferBinding binding = group->GetBindingAsBufferBinding(bindingIndex);
                            GLuint buffer = ToBackend(binding.buffer)->GetHandle();
                            GLuint ssboIndex = indices[bindingIndex];
                            GLuint offset = binding.offset;

                            if (bindingInfo.hasDynamicOffset) {
                                offset += dynamicOffsets[currentDynamicOffsetIndex];
                                ++currentDynamicOffsetIndex;
                            }

                            gl.BindBufferRange(GL_SHADER_STORAGE_BUFFER, ssboIndex, buffer, offset,
                                               binding.size);
                            break;
                        }

                        case wgpu::BindingType::Sampler:
                        case wgpu::BindingType::ComparisonSampler: {
                            Sampler* sampler = ToBackend(group->GetBindingAsSampler(bindingIndex));
                            GLuint samplerIndex = indices[bindingIndex];

                            for (PipelineGL::SamplerUnit unit :
                                 mPipeline->GetTextureUnitsForSampler(samplerIndex)) {
                                // Only use filtering for certain texture units, because int and
                                // uint texture are only complete without filtering
                                if (unit.shouldUseFiltering) {
                                    gl.BindSampler(unit.unit, sampler->GetFilteringHandle());
                                } else {
                                    gl.BindSampler(unit.unit, sampler->GetNonFilteringHandle());
                                }
                            }
                            break;
                        }

                        case wgpu::BindingType::SampledTexture: {
                            TextureView* view =
                                ToBackend(group->GetBindingAsTextureView(bindingIndex));
                            GLuint handle = view->GetHandle();
                            GLenum target = view->GetGLTarget();
                            GLuint viewIndex = indices[bindingIndex];

                            for (auto unit : mPipeline->GetTextureUnitsForTextureView(viewIndex)) {
                                gl.ActiveTexture(GL_TEXTURE0 + unit);
                                gl.BindTexture(target, handle);
                            }
                            break;
                        }

                        case wgpu::BindingType::ReadonlyStorageTexture:
                        case wgpu::BindingType::WriteonlyStorageTexture: {
                            TextureView* view =
                                ToBackend(group->GetBindingAsTextureView(bindingIndex));
                            Texture* texture = ToBackend(view->GetTexture());
                            GLuint handle = texture->GetHandle();
                            GLuint imageIndex = indices[bindingIndex];

                            GLenum access;
                            switch (bindingInfo.type) {
                                case wgpu::BindingType::ReadonlyStorageTexture:
                                    access = GL_READ_ONLY;
                                    break;
                                case wgpu::BindingType::WriteonlyStorageTexture:
                                    access = GL_WRITE_ONLY;
                                    break;
                                default:
                                    UNREACHABLE();
                                    break;
                            }

                            // OpenGL ES only supports either binding a layer or the entire texture
                            // in glBindImageTexture().
                            GLboolean isLayered;
                            if (view->GetLayerCount() == 1) {
                                isLayered = GL_FALSE;
                            } else if (texture->GetArrayLayers() == view->GetLayerCount()) {
                                isLayered = GL_TRUE;
                            } else {
                                UNREACHABLE();
                            }

                            gl.BindImageTexture(imageIndex, handle, view->GetBaseMipLevel(),
                                                isLayered, view->GetBaseArrayLayer(), access,
                                                texture->GetGLFormat().internalFormat);
                            break;
                        }

                        case wgpu::BindingType::StorageTexture:
                            UNREACHABLE();
                            break;

                            // TODO(shaobo.yan@intel.com): Implement dynamic buffer offset.
                    }
                }
            }

            PipelineGL* mPipeline = nullptr;
        };

        void ResolveMultisampledRenderTargets(const OpenGLFunctions& gl,
                                              const BeginRenderPassCmd* renderPass) {
            ASSERT(renderPass != nullptr);

            GLuint readFbo = 0;
            GLuint writeFbo = 0;

            for (uint32_t i :
                 IterateBitSet(renderPass->attachmentState->GetColorAttachmentsMask())) {
                if (renderPass->colorAttachments[i].resolveTarget.Get() != nullptr) {
                    if (readFbo == 0) {
                        ASSERT(writeFbo == 0);
                        gl.GenFramebuffers(1, &readFbo);
                        gl.GenFramebuffers(1, &writeFbo);
                    }

                    const TextureBase* colorTexture =
                        renderPass->colorAttachments[i].view->GetTexture();
                    ASSERT(colorTexture->IsMultisampledTexture());
                    ASSERT(colorTexture->GetArrayLayers() == 1);
                    ASSERT(renderPass->colorAttachments[i].view->GetBaseMipLevel() == 0);

                    GLuint colorHandle = ToBackend(colorTexture)->GetHandle();
                    gl.BindFramebuffer(GL_READ_FRAMEBUFFER, readFbo);
                    gl.FramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                            ToBackend(colorTexture)->GetGLTarget(), colorHandle, 0);

                    const TextureBase* resolveTexture =
                        renderPass->colorAttachments[i].resolveTarget->GetTexture();
                    GLuint resolveTextureHandle = ToBackend(resolveTexture)->GetHandle();
                    GLuint resolveTargetMipmapLevel =
                        renderPass->colorAttachments[i].resolveTarget->GetBaseMipLevel();
                    gl.BindFramebuffer(GL_DRAW_FRAMEBUFFER, writeFbo);
                    if (resolveTexture->GetArrayLayers() == 1) {
                        gl.FramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                                GL_TEXTURE_2D, resolveTextureHandle,
                                                resolveTargetMipmapLevel);
                    } else {
                        GLuint resolveTargetArrayLayer =
                            renderPass->colorAttachments[i].resolveTarget->GetBaseArrayLayer();
                        gl.FramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                                   resolveTextureHandle, resolveTargetMipmapLevel,
                                                   resolveTargetArrayLayer);
                    }

                    gl.BlitFramebuffer(0, 0, renderPass->width, renderPass->height, 0, 0,
                                       renderPass->width, renderPass->height, GL_COLOR_BUFFER_BIT,
                                       GL_NEAREST);
                }
            }

            gl.DeleteFramebuffers(1, &readFbo);
            gl.DeleteFramebuffers(1, &writeFbo);
        }

        // OpenGL SPEC requires the source/destination region must be a region that is contained
        // within srcImage/dstImage. Here the size of the image refers to the virtual size, while
        // Dawn validates texture copy extent with the physical size, so we need to re-calculate the
        // texture copy extent to ensure it should fit in the virtual size of the subresource.
        Extent3D ComputeTextureCopyExtent(const TextureCopy& textureCopy,
                                          const Extent3D& copySize) {
            Extent3D validTextureCopyExtent = copySize;
            const TextureBase* texture = textureCopy.texture.Get();
            Extent3D virtualSizeAtLevel = texture->GetMipLevelVirtualSize(textureCopy.mipLevel);
            if (textureCopy.origin.x + copySize.width > virtualSizeAtLevel.width) {
                ASSERT(texture->GetFormat().isCompressed);
                validTextureCopyExtent.width = virtualSizeAtLevel.width - textureCopy.origin.x;
            }
            if (textureCopy.origin.y + copySize.height > virtualSizeAtLevel.height) {
                ASSERT(texture->GetFormat().isCompressed);
                validTextureCopyExtent.height = virtualSizeAtLevel.height - textureCopy.origin.y;
            }

            return validTextureCopyExtent;
        }

    }  // namespace

    CommandBuffer::CommandBuffer(CommandEncoder* encoder, const CommandBufferDescriptor* descriptor)
        : CommandBufferBase(encoder, descriptor), mCommands(encoder->AcquireCommands()) {
    }

    CommandBuffer::~CommandBuffer() {
        FreeCommands(&mCommands);
    }

    void CommandBuffer::Execute() {
        const OpenGLFunctions& gl = ToBackend(GetDevice())->gl;

        auto TransitionForPass = [](const PassResourceUsage& usages) {
            for (size_t i = 0; i < usages.textures.size(); i++) {
                Texture* texture = ToBackend(usages.textures[i]);
                // Clear textures that are not output attachments. Output attachments will be
                // cleared in BeginRenderPass by setting the loadop to clear when the
                // texture subresource has not been initialized before the render pass.
                if (!(usages.textureUsages[i].usage & wgpu::TextureUsage::OutputAttachment)) {
                    texture->EnsureSubresourceContentInitialized(texture->GetAllSubresources());
                }
            }
        };

        const std::vector<PassResourceUsage>& passResourceUsages = GetResourceUsages().perPass;
        uint32_t nextPassNumber = 0;

        Command type;
        while (mCommands.NextCommandId(&type)) {
            switch (type) {
                case Command::BeginComputePass: {
                    mCommands.NextCommand<BeginComputePassCmd>();
                    TransitionForPass(passResourceUsages[nextPassNumber]);
                    ExecuteComputePass();

                    nextPassNumber++;
                    break;
                }

                case Command::BeginRenderPass: {
                    auto* cmd = mCommands.NextCommand<BeginRenderPassCmd>();
                    TransitionForPass(passResourceUsages[nextPassNumber]);

                    LazyClearRenderPassAttachments(cmd);
                    ExecuteRenderPass(cmd);

                    nextPassNumber++;
                    break;
                }

                case Command::CopyBufferToBuffer: {
                    CopyBufferToBufferCmd* copy = mCommands.NextCommand<CopyBufferToBufferCmd>();

                    ToBackend(copy->source)->EnsureDataInitialized();
                    ToBackend(copy->destination)
                        ->EnsureDataInitializedAsDestination(copy->destinationOffset, copy->size);

                    gl.BindBuffer(GL_PIXEL_PACK_BUFFER, ToBackend(copy->source)->GetHandle());
                    gl.BindBuffer(GL_PIXEL_UNPACK_BUFFER,
                                  ToBackend(copy->destination)->GetHandle());
                    gl.CopyBufferSubData(GL_PIXEL_PACK_BUFFER, GL_PIXEL_UNPACK_BUFFER,
                                         copy->sourceOffset, copy->destinationOffset, copy->size);

                    gl.BindBuffer(GL_PIXEL_PACK_BUFFER, 0);
                    gl.BindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
                    break;
                }

                case Command::CopyBufferToTexture: {
                    CopyBufferToTextureCmd* copy = mCommands.NextCommand<CopyBufferToTextureCmd>();
                    auto& src = copy->source;
                    auto& dst = copy->destination;
                    auto& copySize = copy->copySize;
                    Buffer* buffer = ToBackend(src.buffer.Get());
                    Texture* texture = ToBackend(dst.texture.Get());
                    GLenum target = texture->GetGLTarget();
                    const GLFormat& format = texture->GetGLFormat();

                    buffer->EnsureDataInitialized();

                    ASSERT(texture->GetDimension() == wgpu::TextureDimension::e2D);
                    SubresourceRange subresources = {dst.mipLevel, 1, dst.origin.z,
                                                     copy->copySize.depth};
                    if (IsCompleteSubresourceCopiedTo(texture, copySize, dst.mipLevel)) {
                        texture->SetIsSubresourceContentInitialized(true, subresources);
                    } else {
                        texture->EnsureSubresourceContentInitialized(subresources);
                    }

                    gl.BindBuffer(GL_PIXEL_UNPACK_BUFFER, buffer->GetHandle());
                    gl.ActiveTexture(GL_TEXTURE0);
                    gl.BindTexture(target, texture->GetHandle());

                    const Format& formatInfo = texture->GetFormat();
                    gl.PixelStorei(
                        GL_UNPACK_ROW_LENGTH,
                        src.bytesPerRow / formatInfo.blockByteSize * formatInfo.blockWidth);
                    gl.PixelStorei(GL_UNPACK_IMAGE_HEIGHT, src.rowsPerImage);

                    if (formatInfo.isCompressed) {
                        gl.PixelStorei(GL_UNPACK_COMPRESSED_BLOCK_SIZE, formatInfo.blockByteSize);
                        gl.PixelStorei(GL_UNPACK_COMPRESSED_BLOCK_WIDTH, formatInfo.blockWidth);
                        gl.PixelStorei(GL_UNPACK_COMPRESSED_BLOCK_HEIGHT, formatInfo.blockHeight);
                        gl.PixelStorei(GL_UNPACK_COMPRESSED_BLOCK_DEPTH, 1);

                        ASSERT(texture->GetDimension() == wgpu::TextureDimension::e2D);
                        uint64_t copyDataSize = (copySize.width / formatInfo.blockWidth) *
                                                (copySize.height / formatInfo.blockHeight) *
                                                formatInfo.blockByteSize * copySize.depth;
                        Extent3D copyExtent = ComputeTextureCopyExtent(dst, copySize);

                        if (texture->GetArrayLayers() > 1) {
                            gl.CompressedTexSubImage3D(
                                target, dst.mipLevel, dst.origin.x, dst.origin.y, dst.origin.z,
                                copyExtent.width, copyExtent.height, copyExtent.depth,
                                format.internalFormat, copyDataSize,
                                reinterpret_cast<void*>(static_cast<uintptr_t>(src.offset)));
                        } else {
                            gl.CompressedTexSubImage2D(
                                target, dst.mipLevel, dst.origin.x, dst.origin.y, copyExtent.width,
                                copyExtent.height, format.internalFormat, copyDataSize,
                                reinterpret_cast<void*>(static_cast<uintptr_t>(src.offset)));
                        }
                    } else {
                        switch (texture->GetDimension()) {
                            case wgpu::TextureDimension::e2D:
                                if (texture->GetArrayLayers() > 1) {
                                    gl.TexSubImage3D(target, dst.mipLevel, dst.origin.x,
                                                     dst.origin.y, dst.origin.z, copySize.width,
                                                     copySize.height, copySize.depth, format.format,
                                                     format.type,
                                                     reinterpret_cast<void*>(
                                                         static_cast<uintptr_t>(src.offset)));
                                } else {
                                    gl.TexSubImage2D(target, dst.mipLevel, dst.origin.x,
                                                     dst.origin.y, copySize.width, copySize.height,
                                                     format.format, format.type,
                                                     reinterpret_cast<void*>(
                                                         static_cast<uintptr_t>(src.offset)));
                                }
                                break;

                            default:
                                UNREACHABLE();
                        }
                    }

                    gl.PixelStorei(GL_UNPACK_ROW_LENGTH, 0);
                    gl.PixelStorei(GL_UNPACK_IMAGE_HEIGHT, 0);

                    gl.BindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
                    break;
                }

                case Command::CopyTextureToBuffer: {
                    CopyTextureToBufferCmd* copy = mCommands.NextCommand<CopyTextureToBufferCmd>();
                    auto& src = copy->source;
                    auto& dst = copy->destination;
                    auto& copySize = copy->copySize;
                    Texture* texture = ToBackend(src.texture.Get());
                    Buffer* buffer = ToBackend(dst.buffer.Get());
                    const Format& format = texture->GetFormat();
                    const GLFormat& glFormat = texture->GetGLFormat();
                    GLenum target = texture->GetGLTarget();

                    // TODO(jiawei.shao@intel.com): support texture-to-buffer copy with compressed
                    // texture formats.
                    if (format.isCompressed) {
                        UNREACHABLE();
                    }

                    buffer->EnsureDataInitializedAsDestination(copy);

                    ASSERT(texture->GetDimension() == wgpu::TextureDimension::e2D);
                    SubresourceRange subresources = {src.mipLevel, 1, src.origin.z,
                                                     copy->copySize.depth};
                    texture->EnsureSubresourceContentInitialized(subresources);
                    // The only way to move data from a texture to a buffer in GL is via
                    // glReadPixels with a pack buffer. Create a temporary FBO for the copy.
                    gl.BindTexture(target, texture->GetHandle());

                    GLuint readFBO = 0;
                    gl.GenFramebuffers(1, &readFBO);
                    gl.BindFramebuffer(GL_READ_FRAMEBUFFER, readFBO);

                    GLenum glAttachment = 0;
                    switch (format.aspect) {
                        case Format::Aspect::Color:
                            glAttachment = GL_COLOR_ATTACHMENT0;
                            break;
                        case Format::Aspect::Depth:
                            glAttachment = GL_DEPTH_ATTACHMENT;
                            break;
                        case Format::Aspect::Stencil:
                            glAttachment = GL_STENCIL_ATTACHMENT;
                            break;
                        case Format::Aspect::DepthStencil:
                            glAttachment = GL_DEPTH_STENCIL_ATTACHMENT;
                            break;
                        default:
                            UNREACHABLE();
                            break;
                    }

                    gl.BindBuffer(GL_PIXEL_PACK_BUFFER, buffer->GetHandle());
                    gl.PixelStorei(GL_PACK_ROW_LENGTH, dst.bytesPerRow / format.blockByteSize);
                    gl.PixelStorei(GL_PACK_IMAGE_HEIGHT, dst.rowsPerImage);

                    uint8_t* offset =
                        reinterpret_cast<uint8_t*>(static_cast<uintptr_t>(dst.offset));
                    switch (texture->GetDimension()) {
                        case wgpu::TextureDimension::e2D: {
                            if (texture->GetArrayLayers() == 1) {
                                gl.FramebufferTexture2D(GL_READ_FRAMEBUFFER, glAttachment, target,
                                                        texture->GetHandle(), src.mipLevel);
                                gl.ReadPixels(src.origin.x, src.origin.y, copySize.width,
                                              copySize.height, glFormat.format, glFormat.type,
                                              offset);
                                break;
                            }

                            const uint64_t bytesPerImage = dst.bytesPerRow * dst.rowsPerImage;
                            for (uint32_t layer = 0; layer < copySize.depth; ++layer) {
                                gl.FramebufferTextureLayer(GL_READ_FRAMEBUFFER, glAttachment,
                                                           texture->GetHandle(), src.mipLevel,
                                                           src.origin.z + layer);
                                gl.ReadPixels(src.origin.x, src.origin.y, copySize.width,
                                              copySize.height, glFormat.format, glFormat.type,
                                              offset);

                                offset += bytesPerImage;
                            }

                            break;
                        }

                        default:
                            UNREACHABLE();
                    }

                    gl.PixelStorei(GL_PACK_ROW_LENGTH, 0);
                    gl.PixelStorei(GL_PACK_IMAGE_HEIGHT, 0);

                    gl.BindBuffer(GL_PIXEL_PACK_BUFFER, 0);
                    gl.DeleteFramebuffers(1, &readFBO);
                    break;
                }

                case Command::CopyTextureToTexture: {
                    CopyTextureToTextureCmd* copy =
                        mCommands.NextCommand<CopyTextureToTextureCmd>();
                    auto& src = copy->source;
                    auto& dst = copy->destination;

                    // TODO(jiawei.shao@intel.com): add workaround for the case that imageExtentSrc
                    // is not equal to imageExtentDst. For example when copySize fits in the virtual
                    // size of the source image but does not fit in the one of the destination
                    // image.
                    Extent3D copySize = ComputeTextureCopyExtent(dst, copy->copySize);
                    Texture* srcTexture = ToBackend(src.texture.Get());
                    Texture* dstTexture = ToBackend(dst.texture.Get());
                    SubresourceRange srcRange = {src.mipLevel, 1, src.origin.z,
                                                 copy->copySize.depth};
                    SubresourceRange dstRange = {dst.mipLevel, 1, dst.origin.z,
                                                 copy->copySize.depth};

                    srcTexture->EnsureSubresourceContentInitialized(srcRange);
                    if (IsCompleteSubresourceCopiedTo(dstTexture, copySize, dst.mipLevel)) {
                        dstTexture->SetIsSubresourceContentInitialized(true, dstRange);
                    } else {
                        dstTexture->EnsureSubresourceContentInitialized(dstRange);
                    }
                    gl.CopyImageSubData(srcTexture->GetHandle(), srcTexture->GetGLTarget(),
                                        src.mipLevel, src.origin.x, src.origin.y, src.origin.z,
                                        dstTexture->GetHandle(), dstTexture->GetGLTarget(),
                                        dst.mipLevel, dst.origin.x, dst.origin.y, dst.origin.z,
                                        copySize.width, copySize.height, copy->copySize.depth);
                    break;
                }

                case Command::ResolveQuerySet: {
                    // TODO(hao.x.li@intel.com): Resolve non-precise occlusion query.
                    SkipCommand(&mCommands, type);
                    break;
                }

                case Command::WriteTimestamp: {
                    // WriteTimestamp is not supported on OpenGL
                    UNREACHABLE();
                    break;
                }

                default: {
                    UNREACHABLE();
                    break;
                }
            }
        }
    }

    void CommandBuffer::ExecuteComputePass() {
        const OpenGLFunctions& gl = ToBackend(GetDevice())->gl;
        ComputePipeline* lastPipeline = nullptr;
        BindGroupTracker bindGroupTracker = {};

        Command type;
        while (mCommands.NextCommandId(&type)) {
            switch (type) {
                case Command::EndComputePass: {
                    mCommands.NextCommand<EndComputePassCmd>();
                    return;
                }

                case Command::Dispatch: {
                    DispatchCmd* dispatch = mCommands.NextCommand<DispatchCmd>();
                    bindGroupTracker.Apply(gl);

                    gl.DispatchCompute(dispatch->x, dispatch->y, dispatch->z);
                    // TODO(cwallez@chromium.org): add barriers to the API
                    gl.MemoryBarrier(GL_ALL_BARRIER_BITS);
                    break;
                }

                case Command::DispatchIndirect: {
                    DispatchIndirectCmd* dispatch = mCommands.NextCommand<DispatchIndirectCmd>();
                    bindGroupTracker.Apply(gl);

                    uint64_t indirectBufferOffset = dispatch->indirectOffset;
                    Buffer* indirectBuffer = ToBackend(dispatch->indirectBuffer.Get());

                    gl.BindBuffer(GL_DISPATCH_INDIRECT_BUFFER, indirectBuffer->GetHandle());
                    gl.DispatchComputeIndirect(static_cast<GLintptr>(indirectBufferOffset));
                    // TODO(cwallez@chromium.org): add barriers to the API
                    gl.MemoryBarrier(GL_ALL_BARRIER_BITS);
                    break;
                }

                case Command::SetComputePipeline: {
                    SetComputePipelineCmd* cmd = mCommands.NextCommand<SetComputePipelineCmd>();
                    lastPipeline = ToBackend(cmd->pipeline).Get();
                    lastPipeline->ApplyNow();

                    bindGroupTracker.OnSetPipeline(lastPipeline);
                    break;
                }

                case Command::SetBindGroup: {
                    SetBindGroupCmd* cmd = mCommands.NextCommand<SetBindGroupCmd>();
                    uint32_t* dynamicOffsets = nullptr;
                    if (cmd->dynamicOffsetCount > 0) {
                        dynamicOffsets = mCommands.NextData<uint32_t>(cmd->dynamicOffsetCount);
                    }
                    bindGroupTracker.OnSetBindGroup(cmd->index, cmd->group.Get(),
                                                    cmd->dynamicOffsetCount, dynamicOffsets);
                    break;
                }

                case Command::InsertDebugMarker:
                case Command::PopDebugGroup:
                case Command::PushDebugGroup: {
                    // Due to lack of linux driver support for GL_EXT_debug_marker
                    // extension these functions are skipped.
                    SkipCommand(&mCommands, type);
                    break;
                }

                case Command::WriteTimestamp: {
                    // WriteTimestamp is not supported on OpenGL
                    UNREACHABLE();
                    break;
                }

                default: {
                    UNREACHABLE();
                    break;
                }
            }
        }

        // EndComputePass should have been called
        UNREACHABLE();
    }

    void CommandBuffer::ExecuteRenderPass(BeginRenderPassCmd* renderPass) {
        const OpenGLFunctions& gl = ToBackend(GetDevice())->gl;
        GLuint fbo = 0;

        // Create the framebuffer used for this render pass and calls the correct glDrawBuffers
        {
            // TODO(kainino@chromium.org): This is added to possibly work around an issue seen on
            // Windows/Intel. It should break any feedback loop before the clears, even if there
            // shouldn't be any negative effects from this. Investigate whether it's actually
            // needed.
            gl.BindFramebuffer(GL_READ_FRAMEBUFFER, 0);
            // TODO(kainino@chromium.org): possible future optimization: create these framebuffers
            // at Framebuffer build time (or maybe CommandBuffer build time) so they don't have to
            // be created and destroyed at draw time.
            gl.GenFramebuffers(1, &fbo);
            gl.BindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);

            // Mapping from attachmentSlot to GL framebuffer attachment points. Defaults to zero
            // (GL_NONE).
            std::array<GLenum, kMaxColorAttachments> drawBuffers = {};

            // Construct GL framebuffer

            unsigned int attachmentCount = 0;
            for (uint32_t i :
                 IterateBitSet(renderPass->attachmentState->GetColorAttachmentsMask())) {
                TextureViewBase* textureView = renderPass->colorAttachments[i].view.Get();
                GLuint texture = ToBackend(textureView->GetTexture())->GetHandle();

                // Attach color buffers.
                if (textureView->GetTexture()->GetArrayLayers() == 1) {
                    GLenum target = ToBackend(textureView->GetTexture())->GetGLTarget();
                    gl.FramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, target,
                                            texture, textureView->GetBaseMipLevel());
                } else {
                    gl.FramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i,
                                               texture, textureView->GetBaseMipLevel(),
                                               textureView->GetBaseArrayLayer());
                }
                drawBuffers[i] = GL_COLOR_ATTACHMENT0 + i;
                attachmentCount = i + 1;
            }
            gl.DrawBuffers(attachmentCount, drawBuffers.data());

            if (renderPass->attachmentState->HasDepthStencilAttachment()) {
                TextureViewBase* textureView = renderPass->depthStencilAttachment.view.Get();
                GLuint texture = ToBackend(textureView->GetTexture())->GetHandle();
                const Format& format = textureView->GetTexture()->GetFormat();

                // Attach depth/stencil buffer.
                GLenum glAttachment = 0;
                // TODO(kainino@chromium.org): it may be valid to just always use
                // GL_DEPTH_STENCIL_ATTACHMENT here.
                switch (format.aspect) {
                    case Format::Aspect::Depth:
                        glAttachment = GL_DEPTH_ATTACHMENT;
                        break;
                    case Format::Aspect::Stencil:
                        glAttachment = GL_STENCIL_ATTACHMENT;
                        break;
                    case Format::Aspect::DepthStencil:
                        glAttachment = GL_DEPTH_STENCIL_ATTACHMENT;
                        break;
                    default:
                        UNREACHABLE();
                        break;
                }

                if (textureView->GetTexture()->GetArrayLayers() == 1) {
                    GLenum target = ToBackend(textureView->GetTexture())->GetGLTarget();
                    gl.FramebufferTexture2D(GL_DRAW_FRAMEBUFFER, glAttachment, target, texture,
                                            textureView->GetBaseMipLevel());
                } else {
                    gl.FramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, glAttachment, texture,
                                               textureView->GetBaseMipLevel(),
                                               textureView->GetBaseArrayLayer());
                }
            }
        }

        ASSERT(gl.CheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

        // Set defaults for dynamic state before executing clears and commands.
        PersistentPipelineState persistentPipelineState;
        persistentPipelineState.SetDefaultState(gl);
        gl.BlendColor(0, 0, 0, 0);
        gl.Viewport(0, 0, renderPass->width, renderPass->height);
        gl.DepthRangef(0.0, 1.0);
        gl.Scissor(0, 0, renderPass->width, renderPass->height);

        // Clear framebuffer attachments as needed
        {
            for (uint32_t i :
                 IterateBitSet(renderPass->attachmentState->GetColorAttachmentsMask())) {
                auto* attachmentInfo = &renderPass->colorAttachments[i];

                // Load op - color
                // TODO(cwallez@chromium.org): Choose the clear function depending on the
                // componentType: things work for now because the clear color is always a float, but
                // when that's fixed will lose precision on integer formats when converting to
                // float.
                if (attachmentInfo->loadOp == wgpu::LoadOp::Clear) {
                    gl.ColorMaski(i, true, true, true, true);
                    gl.ClearBufferfv(GL_COLOR, i, &attachmentInfo->clearColor.r);
                }

                if (attachmentInfo->storeOp == wgpu::StoreOp::Clear) {
                    // TODO(natlee@microsoft.com): call glDiscard to do optimization
                }
            }

            if (renderPass->attachmentState->HasDepthStencilAttachment()) {
                auto* attachmentInfo = &renderPass->depthStencilAttachment;
                const Format& attachmentFormat = attachmentInfo->view->GetTexture()->GetFormat();

                // Load op - depth/stencil
                bool doDepthClear = attachmentFormat.HasDepth() &&
                                    (attachmentInfo->depthLoadOp == wgpu::LoadOp::Clear);
                bool doStencilClear = attachmentFormat.HasStencil() &&
                                      (attachmentInfo->stencilLoadOp == wgpu::LoadOp::Clear);

                if (doDepthClear) {
                    gl.DepthMask(GL_TRUE);
                }
                if (doStencilClear) {
                    gl.StencilMask(GetStencilMaskFromStencilFormat(attachmentFormat.format));
                }

                if (doDepthClear && doStencilClear) {
                    gl.ClearBufferfi(GL_DEPTH_STENCIL, 0, attachmentInfo->clearDepth,
                                     attachmentInfo->clearStencil);
                } else if (doDepthClear) {
                    gl.ClearBufferfv(GL_DEPTH, 0, &attachmentInfo->clearDepth);
                } else if (doStencilClear) {
                    const GLint clearStencil = attachmentInfo->clearStencil;
                    gl.ClearBufferiv(GL_STENCIL, 0, &clearStencil);
                }
            }
        }

        RenderPipeline* lastPipeline = nullptr;
        uint64_t indexBufferBaseOffset = 0;

        VertexStateBufferBindingTracker vertexStateBufferBindingTracker;
        BindGroupTracker bindGroupTracker = {};

        auto DoRenderBundleCommand = [&](CommandIterator* iter, Command type) {
            switch (type) {
                case Command::Draw: {
                    DrawCmd* draw = iter->NextCommand<DrawCmd>();
                    vertexStateBufferBindingTracker.Apply(gl);
                    bindGroupTracker.Apply(gl);

                    if (draw->firstInstance > 0) {
                        gl.DrawArraysInstancedBaseInstance(
                            lastPipeline->GetGLPrimitiveTopology(), draw->firstVertex,
                            draw->vertexCount, draw->instanceCount, draw->firstInstance);
                    } else {
                        // This branch is only needed on OpenGL < 4.2
                        gl.DrawArraysInstanced(lastPipeline->GetGLPrimitiveTopology(),
                                               draw->firstVertex, draw->vertexCount,
                                               draw->instanceCount);
                    }
                    break;
                }

                case Command::DrawIndexed: {
                    DrawIndexedCmd* draw = iter->NextCommand<DrawIndexedCmd>();
                    vertexStateBufferBindingTracker.Apply(gl);
                    bindGroupTracker.Apply(gl);

                    wgpu::IndexFormat indexFormat =
                        lastPipeline->GetVertexStateDescriptor()->indexFormat;
                    size_t formatSize = IndexFormatSize(indexFormat);
                    GLenum formatType = IndexFormatType(indexFormat);

                    if (draw->firstInstance > 0) {
                        gl.DrawElementsInstancedBaseVertexBaseInstance(
                            lastPipeline->GetGLPrimitiveTopology(), draw->indexCount, formatType,
                            reinterpret_cast<void*>(draw->firstIndex * formatSize +
                                                    indexBufferBaseOffset),
                            draw->instanceCount, draw->baseVertex, draw->firstInstance);
                    } else {
                        // This branch is only needed on OpenGL < 4.2; ES < 3.2
                        if (draw->baseVertex != 0) {
                            gl.DrawElementsInstancedBaseVertex(
                                lastPipeline->GetGLPrimitiveTopology(), draw->indexCount,
                                formatType,
                                reinterpret_cast<void*>(draw->firstIndex * formatSize +
                                                        indexBufferBaseOffset),
                                draw->instanceCount, draw->baseVertex);
                        } else {
                            // This branch is only needed on OpenGL < 3.2; ES < 3.2
                            gl.DrawElementsInstanced(
                                lastPipeline->GetGLPrimitiveTopology(), draw->indexCount,
                                formatType,
                                reinterpret_cast<void*>(draw->firstIndex * formatSize +
                                                        indexBufferBaseOffset),
                                draw->instanceCount);
                        }
                    }
                    break;
                }

                case Command::DrawIndirect: {
                    DrawIndirectCmd* draw = iter->NextCommand<DrawIndirectCmd>();
                    vertexStateBufferBindingTracker.Apply(gl);
                    bindGroupTracker.Apply(gl);

                    uint64_t indirectBufferOffset = draw->indirectOffset;
                    Buffer* indirectBuffer = ToBackend(draw->indirectBuffer.Get());

                    gl.BindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectBuffer->GetHandle());
                    gl.DrawArraysIndirect(
                        lastPipeline->GetGLPrimitiveTopology(),
                        reinterpret_cast<void*>(static_cast<intptr_t>(indirectBufferOffset)));
                    break;
                }

                case Command::DrawIndexedIndirect: {
                    DrawIndexedIndirectCmd* draw = iter->NextCommand<DrawIndexedIndirectCmd>();
                    vertexStateBufferBindingTracker.Apply(gl);
                    bindGroupTracker.Apply(gl);

                    wgpu::IndexFormat indexFormat =
                        lastPipeline->GetVertexStateDescriptor()->indexFormat;
                    GLenum formatType = IndexFormatType(indexFormat);

                    uint64_t indirectBufferOffset = draw->indirectOffset;
                    Buffer* indirectBuffer = ToBackend(draw->indirectBuffer.Get());

                    gl.BindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectBuffer->GetHandle());
                    gl.DrawElementsIndirect(
                        lastPipeline->GetGLPrimitiveTopology(), formatType,
                        reinterpret_cast<void*>(static_cast<intptr_t>(indirectBufferOffset)));
                    break;
                }

                case Command::InsertDebugMarker:
                case Command::PopDebugGroup:
                case Command::PushDebugGroup: {
                    // Due to lack of linux driver support for GL_EXT_debug_marker
                    // extension these functions are skipped.
                    SkipCommand(iter, type);
                    break;
                }

                case Command::SetRenderPipeline: {
                    SetRenderPipelineCmd* cmd = iter->NextCommand<SetRenderPipelineCmd>();
                    lastPipeline = ToBackend(cmd->pipeline).Get();
                    lastPipeline->ApplyNow(persistentPipelineState);

                    vertexStateBufferBindingTracker.OnSetPipeline(lastPipeline);
                    bindGroupTracker.OnSetPipeline(lastPipeline);
                    break;
                }

                case Command::SetBindGroup: {
                    SetBindGroupCmd* cmd = iter->NextCommand<SetBindGroupCmd>();
                    uint32_t* dynamicOffsets = nullptr;
                    if (cmd->dynamicOffsetCount > 0) {
                        dynamicOffsets = iter->NextData<uint32_t>(cmd->dynamicOffsetCount);
                    }
                    bindGroupTracker.OnSetBindGroup(cmd->index, cmd->group.Get(),
                                                    cmd->dynamicOffsetCount, dynamicOffsets);
                    break;
                }

                case Command::SetIndexBuffer: {
                    SetIndexBufferCmd* cmd = iter->NextCommand<SetIndexBufferCmd>();
                    indexBufferBaseOffset = cmd->offset;
                    vertexStateBufferBindingTracker.OnSetIndexBuffer(cmd->buffer.Get());
                    break;
                }

                case Command::SetVertexBuffer: {
                    SetVertexBufferCmd* cmd = iter->NextCommand<SetVertexBufferCmd>();
                    vertexStateBufferBindingTracker.OnSetVertexBuffer(cmd->slot, cmd->buffer.Get(),
                                                                      cmd->offset);
                    break;
                }

                default:
                    UNREACHABLE();
                    break;
            }
        };

        Command type;
        while (mCommands.NextCommandId(&type)) {
            switch (type) {
                case Command::EndRenderPass: {
                    mCommands.NextCommand<EndRenderPassCmd>();

                    if (renderPass->attachmentState->GetSampleCount() > 1) {
                        ResolveMultisampledRenderTargets(gl, renderPass);
                    }
                    gl.DeleteFramebuffers(1, &fbo);
                    return;
                }

                case Command::SetStencilReference: {
                    SetStencilReferenceCmd* cmd = mCommands.NextCommand<SetStencilReferenceCmd>();
                    persistentPipelineState.SetStencilReference(gl, cmd->reference);
                    break;
                }

                case Command::SetViewport: {
                    SetViewportCmd* cmd = mCommands.NextCommand<SetViewportCmd>();
                    gl.ViewportIndexedf(0, cmd->x, cmd->y, cmd->width, cmd->height);
                    gl.DepthRangef(cmd->minDepth, cmd->maxDepth);
                    break;
                }

                case Command::SetScissorRect: {
                    SetScissorRectCmd* cmd = mCommands.NextCommand<SetScissorRectCmd>();
                    gl.Scissor(cmd->x, cmd->y, cmd->width, cmd->height);
                    break;
                }

                case Command::SetBlendColor: {
                    SetBlendColorCmd* cmd = mCommands.NextCommand<SetBlendColorCmd>();
                    gl.BlendColor(cmd->color.r, cmd->color.g, cmd->color.b, cmd->color.a);
                    break;
                }

                case Command::ExecuteBundles: {
                    ExecuteBundlesCmd* cmd = mCommands.NextCommand<ExecuteBundlesCmd>();
                    auto bundles = mCommands.NextData<Ref<RenderBundleBase>>(cmd->count);

                    for (uint32_t i = 0; i < cmd->count; ++i) {
                        CommandIterator* iter = bundles[i]->GetCommands();
                        iter->Reset();
                        while (iter->NextCommandId(&type)) {
                            DoRenderBundleCommand(iter, type);
                        }
                    }
                    break;
                }

                case Command::WriteTimestamp: {
                    // WriteTimestamp is not supported on OpenGL
                    UNREACHABLE();
                    break;
                }

                default: {
                    DoRenderBundleCommand(&mCommands, type);
                    break;
                }
            }
        }

        // EndRenderPass should have been called
        UNREACHABLE();
    }

}}  // namespace dawn_native::opengl
