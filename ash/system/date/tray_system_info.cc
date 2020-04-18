// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/date/tray_system_info.h"

#include "ash/shelf/shelf.h"
#include "ash/shell.h"
#include "ash/system/date/date_view.h"
#include "ash/system/date/system_info_default_view.h"
#include "ash/system/model/clock_model.h"
#include "ash/system/model/system_tray_model.h"
#include "ash/system/tray/system_tray.h"
#include "ash/system/tray/system_tray_notifier.h"
#include "ash/system/tray/tray_item_view.h"

namespace ash {

TraySystemInfo::TraySystemInfo(SystemTray* system_tray)
    : SystemTrayItem(system_tray, UMA_DATE),
      tray_view_(nullptr),
      default_view_(nullptr) {}

TraySystemInfo::~TraySystemInfo() = default;

const tray::TimeView* TraySystemInfo::GetTimeTrayForTesting() const {
  return tray_view_;
}

const SystemInfoDefaultView* TraySystemInfo::GetDefaultViewForTesting() const {
  return default_view_;
}

views::View* TraySystemInfo::CreateDefaultViewForTesting(LoginStatus status) {
  return CreateDefaultView(status);
}

views::View* TraySystemInfo::CreateTrayView(LoginStatus status) {
  CHECK(tray_view_ == nullptr);
  tray::TimeView::ClockLayout clock_layout =
      system_tray()->shelf()->IsHorizontalAlignment()
          ? tray::TimeView::ClockLayout::HORIZONTAL_CLOCK
          : tray::TimeView::ClockLayout::VERTICAL_CLOCK;
  tray_view_ = new tray::TimeView(clock_layout,
                                  Shell::Get()->system_tray_model()->clock());
  views::View* view = new TrayItemView(this);
  view->AddChildView(tray_view_);
  return view;
}

views::View* TraySystemInfo::CreateDefaultView(LoginStatus status) {
  default_view_ = new SystemInfoDefaultView(this);
  return default_view_;
}

void TraySystemInfo::OnTrayViewDestroyed() {
  tray_view_ = nullptr;
}

void TraySystemInfo::OnDefaultViewDestroyed() {
  default_view_ = nullptr;
}

void TraySystemInfo::UpdateAfterShelfAlignmentChange() {
  if (tray_view_) {
    tray::TimeView::ClockLayout clock_layout =
        system_tray()->shelf()->IsHorizontalAlignment()
            ? tray::TimeView::ClockLayout::HORIZONTAL_CLOCK
            : tray::TimeView::ClockLayout::VERTICAL_CLOCK;
    tray_view_->UpdateClockLayout(clock_layout);
  }
}

}  // namespace ash
