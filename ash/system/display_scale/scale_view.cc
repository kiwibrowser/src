// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/display_scale/scale_view.h"

#include <algorithm>

#include "ash/shell.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/tray/actionable_view.h"
#include "ash/system/tray/system_tray_item.h"
#include "ash/system/tray/tray_constants.h"
#include "ash/system/tray/tray_popup_utils.h"
#include "ash/system/tray/tri_view.h"
#include "base/command_line.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/display/display.h"
#include "ui/display/manager/display_manager.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/accessibility/view_accessibility.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/slider.h"
#include "ui/views/layout/fill_layout.h"

namespace ash {
namespace tray {

ScaleView::ScaleView(SystemTrayItem* owner, bool is_default_view)
    : owner_(owner),
      tri_view_(TrayPopupUtils::CreateMultiTargetRowView()),
      more_button_(nullptr),
      label_(nullptr),
      slider_(nullptr),
      is_default_view_(is_default_view) {
  SetLayoutManager(std::make_unique<views::FillLayout>());
  AddChildView(tri_view_);

  label_ = new views::Label(base::UTF8ToUTF16(base::StringPrintf(
      "%.2f", display::Display::GetForcedDeviceScaleFactor())));
  tri_view_->AddView(TriView::Container::START, label_);

  slider_ = TrayPopupUtils::CreateSlider(this);
  slider_->SetValue((display::Display::GetForcedDeviceScaleFactor() - 1.f) / 2);
  slider_->GetViewAccessibility().OverrideName(
      l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_SCALE_SLIDER));
  tri_view_->AddView(TriView::Container::CENTER, slider_);

  SetBackground(views::CreateThemedSolidBackground(
      this, ui::NativeTheme::kColorId_BubbleBackground));

  if (!is_default_view_) {
    tri_view_->SetContainerVisible(TriView::Container::END, false);
    Layout();
    return;
  }

  more_button_ = new ButtonListenerActionableView(
      owner_, TrayPopupInkDropStyle::INSET_BOUNDS, this);
  TrayPopupUtils::ConfigureContainer(TriView::Container::END, more_button_);

  more_button_->SetInkDropMode(views::InkDropHostView::InkDropMode::ON);
  more_button_->SetBorder(
      views::CreateEmptyBorder(gfx::Insets(0, kTrayPopupButtonEndMargin)));
  tri_view_->AddView(TriView::Container::END, more_button_);

  more_button_->AddChildView(TrayPopupUtils::CreateMoreImageView());
  more_button_->SetAccessibleName(
      l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_SCALE));
  Layout();
}

ScaleView::~ScaleView() = default;

void ScaleView::ButtonPressed(views::Button* sender, const ui::Event& event) {
  if (sender == more_button_)
    owner_->TransitionDetailedView();
}

void ScaleView::SliderValueChanged(views::Slider* sender,
                                   float value,
                                   float old_value,
                                   views::SliderChangeReason reason) {
  if (reason == views::VALUE_CHANGED_BY_USER) {
    label_->SetText(
        base::UTF8ToUTF16(base::StringPrintf("%.2f", 1.f + value * 2.0f)));
  }
}

void ScaleView::SliderDragEnded(views::Slider* sender) {
  display::Display::SetForceDeviceScaleFactor(1.f + slider_->value() * 2.0f);
  ash::Shell::Get()->display_manager()->UpdateDisplays();
}

}  // namespace tray
}  // namespace ash
