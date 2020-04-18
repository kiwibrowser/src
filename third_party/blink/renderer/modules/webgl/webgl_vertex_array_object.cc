// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/webgl/webgl_vertex_array_object.h"

#include "third_party/blink/renderer/modules/webgl/webgl_rendering_context_base.h"

namespace blink {

WebGLVertexArrayObject* WebGLVertexArrayObject::Create(
    WebGLRenderingContextBase* ctx,
    VaoType type) {
  return new WebGLVertexArrayObject(ctx, type);
}

WebGLVertexArrayObject::WebGLVertexArrayObject(WebGLRenderingContextBase* ctx,
                                               VaoType type)
    : WebGLVertexArrayObjectBase(ctx, type) {}

}  // namespace blink
