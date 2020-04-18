// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "class_trace_wrappers_bases.h"

namespace blink {

void CustomScriptWrappable::TraceWrappers(const ScriptWrappableVisitor*) const {
  // Missing ScriptWrappable::TraceWrappers(visitor).
}

void MixinApplication::TraceWrappers(const ScriptWrappableVisitor*) const {
  // Missing MixinClassWithTraceWrappers1::TraceWrappers(visitor).
  // Missing MixinClassWithTraceWrappers2::TraceWrappers(visitor).
}

}  // namespace blink
