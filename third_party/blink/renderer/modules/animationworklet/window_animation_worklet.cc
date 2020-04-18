// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/animationworklet/window_animation_worklet.h"

#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"

namespace blink {

// static
AnimationWorklet* WindowAnimationWorklet::animationWorklet(
    LocalDOMWindow& window) {
  if (!window.GetFrame())
    return nullptr;
  return From(window).animation_worklet_.Get();
}

// Break the following cycle when the context gets detached.
// Otherwise, the worklet object will leak.
//
// window => window.animationWorklet
// => WindowAnimationWorklet
// => AnimationWorklet  <--- break this reference
// => ThreadedWorkletMessagingProxy
// => Document
// => ... => window
void WindowAnimationWorklet::ContextDestroyed(ExecutionContext*) {
  animation_worklet_ = nullptr;
}

void WindowAnimationWorklet::Trace(blink::Visitor* visitor) {
  visitor->Trace(animation_worklet_);
  Supplement<LocalDOMWindow>::Trace(visitor);
  ContextLifecycleObserver::Trace(visitor);
}

// static
WindowAnimationWorklet& WindowAnimationWorklet::From(LocalDOMWindow& window) {
  WindowAnimationWorklet* supplement =
      Supplement<LocalDOMWindow>::From<WindowAnimationWorklet>(window);
  if (!supplement) {
    supplement = new WindowAnimationWorklet(window.GetFrame()->GetDocument());
    ProvideTo(window, supplement);
  }
  return *supplement;
}

WindowAnimationWorklet::WindowAnimationWorklet(Document* document)
    : ContextLifecycleObserver(document),
      animation_worklet_(new AnimationWorklet(document)) {
  DCHECK(GetExecutionContext());
}

const char WindowAnimationWorklet::kSupplementName[] = "WindowAnimationWorklet";

}  // namespace blink
