// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/webgl/webgl_fence_sync.h"

#include "gpu/command_buffer/client/gles2_interface.h"
#include "third_party/blink/renderer/modules/webgl/webgl2_rendering_context_base.h"

namespace blink {

WebGLSync* WebGLFenceSync::Create(WebGL2RenderingContextBase* ctx,
                                  GLenum condition,
                                  GLbitfield flags) {
  return new WebGLFenceSync(ctx, condition, flags);
}

WebGLFenceSync::WebGLFenceSync(WebGL2RenderingContextBase* ctx,
                               GLenum condition,
                               GLbitfield flags)
    : WebGLSync(ctx,
                ctx->ContextGL()->FenceSync(condition, flags),
                GL_SYNC_FENCE) {}

}  // namespace blink
