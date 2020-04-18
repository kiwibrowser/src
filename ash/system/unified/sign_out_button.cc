// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/unified/sign_out_button.h"

#include "ash/session/session_controller.h"
#include "ash/shell.h"
#include "ash/system/tray/tray_constants.h"
#include "ash/system/tray/tray_popup_utils.h"
#include "ash/system/user/login_status.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/canvas.h"
#include "ui/views/animation/flood_fill_ink_drop_ripple.h"
#include "ui/views/animation/ink_drop_highlight.h"
#include "ui/views/animation/ink_drop_impl.h"
#include "ui/views/animation/ink_drop_mask.h"
#include "ui/views/border.h"

namespace ash {

SignOutButton::SignOutButton(views::ButtonListener* listener)
    : views::LabelButton(listener,
                         user::GetLocalizedSignOutStringForStatus(
                             Shell::Get()->session_controller()->login_status(),
                             false /* multiline */)) {
  SetVisible(Shell::Get()->session_controller()->login_status() !=
             LoginStatus::NOT_LOGGED_IN);

  SetEnabledTextColors(kUnifiedMenuTextColor);
  SetHorizontalAlignment(gfx::ALIGN_CENTER);
  SetBorder(views::CreateEmptyBorder(gfx::Insets()));
  label()->SetSubpixelRenderingEnabled(false);
  TrayPopupUtils::ConfigureTrayPopupButton(this);
}

SignOutButton::~SignOutButton() = default;

gfx::Size SignOutButton::CalculatePreferredSize() const {
  return gfx::Size(label()->GetPreferredSize().width() + kTrayItemSize,
                   kTrayItemSize);
}

int SignOutButton::GetHeightForWidth(int width) const {
  return kTrayItemSize;
}

void SignOutButton::PaintButtonContents(gfx::Canvas* canvas) {
  gfx::Rect rect(GetContentsBounds());
  cc::PaintFlags flags;
  flags.setAntiAlias(true);
  flags.setColor(kUnifiedMenuButtonColor);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  canvas->DrawRoundRect(rect, kTrayItemSize / 2, flags);

  views::LabelButton::PaintButtonContents(canvas);
}

std::unique_ptr<views::InkDrop> SignOutButton::CreateInkDrop() {
  return TrayPopupUtils::CreateInkDrop(this);
}

std::unique_ptr<views::InkDropRipple> SignOutButton::CreateInkDropRipple()
    const {
  return TrayPopupUtils::CreateInkDropRipple(
      TrayPopupInkDropStyle::FILL_BOUNDS, this,
      GetInkDropCenterBasedOnLastEvent(), kUnifiedMenuIconColor);
}

std::unique_ptr<views::InkDropHighlight> SignOutButton::CreateInkDropHighlight()
    const {
  return TrayPopupUtils::CreateInkDropHighlight(
      TrayPopupInkDropStyle::FILL_BOUNDS, this, kUnifiedMenuIconColor);
}

std::unique_ptr<views::InkDropMask> SignOutButton::CreateInkDropMask() const {
  return std::make_unique<views::RoundRectInkDropMask>(size(), gfx::Insets(),
                                                       kTrayItemSize / 2);
}

}  // namespace ash
