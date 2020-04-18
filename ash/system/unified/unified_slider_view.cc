// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/unified/unified_slider_view.h"

#include "ash/system/tray/tray_constants.h"
#include "ash/system/unified/top_shortcut_button.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/border.h"
#include "ui/views/layout/box_layout.h"

namespace ash {

UnifiedSliderButton::UnifiedSliderButton(views::ButtonListener* listener,
                                         const gfx::VectorIcon& icon,
                                         int accessible_name_id)
    : TopShortcutButton(listener, icon, accessible_name_id) {}

UnifiedSliderButton::~UnifiedSliderButton() = default;

void UnifiedSliderButton::SetVectorIcon(const gfx::VectorIcon& icon) {
  SetImage(views::Button::STATE_NORMAL,
           gfx::CreateVectorIcon(icon, kUnifiedMenuIconColor));
  SetImage(views::Button::STATE_DISABLED,
           gfx::CreateVectorIcon(icon, kUnifiedMenuIconColor));
}

void UnifiedSliderButton::SetToggled(bool toggled) {
  toggled_ = toggled;
  SchedulePaint();
}

void UnifiedSliderButton::PaintButtonContents(gfx::Canvas* canvas) {
  gfx::Rect rect(GetContentsBounds());
  cc::PaintFlags flags;
  flags.setAntiAlias(true);
  flags.setColor(toggled_ ? kUnifiedMenuButtonColorActive
                          : kUnifiedMenuButtonColor);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  canvas->DrawCircle(gfx::PointF(rect.CenterPoint()), kTrayItemSize / 2, flags);

  views::ImageButton::PaintButtonContents(canvas);
}

UnifiedSliderView::UnifiedSliderView(UnifiedSliderListener* listener,
                                     const gfx::VectorIcon& icon,
                                     int accessible_name_id)
    : button_(new UnifiedSliderButton(listener, icon, accessible_name_id)),
      slider_(new views::Slider(listener)) {
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::kHorizontal, kUnifiedMenuItemPadding,
      kUnifiedTopShortcutSpacing));

  AddChildView(button_);
  AddChildView(slider_);

  slider_->SetBorder(views::CreateEmptyBorder(kUnifiedSliderPadding));
  layout->SetFlexForView(slider_, 1);
}

UnifiedSliderView::~UnifiedSliderView() = default;

}  // namespace ash
