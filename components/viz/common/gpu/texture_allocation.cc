// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/common/gpu/texture_allocation.h"

#include "components/viz/common/resources/resource_format_utils.h"
#include "components/viz/common/resources/resource_sizes.h"
#include "gpu/GLES2/gl2extchromium.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "gpu/command_buffer/client/raster_interface.h"
#include "gpu/command_buffer/common/capabilities.h"
#include "gpu/command_buffer/common/gpu_memory_buffer_support.h"
#include "ui/gfx/color_space.h"
#include "ui/gfx/geometry/size.h"

namespace viz {

// static
TextureAllocation TextureAllocation::MakeTextureId(
    gpu::gles2::GLES2Interface* gl,
    const gpu::Capabilities& caps,
    ResourceFormat format,
    bool use_gpu_memory_buffer_resources,
    bool for_framebuffer_attachment) {
  uint32_t texture_target = GL_TEXTURE_2D;

  bool overlay_candidate = use_gpu_memory_buffer_resources &&
                           caps.texture_storage_image &&
                           IsGpuMemoryBufferFormatSupported(format);
  if (overlay_candidate) {
    texture_target = gpu::GetBufferTextureTarget(gfx::BufferUsage::SCANOUT,
                                                 BufferFormat(format), caps);
  }

  uint32_t texture_id;
  gl->GenTextures(1, &texture_id);
  gl->BindTexture(texture_target, texture_id);
  gl->TexParameteri(texture_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  gl->TexParameteri(texture_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  gl->TexParameteri(texture_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  gl->TexParameteri(texture_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  if (for_framebuffer_attachment && caps.texture_usage) {
    // Set GL_FRAMEBUFFER_ATTACHMENT_ANGLE since we'll be binding these
    // textures as a framebuffer for drawing directly to them on the gpu.
    gl->TexParameteri(texture_target, GL_TEXTURE_USAGE_ANGLE,
                      GL_FRAMEBUFFER_ATTACHMENT_ANGLE);
  }
  gl->BindTexture(texture_target, 0);
  return {texture_id, texture_target, overlay_candidate};
}

// static
void TextureAllocation::AllocateStorage(gpu::gles2::GLES2Interface* gl,
                                        const gpu::Capabilities& caps,
                                        ResourceFormat format,
                                        const gfx::Size& size,
                                        const TextureAllocation& alloc,
                                        const gfx::ColorSpace& color_space) {
  gl->BindTexture(alloc.texture_target, alloc.texture_id);
  // Allocate backing storage for the texture. The best choice is to use
  // GpuMemoryBuffers if we can use the texture as an overlay and its asked
  // for by the caller. Else we try to make the texture have immutable
  // storage, and finally fall back to standard storage if we must.
  if (alloc.overlay_candidate) {
    // |overlay_candidate| was only set when these were true, and
    // |use_gpu_memory_buffer_resources| was specified by the caller.
    DCHECK(caps.texture_storage_image);
    DCHECK(IsGpuMemoryBufferFormatSupported(format));

    gl->TexStorage2DImageCHROMIUM(
        alloc.texture_target, TextureStorageFormat(format), GL_SCANOUT_CHROMIUM,
        size.width(), size.height());
    if (color_space.IsValid()) {
      gl->SetColorSpaceMetadataCHROMIUM(
          alloc.texture_id, reinterpret_cast<GLColorSpace>(
                                const_cast<gfx::ColorSpace*>(&color_space)));
    }
  } else if (caps.texture_storage) {
    gl->TexStorage2DEXT(alloc.texture_target, 1, TextureStorageFormat(format),
                        size.width(), size.height());
  } else {
    gl->TexImage2D(alloc.texture_target, 0, GLInternalFormat(format),
                   size.width(), size.height(), 0, GLDataFormat(format),
                   GLDataType(format), nullptr);
  }
}

// static
void TextureAllocation::AllocateStorage(gpu::raster::RasterInterface* ri,
                                        const gpu::Capabilities& caps,
                                        ResourceFormat format,
                                        const gfx::Size& size,
                                        const TextureAllocation& alloc,
                                        const gfx::ColorSpace& color_space) {
  // ETC1 resources cannot be preallocated.
  if (format == ETC1)
    return;
  ri->TexStorage2D(alloc.texture_id, 1, size.width(), size.height());
  if (alloc.overlay_candidate && color_space.IsValid()) {
    ri->SetColorSpaceMetadata(alloc.texture_id,
                              reinterpret_cast<GLColorSpace>(
                                  const_cast<gfx::ColorSpace*>(&color_space)));
  }
}

void TextureAllocation::UploadStorage(gpu::gles2::GLES2Interface* gl,
                                      const gpu::Capabilities& caps,
                                      ResourceFormat format,
                                      const gfx::Size& size,
                                      const TextureAllocation& alloc,
                                      const gfx::ColorSpace& color_space,
                                      const void* pixels) {
  if (format == ETC1) {
    DCHECK_EQ(alloc.texture_target, static_cast<GLenum>(GL_TEXTURE_2D));
    int num_bytes = ResourceSizes::CheckedSizeInBytes<int>(size, ETC1);

    gl->BindTexture(alloc.texture_target, alloc.texture_id);
    gl->CompressedTexImage2D(alloc.texture_target, 0, GLInternalFormat(ETC1),
                             size.width(), size.height(), 0, num_bytes,
                             const_cast<void*>(pixels));
  } else {
    AllocateStorage(gl, caps, format, size, alloc, color_space);
    gl->TexSubImage2D(alloc.texture_target, 0, 0, 0, size.width(),
                      size.height(), GLDataFormat(format), GLDataType(format),
                      const_cast<void*>(pixels));
  }
}

}  // namespace viz
