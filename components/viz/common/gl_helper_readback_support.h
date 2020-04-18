// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_COMMON_GL_HELPER_READBACK_SUPPORT_H_
#define COMPONENTS_VIZ_COMMON_GL_HELPER_READBACK_SUPPORT_H_

#include <stddef.h>

#include <vector>

#include "components/viz/common/gl_helper.h"

namespace viz {

class GLHelperReadbackSupport {
 public:
  enum FormatSupport { SUPPORTED, SWIZZLE, NOT_SUPPORTED };

  explicit GLHelperReadbackSupport(gpu::gles2::GLES2Interface* gl);

  ~GLHelperReadbackSupport();

  // For a given color type retrieve whether readback is supported and if so
  // how it should be performed. The |format|, |type| and |bytes_per_pixel| are
  // the values that should be used with glReadPixels to facilitate the
  // readback. If |can_swizzle| is true then this method will return SWIZZLE if
  // the data needs to be swizzled before using the returned |format| otherwise
  // the method will return SUPPORTED to indicate that readback is permitted of
  // this color othewise NOT_SUPPORTED will be returned.  This method always
  // overwrites the out values irrespective of the return value.
  FormatSupport GetReadbackConfig(SkColorType color_type,
                                  bool can_swizzle,
                                  GLenum* format,
                                  GLenum* type,
                                  size_t* bytes_per_pixel);
  // Provides the additional readback format/type pairing for a render target
  // of a given format/type pairing
  void GetAdditionalFormat(GLenum format,
                           GLenum type,
                           GLenum* format_out,
                           GLenum* type_out);

 private:
  struct FormatCacheEntry {
    GLenum format;
    GLenum type;
    GLenum read_format;
    GLenum read_type;
  };

  // This populates the format_support_table with the list of supported
  // formats.
  void InitializeReadbackSupport();

  // This api is called  once per format and it is done in the
  // InitializeReadbackSupport. We should not use this any where
  // except the InitializeReadbackSupport.Calling this at other places
  // can distrub the state of normal gl operations.
  void CheckForReadbackSupport(SkColorType texture_format);

  // Helper functions for checking the supported texture formats.
  // Avoid using this API in between texture operations, as this does some
  // teture opertions (bind, attach) internally.
  bool SupportsFormat(GLenum format, GLenum type);

  FormatSupport format_support_table_[kLastEnum_SkColorType + 1];

  gpu::gles2::GLES2Interface* gl_;
  std::vector<struct FormatCacheEntry> format_cache_;
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_COMMON_GL_HELPER_READBACK_SUPPORT_H_
