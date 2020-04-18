// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_CHROMEOS_IME_MODE_INDICATOR_VIEW_H_
#define UI_CHROMEOS_IME_MODE_INDICATOR_VIEW_H_

#include "base/macros.h"
#include "base/strings/string16.h"
#include "base/timer/timer.h"
#include "ui/chromeos/ui_chromeos_export.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/bubble/bubble_dialog_delegate.h"
#include "ui/views/widget/widget.h"

namespace views {
class Label;
}  // namespace views

namespace ui {
namespace ime {

// A small bubble that shows the short name of the current IME (e.g. "DV" for
// Dvorak) after switching IMEs with an accelerator (e.g. Ctrl-Space).
class UI_CHROMEOS_EXPORT ModeIndicatorView
    : public views::BubbleDialogDelegateView {
 public:
  class Delegate {
   public:
    virtual ~Delegate() = default;

    // Configures the InitParams to place the bubble in the right container.
    virtual void InitWidgetContainer(views::Widget::InitParams* params) = 0;
  };

  ModeIndicatorView(Delegate* delegate,
                    const gfx::Rect& cursor_bounds,
                    const base::string16& label);
  ~ModeIndicatorView() override;

  // Show the mode indicator then hide with fading animation.
  void ShowAndFadeOut();

  // views::BubbleDialogDelegateView override:
  void OnBeforeBubbleWidgetInit(views::Widget::InitParams* params,
                                views::Widget* widget) const override;
  gfx::Size CalculatePreferredSize() const override;
  const char* GetClassName() const override;
  int GetDialogButtons() const override;
  void Init() override;

 protected:
  // views::WidgetDelegateView overrides:
  views::NonClientFrameView* CreateNonClientFrameView(
      views::Widget* widget) override;

 private:
  Delegate* delegate_;
  gfx::Rect cursor_bounds_;
  views::Label* label_view_;
  base::OneShotTimer timer_;

  DISALLOW_COPY_AND_ASSIGN(ModeIndicatorView);
};

}  // namespace ime
}  // namespace ui

#endif  // UI_CHROMEOS_IME_MODE_INDICATOR_VIEW_H_
