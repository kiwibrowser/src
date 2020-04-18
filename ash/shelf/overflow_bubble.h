// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SHELF_OVERFLOW_BUBBLE_H_
#define ASH_SHELF_OVERFLOW_BUBBLE_H_

#include "base/macros.h"
#include "ui/views/pointer_watcher.h"
#include "ui/views/widget/widget_observer.h"

namespace ui {
class PointerEvent;
}

namespace ash {
class OverflowBubbleView;
class OverflowButton;
class Shelf;
class ShelfView;

// OverflowBubble shows shelf items that won't fit on the main shelf in a
// separate bubble.
class OverflowBubble : public views::PointerWatcher,
                       public views::WidgetObserver {
 public:
  // |shelf| is the shelf that spawns the bubble.
  explicit OverflowBubble(Shelf* shelf);
  ~OverflowBubble() override;

  // Shows an bubble pointing to |overflow_button| with |shelf_view| as its
  // content.  This |shelf_view| is different than the main shelf's view and
  // only contains the overflow items.
  void Show(OverflowButton* overflow_button, ShelfView* shelf_view);

  void Hide();

  bool IsShowing() const { return !!bubble_; }
  ShelfView* shelf_view() { return shelf_view_; }
  OverflowBubbleView* bubble_view() { return bubble_; }

 private:
  void ProcessPressedEvent(const gfx::Point& event_location_in_screen);

  // views::PointerWatcher:
  void OnPointerEventObserved(const ui::PointerEvent& event,
                              const gfx::Point& location_in_screen,
                              gfx::NativeView target) override;

  // Overridden from views::WidgetObserver:
  void OnWidgetDestroying(views::Widget* widget) override;

  Shelf* shelf_;
  OverflowBubbleView* bubble_;       // Owned by views hierarchy.
  OverflowButton* overflow_button_;  // Owned by ShelfView.

  // ShelfView containing the overflow items. Owned by |bubble_|.
  ShelfView* shelf_view_;

  DISALLOW_COPY_AND_ASSIGN(OverflowBubble);
};

}  // namespace ash

#endif  // ASH_SHELF_OVERFLOW_BUBBLE_H_
