// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/splitview/split_view_highlight_view.h"

#include "ash/wm/overview/rounded_rect_view.h"
#include "ash/wm/splitview/split_view_utils.h"
#include "ui/gfx/canvas.h"
#include "ui/views/view.h"

namespace ash {

namespace {

// The amount of round applied to the corners of the highlight views.
constexpr int kHighlightScreenRoundRectRadiusDp = 4;

constexpr int kRoundRectPaddingDp = 10;

gfx::Transform CalculateTransformFromRects(const gfx::Rect& src,
                                           const gfx::Rect& dst,
                                           bool landscape) {
  // In portrait, rtl will have no effect on this view.
  const bool is_rtl = base::i18n::IsRTL() && landscape;
  const bool should_scale =
      src.width() != dst.width() || src.height() != dst.height();

  // Add a translatation. In rtl, translate in the opposite direction to account
  // for the flip.
  gfx::Transform transform;
  transform.Translate(is_rtl && !should_scale ? src.origin() - dst.origin()
                                              : dst.origin() - src.origin());
  if (should_scale) {
    // In rtl a extra translation needs to be added to account for the flipped
    // scaling.
    if (is_rtl) {
      int x_translation = 0;
      if ((src.x() > dst.x() && src.width() < dst.width()) ||
          (src.x() == dst.x() && src.width() > dst.width())) {
        x_translation = std::abs(dst.width() - src.width());
      } else {
        x_translation = -std::abs(dst.width() - src.width());
      }
      transform.Translate(gfx::Vector2d(x_translation, 0));
    }
    transform.Scale(
        static_cast<float>(dst.width()) / static_cast<float>(src.width()),
        static_cast<float>(dst.height()) / static_cast<float>(src.height()));
  }
  return transform;
}

}  // namespace

SplitViewHighlightView::SplitViewHighlightView(bool is_right_or_bottom)
    : is_right_or_bottom_(is_right_or_bottom) {
  left_top_ =
      new RoundedRectView(kHighlightScreenRoundRectRadiusDp, SK_ColorWHITE);
  right_bottom_ =
      new RoundedRectView(kHighlightScreenRoundRectRadiusDp, SK_ColorWHITE);

  left_top_->SetPaintToLayer();
  left_top_->layer()->SetFillsBoundsOpaquely(false);
  right_bottom_->SetPaintToLayer();
  right_bottom_->layer()->SetFillsBoundsOpaquely(false);

  middle_ = new views::View();
  middle_->SetPaintToLayer(ui::LAYER_SOLID_COLOR);
  middle_->layer()->SetColor(SK_ColorWHITE);

  AddChildView(left_top_);
  AddChildView(right_bottom_);
  AddChildView(middle_);
}

SplitViewHighlightView::~SplitViewHighlightView() = default;

void SplitViewHighlightView::SetBounds(const gfx::Rect& bounds,
                                       bool landscape,
                                       bool animate) {
  if (bounds == this->bounds() && landscape == landscape_)
    return;

  landscape_ = landscape;

  const gfx::Rect old_bounds = this->bounds();
  const gfx::Vector2d offset = old_bounds.origin() - bounds.origin();
  SetBoundsRect(bounds);
  // Shift the bounds of the right bottom view if needed before applying the
  // transform.
  const bool slides_from_right = base::i18n::IsRTL() && landscape
                                     ? !is_right_or_bottom_
                                     : is_right_or_bottom_;
  if (slides_from_right && animate && !offset.IsZero()) {
    gfx::Rect old_left_top_bounds = left_top_->bounds();
    gfx::Rect old_right_middle_bounds = right_bottom_->bounds();
    gfx::Rect old_middle_bounds = middle_->bounds();

    old_left_top_bounds.Offset(offset);
    old_right_middle_bounds.Offset(offset);
    old_middle_bounds.Offset(offset);

    left_top_->SetBoundsRect(old_left_top_bounds);
    right_bottom_->SetBoundsRect(old_right_middle_bounds);
    middle_->SetBoundsRect(old_middle_bounds);
  }

  // Calculate the new bounds. The middle should take as much space as possible,
  // and the other two should take just enough space so they can display rounded
  // corners.
  gfx::Rect left_top_bounds, right_bottom_bounds;
  gfx::Rect middle_bounds = bounds;

  // The thickness of the two outer views should be the amount of rounding, plus
  // a little padding. There will be some overlap to simply the code (we use a
  // rectangle that is rounded on all sides, but cover half the sides instead of
  // creating a new class that is only rounded on half the sides).
  const int thickness = kHighlightScreenRoundRectRadiusDp + kRoundRectPaddingDp;
  if (landscape) {
    left_top_bounds = gfx::Rect(0, 0, thickness, bounds.height());
    right_bottom_bounds = left_top_bounds;
    right_bottom_bounds.Offset(bounds.width() - thickness, 0);
    middle_bounds.Offset(-bounds.x(), -bounds.y());
    middle_bounds.Inset(kHighlightScreenRoundRectRadiusDp, 0);
  } else {
    left_top_bounds = gfx::Rect(0, 0, bounds.width(), thickness);
    right_bottom_bounds = left_top_bounds;
    right_bottom_bounds.Offset(0, bounds.height() - thickness);
    middle_bounds.Offset(-bounds.x(), -bounds.y());
    middle_bounds.Inset(0, kHighlightScreenRoundRectRadiusDp);
  }

  left_top_bounds = GetMirroredRect(left_top_bounds);
  right_bottom_bounds = GetMirroredRect(right_bottom_bounds);
  middle_bounds = GetMirroredRect(middle_bounds);

  // If |animate|, calculate the needed transform from old bounds to new bounds
  // and apply it. Otherwise set the new bounds and reset the transforms on all
  // items.
  if (animate) {
    DoSplitviewTransformAnimation(
        middle_->layer(), SPLITVIEW_ANIMATION_PREVIEW_AREA_SLIDE_IN_OUT,
        CalculateTransformFromRects(middle_->bounds(), middle_bounds,
                                    landscape),
        nullptr);
    DoSplitviewTransformAnimation(
        left_top_->layer(), SPLITVIEW_ANIMATION_PREVIEW_AREA_SLIDE_IN_OUT,
        CalculateTransformFromRects(left_top_->bounds(), left_top_bounds,
                                    landscape),
        nullptr);
    DoSplitviewTransformAnimation(
        right_bottom_->layer(), SPLITVIEW_ANIMATION_PREVIEW_AREA_SLIDE_IN_OUT,
        CalculateTransformFromRects(right_bottom_->bounds(),
                                    right_bottom_bounds, landscape),
        nullptr);
  } else {
    left_top_->layer()->SetTransform(gfx::Transform());
    right_bottom_->layer()->SetTransform(gfx::Transform());
    middle_->layer()->SetTransform(gfx::Transform());

    left_top_->SetBoundsRect(left_top_bounds);
    right_bottom_->SetBoundsRect(right_bottom_bounds);
    middle_->SetBoundsRect(middle_bounds);
  }
}

void SplitViewHighlightView::SetColor(SkColor color) {
  left_top_->SetBackgroundColor(color);
  right_bottom_->SetBackgroundColor(color);
  middle_->layer()->SetColor(color);
}

}  // namespace ash