// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_CONTROLS_SCROLLBAR_SCROLL_BAR_VIEWS_H_
#define UI_VIEWS_CONTROLS_SCROLLBAR_SCROLL_BAR_VIEWS_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "ui/gfx/geometry/point.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/scrollbar/base_scroll_bar.h"
#include "ui/views/view.h"

namespace gfx {
class Canvas;
}

namespace views {

// Views implementation for the scrollbar.
class VIEWS_EXPORT ScrollBarViews : public BaseScrollBar,
                                    public ButtonListener {
 public:
  static const char kViewClassName[];

  // Creates new scrollbar, either horizontal or vertical.
  explicit ScrollBarViews(bool horizontal);
  ~ScrollBarViews() override;

  static int GetVerticalScrollBarWidth(const ui::NativeTheme* theme);

 protected:
  // View overrides:
  void Layout() override;
  void OnPaint(gfx::Canvas* canvas) override;
  gfx::Size CalculatePreferredSize() const override;
  const char* GetClassName() const override;

  // ScrollBar overrides:
  int GetThickness() const override;

  // BaseButton::ButtonListener overrides:
  void ButtonPressed(Button* sender, const ui::Event& event) override;

  // Returns the area for the track. This is the area of the scrollbar minus
  // the size of the arrow buttons.
  gfx::Rect GetTrackBounds() const override;

 private:
  static int GetHorizontalScrollBarHeight(const ui::NativeTheme* theme);

  // The scroll bar buttons (Up/Down, Left/Right).
  Button* prev_button_;
  Button* next_button_;

  ui::NativeTheme::ExtraParams params_;
  ui::NativeTheme::Part part_;
  ui::NativeTheme::State state_;

  DISALLOW_COPY_AND_ASSIGN(ScrollBarViews);
};

}  // namespace views

#endif  // UI_VIEWS_CONTROLS_SCROLLBAR_SCROLL_BAR_VIEWS_H_
