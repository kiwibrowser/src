// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/power/power_button_menu_item_view.h"

#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/font.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/accessibility/view_accessibility.h"
#include "ui/views/border.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"

namespace ash {

namespace {

// Color of the image icon.
constexpr SkColor kItemIconColor = SkColorSetARGB(0xFF, 0x20, 0x21, 0x24);

// Color of the title of the label.
constexpr SkColor kItemTitleColor = SkColorSetARGB(0xFF, 0x5F, 0x63, 0x68);

// Size of the image icon in pixels.
constexpr int kIconSize = 24;

// Top padding of the image icon to the top of the item view.
constexpr int kIconTopPadding = 17;

// Top padding of the label of title to the top of the item view.
constexpr int kTitleTopPadding = 52;

// The amount of rounding applied to the corners of the focused menu item.
constexpr int kFocusedItemRoundRectRadiusDp = 8;

// Color of the focused menu item's border.
constexpr SkColor kFocusedItemBorderColor =
    SkColorSetARGB(0x66, 0x1A, 0x73, 0xE8);

// Color of the focused menu item.
constexpr SkColor kFocusedItemColor = SkColorSetARGB(0x0A, 0x1A, 0x73, 0xE8);

}  // namespace

PowerButtonMenuItemView::PowerButtonMenuItemView(
    views::ButtonListener* listener,
    const gfx::VectorIcon& icon,
    const base::string16& title_text)
    : views::ImageButton(listener),
      icon_view_(new views::ImageView),
      title_(new views::Label) {
  SetFocusBehavior(FocusBehavior::ALWAYS);
  SetFocusPainter(nullptr);
  SetPaintToLayer();
  icon_view_->SetImage(gfx::CreateVectorIcon(icon, kItemIconColor));
  AddChildView(icon_view_);

  title_->SetBackgroundColor(SK_ColorTRANSPARENT);
  title_->SetAutoColorReadabilityEnabled(false);
  title_->SetEnabledColor(kItemTitleColor);
  title_->SetText(title_text);
  AddChildView(title_);
  GetViewAccessibility().OverrideRole(ax::mojom::Role::kMenuItem);
  GetViewAccessibility().OverrideName(title_->text());

  SetBorder(views::CreateEmptyBorder(kItemBorderThickness, kItemBorderThickness,
                                     kItemBorderThickness,
                                     kItemBorderThickness));
}

PowerButtonMenuItemView::~PowerButtonMenuItemView() = default;

void PowerButtonMenuItemView::Layout() {
  const gfx::Rect rect(GetContentsBounds());

  gfx::Rect icon_rect(rect);
  icon_rect.ClampToCenteredSize(gfx::Size(kIconSize, kIconSize));
  icon_rect.set_y(kIconTopPadding);
  icon_view_->SetBoundsRect(icon_rect);

  gfx::Rect title_rect(rect);
  title_rect.ClampToCenteredSize(
      gfx::Size(kMenuItemWidth, title_->font_list().GetHeight()));
  title_rect.set_y(kTitleTopPadding);
  title_->SetBoundsRect(title_rect);
}

gfx::Size PowerButtonMenuItemView::CalculatePreferredSize() const {
  return gfx::Size(kMenuItemWidth + 2 * kItemBorderThickness,
                   kMenuItemHeight + 2 * kItemBorderThickness);
}

void PowerButtonMenuItemView::OnFocus() {
  parent()->SetFocusBehavior(FocusBehavior::NEVER);
  NotifyAccessibilityEvent(ax::mojom::Event::kSelection, true);
  SchedulePaint();
}

void PowerButtonMenuItemView::OnBlur() {
  SchedulePaint();
}

void PowerButtonMenuItemView::PaintButtonContents(gfx::Canvas* canvas) {
  views::View::OnPaint(canvas);
  canvas->DrawColor(SK_ColorWHITE);
  if (!HasFocus() || GetContentsBounds().IsEmpty())
    return;

  SetBorder(views::CreateRoundedRectBorder(kItemBorderThickness,
                                           kFocusedItemRoundRectRadiusDp,
                                           kFocusedItemBorderColor));
  canvas->FillRect(GetContentsBounds(), kFocusedItemColor);
  views::View::OnPaintBorder(canvas);
}

}  // namespace ash
