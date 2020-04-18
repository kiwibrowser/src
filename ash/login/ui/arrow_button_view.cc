// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/login/ui/arrow_button_view.h"

#include "ash/resources/vector_icons/vector_icons.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/paint_vector_icon.h"

namespace ash {
namespace {

// Arrow icon size.
constexpr int kArrowIconSizeDp = 20;

}  // namespace

ArrowButtonView::ArrowButtonView(views::ButtonListener* listener, int size)
    : LoginButton(listener), size_(size) {
  SetPreferredSize(gfx::Size(size, size));
  SetFocusBehavior(FocusBehavior::ALWAYS);

  // Layer rendering is needed for animation.
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);

  SetImage(Button::STATE_NORMAL,
           gfx::CreateVectorIcon(kLockScreenArrowIcon, kArrowIconSizeDp,
                                 SK_ColorWHITE));
}

ArrowButtonView::~ArrowButtonView() = default;

void ArrowButtonView::PaintButtonContents(gfx::Canvas* canvas) {
  const gfx::Rect rect(GetContentsBounds());

  // Draw background.
  cc::PaintFlags flags;
  flags.setAntiAlias(true);
  flags.setColor(background_color_);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  canvas->DrawCircle(gfx::PointF(rect.CenterPoint()), size_ / 2, flags);

  // Draw arrow icon.
  views::ImageButton::PaintButtonContents(canvas);
}

void ArrowButtonView::SetBackgroundColor(SkColor color) {
  background_color_ = color;
  SchedulePaint();
}

}  // namespace ash
