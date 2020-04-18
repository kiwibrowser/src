// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/frame/screen_orientation_controller.h"

namespace blink {

ScreenOrientationController::ScreenOrientationController(LocalFrame& frame)
    : Supplement<LocalFrame>(frame) {}

// static
ScreenOrientationController* ScreenOrientationController::From(
    LocalFrame& frame) {
  return Supplement<LocalFrame>::From<ScreenOrientationController>(frame);
}

void ScreenOrientationController::Trace(blink::Visitor* visitor) {
  Supplement<LocalFrame>::Trace(visitor);
}

// static
void ScreenOrientationController::ProvideTo(
    LocalFrame& frame,
    ScreenOrientationController* controller) {
  Supplement<LocalFrame>::ProvideTo(frame, controller);
}

// static
const char ScreenOrientationController::kSupplementName[] =
    "ScreenOrientationController";

}  // namespace blink
