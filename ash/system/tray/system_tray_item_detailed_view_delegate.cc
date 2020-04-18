// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/tray/system_tray_item_detailed_view_delegate.h"

#include "ash/system/tray/system_tray.h"
#include "ash/system/tray/system_tray_item.h"
#include "ash/system/tray/tray_constants.h"

namespace ash {

SystemTrayItemDetailedViewDelegate::SystemTrayItemDetailedViewDelegate(
    SystemTrayItem* owner)
    : owner_(owner) {}

SystemTrayItemDetailedViewDelegate::~SystemTrayItemDetailedViewDelegate() =
    default;

void SystemTrayItemDetailedViewDelegate::TransitionToMainView(
    bool restore_focus) {
  if (restore_focus)
    owner_->set_restore_focus(true);

  transition_delay_timer_.Start(
      FROM_HERE,
      base::TimeDelta::FromMilliseconds(kTrayDetailedViewTransitionDelayMs),
      this, &SystemTrayItemDetailedViewDelegate::DoTransitionToMainView);
}

void SystemTrayItemDetailedViewDelegate::CloseBubble() {
  if (owner_->system_tray())
    owner_->system_tray()->CloseBubble();
}

void SystemTrayItemDetailedViewDelegate::DoTransitionToMainView() {
  if (!owner_->system_tray())
    return;
  owner_->system_tray()->ShowDefaultView(BUBBLE_USE_EXISTING,
                                         false /* show_by_click */);
  owner_->set_restore_focus(false);
}

}  // namespace ash
