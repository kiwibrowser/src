// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SHELF_OVERFLOW_BUTTON_H_
#define ASH_SHELF_OVERFLOW_BUTTON_H_

#include "ash/ash_export.h"
#include "base/macros.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/controls/button/button.h"

namespace ash {

class Shelf;
class ShelfView;

// Shelf overflow chevron button.
class ASH_EXPORT OverflowButton : public views::Button {
 public:
  // |shelf_view| is the view containing this button.
  OverflowButton(ShelfView* shelf_view, Shelf* shelf);
  ~OverflowButton() override;

  void OnShelfAlignmentChanged();
  void OnOverflowBubbleShown();
  void OnOverflowBubbleHidden();

  // Updates background and schedules a paint.
  void UpdateShelfItemBackground(SkColor color);

 private:
  friend class OverflowButtonTestApi;

  enum class ChevronDirection { UP, DOWN, LEFT, RIGHT };

  // Returns the direction of chevron image based on the shelf alignment and
  // overflow state.
  ChevronDirection GetChevronDirection() const;

  // Updates the chevron image according to GetChevronDirection().
  void UpdateChevronImage();

  // views::Button:
  std::unique_ptr<views::InkDrop> CreateInkDrop() override;
  std::unique_ptr<views::InkDropRipple> CreateInkDropRipple() const override;
  bool ShouldEnterPushedState(const ui::Event& event) override;
  void NotifyClick(const ui::Event& event) override;
  std::unique_ptr<views::InkDropMask> CreateInkDropMask() const override;
  void PaintButtonContents(gfx::Canvas* canvas) override;

  // Helper functions to paint the background and foreground of the button
  // at |bounds|.
  void PaintBackground(gfx::Canvas* canvas, const gfx::Rect& bounds);
  void PaintForeground(gfx::Canvas* canvas, const gfx::Rect& bounds);

  // Calculates the bounds of the overflow button based on the shelf alignment
  // and overflow shelf visibility.
  gfx::Rect CalculateButtonBounds() const;

  // The original upward chevron image.
  const gfx::ImageSkia upward_image_;

  // Cached rotations of |upward_image_|.
  gfx::ImageSkia downward_image_;
  gfx::ImageSkia leftward_image_;
  gfx::ImageSkia rightward_image_;

  // Current chevron image which is a pointer to one of the above images
  // according to current shelf alignment and overflow shelf visibility.
  const gfx::ImageSkia* chevron_image_;

  ShelfView* shelf_view_;
  Shelf* shelf_;

  // Color used to paint the background.
  SkColor background_color_;

  DISALLOW_COPY_AND_ASSIGN(OverflowButton);
};

}  // namespace ash

#endif  // ASH_SHELF_OVERFLOW_BUTTON_H_
