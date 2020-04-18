// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/tray/system_tray_item.h"

#include "ash/public/cpp/ash_features.h"
#include "ash/system/status_area_widget.h"
#include "ash/system/tray/system_tray.h"
#include "ash/system/tray/tray_constants.h"
#include "ash/system/unified/unified_system_tray.h"
#include "base/timer/timer.h"
#include "ui/views/view.h"

namespace ash {

SystemTrayItem::SystemTrayItem(SystemTray* system_tray, UmaType uma_type)
    : system_tray_(system_tray), uma_type_(uma_type), restore_focus_(false) {}

SystemTrayItem::~SystemTrayItem() = default;

views::View* SystemTrayItem::CreateTrayView(LoginStatus status) {
  return nullptr;
}

views::View* SystemTrayItem::CreateDefaultView(LoginStatus status) {
  return nullptr;
}

views::View* SystemTrayItem::CreateDetailedView(LoginStatus status) {
  return nullptr;
}

void SystemTrayItem::OnTrayViewDestroyed() {}

void SystemTrayItem::OnDefaultViewDestroyed() {}

void SystemTrayItem::OnDetailedViewDestroyed() {}

void SystemTrayItem::TransitionDetailedView() {
  transition_delay_timer_.Start(
      FROM_HERE,
      base::TimeDelta::FromMilliseconds(kTrayDetailedViewTransitionDelayMs),
      base::Bind(&SystemTray::ShowDetailedView, base::Unretained(system_tray()),
                 this, 0, BUBBLE_USE_EXISTING));
}

void SystemTrayItem::UpdateAfterLoginStatusChange(LoginStatus status) {}

void SystemTrayItem::UpdateAfterShelfAlignmentChange() {}

void SystemTrayItem::ShowDetailedView(int for_seconds) {
  system_tray()->ShowDetailedView(this, for_seconds, BUBBLE_CREATE_NEW);
}

void SystemTrayItem::SetDetailedViewCloseDelay(int for_seconds) {
  system_tray()->SetDetailedViewCloseDelay(for_seconds);
}

void SystemTrayItem::HideDetailedView() {
  system_tray()->HideDetailedView(this);
}

bool SystemTrayItem::ShouldShowShelf() const {
  return true;
}

bool SystemTrayItem::IsUnifiedBubbleShown() {
  return features::IsSystemTrayUnifiedEnabled() && system_tray()
                                                       ->shelf()
                                                       ->GetStatusAreaWidget()
                                                       ->unified_system_tray()
                                                       ->IsBubbleShown();
}

views::View* SystemTrayItem::GetItemToRestoreFocusTo() {
  return nullptr;
}

}  // namespace ash
