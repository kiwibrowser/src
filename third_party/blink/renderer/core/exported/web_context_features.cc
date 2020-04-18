// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/public/web/web_context_features.h"

#include "third_party/blink/renderer/core/context_features/context_feature_settings.h"
#include "third_party/blink/renderer/platform/bindings/dom_wrapper_world.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"

namespace blink {

// static
void WebContextFeatures::EnableMojoJS(v8::Local<v8::Context> context,
                                      bool enable) {
  ScriptState* script_state = ScriptState::From(context);
  DCHECK(script_state->World().IsMainWorld());
  ContextFeatureSettings::From(
      ExecutionContext::From(script_state),
      ContextFeatureSettings::CreationMode::kCreateIfNotExists)
      ->enableMojoJS(enable);
}

}  // namespace blink
