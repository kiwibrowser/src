// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/app_list/views/suggestion_chip_view.h"

#include "ui/gfx/canvas.h"
#include "ui/views/background.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"

namespace app_list {

namespace {

// Colors.
constexpr SkColor kBackgroundColor = SK_ColorWHITE;
constexpr SkColor kStrokeColor = SkColorSetRGB(0xDA, 0xDC, 0xE0);  // G Grey 300
constexpr SkColor kTextColor = SkColorSetRGB(0x3C, 0x40, 0x43);    // G Grey 800

// Dimensions.
constexpr int kIconMarginDip = 8;
constexpr int kPaddingDip = 16;
constexpr int kPreferredHeightDip = 32;
constexpr int kStrokeWidthDip = 1;

}  // namespace

// Params ----------------------------------------------------------------------

SuggestionChipView::Params::Params() = default;

SuggestionChipView::Params::~Params() = default;

// SuggestionChipView ----------------------------------------------------------

SuggestionChipView::SuggestionChipView(const Params& params,
                                       SuggestionChipListener* listener)
    : icon_view_(new views::ImageView()),
      text_view_(new views::Label()),
      listener_(listener) {
  InitLayout(params);
}

SuggestionChipView::~SuggestionChipView() = default;

gfx::Size SuggestionChipView::CalculatePreferredSize() const {
  const int preferred_width = views::View::CalculatePreferredSize().width();
  return gfx::Size(preferred_width, GetHeightForWidth(preferred_width));
}

int SuggestionChipView::GetHeightForWidth(int width) const {
  return kPreferredHeightDip;
}

void SuggestionChipView::ChildVisibilityChanged(views::View* child) {
  // When icon visibility is modified we need to update layout padding.
  if (child == icon_view_) {
    const int padding_left_dip =
        icon_view_->visible() ? kIconMarginDip : kPaddingDip;
    layout_manager_->set_inside_border_insets(
        gfx::Insets(0, padding_left_dip, 0, kPaddingDip));
  }
  PreferredSizeChanged();
}

void SuggestionChipView::InitLayout(const Params& params) {
  // Layout padding differs depending on icon visibility.
  const int padding_left_dip = params.icon ? kIconMarginDip : kPaddingDip;

  layout_manager_ = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets(0, padding_left_dip, 0, kPaddingDip), kIconMarginDip));

  layout_manager_->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::CROSS_AXIS_ALIGNMENT_CENTER);

  // Icon.
  icon_view_->SetImageSize(gfx::Size(kIconSizeDip, kIconSizeDip));
  icon_view_->SetPreferredSize(gfx::Size(kIconSizeDip, kIconSizeDip));

  if (params.icon)
    icon_view_->SetImage(params.icon.value());
  else
    icon_view_->SetVisible(false);

  AddChildView(icon_view_);

  // Text.
  text_view_->SetAutoColorReadabilityEnabled(false);
  text_view_->SetEnabledColor(kTextColor);
  text_view_->SetFontList(text_view_->font_list().DeriveWithSizeDelta(2));
  text_view_->SetText(params.text);
  AddChildView(text_view_);
}

void SuggestionChipView::OnPaintBackground(gfx::Canvas* canvas) {
  cc::PaintFlags flags;
  flags.setAntiAlias(true);

  gfx::Rect bounds = GetContentsBounds();

  // Background.
  flags.setColor(kBackgroundColor);
  canvas->DrawRoundRect(bounds, height() / 2, flags);

  // Stroke should be drawn within our contents bounds.
  bounds.Inset(gfx::Insets(kStrokeWidthDip));

  // Stroke.
  flags.setColor(kStrokeColor);
  flags.setStrokeWidth(kStrokeWidthDip);
  flags.setStyle(cc::PaintFlags::Style::kStroke_Style);
  canvas->DrawRoundRect(bounds, height() / 2, flags);
}

void SuggestionChipView::OnGestureEvent(ui::GestureEvent* event) {
  if (event->type() == ui::ET_GESTURE_TAP) {
    if (listener_)
      listener_->OnSuggestionChipPressed(this);
    event->SetHandled();
  }
}

bool SuggestionChipView::OnMousePressed(const ui::MouseEvent& event) {
  if (listener_)
    listener_->OnSuggestionChipPressed(this);
  return true;
}

void SuggestionChipView::SetIcon(const gfx::ImageSkia& icon) {
  icon_view_->SetImage(icon);
  icon_view_->SetVisible(true);
}

const base::string16& SuggestionChipView::GetText() const {
  return text_view_->text();
}

}  // namespace app_list
