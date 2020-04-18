// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shelf/overflow_button.h"

#include <memory>

#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/shelf/shelf.h"
#include "ash/shelf/shelf_constants.h"
#include "ash/shelf/shelf_view.h"
#include "ash/strings/grit/ash_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/skbitmap_operations.h"
#include "ui/views/animation/flood_fill_ink_drop_ripple.h"
#include "ui/views/animation/ink_drop_impl.h"
#include "ui/views/animation/ink_drop_mask.h"

namespace ash {

OverflowButton::OverflowButton(ShelfView* shelf_view, Shelf* shelf)
    : Button(nullptr),
      upward_image_(gfx::CreateVectorIcon(kShelfOverflowIcon, kShelfIconColor)),
      chevron_image_(nullptr),
      shelf_view_(shelf_view),
      shelf_(shelf),
      background_color_(kShelfDefaultBaseColor) {
  DCHECK(shelf_view_);

  SetInkDropMode(InkDropMode::ON);
  set_ink_drop_base_color(kShelfInkDropBaseColor);
  set_ink_drop_visible_opacity(kShelfInkDropVisibleOpacity);
  set_hide_ink_drop_when_showing_context_menu(false);

  SetFocusBehavior(FocusBehavior::ACCESSIBLE_ONLY);
  SetAccessibleName(l10n_util::GetStringUTF16(IDS_ASH_SHELF_OVERFLOW_NAME));

  UpdateChevronImage();
}

OverflowButton::~OverflowButton() = default;

void OverflowButton::OnShelfAlignmentChanged() {
  UpdateChevronImage();
}

void OverflowButton::OnOverflowBubbleShown() {
  AnimateInkDrop(views::InkDropState::ACTIVATED, nullptr);
  UpdateChevronImage();
}

void OverflowButton::OnOverflowBubbleHidden() {
  AnimateInkDrop(views::InkDropState::DEACTIVATED, nullptr);
  UpdateChevronImage();
}

void OverflowButton::UpdateShelfItemBackground(SkColor color) {
  background_color_ = color;
  SchedulePaint();
}

OverflowButton::ChevronDirection OverflowButton::GetChevronDirection() const {
  switch (shelf_->alignment()) {
    case SHELF_ALIGNMENT_LEFT:
      if (shelf_view_->IsShowingOverflowBubble())
        return ChevronDirection::LEFT;
      return ChevronDirection::RIGHT;
    case SHELF_ALIGNMENT_RIGHT:
      if (shelf_view_->IsShowingOverflowBubble())
        return ChevronDirection::RIGHT;
      return ChevronDirection::LEFT;
    default:
      if (shelf_view_->IsShowingOverflowBubble())
        return ChevronDirection::DOWN;
      return ChevronDirection::UP;
  }
}

void OverflowButton::UpdateChevronImage() {
  switch (GetChevronDirection()) {
    case ChevronDirection::UP:
      chevron_image_ = &upward_image_;
      break;
    case ChevronDirection::DOWN:
      if (downward_image_.isNull()) {
        downward_image_ = gfx::ImageSkiaOperations::CreateRotatedImage(
            upward_image_, SkBitmapOperations::ROTATION_180_CW);
      }
      chevron_image_ = &downward_image_;
      break;
    case ChevronDirection::LEFT:
      if (leftward_image_.isNull()) {
        leftward_image_ = gfx::ImageSkiaOperations::CreateRotatedImage(
            upward_image_, SkBitmapOperations::ROTATION_270_CW);
      }
      chevron_image_ = &leftward_image_;
      break;
    case ChevronDirection::RIGHT:
      if (rightward_image_.isNull()) {
        rightward_image_ = gfx::ImageSkiaOperations::CreateRotatedImage(
            upward_image_, SkBitmapOperations::ROTATION_90_CW);
      }
      chevron_image_ = &rightward_image_;
      break;
  }
  SchedulePaint();
}

std::unique_ptr<views::InkDrop> OverflowButton::CreateInkDrop() {
  std::unique_ptr<views::InkDropImpl> ink_drop =
      CreateDefaultFloodFillInkDropImpl();
  ink_drop->SetShowHighlightOnHover(false);
  ink_drop->SetAutoHighlightMode(views::InkDropImpl::AutoHighlightMode::NONE);
  return std::move(ink_drop);
}

std::unique_ptr<views::InkDropRipple> OverflowButton::CreateInkDropRipple()
    const {
  gfx::Insets insets = GetLocalBounds().InsetsFrom(CalculateButtonBounds());
  return std::make_unique<views::FloodFillInkDropRipple>(
      size(), insets, GetInkDropCenterBasedOnLastEvent(), GetInkDropBaseColor(),
      ink_drop_visible_opacity());
}

bool OverflowButton::ShouldEnterPushedState(const ui::Event& event) {
  if (shelf_view_->IsShowingOverflowBubble())
    return false;

  return Button::ShouldEnterPushedState(event);
}

void OverflowButton::NotifyClick(const ui::Event& event) {
  Button::NotifyClick(event);
  shelf_view_->ButtonPressed(this, event, GetInkDrop());
}

std::unique_ptr<views::InkDropMask> OverflowButton::CreateInkDropMask() const {
  gfx::Insets insets = GetLocalBounds().InsetsFrom(CalculateButtonBounds());
  return std::make_unique<views::RoundRectInkDropMask>(
      size(), insets, kOverflowButtonCornerRadius);
}

void OverflowButton::PaintButtonContents(gfx::Canvas* canvas) {
  gfx::Rect bounds = CalculateButtonBounds();
  PaintBackground(canvas, bounds);
  PaintForeground(canvas, bounds);
}

void OverflowButton::PaintBackground(gfx::Canvas* canvas,
                                     const gfx::Rect& bounds) {
  cc::PaintFlags flags;
  flags.setAntiAlias(true);
  flags.setColor(background_color_);
  canvas->DrawRoundRect(bounds, kOverflowButtonCornerRadius, flags);
}

void OverflowButton::PaintForeground(gfx::Canvas* canvas,
                                     const gfx::Rect& bounds) {
  DCHECK(chevron_image_);
  canvas->DrawImageInt(
      *chevron_image_,
      bounds.x() + ((bounds.width() - chevron_image_->width()) / 2),
      bounds.y() + ((bounds.height() - chevron_image_->height()) / 2));
}

gfx::Rect OverflowButton::CalculateButtonBounds() const {
  ShelfAlignment alignment = shelf_->alignment();
  gfx::Rect content_bounds = GetContentsBounds();
  // Align the button to the top of a bottom-aligned shelf, to the right edge
  // a left-aligned shelf, and to the left edge of a right-aligned shelf.
  const int inset = (kShelfSize - kOverflowButtonSize) / 2;
  const int x = alignment == SHELF_ALIGNMENT_LEFT
                    ? content_bounds.right() - inset - kOverflowButtonSize
                    : content_bounds.x() + inset;
  return gfx::Rect(x, content_bounds.y() + inset, kOverflowButtonSize,
                   kOverflowButtonSize);
}

}  // namespace ash
