// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SHELL_TOPLEVEL_WINDOW_H_
#define ASH_SHELL_TOPLEVEL_WINDOW_H_

#include "base/macros.h"
#include "ui/views/widget/widget_delegate.h"

namespace ash {
namespace shell {

class ToplevelWindow : public views::WidgetDelegateView {
 public:
  struct CreateParams {
    CreateParams();

    bool can_resize;
    bool can_maximize;
    bool use_saved_placement;
  };
  static views::Widget* CreateToplevelWindow(const CreateParams& params);

  // Clears saved show state and bounds used to position
  // a new window.
  static void ClearSavedStateForTest();

 private:
  explicit ToplevelWindow(const CreateParams& params);
  ~ToplevelWindow() override;

  // Overridden from views::View:
  void OnPaint(gfx::Canvas* canvas) override;

  // Overridden from views::WidgetDelegate:
  base::string16 GetWindowTitle() const override;
  void SaveWindowPlacement(const gfx::Rect& bounds,
                           ui::WindowShowState show_state) override;
  bool GetSavedWindowPlacement(const views::Widget* widget,
                               gfx::Rect* bounds,
                               ui::WindowShowState* show_state) const override;
  bool CanResize() const override;
  bool CanMaximize() const override;
  bool CanMinimize() const override;

  const CreateParams params_;

  DISALLOW_COPY_AND_ASSIGN(ToplevelWindow);
};

}  // namespace shell
}  // namespace ash

#endif  // ASH_SHELL_TOPLEVEL_WINDOW_H_
