// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/accessibility/accessibility_event_recorder.h"

#include "build/build_config.h"

namespace content {

AccessibilityEventRecorder::AccessibilityEventRecorder(
    BrowserAccessibilityManager* manager,
    base::ProcessId pid)
    : manager_(manager) {}

AccessibilityEventRecorder::~AccessibilityEventRecorder() {
}

#if !defined(OS_WIN) && !defined(OS_MACOSX)
// static
AccessibilityEventRecorder* AccessibilityEventRecorder::Create(
    BrowserAccessibilityManager* manager,
    base::ProcessId pid) {
  return new AccessibilityEventRecorder(manager, pid);
}
#endif

void AccessibilityEventRecorder::OnEvent(std::string event) {
  event_logs_.push_back(event);
  if (callback_)
    callback_.Run(event);
}

}  // namespace content
