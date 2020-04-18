// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/display_scale/scale_detailed_view.h"

#include "ash/shell.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/tray/hover_highlight_view.h"
#include "ash/system/tray/tray_popup_utils.h"
#include "base/command_line.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/display/display.h"
#include "ui/display/display_switches.h"
#include "ui/display/manager/display_manager.h"
#include "ui/views/controls/scroll_view.h"

namespace ash {
namespace tray {
namespace {

const double scales[] = {1.0f,  1.25f, 1.33f, 1.5f,  1.66f,
                         1.75f, 1.9f,  2.0f,  2.25f, 2.5f};
constexpr double kEpsilon = 0.01;

bool IsSameScaleFactor(double new_value) {
  return (std::abs(display::Display::GetForcedDeviceScaleFactor() - new_value) <
          kEpsilon);
}

}  // namespace

ScaleDetailedView::ScaleDetailedView(DetailedViewDelegate* delegate)
    : TrayDetailedView(delegate) {
  CreateScrollableList();
  CreateTitleRow(IDS_ASH_STATUS_TRAY_SCALE);
  UpdateScrollableList();
  Layout();
}

ScaleDetailedView::~ScaleDetailedView() = default;

HoverHighlightView* ScaleDetailedView::AddScrollListItem(
    const base::string16& text,
    bool highlight,
    bool checked) {
  HoverHighlightView* container = new HoverHighlightView(this);

  container->AddLabelRow(text);
  TrayPopupUtils::InitializeAsCheckableRow(container, checked);

  scroll_content()->AddChildView(container);
  return container;
}

void ScaleDetailedView::UpdateScrollableList() {
  scroll_content()->RemoveAllChildViews(true);
  view_to_scale_.clear();

  for (double scale : scales) {
    HoverHighlightView* container = AddScrollListItem(
        base::UTF8ToUTF16(base::StringPrintf("%.2f", scale)),
        false /* highlight */, IsSameScaleFactor(scale) /* checkmark icon */);
    view_to_scale_[container] = scale;
  }
}

void ScaleDetailedView::HandleViewClicked(views::View* view) {
  // The selected dsf is already active.
  double new_scale = view_to_scale_[view];
  if (IsSameScaleFactor(new_scale))
    return;
  display::Display::SetForceDeviceScaleFactor(new_scale);
  ash::Shell::Get()->display_manager()->UpdateDisplays();
  UpdateScrollableList();
  Layout();
}

}  // namespace tray
}  // namespace ash
