// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/wake_lock/screen_wake_lock.h"

#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/screen.h"
#include "third_party/blink/renderer/core/page/page_visibility_state.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"

namespace blink {

// static
bool ScreenWakeLock::keepAwake(Screen& screen) {
  ScreenWakeLock* screen_wake_lock = FromScreen(screen);
  if (!screen_wake_lock)
    return false;

  return screen_wake_lock->keepAwake();
}

// static
void ScreenWakeLock::setKeepAwake(Screen& screen, bool keep_awake) {
  ScreenWakeLock* screen_wake_lock = FromScreen(screen);
  if (screen_wake_lock)
    screen_wake_lock->setKeepAwake(keep_awake);
}

// static
const char ScreenWakeLock::kSupplementName[] = "ScreenWakeLock";

// static
ScreenWakeLock* ScreenWakeLock::From(LocalFrame* frame) {
  if (!RuntimeEnabledFeatures::WakeLockEnabled())
    return nullptr;
  ScreenWakeLock* supplement =
      Supplement<LocalFrame>::From<ScreenWakeLock>(frame);
  if (!supplement) {
    supplement = new ScreenWakeLock(*frame);
    ProvideTo(*frame, supplement);
  }
  return supplement;
}

void ScreenWakeLock::PageVisibilityChanged() {
  NotifyService();
}

void ScreenWakeLock::ContextDestroyed(ExecutionContext*) {
  setKeepAwake(false);
}

void ScreenWakeLock::Trace(blink::Visitor* visitor) {
  Supplement<LocalFrame>::Trace(visitor);
  PageVisibilityObserver::Trace(visitor);
  ContextLifecycleObserver::Trace(visitor);
}

ScreenWakeLock::ScreenWakeLock(LocalFrame& frame)
    : Supplement<LocalFrame>(frame),
      ContextLifecycleObserver(frame.GetDocument()),
      PageVisibilityObserver(frame.GetPage()),
      keep_awake_(false) {
  DCHECK(!service_.is_bound());
  frame.GetInterfaceProvider().GetInterface(mojo::MakeRequest(&service_));
}

bool ScreenWakeLock::keepAwake() const {
  return keep_awake_;
}

void ScreenWakeLock::setKeepAwake(bool keep_awake) {
  keep_awake_ = keep_awake;
  NotifyService();
}

// static
ScreenWakeLock* ScreenWakeLock::FromScreen(Screen& screen) {
  return screen.GetFrame() ? ScreenWakeLock::From(screen.GetFrame()) : nullptr;
}

void ScreenWakeLock::NotifyService() {
  if (!service_)
    return;

  if (keep_awake_ && GetPage() && GetPage()->IsPageVisible())
    service_->RequestWakeLock();
  else
    service_->CancelWakeLock();
}

}  // namespace blink
