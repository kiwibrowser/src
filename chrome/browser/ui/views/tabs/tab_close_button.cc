// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/tabs/tab_close_button.h"

#include <map>
#include <memory>
#include <vector>

#include "base/hash.h"
#include "base/no_destructor.h"
#include "base/stl_util.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/views/tabs/tab.h"
#include "chrome/browser/ui/views/tabs/tab_controller.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/gfx/animation/tween.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/rect_based_targeting_utils.h"

#if defined(USE_AURA)
#include "ui/aura/env.h"
#endif

using MD = ui::MaterialDesignController;

TabCloseButton::TabCloseButton(views::ButtonListener* listener,
                               MouseEventCallback mouse_event_callback)
    : views::ImageButton(listener),
      mouse_event_callback_(std::move(mouse_event_callback)) {
  SetEventTargeter(std::make_unique<views::ViewTargeter>(this));
  SetAccessibleName(l10n_util::GetStringUTF16(IDS_ACCNAME_CLOSE));
  // Disable animation so that the red danger sign shows up immediately
  // to help avoid mis-clicks.
  SetAnimationDuration(0);
}

TabCloseButton::~TabCloseButton() {}

void TabCloseButton::SetTabColor(SkColor color, bool tab_color_is_dark) {
  SkColor hover_color = SkColorSetRGB(0xDB, 0x44, 0x37);
  SkColor pressed_color = SkColorSetRGB(0xA8, 0x35, 0x2A);
  SkColor icon_color = SK_ColorWHITE;
  if (MD::IsRefreshUi()) {
    hover_color = tab_color_is_dark ? gfx::kGoogleGrey700 : gfx::kGoogleGrey200;
    pressed_color =
        tab_color_is_dark ? gfx::kGoogleGrey600 : gfx::kGoogleGrey300;
    icon_color = color;
  }
  GenerateImages(false, color, icon_color, hover_color, pressed_color);
}

void TabCloseButton::ActiveStateChanged(const Tab* parent_tab) {
  SkColor icon_color =
      parent_tab->GetCloseTabButtonColor(views::Button::STATE_NORMAL);
  GenerateImages(
      true, icon_color, icon_color,
      parent_tab->GetCloseTabButtonColor(views::Button::STATE_HOVERED),
      parent_tab->GetCloseTabButtonColor(views::Button::STATE_PRESSED));
}

views::View* TabCloseButton::GetTooltipHandlerForPoint(
    const gfx::Point& point) {
  // Tab close button has no children, so tooltip handler should be the same
  // as the event handler. In addition, a hit test has to be performed for the
  // point (as GetTooltipHandlerForPoint() is responsible for it).
  if (!HitTestPoint(point))
    return nullptr;
  return GetEventHandlerForPoint(point);
}

bool TabCloseButton::OnMousePressed(const ui::MouseEvent& event) {
  mouse_event_callback_.Run(this, event);

  bool handled = ImageButton::OnMousePressed(event);
  // Explicitly mark midle-mouse clicks as non-handled to ensure the tab
  // sees them.
  return !event.IsMiddleMouseButton() && handled;
}

void TabCloseButton::OnMouseMoved(const ui::MouseEvent& event) {
  mouse_event_callback_.Run(this, event);
  Button::OnMouseMoved(event);
}

void TabCloseButton::OnMouseReleased(const ui::MouseEvent& event) {
  mouse_event_callback_.Run(this, event);
  Button::OnMouseReleased(event);
}

void TabCloseButton::OnGestureEvent(ui::GestureEvent* event) {
  // Consume all gesture events here so that the parent (Tab) does not
  // start consuming gestures.
  ImageButton::OnGestureEvent(event);
  event->SetHandled();
}

const char* TabCloseButton::GetClassName() const {
  return "TabCloseButton";
}

void TabCloseButton::PaintButtonContents(gfx::Canvas* canvas) {
  canvas->SaveLayerAlpha(GetOpacity());
  views::ImageButton::PaintButtonContents(canvas);
  canvas->Restore();
}

views::View* TabCloseButton::TargetForRect(views::View* root,
                                           const gfx::Rect& rect) {
  CHECK_EQ(root, this);

  if (!views::UsePointBasedTargeting(rect))
    return ViewTargeterDelegate::TargetForRect(root, rect);

  // Ignore the padding set on the button.
  gfx::Rect contents_bounds = GetMirroredRect(GetContentsBounds());

#if defined(USE_AURA)
  // Include the padding in hit-test for touch events.
  // TODO(pkasting): It seems like touch events would generate rects rather
  // than points and thus use the TargetForRect() call above.  If this is
  // reached, it may be from someone calling GetEventHandlerForPoint() while a
  // touch happens to be occurring.  In such a case, maybe we don't want this
  // code to run?  It's possible this block should be removed, or maybe this
  // whole function deleted.  Note that in these cases, we should probably
  // also remove the padding on the close button bounds (see Tab::Layout()),
  // as it will be pointless.
  if (aura::Env::GetInstance()->is_touch_down())
    contents_bounds = GetLocalBounds();
#endif

  return contents_bounds.Intersects(rect) ? this : parent();
}

bool TabCloseButton::GetHitTestMask(gfx::Path* mask) const {
  // We need to define this so hit-testing won't include the border region.
  mask->addRect(gfx::RectToSkRect(GetMirroredRect(GetContentsBounds())));
  return true;
}

SkAlpha TabCloseButton::GetOpacity() {
  Tab* tab = static_cast<Tab*>(parent());
  if (!MD::IsRefreshUi() || IsMouseHovered() || tab->IsActive())
    return SK_AlphaOPAQUE;
  const double animation_value = tab->hover_controller()->GetAnimationValue();
  return gfx::Tween::IntValueBetween(animation_value, 0, 255);
}

void TabCloseButton::GenerateImages(bool is_touch,
                                    SkColor normal_icon_color,
                                    SkColor hover_pressed_icon_color,
                                    SkColor hover_highlight_color,
                                    SkColor pressed_highlight_color) {
  const gfx::VectorIcon& button_icon =
      is_touch ? kTabCloseButtonTouchIcon : kTabCloseNormalIcon;
  const gfx::VectorIcon& highlight = is_touch
                                         ? kTabCloseButtonTouchHighlightIcon
                                         : kTabCloseButtonHighlightIcon;
  const gfx::ImageSkia& normal =
      gfx::CreateVectorIcon(button_icon, normal_icon_color);
  const gfx::ImageSkia& hover_pressed =
      normal_icon_color != hover_pressed_icon_color
          ? gfx::CreateVectorIcon(button_icon, hover_pressed_icon_color)
          : normal;

  const gfx::ImageSkia& hover_highlight =
      gfx::CreateVectorIcon(highlight, hover_highlight_color);
  const gfx::ImageSkia& pressed_highlight =
      gfx::CreateVectorIcon(highlight, pressed_highlight_color);
  const gfx::ImageSkia& hover =
      gfx::ImageSkiaOperations::CreateSuperimposedImage(hover_highlight,
                                                        hover_pressed);
  const gfx::ImageSkia& pressed =
      gfx::ImageSkiaOperations::CreateSuperimposedImage(pressed_highlight,
                                                        hover_pressed);
  SetImage(views::Button::STATE_NORMAL, normal);
  SetImage(views::Button::STATE_HOVERED, hover);
  SetImage(views::Button::STATE_PRESSED, pressed);
}
