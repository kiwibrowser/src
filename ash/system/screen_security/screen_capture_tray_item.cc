// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/screen_security/screen_capture_tray_item.h"

#include <utility>

#include "ash/metrics/user_metrics_action.h"
#include "ash/metrics/user_metrics_recorder.h"
#include "ash/public/cpp/ash_features.h"

#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/shell.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/tray/system_tray_notifier.h"
#include "ui/base/l10n/l10n_util.h"

namespace ash {

ScreenCaptureTrayItem::ScreenCaptureTrayItem(SystemTray* system_tray)
    : ScreenTrayItem(system_tray, UMA_SCREEN_CAPTURE) {
  Shell::Get()->AddShellObserver(this);
  Shell::Get()->system_tray_notifier()->AddScreenCaptureObserver(this);
}

ScreenCaptureTrayItem::~ScreenCaptureTrayItem() {
  Shell::Get()->RemoveShellObserver(this);
  Shell::Get()->system_tray_notifier()->RemoveScreenCaptureObserver(this);
}

views::View* ScreenCaptureTrayItem::CreateDefaultView(LoginStatus status) {
  set_default_view(new tray::ScreenStatusView(
      this, screen_capture_status_,
      l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_SCREEN_CAPTURE_STOP)));
  return default_view();
}

void ScreenCaptureTrayItem::RecordStoppedFromDefaultViewMetric() {
  Shell::Get()->metrics()->RecordUserMetricsAction(
      UMA_STATUS_AREA_SCREEN_CAPTURE_DEFAULT_STOP);
}

void ScreenCaptureTrayItem::OnScreenCaptureStart(
    const base::Closure& stop_callback,
    const base::string16& screen_capture_status) {
  screen_capture_status_ = screen_capture_status;

  // We do not want to show the screen capture tray item and the chromecast
  // casting tray item at the same time. We will hide this tray item.
  //
  // This suppression technique is currently dependent on the order
  // that OnScreenCaptureStart and OnCastingSessionStartedOrStopped
  // get invoked. OnCastingSessionStartedOrStopped currently gets
  // called first.
  if (is_casting_)
    return;

  Start(stop_callback);
}

void ScreenCaptureTrayItem::OnScreenCaptureStop() {
  // We do not need to run the stop callback when
  // screen capture is stopped externally.
  SetIsStarted(false);
  Update();
}

void ScreenCaptureTrayItem::OnCastingSessionStartedOrStopped(bool started) {
  is_casting_ = started;
}

}  // namespace ash
