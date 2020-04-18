// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/frame/frame_header_util.h"

#include <algorithm>

#include "ui/aura/window.h"
#include "ui/compositor/layer.h"
#include "ui/compositor/layer_animator.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/view.h"
#include "ui/views/widget/native_widget_aura.h"
#include "ui/views/widget/widget.h"

namespace {

// Radius of the header's top corners when the window is restored.
const int kTopCornerRadiusWhenRestored = 2;

// Distance between left edge of the window and the leftmost view.
const int kLeftViewXInset = 9;

// Space between the title text and the caption buttons.
const int kTitleCaptionButtonSpacing = 5;

// Space between window icon and title text.
const int kTitleIconOffsetX = 5;

// Space between window edge and title text, when there is no icon.
const int kTitleNoIconOffsetX = 8;

// In the pre-Ash era the web content area had a frame along the left edge, so
// user-generated theme images for the new tab page assume they are shifted
// right relative to the header.  Now that we have removed the left edge frame
// we need to copy the theme image for the window header from a few pixels
// inset to preserve alignment with the NTP image, or else we'll break a bunch
// of existing themes.  We do something similar on OS X for the same reason.
const int kThemeFrameImageInsetX = 5;

}  // namespace

namespace ash {

// static
int FrameHeaderUtil::GetTopCornerRadiusWhenRestored() {
  return kTopCornerRadiusWhenRestored;
}

// static
int FrameHeaderUtil::GetLeftViewXInset() {
  return kLeftViewXInset;
}

// static
int FrameHeaderUtil::GetThemeBackgroundXInset() {
  return kThemeFrameImageInsetX;
}

// static
gfx::Rect FrameHeaderUtil::GetAvailableTitleBounds(
    const views::View* left_view,
    const views::View* right_view,
    int header_height) {
  const int x = left_view ? left_view->bounds().right() + kTitleIconOffsetX
                          : kTitleNoIconOffsetX;
  const int title_height =
      views::NativeWidgetAura::GetWindowTitleFontList().GetHeight();
  DCHECK_LE(right_view->height(), header_height);
  // We want to align the center points of the header and title vertically.
  // Note that we can't just do (header_height - title_height) / 2, since this
  // won't make the center points align perfectly vertically due to rounding.
  // Floor when computing the center of |header_height| and when computing the
  // center of the text.
  const int header_center_y = header_height / 2;
  const int title_center_y = title_height / 2;
  const int y = std::max(0, header_center_y - title_center_y);
  const int width =
      std::max(0, right_view->x() - kTitleCaptionButtonSpacing - x);
  return gfx::Rect(x, y, width, title_height);
}

// static
bool FrameHeaderUtil::CanAnimateActivation(views::Widget* widget) {
  // Do not animate the header if the parent (e.g.
  // kShellWindowId_DefaultContainer) is already animating. All of the
  // implementers of FrameHeader animate activation by continuously painting
  // during the animation. This gives the parent's animation a slower frame
  // rate.
  // TODO(sky): Expose a better way to determine this rather than assuming the
  // parent is a toplevel container.
  aura::Window* window = widget->GetNativeWindow();
  // TODO(sky): parent()->layer() is for mash until animations ported.
  if (!window || !window->parent() || !window->parent()->layer())
    return true;

  ui::LayerAnimator* parent_layer_animator =
      window->parent()->layer()->GetAnimator();
  return !parent_layer_animator->IsAnimatingProperty(
             ui::LayerAnimationElement::OPACITY) &&
         !parent_layer_animator->IsAnimatingProperty(
             ui::LayerAnimationElement::VISIBILITY);
}

}  // namespace ash
