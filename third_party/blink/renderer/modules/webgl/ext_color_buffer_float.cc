// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/webgl/ext_color_buffer_float.h"

namespace blink {

EXTColorBufferFloat::EXTColorBufferFloat(WebGLRenderingContextBase* context)
    : WebGLExtension(context) {
  context->ExtensionsUtil()->EnsureExtensionEnabled(
      "GL_EXT_color_buffer_float");
}

WebGLExtensionName EXTColorBufferFloat::GetName() const {
  return kEXTColorBufferFloatName;
}

EXTColorBufferFloat* EXTColorBufferFloat::Create(
    WebGLRenderingContextBase* context) {
  return new EXTColorBufferFloat(context);
}

bool EXTColorBufferFloat::Supported(WebGLRenderingContextBase* context) {
  return context->ExtensionsUtil()->SupportsExtension(
      "GL_EXT_color_buffer_float");
}

const char* EXTColorBufferFloat::ExtensionName() {
  return "EXT_color_buffer_float";
}

}  // namespace blink
