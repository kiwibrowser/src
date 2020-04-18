// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/ui/screen_capture_notification_ui_chromeos.h"

#include "ash/shell.h"
#include "ash/system/tray/system_tray_notifier.h"

namespace chromeos {

ScreenCaptureNotificationUIChromeOS::ScreenCaptureNotificationUIChromeOS(
    const base::string16& text)
    : text_(text) {
}

ScreenCaptureNotificationUIChromeOS::~ScreenCaptureNotificationUIChromeOS() {
  // MediaStreamCaptureIndicator will delete ScreenCaptureNotificationUI object
  // after it stops screen capture.
  ash::Shell::Get()->system_tray_notifier()->NotifyScreenCaptureStop();
}

gfx::NativeViewId ScreenCaptureNotificationUIChromeOS::OnStarted(
    const base::Closure& stop_callback) {
  ash::Shell::Get()->system_tray_notifier()->NotifyScreenCaptureStart(
      stop_callback, text_);
  return 0;
}

}  // namespace chromeos

// static
std::unique_ptr<ScreenCaptureNotificationUI>
ScreenCaptureNotificationUI::Create(const base::string16& text) {
  return std::unique_ptr<ScreenCaptureNotificationUI>(
      new chromeos::ScreenCaptureNotificationUIChromeOS(text));
}
