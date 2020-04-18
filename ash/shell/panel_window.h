// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SHELL_PANEL_WINDOW_H_
#define ASH_SHELL_PANEL_WINDOW_H_

#include "base/macros.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

namespace ash {

// Example Class for panel windows (Widget::InitParams::TYPE_PANEL).
// Instances of PanelWindow will get added to the PanelContainer top level
// window which manages the panel layout through PanelLayoutManager.
class PanelWindow : public views::WidgetDelegateView {
 public:
  explicit PanelWindow(const std::string& name);
  ~PanelWindow() override;

  // Creates the widget for the panel window using |params_|.
  views::Widget* CreateWidget();

  const std::string& name() { return name_; }
  views::Widget::InitParams& params() { return params_; }

  // Creates a panel window and returns the associated widget.
  static views::Widget* CreatePanelWindow(const gfx::Rect& rect);

 private:
  // Overridden from views::View:
  gfx::Size CalculatePreferredSize() const override;
  void OnPaint(gfx::Canvas* canvas) override;

  // Overridden from views::WidgetDelegate:
  base::string16 GetWindowTitle() const override;
  bool CanResize() const override;
  bool CanMaximize() const override;
  bool CanMinimize() const override;
  views::NonClientFrameView* CreateNonClientFrameView(
      views::Widget* widget) override;

  std::string name_;
  views::Widget::InitParams params_;

  DISALLOW_COPY_AND_ASSIGN(PanelWindow);
};

}  // namespace ash

#endif  // ASH_SHELL_PANEL_WINDOW_H_
