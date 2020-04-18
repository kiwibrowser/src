// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/common/gpu_memory_buffer_support.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2extchromium.h>

#include "base/logging.h"
#include "build/build_config.h"
#include "gpu/command_buffer/common/capabilities.h"

namespace gpu {

namespace {

gfx::BufferFormat BufferFormatForInternalFormat(unsigned internalformat) {
  switch (internalformat) {
    case GL_RED_EXT:
      return gfx::BufferFormat::R_8;
    case GL_R16_EXT:
      return gfx::BufferFormat::R_16;
    case GL_RG_EXT:
      return gfx::BufferFormat::RG_88;
    case GL_RGB:
      return gfx::BufferFormat::BGRX_8888;
    case GL_RGBA:
      return gfx::BufferFormat::RGBA_8888;
    case GL_BGRA_EXT:
      return gfx::BufferFormat::BGRA_8888;
    case GL_ATC_RGB_AMD:
      return gfx::BufferFormat::ATC;
    case GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD:
      return gfx::BufferFormat::ATCIA;
    case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
      return gfx::BufferFormat::DXT1;
    case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
      return gfx::BufferFormat::DXT5;
    case GL_ETC1_RGB8_OES:
      return gfx::BufferFormat::ETC1;
    case GL_RGB_YCRCB_420_CHROMIUM:
      return gfx::BufferFormat::YVU_420;
    case GL_RGB_YCBCR_420V_CHROMIUM:
      return gfx::BufferFormat::YUV_420_BIPLANAR;
    case GL_RGB_YCBCR_422_CHROMIUM:
      return gfx::BufferFormat::UYVY_422;
    default:
      NOTREACHED();
      return gfx::BufferFormat::RGBA_8888;
  }
}

}  // namespace

bool IsImageFormatCompatibleWithGpuMemoryBufferFormat(
    unsigned internalformat,
    gfx::BufferFormat format) {
  switch (format) {
    case gfx::BufferFormat::ATC:
    case gfx::BufferFormat::ATCIA:
    case gfx::BufferFormat::BGRA_8888:
    case gfx::BufferFormat::BGRX_8888:
    case gfx::BufferFormat::DXT1:
    case gfx::BufferFormat::DXT5:
    case gfx::BufferFormat::ETC1:
    case gfx::BufferFormat::R_8:
    case gfx::BufferFormat::R_16:
    case gfx::BufferFormat::RG_88:
    case gfx::BufferFormat::RGBA_8888:
    case gfx::BufferFormat::YVU_420:
    case gfx::BufferFormat::YUV_420_BIPLANAR:
    case gfx::BufferFormat::UYVY_422:
      return format == BufferFormatForInternalFormat(internalformat);
    case gfx::BufferFormat::BGR_565:
    case gfx::BufferFormat::RGBX_8888:
      return internalformat == GL_RGB;
    case gfx::BufferFormat::BGRX_1010102:
    case gfx::BufferFormat::RGBX_1010102:
      return internalformat == GL_RGB10_A2_EXT;
    case gfx::BufferFormat::RGBA_4444:
    case gfx::BufferFormat::RGBA_F16:
      return internalformat == GL_RGBA;
  }

  NOTREACHED();
  return false;
}

bool IsImageFromGpuMemoryBufferFormatSupported(
    gfx::BufferFormat format,
    const gpu::Capabilities& capabilities) {
  switch (format) {
    case gfx::BufferFormat::ATC:
    case gfx::BufferFormat::ATCIA:
      return capabilities.texture_format_atc;
    case gfx::BufferFormat::BGRA_8888:
    case gfx::BufferFormat::BGRX_8888:
      return capabilities.texture_format_bgra8888;
    case gfx::BufferFormat::DXT1:
      return capabilities.texture_format_dxt1;
    case gfx::BufferFormat::DXT5:
      return capabilities.texture_format_dxt5;
    case gfx::BufferFormat::ETC1:
      return capabilities.texture_format_etc1;
    case gfx::BufferFormat::R_16:
      return capabilities.texture_norm16;
    case gfx::BufferFormat::R_8:
    case gfx::BufferFormat::RG_88:
      return capabilities.texture_rg;
    case gfx::BufferFormat::UYVY_422:
      return capabilities.image_ycbcr_422;
    case gfx::BufferFormat::BGRX_1010102:
      return capabilities.image_xr30;
    case gfx::BufferFormat::RGBX_1010102:
      return capabilities.image_xb30;
    case gfx::BufferFormat::BGR_565:
    case gfx::BufferFormat::RGBA_4444:
    case gfx::BufferFormat::RGBA_8888:
    case gfx::BufferFormat::RGBX_8888:
    case gfx::BufferFormat::YVU_420:
      return true;
    case gfx::BufferFormat::RGBA_F16:
      return capabilities.texture_half_float_linear;
    case gfx::BufferFormat::YUV_420_BIPLANAR:
      return capabilities.image_ycbcr_420v;
  }

  NOTREACHED();
  return false;
}

bool IsImageSizeValidForGpuMemoryBufferFormat(const gfx::Size& size,
                                              gfx::BufferFormat format) {
  switch (format) {
    case gfx::BufferFormat::ATC:
    case gfx::BufferFormat::ATCIA:
    case gfx::BufferFormat::DXT1:
    case gfx::BufferFormat::DXT5:
    case gfx::BufferFormat::ETC1:
      // Compressed images must have a width and height that's evenly divisible
      // by the block size.
      return size.width() % 4 == 0 && size.height() % 4 == 0;
    case gfx::BufferFormat::R_8:
    case gfx::BufferFormat::R_16:
    case gfx::BufferFormat::RG_88:
    case gfx::BufferFormat::BGR_565:
    case gfx::BufferFormat::RGBA_4444:
    case gfx::BufferFormat::RGBA_8888:
    case gfx::BufferFormat::RGBX_8888:
    case gfx::BufferFormat::BGRA_8888:
    case gfx::BufferFormat::BGRX_8888:
    case gfx::BufferFormat::BGRX_1010102:
    case gfx::BufferFormat::RGBX_1010102:
    case gfx::BufferFormat::RGBA_F16:
      return true;
    case gfx::BufferFormat::YVU_420:
    case gfx::BufferFormat::YUV_420_BIPLANAR:
      // U and V planes are subsampled by a factor of 2.
      return size.width() % 2 == 0 && size.height() % 2 == 0;
    case gfx::BufferFormat::UYVY_422:
      return size.width() % 2 == 0;
  }

  NOTREACHED();
  return false;
}

uint32_t GetPlatformSpecificTextureTarget() {
#if defined(OS_MACOSX)
  return GL_TEXTURE_RECTANGLE_ARB;
#elif defined(OS_ANDROID) || defined(OS_LINUX)
  return GL_TEXTURE_EXTERNAL_OES;
#elif defined(OS_WIN)
  return GL_TEXTURE_2D;
#else
  return 0;
#endif
}

GPU_EXPORT uint32_t GetBufferTextureTarget(gfx::BufferUsage usage,
                                           gfx::BufferFormat format,
                                           const Capabilities& capabilities) {
  bool found = base::ContainsValue(capabilities.texture_target_exception_list,
                                   gfx::BufferUsageAndFormat(usage, format));
  return found ? gpu::GetPlatformSpecificTextureTarget() : GL_TEXTURE_2D;
}

}  // namespace gpu
