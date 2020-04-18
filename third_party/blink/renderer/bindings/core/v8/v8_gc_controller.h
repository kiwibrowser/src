/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_V8_GC_CONTROLLER_H_
#define THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_V8_GC_CONTROLLER_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "v8/include/v8-profiler.h"
#include "v8/include/v8.h"

namespace blink {

class Node;

class CORE_EXPORT V8GCController {
  STATIC_ONLY(V8GCController);

 public:
  static void GcPrologue(v8::Isolate*, v8::GCType, v8::GCCallbackFlags);
  static void GcEpilogue(v8::Isolate*, v8::GCType, v8::GCCallbackFlags);

  static void CollectGarbage(v8::Isolate*, bool only_minor_gc = false);
  // You should use collectAllGarbageForTesting() when you want to collect all
  // V8 & Blink objects. It runs multiple V8 GCs to collect references
  // that cross the binding boundary. collectAllGarbage() also runs multipe
  // Oilpan GCs to collect a chain of persistent handles.
  static void CollectAllGarbageForTesting(v8::Isolate*);

  static Node* OpaqueRootForGC(v8::Isolate*, Node*);

  // Called when Oilpan traces references from V8 wrappers to DOM wrappables.
  static void TraceDOMWrappers(v8::Isolate*, Visitor*);

  // Called upon terminating a thread when Oilpan clears references from V8
  // wrappers to DOM wrappables.
  static void ClearDOMWrappers(v8::Isolate*);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_V8_GC_CONTROLLER_H_
