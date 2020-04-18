// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/display_scale/tray_scale.h"

#include "ash/public/cpp/ash_switches.h"
#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/root_window_controller.h"
#include "ash/system/display_scale/scale_detailed_view.h"
#include "ash/system/display_scale/scale_view.h"
#include "ash/system/tray/system_tray.h"
#include "ash/system/tray/system_tray_item_detailed_view_delegate.h"
#include "ash/system/tray/tray_constants.h"
#include "base/command_line.h"
#include "ui/views/view.h"

namespace ash {
namespace {

bool IsDisplayScaleTrayEnabled() {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kAshEnableScaleSettingsTray);
}

}  // namespace

TrayScale::TrayScale(SystemTray* system_tray)
    : SystemTrayItem(system_tray, UMA_NOT_RECORDED),
      detailed_view_delegate_(
          std::make_unique<SystemTrayItemDetailedViewDelegate>(this)) {}

TrayScale::~TrayScale() = default;

views::View* TrayScale::CreateDefaultView(LoginStatus status) {
  if (!IsDisplayScaleTrayEnabled())
    return nullptr;
  scale_view_ = new tray::ScaleView(this, true);
  return scale_view_;
}

views::View* TrayScale::CreateDetailedView(LoginStatus status) {
  if (!IsDisplayScaleTrayEnabled())
    return nullptr;
  scale_detail_view_ =
      new tray::ScaleDetailedView(detailed_view_delegate_.get());
  return scale_detail_view_;
}

void TrayScale::OnDefaultViewDestroyed() {
  scale_view_ = nullptr;
}

void TrayScale::OnDetailedViewDestroyed() {
  scale_detail_view_ = nullptr;
}

bool TrayScale::ShouldShowShelf() const {
  return false;
}

}  // namespace ash
