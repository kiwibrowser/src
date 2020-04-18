// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/webgl/webgl_compressed_texture_etc.h"

#include "third_party/blink/renderer/modules/webgl/webgl_rendering_context_base.h"

namespace blink {

WebGLCompressedTextureETC::WebGLCompressedTextureETC(
    WebGLRenderingContextBase* context)
    : WebGLExtension(context) {
  context->AddCompressedTextureFormat(GL_COMPRESSED_R11_EAC);
  context->AddCompressedTextureFormat(GL_COMPRESSED_SIGNED_R11_EAC);
  context->AddCompressedTextureFormat(GL_COMPRESSED_RGB8_ETC2);
  context->AddCompressedTextureFormat(GL_COMPRESSED_SRGB8_ETC2);
  context->AddCompressedTextureFormat(
      GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2);
  context->AddCompressedTextureFormat(
      GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2);
  context->AddCompressedTextureFormat(GL_COMPRESSED_RG11_EAC);
  context->AddCompressedTextureFormat(GL_COMPRESSED_SIGNED_RG11_EAC);
  context->AddCompressedTextureFormat(GL_COMPRESSED_RGBA8_ETC2_EAC);
  context->AddCompressedTextureFormat(GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC);
}

WebGLExtensionName WebGLCompressedTextureETC::GetName() const {
  return kWebGLCompressedTextureETCName;
}

WebGLCompressedTextureETC* WebGLCompressedTextureETC::Create(
    WebGLRenderingContextBase* context) {
  return new WebGLCompressedTextureETC(context);
}

bool WebGLCompressedTextureETC::Supported(WebGLRenderingContextBase* context) {
  Extensions3DUtil* extensions_util = context->ExtensionsUtil();
  return extensions_util->SupportsExtension(
      "GL_CHROMIUM_compressed_texture_etc");
}

const char* WebGLCompressedTextureETC::ExtensionName() {
  return "WEBGL_compressed_texture_etc";
}

}  // namespace blink
