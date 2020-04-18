// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_OVERLAY_OVERLAY_WINDOW_VIEWS_H_
#define CHROME_BROWSER_UI_VIEWS_OVERLAY_OVERLAY_WINDOW_VIEWS_H_

#include "content/public/browser/overlay_window.h"

#include "ui/gfx/geometry/size.h"
#include "ui/views/widget/widget.h"

namespace views {
class ImageButton;
class ToggleImageButton;
}  // namespace views

// The Chrome desktop implementation of OverlayWindow. This will only be
// implemented in views, which will support all desktop platforms.
class OverlayWindowViews : public content::OverlayWindow, public views::Widget {
 public:
  explicit OverlayWindowViews(
      content::PictureInPictureWindowController* controller);
  ~OverlayWindowViews() override;

  // OverlayWindow:
  bool IsActive() const override;
  void Close() override;
  void Show() override;
  void Hide() override;
  bool IsVisible() const override;
  bool IsAlwaysOnTop() const override;
  ui::Layer* GetLayer() override;
  gfx::Rect GetBounds() const override;
  void UpdateVideoSize(const gfx::Size& natural_size) override;
  ui::Layer* GetVideoLayer() override;
  ui::Layer* GetControlsBackgroundLayer() override;
  ui::Layer* GetCloseControlsLayer() override;
  ui::Layer* GetPlayPauseControlsLayer() override;
  gfx::Rect GetCloseControlsBounds() override;
  gfx::Rect GetPlayPauseControlsBounds() override;

  // views::Widget:
  gfx::Size GetMinimumSize() const override;
  gfx::Size GetMaximumSize() const override;
  void OnNativeWidgetWorkspaceChanged() override;
  void OnMouseEvent(ui::MouseEvent* event) override;

  // views::internal::NativeWidgetDelegate:
  void OnNativeWidgetMove() override;
  void OnNativeWidgetSizeChanged(const gfx::Size& new_size) override;

 private:
  // Determine the intended bounds of |this|. This should be called when there
  // is reason for the bounds to change, such as switching primary displays or
  // playing a new video (i.e. different aspect ratio). This also updates
  // |min_size_| and |max_size_|.
  gfx::Rect CalculateAndUpdateBounds();

  // Set up the views::Views that will be shown on the window.
  void SetUpViews();

  // Update |current_size_| closest to the |new_size| while adhering to the
  // aspect ratio of the video, which is retrieved from |natural_size_|.
  void UpdateCurrentSizeWithAspectRatio(gfx::Size new_size);

  // Not owned; |controller_| owns |this|.
  content::PictureInPictureWindowController* controller_;

  // The upper and lower bounds of |current_size_|. These are determined by the
  // size of the primary display work area when Picture-in-Picture is initiated.
  // TODO(apacible): Update these bounds when the display the window is on
  // changes. http://crbug.com/819673
  gfx::Size min_size_;
  gfx::Size max_size_;

  // Current bounds of the Picture-in-Picture window.
  gfx::Rect current_bounds_;

  // The natural size of the video to show. This is used to compute sizing and
  // ensuring factors such as aspect ratio is maintained.
  gfx::Size natural_size_;

  // Views to be shown.
  std::unique_ptr<views::View> video_view_;
  std::unique_ptr<views::View> controls_background_view_;
  std::unique_ptr<views::ImageButton> close_controls_view_;
  std::unique_ptr<views::ToggleImageButton> play_pause_controls_view_;

  DISALLOW_COPY_AND_ASSIGN(OverlayWindowViews);
};

#endif  // CHROME_BROWSER_UI_VIEWS_OVERLAY_OVERLAY_WINDOW_VIEWS_H_
