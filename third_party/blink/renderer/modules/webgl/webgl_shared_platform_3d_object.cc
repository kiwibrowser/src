// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/webgl/webgl_shared_platform_3d_object.h"

#include "third_party/blink/renderer/modules/webgl/webgl_rendering_context_base.h"

namespace blink {

WebGLSharedPlatform3DObject::WebGLSharedPlatform3DObject(
    WebGLRenderingContextBase* ctx)
    : WebGLSharedObject(ctx), object_(0) {}

void WebGLSharedPlatform3DObject::SetObject(GLuint object) {
  // object==0 && deleted==false indicating an uninitialized state;
  DCHECK(!object_);
  DCHECK(!IsDeleted());
  object_ = object;
}

bool WebGLSharedPlatform3DObject::HasObject() const {
  return object_ != 0;
}

}  // namespace blink
