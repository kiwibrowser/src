// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/date/system_info_default_view.h"

#include <memory>

#include "ash/shell.h"
#include "ash/system/date/date_view.h"
#include "ash/system/model/system_tray_model.h"
#include "ash/system/power/power_status.h"
#include "ash/system/power/power_status_view.h"
#include "ash/system/tray/tray_constants.h"
#include "ash/system/tray/tray_popup_utils.h"
#include "ash/system/tray/tri_view.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/controls/separator.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"

namespace ash {

// The minimum number of menu button widths that the date view should span
// horizontally.
const int kMinNumTileWidths = 2;

// The maximum number of menu button widths that the date view should span
// horizontally.
const int kMaxNumTileWidths = 3;

SystemInfoDefaultView::SystemInfoDefaultView(SystemTrayItem* owner)
    : date_view_(nullptr),
      tri_view_(TrayPopupUtils::CreateMultiTargetRowView()) {
  tri_view_->SetMinHeight(kTrayPopupSystemInfoRowHeight);
  AddChildView(tri_view_);
  SetLayoutManager(std::make_unique<views::FillLayout>());

  date_view_ =
      new tray::DateView(owner, Shell::Get()->system_tray_model()->clock());
  tri_view_->AddView(TriView::Container::START, date_view_);

  if (PowerStatus::Get()->IsBatteryPresent()) {
    power_status_view_ = new ash::PowerStatusView();
    std::unique_ptr<views::BoxLayout> box_layout =
        std::make_unique<views::BoxLayout>(views::BoxLayout::kHorizontal);
    box_layout->set_cross_axis_alignment(
        views::BoxLayout::CROSS_AXIS_ALIGNMENT_CENTER);
    box_layout->set_inside_border_insets(
        gfx::Insets(0, 0, 0, kTrayPopupLabelRightPadding));
    tri_view_->SetContainerLayout(TriView::Container::CENTER,
                                  std::move(box_layout));

    tri_view_->AddView(TriView::Container::CENTER,
                       TrayPopupUtils::CreateVerticalSeparator());
    tri_view_->AddView(TriView::Container::CENTER, power_status_view_);
  }
  tri_view_->SetContainerVisible(TriView::Container::END, false);

  if (TrayPopupUtils::CanOpenWebUISettings())
    date_view_->SetAction(tray::DateView::DateAction::SHOW_DATE_SETTINGS);
}

SystemInfoDefaultView::~SystemInfoDefaultView() = default;

tray::DateView* SystemInfoDefaultView::GetDateView() {
  return date_view_;
}

const tray::DateView* SystemInfoDefaultView::GetDateView() const {
  return date_view_;
}

void SystemInfoDefaultView::Layout() {
  gfx::Size min_start_size = tri_view_->GetMinSize(TriView::Container::START);
  min_start_size.set_width(
      CalculateDateViewWidth(date_view_->GetPreferredSize().width()));
  tri_view_->SetMinSize(TriView::Container::START, min_start_size);

  views::View::Layout();
}

int SystemInfoDefaultView::CalculateDateViewWidth(int preferred_width) {
  const float snap_to_width = kSeparatorWidth + kMenuButtonSize;
  int num_extra_tile_widths = 0;
  if (preferred_width > kMenuButtonSize) {
    const float extra_width = preferred_width - kMenuButtonSize;
    const float preferred_width_ratio = extra_width / snap_to_width;
    num_extra_tile_widths = std::ceil(preferred_width_ratio);
  }
  num_extra_tile_widths =
      std::max(kMinNumTileWidths - 1,
               std::min(num_extra_tile_widths, kMaxNumTileWidths - 1));

  return kMenuButtonSize + num_extra_tile_widths * snap_to_width;
}

}  // namespace ash
