// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/frame/frame_header.h"

#include "ash/frame/caption_buttons/caption_button_model.h"
#include "ash/frame/caption_buttons/frame_caption_button_container_view.h"
#include "ash/frame/frame_header_util.h"
#include "ash/public/cpp/ash_layout_constants.h"
#include "ash/public/cpp/vector_icons/vector_icons.h"
#include "ash/resources/vector_icons/vector_icons.h"
#include "base/logging.h"  // DCHECK
#include "ui/gfx/canvas.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/scoped_canvas.h"
#include "ui/views/view.h"
#include "ui/views/widget/native_widget_aura.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

namespace ash {

namespace {
// Duration of crossfade animation for activating and deactivating frame.
const int kActivationCrossfadeDurationMs = 200;
}  // namespace

///////////////////////////////////////////////////////////////////////////////
// FrameHeader, public:

FrameHeader::~FrameHeader() = default;

int FrameHeader::GetMinimumHeaderWidth() const {
  // Ensure we have enough space for the window icon and buttons. We allow
  // the title string to collapse to zero width.
  return GetTitleBounds().x() +
         caption_button_container_->GetMinimumSize().width();
}

void FrameHeader::PaintHeader(gfx::Canvas* canvas, Mode mode) {
  Mode old_mode = mode_;
  mode_ = mode;

  if (mode_ != old_mode) {
    UpdateCaptionButtonColors();

    if (!initial_paint_ &&
        FrameHeaderUtil::CanAnimateActivation(target_widget_)) {
      activation_animation_.SetSlideDuration(kActivationCrossfadeDurationMs);
      if (mode_ == MODE_ACTIVE)
        activation_animation_.Show();
      else
        activation_animation_.Hide();
    } else {
      if (mode_ == MODE_ACTIVE)
        activation_animation_.Reset(1);
      else
        activation_animation_.Reset(0);
    }
    initial_paint_ = false;
  }

  DoPaintHeader(canvas);
}

void FrameHeader::LayoutHeader() {
  LayoutHeaderInternal();
  // Default to the header height; owning code may override via
  // SetHeaderHeightForPainting().
  painted_height_ = GetHeaderHeight();
}

int FrameHeader::GetHeaderHeight() const {
  return caption_button_container_->height();
}

int FrameHeader::GetHeaderHeightForPainting() const {
  return painted_height_;
}

void FrameHeader::SetHeaderHeightForPainting(int height) {
  painted_height_ = height;
}

void FrameHeader::SchedulePaintForTitle() {
  view_->SchedulePaintInRect(GetTitleBounds());
}

void FrameHeader::SetPaintAsActive(bool paint_as_active) {
  caption_button_container_->SetPaintAsActive(paint_as_active);
  if (back_button_)
    back_button_->set_paint_as_active(paint_as_active);
  UpdateCaptionButtonColors();
}

void FrameHeader::SetWidthInPixels(int width_in_pixels) {
  NOTREACHED() << "Frame does not support drawing width in pixels";
}

void FrameHeader::OnShowStateChanged(ui::WindowShowState show_state) {
  if (show_state == ui::SHOW_STATE_MINIMIZED)
    return;

  LayoutHeaderInternal();
}

void FrameHeader::SetLeftHeaderView(views::View* left_header_view) {
  left_header_view_ = left_header_view;
}

void FrameHeader::SetBackButton(FrameCaptionButton* back_button) {
  back_button_ = back_button;
  if (back_button_) {
    back_button_->SetColorMode(button_color_mode_);
    back_button_->SetBackgroundColor(GetCurrentFrameColor());
    back_button_->SetImage(CAPTION_BUTTON_ICON_BACK,
                           FrameCaptionButton::ANIMATE_NO,
                           kWindowControlBackIcon);
  }
}

FrameCaptionButton* FrameHeader::GetBackButton() const {
  return back_button_;
}

void FrameHeader::SetFrameColors(SkColor active_frame_color,
                                 SkColor inactive_frame_color) {
  DoSetFrameColors(active_frame_color, inactive_frame_color);
}

///////////////////////////////////////////////////////////////////////////////
// gfx::AnimationDelegate overrides:

void FrameHeader::AnimationProgressed(const gfx::Animation* animation) {
  view_->SchedulePaintInRect(GetPaintedBounds());
}

///////////////////////////////////////////////////////////////////////////////
// FrameHeader, protected:

FrameHeader::FrameHeader(views::Widget* target_widget, views::View* view)
    : target_widget_(target_widget), view_(view) {
  DCHECK(target_widget);
  DCHECK(view);
}

gfx::Rect FrameHeader::GetPaintedBounds() const {
  return gfx::Rect(view_->width(), painted_height_);
}

void FrameHeader::UpdateCaptionButtonColors() {
  caption_button_container_->SetColorMode(button_color_mode_);
  caption_button_container_->SetBackgroundColor(GetCurrentFrameColor());
  if (back_button_) {
    back_button_->SetColorMode(button_color_mode_);
    back_button_->SetBackgroundColor(GetCurrentFrameColor());
  }
}

void FrameHeader::PaintTitleBar(gfx::Canvas* canvas) {
  views::WidgetDelegate* target_widget_delegate =
      target_widget_->widget_delegate();
  if (target_widget_delegate &&
      target_widget_delegate->ShouldShowWindowTitle() &&
      !target_widget_delegate->GetWindowTitle().empty()) {
    canvas->DrawStringRectWithFlags(
        target_widget_delegate->GetWindowTitle(),
        views::NativeWidgetAura::GetWindowTitleFontList(), GetTitleColor(),
        GetTitleBounds(), gfx::Canvas::NO_SUBPIXEL_RENDERING);
  }
}

void FrameHeader::SetCaptionButtonContainer(
    FrameCaptionButtonContainerView* caption_button_container) {
  caption_button_container_ = caption_button_container;
  caption_button_container_->SetButtonImage(CAPTION_BUTTON_ICON_MINIMIZE,
                                            kWindowControlMinimizeIcon);
  caption_button_container_->SetButtonImage(CAPTION_BUTTON_ICON_MENU,
                                            kWindowControlMenuIcon);
  caption_button_container_->SetButtonImage(CAPTION_BUTTON_ICON_CLOSE,
                                            kWindowControlCloseIcon);
  caption_button_container_->SetButtonImage(CAPTION_BUTTON_ICON_LEFT_SNAPPED,
                                            kWindowControlLeftSnappedIcon);
  caption_button_container_->SetButtonImage(CAPTION_BUTTON_ICON_RIGHT_SNAPPED,
                                            kWindowControlRightSnappedIcon);
}

///////////////////////////////////////////////////////////////////////////////
// FrameHeader, private:

void FrameHeader::LayoutHeaderInternal() {
  bool use_zoom_icons = caption_button_container()->model()->InZoomMode();
  const gfx::VectorIcon& restore_icon =
      use_zoom_icons ? kWindowControlDezoomIcon : kWindowControlRestoreIcon;
  const gfx::VectorIcon& maximize_icon =
      use_zoom_icons ? kWindowControlZoomIcon : kWindowControlMaximizeIcon;
  const gfx::VectorIcon& icon =
      target_widget_->IsMaximized() || target_widget_->IsFullscreen()
          ? restore_icon
          : maximize_icon;
  caption_button_container()->SetButtonImage(
      CAPTION_BUTTON_ICON_MAXIMIZE_RESTORE, icon);

  caption_button_container()->SetButtonSize(
      GetAshLayoutSize(GetButtonLayoutSize()));

  const gfx::Size caption_button_container_size =
      caption_button_container()->GetPreferredSize();
  caption_button_container()->SetBounds(
      view_->width() - caption_button_container_size.width(), 0,
      caption_button_container_size.width(),
      caption_button_container_size.height());

  caption_button_container()->Layout();

  int origin = 0;
  if (back_button_) {
    gfx::Size size = back_button_->GetPreferredSize();
    back_button_->SetBounds(0, 0, size.width(),
                            caption_button_container_size.height());
    origin = back_button_->bounds().right();
  }

  if (left_header_view_) {
    // Vertically center the left header view (typically the window icon) with
    // respect to the caption button container.
    const gfx::Size icon_size(left_header_view_->GetPreferredSize());
    const int icon_offset_y = (GetHeaderHeight() - icon_size.height()) / 2;
    left_header_view_->SetBounds(FrameHeaderUtil::GetLeftViewXInset() + origin,
                                 icon_offset_y, icon_size.width(),
                                 icon_size.height());
  }
}

gfx::Rect FrameHeader::GetTitleBounds() const {
  views::View* left_view = left_header_view_ ? left_header_view_ : back_button_;
  return view_->GetMirroredRect(FrameHeaderUtil::GetAvailableTitleBounds(
      left_view, caption_button_container_, GetHeaderHeight()));
}

}  // namespace ash
