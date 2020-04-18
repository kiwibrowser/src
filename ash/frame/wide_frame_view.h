// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_FRAME_WIDE_FRAME_VIEW_H_
#define ASH_FRAME_WIDE_FRAME_VIEW_H_

#include "ash/ash_export.h"
#include "ash/frame/caption_buttons/caption_button_model.h"
#include "ash/public/cpp/immersive/immersive_fullscreen_controller_delegate.h"
#include "ash/shell.h"
#include "ash/shell_observer.h"
#include "ui/aura/window_observer.h"
#include "ui/display/display_observer.h"
#include "ui/views/widget/widget_delegate.h"

namespace views {
class Widget;
}

namespace ash {
class HeaderView;
class ImmersiveFullscreenController;

// WideFrameView is used for the case where the widget's
// maximzed/fullscreen doesn't cover the entire workarea/display area
// but the caption frame should occupy the full width and placed at the top
// of the display.
// TODO(oshima): Currently client is responsible for hooking this up to
// the target widget because ImmersiveFullscreenController is not owned by
// CustomFrameViewAsh. Investigate if we integrate this into
// CustomFrameViewAsh.
class ASH_EXPORT WideFrameView
    : public views::WidgetDelegateView,
      public aura::WindowObserver,
      public display::DisplayObserver,
      public ash::ImmersiveFullscreenControllerDelegate,
      public ash::ShellObserver {
 public:
  // Creates wide frame for |target| widget. It's caller's responsibility
  // to Close when the wide frame is no longer necessary.
  static WideFrameView* Create(views::Widget* target);

  // Initialize |immersive_fullscreen_controller| so that the controller reveals
  // and |hides_header_| in immersive mode.
  void Init(ash::ImmersiveFullscreenController* controller);

  // Show/Closes the frame.
  void Show();
  void Close();

  // Set the caption model for caption buttions on this frame.
  void SetCaptionButtonModel(std::unique_ptr<ash::CaptionButtonModel> mode);

  ash::HeaderView* header_view() { return header_view_; }

 private:
  static gfx::Rect GetFrameBounds(views::Widget* target);

  WideFrameView(views::Widget* target, views::Widget* frame_widget);
  ~WideFrameView() override;

  // views::View:
  void Layout() override;

  // aura::WindowObserver:
  void OnWindowDestroying(aura::Window* window) override;

  // display::DisplayObserver:
  void OnDisplayMetricsChanged(const display::Display& display,
                               uint32_t changed_metrics) override;

  // ash::ImmersiveFullscreenControllerDelegate:
  void OnImmersiveRevealStarted() override;
  void OnImmersiveRevealEnded() override;
  void OnImmersiveFullscreenEntered() override;
  void OnImmersiveFullscreenExited() override;
  void SetVisibleFraction(double visible_fraction) override;
  std::vector<gfx::Rect> GetVisibleBoundsInScreen() const override;

  // ash::ShellObserver:
  void OnOverviewModeStarting() override;
  void OnOverviewModeEnded() override;

  ash::HeaderView* GetTargetHeaderView();

  // The target widget this frame will control.
  views::Widget* target_;

  // The widget that hosts the wide frame.
  views::Widget* widget_;

  ash::HeaderView* header_view_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(WideFrameView);
};

}  // namespace ash

#endif  // ASH_FRAME_WIDE_FRAME_VIEW_H_
