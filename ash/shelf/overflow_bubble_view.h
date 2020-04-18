// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SHELF_OVERFLOW_BUBBLE_VIEW_H_
#define ASH_SHELF_OVERFLOW_BUBBLE_VIEW_H_

#include "ash/ash_export.h"
#include "ash/shelf/shelf_background_animator.h"
#include "ash/shelf/shelf_background_animator_observer.h"
#include "base/macros.h"
#include "ui/views/bubble/bubble_dialog_delegate.h"

namespace views {
class View;
}

namespace ash {
class Shelf;

// OverflowBubbleView hosts a ShelfView to display overflown items.
// Exports to access this class from OverflowBubbleViewTestAPI.
class ASH_EXPORT OverflowBubbleView : public views::BubbleDialogDelegateView,
                                      public ShelfBackgroundAnimatorObserver {
 public:
  explicit OverflowBubbleView(Shelf* shelf);
  ~OverflowBubbleView() override;

  // |anchor| is the overflow button on the main shelf. |shelf_view| is the
  // ShelfView containing the overflow items.
  void InitOverflowBubble(views::View* anchor, views::View* shelf_view);

  // views::BubbleDialogDelegateView overrides:
  int GetDialogButtons() const override;
  gfx::Rect GetBubbleBounds() override;

 private:
  friend class OverflowBubbleViewTestAPI;

  void ScrollByXOffset(int x_offset);
  void ScrollByYOffset(int y_offset);

  // views::View overrides:
  gfx::Size CalculatePreferredSize() const override;
  void Layout() override;
  void ChildPreferredSizeChanged(views::View* child) override;
  bool OnMouseWheel(const ui::MouseWheelEvent& event) override;

  // ui::EventHandler overrides:
  void OnScrollEvent(ui::ScrollEvent* event) override;

  // ShelfBackgroundAnimatorObserver:
  void UpdateShelfBackground(SkColor color) override;

  Shelf* shelf_;
  views::View* shelf_view_;  // Owned by views hierarchy.
  gfx::Vector2d scroll_offset_;

  ShelfBackgroundAnimator background_animator_;

  DISALLOW_COPY_AND_ASSIGN(OverflowBubbleView);
};

}  // namespace ash

#endif  // ASH_SHELF_OVERFLOW_BUBBLE_VIEW_H_
