// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_WEBGL_WEBGL_COMPRESSED_TEXTURE_ETC1_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_WEBGL_WEBGL_COMPRESSED_TEXTURE_ETC1_H_

#include "third_party/blink/renderer/modules/webgl/webgl_extension.h"

namespace blink {

class WebGLCompressedTextureETC1 final : public WebGLExtension {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static WebGLCompressedTextureETC1* Create(WebGLRenderingContextBase*);
  static bool Supported(WebGLRenderingContextBase*);
  static const char* ExtensionName();

  WebGLExtensionName GetName() const override;

 private:
  explicit WebGLCompressedTextureETC1(WebGLRenderingContextBase*);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_WEBGL_WEBGL_COMPRESSED_TEXTURE_ETC1_H_
