// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_HTML_CUSTOM_V0_CUSTOM_ELEMENT_MICROTASK_DISPATCHER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_HTML_CUSTOM_V0_CUSTOM_ELEMENT_MICROTASK_DISPATCHER_H_

#include "base/macros.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

class V0CustomElementCallbackQueue;

class V0CustomElementMicrotaskDispatcher final
    : public GarbageCollected<V0CustomElementMicrotaskDispatcher> {
 public:
  static V0CustomElementMicrotaskDispatcher& Instance();

  void Enqueue(V0CustomElementCallbackQueue*);

  bool ElementQueueIsEmpty() { return elements_.IsEmpty(); }

  void Trace(blink::Visitor*);

 private:
  V0CustomElementMicrotaskDispatcher();

  void EnsureMicrotaskScheduledForElementQueue();
  void EnsureMicrotaskScheduled();

  static void Dispatch();
  void DoDispatch();

  bool has_scheduled_microtask_;
  enum { kQuiescent, kResolving, kDispatchingCallbacks } phase_;

  HeapVector<Member<V0CustomElementCallbackQueue>> elements_;

  DISALLOW_COPY_AND_ASSIGN(V0CustomElementMicrotaskDispatcher);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_HTML_CUSTOM_V0_CUSTOM_ELEMENT_MICROTASK_DISPATCHER_H_
