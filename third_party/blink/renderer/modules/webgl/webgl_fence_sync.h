// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_WEBGL_WEBGL_FENCE_SYNC_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_WEBGL_WEBGL_FENCE_SYNC_H_

#include "third_party/blink/renderer/modules/webgl/webgl_sync.h"

namespace blink {

class WebGL2RenderingContextBase;

class WebGLFenceSync : public WebGLSync {
 public:
  static WebGLSync* Create(WebGL2RenderingContextBase*,
                           GLenum condition,
                           GLbitfield flags);

 protected:
  WebGLFenceSync(WebGL2RenderingContextBase*,
                 GLenum condition,
                 GLbitfield flags);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_WEBGL_WEBGL_FENCE_SYNC_H_
