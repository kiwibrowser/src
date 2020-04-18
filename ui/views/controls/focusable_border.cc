// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/controls/focusable_border.h"

#include "cc/paint/paint_flags.h"
#include "third_party/skia/include/core/SkPath.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/safe_integer_conversions.h"
#include "ui/gfx/scoped_canvas.h"
#include "ui/gfx/skia_util.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/controls/textfield/textfield.h"

namespace {

const int kInsetSize = 1;

}  // namespace

namespace views {

FocusableBorder::FocusableBorder() : insets_(kInsetSize) {}

FocusableBorder::~FocusableBorder() {
}

void FocusableBorder::SetColorId(
    const base::Optional<ui::NativeTheme::ColorId>& color_id) {
  override_color_id_ = color_id;
}

void FocusableBorder::Paint(const View& view, gfx::Canvas* canvas) {
  cc::PaintFlags flags;
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setColor(GetCurrentColor(view));

  gfx::ScopedCanvas scoped(canvas);
  float dsf = canvas->UndoDeviceScaleFactor();

  const int stroke_width_px =
      ui::MaterialDesignController::IsSecondaryUiMaterial()
          ? 1
          : gfx::ToFlooredInt(dsf);
  flags.setStrokeWidth(SkIntToScalar(stroke_width_px));

  // Scale the rect and snap to pixel boundaries.
  gfx::RectF rect(gfx::ScaleToEnclosedRect(view.GetLocalBounds(), dsf));
  rect.Inset(gfx::InsetsF(stroke_width_px / 2.0f));

  SkPath path;
  if (ui::MaterialDesignController::IsSecondaryUiMaterial()) {
    flags.setAntiAlias(true);
    float corner_radius_px = kCornerRadiusDp * dsf;
    path.addRoundRect(gfx::RectFToSkRect(rect), corner_radius_px,
                      corner_radius_px);
  } else {
    path.addRect(gfx::RectFToSkRect(rect), SkPath::kCW_Direction);
  }

  canvas->DrawPath(path, flags);
}

gfx::Insets FocusableBorder::GetInsets() const {
  return insets_;
}

gfx::Size FocusableBorder::GetMinimumSize() const {
  return gfx::Size();
}

void FocusableBorder::SetInsets(int top, int left, int bottom, int right) {
  insets_.Set(top, left, bottom, right);
}

void FocusableBorder::SetInsets(int vertical, int horizontal) {
  SetInsets(vertical, horizontal, vertical, horizontal);
}

SkColor FocusableBorder::GetCurrentColor(const View& view) const {
  ui::NativeTheme::ColorId color_id =
      ui::NativeTheme::kColorId_UnfocusedBorderColor;
  if (override_color_id_) {
    color_id = *override_color_id_;
  } else if (view.HasFocus() &&
             !ui::MaterialDesignController::IsSecondaryUiMaterial()) {
    // Note with --secondary-ui-md there is a FocusRing indicator, so the border
    // retains its unfocused color.
    color_id = ui::NativeTheme::kColorId_FocusedBorderColor;
  }

  SkColor color = view.GetNativeTheme()->GetSystemColor(color_id);
  if (ui::MaterialDesignController::IsSecondaryUiMaterial() &&
      !view.enabled()) {
    return color_utils::BlendTowardOppositeLuma(color,
                                                gfx::kDisabledControlAlpha);
  }
  return color;
}

}  // namespace views
