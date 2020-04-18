// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/unified/top_shortcut_button.h"

#include "ash/system/tray/tray_constants.h"
#include "ash/system/tray/tray_popup_utils.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/animation/flood_fill_ink_drop_ripple.h"
#include "ui/views/animation/ink_drop_highlight.h"
#include "ui/views/animation/ink_drop_impl.h"
#include "ui/views/animation/ink_drop_mask.h"

namespace ash {

TopShortcutButton::TopShortcutButton(views::ButtonListener* listener,
                                     const gfx::VectorIcon& icon,
                                     int accessible_name_id)
    : views::ImageButton(listener) {
  SetPreferredSize(gfx::Size(kTrayItemSize, kTrayItemSize));

  SetImage(views::Button::STATE_NORMAL,
           gfx::CreateVectorIcon(icon, kUnifiedMenuIconColor));
  SetImage(views::Button::STATE_DISABLED,
           gfx::CreateVectorIcon(icon, kUnifiedMenuIconColorDisabled));

  SetImageAlignment(ALIGN_CENTER, ALIGN_MIDDLE);
  SetTooltipText(l10n_util::GetStringUTF16(accessible_name_id));

  TrayPopupUtils::ConfigureTrayPopupButton(this);
}

TopShortcutButton::~TopShortcutButton() = default;

void TopShortcutButton::PaintButtonContents(gfx::Canvas* canvas) {
  gfx::Rect rect(GetContentsBounds());
  cc::PaintFlags flags;
  flags.setAntiAlias(true);
  flags.setColor(kUnifiedMenuButtonColor);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  canvas->DrawCircle(gfx::PointF(rect.CenterPoint()), kTrayItemSize / 2, flags);

  views::ImageButton::PaintButtonContents(canvas);
}

std::unique_ptr<views::InkDrop> TopShortcutButton::CreateInkDrop() {
  return TrayPopupUtils::CreateInkDrop(this);
}

std::unique_ptr<views::InkDropRipple> TopShortcutButton::CreateInkDropRipple()
    const {
  return TrayPopupUtils::CreateInkDropRipple(
      TrayPopupInkDropStyle::FILL_BOUNDS, this,
      GetInkDropCenterBasedOnLastEvent(), kUnifiedMenuIconColor);
}

std::unique_ptr<views::InkDropHighlight>
TopShortcutButton::CreateInkDropHighlight() const {
  return TrayPopupUtils::CreateInkDropHighlight(
      TrayPopupInkDropStyle::FILL_BOUNDS, this, kUnifiedMenuIconColor);
}

std::unique_ptr<views::InkDropMask> TopShortcutButton::CreateInkDropMask()
    const {
  return std::make_unique<views::CircleInkDropMask>(
      size(), GetContentsBounds().CenterPoint(), kTrayItemSize / 2);
}

}  // namespace ash
