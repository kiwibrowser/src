// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/webgl/ext_disjoint_timer_query_webgl2.h"

#include "gpu/command_buffer/client/gles2_interface.h"
#include "third_party/blink/renderer/bindings/modules/v8/webgl_any.h"
#include "third_party/blink/renderer/modules/webgl/webgl_rendering_context_base.h"

namespace blink {

WebGLExtensionName EXTDisjointTimerQueryWebGL2::GetName() const {
  return kEXTDisjointTimerQueryWebGL2Name;
}

EXTDisjointTimerQueryWebGL2* EXTDisjointTimerQueryWebGL2::Create(
    WebGLRenderingContextBase* context) {
  EXTDisjointTimerQueryWebGL2* o = new EXTDisjointTimerQueryWebGL2(context);
  return o;
}

bool EXTDisjointTimerQueryWebGL2::Supported(
    WebGLRenderingContextBase* context) {
  return context->ExtensionsUtil()->SupportsExtension(
      "GL_EXT_disjoint_timer_query");
}

const char* EXTDisjointTimerQueryWebGL2::ExtensionName() {
  return "EXT_disjoint_timer_query_webgl2";
}

void EXTDisjointTimerQueryWebGL2::queryCounterEXT(WebGLQuery* query,
                                                  GLenum target) {
  WebGLExtensionScopedContext scoped(this);
  if (scoped.IsLost())
    return;

  DCHECK(query);
  if (query->IsDeleted() ||
      !query->Validate(scoped.Context()->ContextGroup(), scoped.Context())) {
    scoped.Context()->SynthesizeGLError(GL_INVALID_OPERATION, "queryCounterEXT",
                                        "invalid query");
    return;
  }

  if (target != GL_TIMESTAMP_EXT) {
    scoped.Context()->SynthesizeGLError(GL_INVALID_ENUM, "queryCounterEXT",
                                        "invalid target");
    return;
  }

  if (query->HasTarget() && query->GetTarget() != target) {
    scoped.Context()->SynthesizeGLError(GL_INVALID_OPERATION, "queryCounterEXT",
                                        "target does not match query");
    return;
  }

  // Timestamps are disabled in WebGL due to lack of driver support on multiple
  // platforms, so we don't actually perform a GL call.
  query->SetTarget(target);
  query->ResetCachedResult();
}

void EXTDisjointTimerQueryWebGL2::Trace(blink::Visitor* visitor) {
  WebGLExtension::Trace(visitor);
}

EXTDisjointTimerQueryWebGL2::EXTDisjointTimerQueryWebGL2(
    WebGLRenderingContextBase* context)
    : WebGLExtension(context) {
  context->ExtensionsUtil()->EnsureExtensionEnabled(
      "GL_EXT_disjoint_timer_query_webgl2");
}

}  // namespace blink
