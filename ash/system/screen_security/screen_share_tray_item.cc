// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/screen_security/screen_share_tray_item.h"

#include <utility>

#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/shell.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/tray/system_tray_notifier.h"
#include "ui/base/l10n/l10n_util.h"

namespace ash {

ScreenShareTrayItem::ScreenShareTrayItem(SystemTray* system_tray)
    : ScreenTrayItem(system_tray, UMA_SCREEN_SHARE) {
  Shell::Get()->system_tray_notifier()->AddScreenShareObserver(this);
}

ScreenShareTrayItem::~ScreenShareTrayItem() {
  Shell::Get()->system_tray_notifier()->RemoveScreenShareObserver(this);
}

views::View* ScreenShareTrayItem::CreateDefaultView(LoginStatus status) {
  set_default_view(new tray::ScreenStatusView(
      this,
      l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_SCREEN_SHARE_BEING_HELPED),
      l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_SCREEN_SHARE_STOP)));
  return default_view();
}

void ScreenShareTrayItem::RecordStoppedFromDefaultViewMetric() {
}

void ScreenShareTrayItem::OnScreenShareStart(
    const base::Closure& stop_callback,
    const base::string16& helper_name) {
  Start(stop_callback);
}

void ScreenShareTrayItem::OnScreenShareStop() {
  // We do not need to run the stop callback
  // when screening is stopped externally.
  SetIsStarted(false);
  Update();
}

}  // namespace ash
