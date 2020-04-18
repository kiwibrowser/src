// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_WEBGL_WEBGL_COMPRESSED_TEXTURE_S3TC_SRGB_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_WEBGL_WEBGL_COMPRESSED_TEXTURE_S3TC_SRGB_H_

#include "third_party/blink/renderer/modules/webgl/webgl_extension.h"

namespace blink {

class WebGLCompressedTextureS3TCsRGB final : public WebGLExtension {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static WebGLCompressedTextureS3TCsRGB* Create(WebGLRenderingContextBase*);
  static bool Supported(WebGLRenderingContextBase*);
  static const char* ExtensionName();

  WebGLExtensionName GetName() const override;

 private:
  explicit WebGLCompressedTextureS3TCsRGB(WebGLRenderingContextBase*);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_WEBGL_WEBGL_COMPRESSED_TEXTURE_S3TC_SRGB_H_
