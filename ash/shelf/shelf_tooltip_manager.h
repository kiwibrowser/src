// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SHELF_SHELF_TOOLTIP_MANAGER_H_
#define ASH_SHELF_SHELF_TOOLTIP_MANAGER_H_

#include "ash/ash_export.h"
#include "ash/shelf/shelf_observer.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "ui/events/event_handler.h"
#include "ui/views/pointer_watcher.h"

namespace views {
class BubbleDialogDelegateView;
class View;
}

namespace ash {
class ShelfView;

// ShelfTooltipManager manages the tooltip bubble that appears for shelf items.
class ASH_EXPORT ShelfTooltipManager : public ui::EventHandler,
                                       public views::PointerWatcher,
                                       public ShelfObserver {
 public:
  explicit ShelfTooltipManager(ShelfView* shelf_view);
  ~ShelfTooltipManager() override;

  // Initializes the tooltip manager once the shelf is shown.
  void Init();

  // Closes the tooltip.
  void Close();

  // Returns true if the tooltip is currently visible.
  bool IsVisible() const;

  // Returns the view to which the tooltip bubble is anchored. May be null.
  views::View* GetCurrentAnchorView() const;

  // Show the tooltip bubble for the specified view.
  void ShowTooltip(views::View* view);
  void ShowTooltipWithDelay(views::View* view);

  // Set the timer delay in ms for testing.
  void set_timer_delay_for_test(int timer_delay) { timer_delay_ = timer_delay; }

 protected:
  // ui::EventHandler overrides:
  void OnMouseEvent(ui::MouseEvent* event) override;

  // views::PointerWatcher overrides:
  void OnPointerEventObserved(const ui::PointerEvent& event,
                              const gfx::Point& location_in_screen,
                              gfx::NativeView target) override;

  // ShelfObserver overrides:
  void WillChangeVisibilityState(ShelfVisibilityState new_state) override;
  void OnAutoHideStateChanged(ShelfAutoHideState new_state) override;

 private:
  friend class ShelfViewTest;
  friend class ShelfTooltipManagerTest;

  // A helper function to check for shelf visibility and view validity.
  bool ShouldShowTooltipForView(views::View* view);

  int timer_delay_;
  base::OneShotTimer timer_;

  ShelfView* shelf_view_;
  views::BubbleDialogDelegateView* bubble_;

  base::WeakPtrFactory<ShelfTooltipManager> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ShelfTooltipManager);
};

}  // namespace ash

#endif  // ASH_SHELF_SHELF_TOOLTIP_MANAGER_H_
