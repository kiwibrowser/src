// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/common/gl_helper_readback_support.h"
#include "base/logging.h"
#include "gpu/GLES2/gl2extchromium.h"
#include "third_party/skia/include/core/SkImageInfo.h"

namespace viz {

GLHelperReadbackSupport::GLHelperReadbackSupport(gpu::gles2::GLES2Interface* gl)
    : gl_(gl) {
  InitializeReadbackSupport();
}

GLHelperReadbackSupport::~GLHelperReadbackSupport() {}

void GLHelperReadbackSupport::InitializeReadbackSupport() {
  // We are concerned about 16, 32-bit formats only.  The below are the most
  // used 16, 32-bit formats.  In future if any new format support is needed
  // that should be added here.  Initialize the array with
  // GLHelperReadbackSupport::NOT_SUPPORTED as we dont know the supported
  // formats yet.
  for (int i = 0; i <= kLastEnum_SkColorType; ++i) {
    format_support_table_[i] = GLHelperReadbackSupport::NOT_SUPPORTED;
  }
  // TODO(sikugu): kAlpha_8_SkColorType support check is failing on mesa.
  // See crbug.com/415667.
  CheckForReadbackSupport(kRGB_565_SkColorType);
  CheckForReadbackSupport(kARGB_4444_SkColorType);
  CheckForReadbackSupport(kRGBA_8888_SkColorType);
  CheckForReadbackSupport(kBGRA_8888_SkColorType);
  // Further any formats, support should be checked here.
}

void GLHelperReadbackSupport::CheckForReadbackSupport(
    SkColorType texture_format) {
  bool supports_format = false;
  switch (texture_format) {
    case kRGB_565_SkColorType:
      supports_format = SupportsFormat(GL_RGB, GL_UNSIGNED_SHORT_5_6_5);
      break;
    case kRGBA_8888_SkColorType:
      // This is the baseline, assume always true.
      supports_format = true;
      break;
    case kBGRA_8888_SkColorType:
      supports_format = SupportsFormat(GL_BGRA_EXT, GL_UNSIGNED_BYTE);
      break;
    case kARGB_4444_SkColorType:
      supports_format = false;
      break;
    default:
      NOTREACHED();
      supports_format = false;
      break;
  }
  DCHECK((int)texture_format <= (int)kLastEnum_SkColorType);
  format_support_table_[texture_format] =
      supports_format ? GLHelperReadbackSupport::SUPPORTED
                      : GLHelperReadbackSupport::NOT_SUPPORTED;
}

void GLHelperReadbackSupport::GetAdditionalFormat(GLenum format,
                                                  GLenum type,
                                                  GLenum* format_out,
                                                  GLenum* type_out) {
  for (unsigned int i = 0; i < format_cache_.size(); i++) {
    if (format_cache_[i].format == format && format_cache_[i].type == type) {
      *format_out = format_cache_[i].read_format;
      *type_out = format_cache_[i].read_type;
      return;
    }
  }

  const int kTestSize = 64;
  ScopedTexture dst_texture(gl_);
  ScopedTextureBinder<GL_TEXTURE_2D> texture_binder(gl_, dst_texture);
  gl_->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  gl_->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  gl_->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  gl_->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  gl_->TexImage2D(GL_TEXTURE_2D, 0, format, kTestSize, kTestSize, 0, format,
                  type, nullptr);
  ScopedFramebuffer dst_framebuffer(gl_);
  ScopedFramebufferBinder<GL_FRAMEBUFFER> framebuffer_binder(gl_,
                                                             dst_framebuffer);
  gl_->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                            dst_texture, 0);
  GLint format_tmp = 0, type_tmp = 0;
  gl_->GetIntegerv(GL_IMPLEMENTATION_COLOR_READ_FORMAT, &format_tmp);
  gl_->GetIntegerv(GL_IMPLEMENTATION_COLOR_READ_TYPE, &type_tmp);
  *format_out = format_tmp;
  *type_out = type_tmp;

  struct FormatCacheEntry entry = {format, type, *format_out, *type_out};
  format_cache_.push_back(entry);
}

bool GLHelperReadbackSupport::SupportsFormat(GLenum format, GLenum type) {
  // GLES2.0 Specification says this pairing is always supported
  // with additional format from GL_IMPLEMENTATION_COLOR_READ_FORMAT/TYPE
  if (format == GL_RGBA && type == GL_UNSIGNED_BYTE)
    return true;

  bool supports_format = false;
  GLenum ext_format = 0, ext_type = 0;
  GetAdditionalFormat(format, type, &ext_format, &ext_type);
  if ((ext_format == format) && (ext_type == type)) {
    supports_format = true;
  }
  return supports_format;
}

GLHelperReadbackSupport::FormatSupport
GLHelperReadbackSupport::GetReadbackConfig(SkColorType color_type,
                                           bool can_swizzle,
                                           GLenum* format,
                                           GLenum* type,
                                           size_t* bytes_per_pixel) {
  DCHECK(format && type && bytes_per_pixel);
  *bytes_per_pixel = 4;
  *type = GL_UNSIGNED_BYTE;
  GLenum new_format = 0, new_type = 0;
  switch (color_type) {
    case kRGB_565_SkColorType:
      if (format_support_table_[color_type] ==
          GLHelperReadbackSupport::SUPPORTED) {
        *format = GL_RGB;
        *type = GL_UNSIGNED_SHORT_5_6_5;
        *bytes_per_pixel = 2;
        return GLHelperReadbackSupport::SUPPORTED;
      }
      break;
    case kRGBA_8888_SkColorType:
      *format = GL_RGBA;
      if (can_swizzle) {
        // If GL_BGRA_EXT is advertised as the readback format through
        // GL_IMPLEMENTATION_COLOR_READ_FORMAT then assume it is preferred by
        // the implementation for performance.
        GetAdditionalFormat(*format, *type, &new_format, &new_type);

        if (new_format == GL_BGRA_EXT && new_type == GL_UNSIGNED_BYTE) {
          *format = GL_BGRA_EXT;
          return GLHelperReadbackSupport::SWIZZLE;
        }
      }
      return GLHelperReadbackSupport::SUPPORTED;
    case kBGRA_8888_SkColorType:
      *format = GL_BGRA_EXT;
      if (format_support_table_[color_type] ==
          GLHelperReadbackSupport::SUPPORTED)
        return GLHelperReadbackSupport::SUPPORTED;

      if (can_swizzle) {
        *format = GL_RGBA;
        return GLHelperReadbackSupport::SWIZZLE;
      }

      break;
    case kARGB_4444_SkColorType:
      return GLHelperReadbackSupport::NOT_SUPPORTED;
    default:
      NOTREACHED();
      break;
  }

  return GLHelperReadbackSupport::NOT_SUPPORTED;
}

}  // namespace viz
