// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_INITIALIZE_V8_EXTRAS_BINDING_H_
#define THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_INITIALIZE_V8_EXTRAS_BINDING_H_

#include "third_party/blink/renderer/core/core_export.h"

namespace blink {

class ScriptState;

// Add the Javascript function countUse() to the "binding" object that is
// exposed to the Javascript streams implementations.
//
// It must be called during initialisation of the V8 context.
//
// binding.countUse() takes a string and calls UseCounter::Count() on the
// matching ID. It only does anything the first time it is called in a
// particular execution context. The use of a string argument avoids duplicating
// the IDs in the JS files, but means that JS code should avoid calling it more
// than once to avoid unnecessary overhead. Only string IDs that this code
// specifically knows about will work.
//
// countUse() is not available during snapshot creation.
void CORE_EXPORT InitializeV8ExtrasBinding(ScriptState*);

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_INITIALIZE_V8_EXTRAS_BINDING_H_
