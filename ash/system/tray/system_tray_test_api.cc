// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/tray/system_tray_test_api.h"

#include "ash/shell.h"
#include "ash/system/date/date_view.h"
#include "ash/system/date/tray_system_info.h"
#include "ash/system/enterprise/tray_enterprise.h"
#include "ash/system/network/tray_network.h"
#include "ash/system/tray/system_tray.h"
#include "base/run_loop.h"
#include "base/strings/string16.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "ui/compositor/scoped_animation_duration_scale_mode.h"
#include "ui/gfx/geometry/point.h"
#include "ui/views/controls/label.h"
#include "ui/views/view.h"

namespace ash {
namespace {

// Returns true if this view or any child view has the given tooltip.
bool HasChildWithTooltip(views::View* view,
                         const base::string16& given_tooltip) {
  base::string16 tooltip;
  view->GetTooltipText(gfx::Point(), &tooltip);
  if (tooltip == given_tooltip)
    return true;

  for (int i = 0; i < view->child_count(); ++i) {
    if (HasChildWithTooltip(view->child_at(i), given_tooltip))
      return true;
  }

  return false;
}

}  // namespace

SystemTrayTestApi::SystemTrayTestApi(SystemTray* tray) : tray_(tray) {}

SystemTrayTestApi::~SystemTrayTestApi() = default;

// static
void SystemTrayTestApi::BindRequest(mojom::SystemTrayTestApiRequest request) {
  SystemTray* tray = Shell::Get()->GetPrimarySystemTray();
  mojo::MakeStrongBinding(std::make_unique<SystemTrayTestApi>(tray),
                          std::move(request));
}

void SystemTrayTestApi::DisableAnimations(DisableAnimationsCallback cb) {
  disable_animations_ = std::make_unique<ui::ScopedAnimationDurationScaleMode>(
      ui::ScopedAnimationDurationScaleMode::ZERO_DURATION);
  std::move(cb).Run();
}

void SystemTrayTestApi::IsTrayBubbleOpen(IsTrayBubbleOpenCallback cb) {
  std::move(cb).Run(tray_->HasSystemBubble());
}

void SystemTrayTestApi::IsTrayViewVisible(int view_id,
                                          IsTrayViewVisibleCallback cb) {
  // Search for the view among the tray icons.
  views::View* view = tray_->GetViewByID(view_id);
  std::move(cb).Run(view && view->visible());
}

void SystemTrayTestApi::ShowBubble(ShowBubbleCallback cb) {
  tray_->ShowDefaultView(ash::BUBBLE_CREATE_NEW, false /* show_by_click */);
  std::move(cb).Run();
}

void SystemTrayTestApi::ShowDetailedView(mojom::TrayItem item,
                                         ShowDetailedViewCallback cb) {
  SystemTrayItem* tray_item;
  switch (item) {
    case mojom::TrayItem::kEnterprise:
      tray_item = tray_->tray_enterprise_;
      break;
    case mojom::TrayItem::kNetwork:
      tray_item = tray_->tray_network_;
      break;
  }
  tray_->ShowDetailedView(tray_item, 0 /* delay */, BUBBLE_CREATE_NEW);
  std::move(cb).Run();
}

void SystemTrayTestApi::IsBubbleViewVisible(int view_id,
                                            IsBubbleViewVisibleCallback cb) {
  views::View* view = GetBubbleView(view_id);
  std::move(cb).Run(view && view->visible());
}

void SystemTrayTestApi::GetBubbleViewTooltip(int view_id,
                                             GetBubbleViewTooltipCallback cb) {
  base::string16 tooltip;
  views::View* view = GetBubbleView(view_id);
  if (view)
    view->GetTooltipText(gfx::Point(), &tooltip);
  std::move(cb).Run(tooltip);
}

void SystemTrayTestApi::GetBubbleLabelText(int view_id,
                                           GetBubbleLabelTextCallback cb) {
  base::string16 text;
  views::View* view = GetBubbleView(view_id);
  if (view)
    text = static_cast<views::Label*>(view)->text();
  std::move(cb).Run(text);
}

void SystemTrayTestApi::Is24HourClock(Is24HourClockCallback cb) {
  base::HourClockType type = tray_->tray_system_info_->GetTimeTrayForTesting()
                                 ->GetHourTypeForTesting();
  std::move(cb).Run(type == base::k24HourClock);
}

views::View* SystemTrayTestApi::GetBubbleView(int view_id) const {
  return tray_->GetSystemBubble()->bubble_view()->GetViewByID(view_id);
}

}  // namespace ash
