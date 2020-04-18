// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/frame/frame_border_hit_test.h"

#include "ash/frame/caption_buttons/frame_caption_button_container_view.h"
#include "ash/public/cpp/ash_constants.h"
#include "ash/shell_port.h"
#include "ui/base/hit_test.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"
#include "ui/views/window/non_client_view.h"

namespace ash {

int FrameBorderNonClientHitTest(
    views::NonClientFrameView* view,
    const views::View* back_button,
    const FrameCaptionButtonContainerView* caption_button_container,
    const gfx::Point& point_in_widget) {
  gfx::Rect expanded_bounds = view->bounds();
  int outside_bounds = kResizeOutsideBoundsSize;

  if (ShellPort::Get()->IsTouchDown())
    outside_bounds *= kResizeOutsideBoundsScaleForTouch;
  expanded_bounds.Inset(-outside_bounds, -outside_bounds);

  if (!expanded_bounds.Contains(point_in_widget))
    return HTNOWHERE;

  // Check the frame first, as we allow a small area overlapping the contents
  // to be used for resize handles.
  views::Widget* frame = view->GetWidget();
  bool can_ever_resize = frame->widget_delegate()->CanResize();
  // Don't allow overlapping resize handles when the window is maximized or
  // fullscreen, as it can't be resized in those states.
  int resize_border = kResizeInsideBoundsSize;
  if (frame->IsMaximized() || frame->IsFullscreen()) {
    resize_border = 0;
    can_ever_resize = false;
  }
  int frame_component = view->GetHTComponentForFrame(
      point_in_widget, resize_border, resize_border, kResizeAreaCornerSize,
      kResizeAreaCornerSize, can_ever_resize);
  if (frame_component != HTNOWHERE)
    return frame_component;

  int client_component =
      frame->client_view()->NonClientHitTest(point_in_widget);
  if (client_component != HTNOWHERE)
    return client_component;

  if (caption_button_container->visible()) {
    gfx::Point point_in_caption_button_container(point_in_widget);
    views::View::ConvertPointFromWidget(caption_button_container,
                                        &point_in_caption_button_container);
    int caption_button_component = caption_button_container->NonClientHitTest(
        point_in_caption_button_container);
    if (caption_button_component != HTNOWHERE)
      return caption_button_component;
  }

  if (back_button) {
    gfx::Point point_in_back_button(point_in_widget);
    views::View::ConvertPointFromWidget(back_button, &point_in_back_button);
    // Using HTMENU as a placeholder to pass through the event to button.
    if (back_button->GetLocalBounds().Contains(point_in_back_button))
      return HTMENU;
  }
  // Caption is a safe default.
  return HTCAPTION;
}

}  // namespace ash
