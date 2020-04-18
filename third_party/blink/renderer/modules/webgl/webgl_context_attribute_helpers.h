// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_WEBGL_WEBGL_CONTEXT_ATTRIBUTE_HELPERS_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_WEBGL_WEBGL_CONTEXT_ATTRIBUTE_HELPERS_H_

#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/core/html/canvas/canvas_context_creation_attributes_core.h"
#include "third_party/blink/renderer/modules/webgl/webgl_context_attributes.h"

namespace blink {

WebGLContextAttributes ToWebGLContextAttributes(
    const CanvasContextCreationAttributesCore&);

// Set up the attributes that can be used to create a GL context via the
// Platform API.
Platform::ContextAttributes ToPlatformContextAttributes(
    const CanvasContextCreationAttributesCore&,
    unsigned web_gl_version,
    bool support_own_offscreen_surface);

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_WEBGL_WEBGL_CONTEXT_ATTRIBUTE_HELPERS_H_
