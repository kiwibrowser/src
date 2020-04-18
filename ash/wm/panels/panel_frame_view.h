// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_PANELS_PANEL_FRAME_VIEW_H_
#define ASH_WM_PANELS_PANEL_FRAME_VIEW_H_

#include <memory>

#include "ash/ash_export.h"
#include "ash/shell_observer.h"
#include "base/macros.h"
#include "ui/views/window/non_client_view.h"

namespace views {
class ImageView;
}

namespace ash {
class DefaultFrameHeader;
class FrameCaptionButtonContainerView;

class ASH_EXPORT PanelFrameView : public views::NonClientFrameView,
                                  public ShellObserver {
 public:
  // Internal class name.
  static const char kViewClassName[];

  enum FrameType { FRAME_NONE, FRAME_ASH };

  PanelFrameView(views::Widget* frame, FrameType frame_type);
  ~PanelFrameView() override;

  // Sets the active and inactive frame colors. Note the inactive frame color
  // will have some transparency added when the frame is drawn.
  void SetFrameColors(SkColor active_frame_color, SkColor inactive_frame_color);

  // views::View:
  const char* GetClassName() const override;

 private:
  void InitFrameHeader();

  aura::Window* GetWidgetWindow();

  // Height from top of window to top of client area.
  int NonClientTopBorderHeight() const;

  // views::NonClientFrameView:
  gfx::Rect GetBoundsForClientView() const override;
  gfx::Rect GetWindowBoundsForClientBounds(
      const gfx::Rect& client_bounds) const override;
  int NonClientHitTest(const gfx::Point& point) override;
  void GetWindowMask(const gfx::Size& size, gfx::Path* window_mask) override;
  void ResetWindowControls() override;
  void UpdateWindowIcon() override;
  void UpdateWindowTitle() override;
  void SizeConstraintsChanged() override;

  // views::View:
  gfx::Size GetMinimumSize() const override;
  void Layout() override;
  void OnPaint(gfx::Canvas* canvas) override;

  // ShellObserver:
  void OnOverviewModeStarting() override;
  void OnOverviewModeEnded() override;

  // Child View class describing the panel's title bar behavior
  // and buttons, owned by the view hierarchy
  views::Widget* frame_;
  FrameCaptionButtonContainerView* caption_button_container_;
  views::ImageView* window_icon_;
  gfx::Rect client_view_bounds_;

  // Helper class for painting the header.
  std::unique_ptr<DefaultFrameHeader> frame_header_;

  DISALLOW_COPY_AND_ASSIGN(PanelFrameView);
};
}

#endif  // ASH_WM_PANELS_PANEL_FRAME_VIEW_H_
