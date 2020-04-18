// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/screen_capture_notification_ui.h"

#include "base/logging.h"

// Stub implementation of the ScreenCaptureNotificationUI interface.
class ScreenCaptureNotificationUIStub : public ScreenCaptureNotificationUI {
 public:
  ScreenCaptureNotificationUIStub() {}
  ~ScreenCaptureNotificationUIStub() override {}

  gfx::NativeViewId OnStarted(const base::Closure& stop_callback) override {
    NOTIMPLEMENTED();
    return 0;
  }
};

// static
std::unique_ptr<ScreenCaptureNotificationUI>
ScreenCaptureNotificationUI::Create(const base::string16& title) {
  return std::unique_ptr<ScreenCaptureNotificationUI>(
      new ScreenCaptureNotificationUIStub());
}
