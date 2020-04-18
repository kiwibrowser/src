// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_TOOLBAR_TOOLBAR_INK_DROP_UTIL_H_
#define CHROME_BROWSER_UI_VIEWS_TOOLBAR_TOOLBAR_INK_DROP_UTIL_H_

#include <memory>

#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/layout_constants.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/theme_provider.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/animation/flood_fill_ink_drop_ripple.h"
#include "ui/views/animation/ink_drop_highlight.h"
#include "ui/views/animation/ink_drop_impl.h"
#include "ui/views/animation/ink_drop_mask.h"
#include "ui/views/animation/ink_drop_ripple.h"

constexpr float kTouchToolbarInkDropVisibleOpacity = 0.06f;
constexpr float kTouchToolbarHighlightVisibleOpacity = 0.08f;

// The below utility functions are templated since we have two different types
// of buttons on the toolbar (ToolbarButton and AppMenuButton) which don't share
// the same base classes (ImageButton and MenuButton respectively), and these
// functions need to call into the base classes' default implementations when
// needed.
// TODO: Consider making ToolbarButton and AppMenuButton share a common base
// class https://crbug.com/819854.

// Creates insets for a host view so that when insetting from the host view
// the resulting mask or inkdrop has the desired inkdrop size.
template <class BaseInkDropHostView>
gfx::Insets GetInkDropInsets(BaseInkDropHostView* host_view) {
  gfx::Insets inkdrop_insets;
  const gfx::Insets host_insets = host_view->GetInsets();
  // If content is not centered (leftmost or rightmost toolbar button), inset
  // the inkdrop mask accordingly.
  if (host_insets.left() > host_insets.right()) {
    inkdrop_insets +=
        gfx::Insets(0, host_insets.left() - host_insets.right(), 0, 0);
  } else if (host_insets.right() > host_insets.left()) {
    inkdrop_insets +=
        gfx::Insets(0, 0, 0, host_insets.right() - host_insets.left());
  }

  // Inset the inkdrop insets so that the end result matches the target inkdrop
  // dimensions.
  const gfx::Size host_size = host_view->size();
  const int inkdrop_dimensions = GetLayoutConstant(LOCATION_BAR_HEIGHT);
  inkdrop_insets += gfx::Insets((host_size.height() - inkdrop_dimensions) / 2);

  return inkdrop_insets;
}

// Creates the appropriate ink drop for the calling button. When the touch-
// optimized UI is not enabled, it uses the default implementation of the
// calling button's base class (the template argument BaseInkDropHostView).
// Otherwise, it uses an ink drop that shows a highlight on hover that is kept
// and combined with the ripple when the ripple is shown.
template <class BaseInkDropHostView>
std::unique_ptr<views::InkDrop> CreateToolbarInkDrop(
    BaseInkDropHostView* host_view) {
  if (!ui::MaterialDesignController::IsNewerMaterialUi())
    return host_view->BaseInkDropHostView::CreateInkDrop();

  auto ink_drop =
      std::make_unique<views::InkDropImpl>(host_view, host_view->size());
  ink_drop->SetAutoHighlightMode(
      views::InkDropImpl::AutoHighlightMode::SHOW_ON_RIPPLE);
  ink_drop->SetShowHighlightOnHover(true);
  return ink_drop;
}

// Creates the appropriate ink drop ripple for the calling button. When the
// touch-optimized UI is not enabled, it uses the default implementation of the
// calling button's base class (the template argument BaseInkDropHostView).
// Otherwise, it uses a |FloodFillInkDropRipple|.
template <class BaseInkDropHostView>
std::unique_ptr<views::InkDropRipple> CreateToolbarInkDropRipple(
    const BaseInkDropHostView* host_view,
    const gfx::Point& center_point) {
  if (!ui::MaterialDesignController::IsNewerMaterialUi())
    return host_view->BaseInkDropHostView::CreateInkDropRipple();

  return std::make_unique<views::FloodFillInkDropRipple>(
      host_view->size(), GetInkDropInsets(host_view), center_point,
      host_view->GetInkDropBaseColor(), host_view->ink_drop_visible_opacity());
}

// Creates the appropriate ink drop highlight for the calling button. When the
// touch-optimized UI is not enabled, it uses the default implementation of the
// calling button's base class (the template argument BaseInkDropHostView).
// Otherwise, it uses a kTouchInkDropHighlightSize circular highlight.
template <class BaseInkDropHostView>
std::unique_ptr<views::InkDropHighlight> CreateToolbarInkDropHighlight(
    const BaseInkDropHostView* host_view,
    const gfx::Point& center_point) {
  if (!ui::MaterialDesignController::IsNewerMaterialUi())
    return host_view->BaseInkDropHostView::CreateInkDropHighlight();

  const int highlight_dimensions = GetLayoutConstant(LOCATION_BAR_HEIGHT);
  const gfx::Size highlight_size(highlight_dimensions, highlight_dimensions);

  auto highlight = std::make_unique<views::InkDropHighlight>(
      highlight_size, host_view->ink_drop_large_corner_radius(),
      gfx::PointF(center_point), host_view->GetInkDropBaseColor());
  highlight->set_visible_opacity(kTouchToolbarHighlightVisibleOpacity);
  return highlight;
}

// Creates the appropriate ink drop mask for the calling button. When the
// touch-optimized UI is not enabled, it uses the default implementation of the
// calling button's base class (the template argument BaseInkDropHostView).
// Otherwise, it uses a circular mask that has the same size as that of the
// highlight, which is needed to make the flood
// fill ripple fill a circle rather than a default square shape.
template <class BaseInkDropHostView>
std::unique_ptr<views::InkDropMask> CreateToolbarInkDropMask(
    const BaseInkDropHostView* host_view) {
  if (!ui::MaterialDesignController::IsNewerMaterialUi())
    return host_view->BaseInkDropHostView::CreateInkDropMask();

  return std::make_unique<views::RoundRectInkDropMask>(
      host_view->size(), GetInkDropInsets(host_view),
      host_view->ink_drop_large_corner_radius());
}

// Returns the ink drop base color that should be used by all toolbar buttons.
SkColor GetToolbarInkDropBaseColor(const views::View* host_view);

#endif  // CHROME_BROWSER_UI_VIEWS_TOOLBAR_TOOLBAR_INK_DROP_UTIL_H_
