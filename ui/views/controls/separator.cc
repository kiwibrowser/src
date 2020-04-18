// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/controls/separator.h"

#include "ui/accessibility/ax_node_data.h"
#include "ui/gfx/canvas.h"
#include "ui/native_theme/native_theme.h"

namespace views {

// static
const char Separator::kViewClassName[] = "Separator";

// static
const int Separator::kThickness = 1;

Separator::Separator() {}

Separator::~Separator() {}

void Separator::SetColor(SkColor color) {
  overridden_color_ = color;
  SchedulePaint();
}

void Separator::SetPreferredHeight(int height) {
  preferred_height_ = height;
  PreferredSizeChanged();
}

////////////////////////////////////////////////////////////////////////////////
// Separator, View overrides:

gfx::Size Separator::CalculatePreferredSize() const {
  gfx::Size size(kThickness, preferred_height_);
  gfx::Insets insets = GetInsets();
  size.Enlarge(insets.width(), insets.height());
  return size;
}

void Separator::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  node_data->role = ax::mojom::Role::kSplitter;
}

void Separator::OnPaint(gfx::Canvas* canvas) {
  SkColor color = overridden_color_
                      ? *overridden_color_
                      : GetNativeTheme()->GetSystemColor(
                            ui::NativeTheme::kColorId_SeparatorColor);

  // The separator fills its bounds, but avoid filling partial pixels.
  float dsf = canvas->UndoDeviceScaleFactor();
  gfx::RectF contents = gfx::ScaleRect(gfx::RectF(GetContentsBounds()), dsf);
  canvas->FillRect(gfx::ToEnclosedRect(contents), color);

  View::OnPaint(canvas);
}

const char* Separator::GetClassName() const {
  return kViewClassName;
}

}  // namespace views
