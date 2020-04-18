// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_OVERVIEW_WINDOW_GRID_H_
#define ASH_WM_OVERVIEW_WINDOW_GRID_H_

#include <stddef.h>

#include <memory>
#include <set>
#include <vector>

#include "ash/wm/overview/window_selector.h"
#include "ash/wm/window_state_observer.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observer.h"
#include "ui/aura/window_observer.h"
#include "ui/gfx/geometry/rect.h"

namespace ui {
class Shadow;
}

namespace views {
class Widget;
}

namespace ash {

class OverviewWindowAnimationObserver;
class WindowSelectorItem;

// Represents a grid of windows in the Overview Mode in a particular root
// window, and manages a selection widget that can be moved with the arrow keys.
// The idea behind the movement strategy is that it should be possible to access
// any window pressing a given arrow key repeatedly.
// +-------+  +-------+  +-------+
// |   0   |  |   1   |  |   2   |
// +-------+  +-------+  +-------+
// +-------+  +-------+  +-------+
// |   3   |  |   4   |  |   5   |
// +-------+  +-------+  +-------+
// +-------+
// |   6   |
// +-------+
// Example sequences:
//  - Going right to left
//    0, 1, 2, 3, 4, 5, 6
// The selector is switched to the next window grid (if available) or wrapped if
// it reaches the end of its movement sequence.
class ASH_EXPORT WindowGrid : public aura::WindowObserver,
                              public wm::WindowStateObserver {
 public:
  WindowGrid(aura::Window* root_window,
             const std::vector<aura::Window*>& window_list,
             WindowSelector* window_selector,
             const gfx::Rect& bounds_in_screen);
  ~WindowGrid() override;

  // Exits overview mode, fading out the |shield_widget_| if necessary.
  void Shutdown();

  // Prepares the windows in this grid for overview. This will restore all
  // minimized windows and ensure they are visible.
  void PrepareForOverview();

  // Positions all the windows in rows of equal height scaling each window to
  // fit that height.
  // Layout is done in 2 stages maintaining fixed MRU ordering.
  // 1. Optimal height is determined. In this stage |height| is bisected to find
  //    maximum height which still allows all the windows to fit.
  // 2. Row widths are balanced. In this stage the available width is reduced
  //    until some windows are no longer fitting or until the difference between
  //    the narrowest and the widest rows starts growing.
  // Overall this achieves the goals of maximum size for previews (or maximum
  // row height which is equivalent assuming fixed height), balanced rows and
  // minimal wasted space.
  // Optionally animates the windows to their targets when |animate| is true.
  // If |ignored_item| is not null and is an item in |window_list_|, that item
  // is not positioned. This is for split screen.
  void PositionWindows(bool animate,
                       WindowSelectorItem* ignored_item = nullptr);

  // Updates |selected_index_| according to the specified |direction| and calls
  // MoveSelectionWidget(). Returns |true| if the new selection index is out of
  // this window grid bounds.
  bool Move(WindowSelector::Direction direction, bool animate);

  // Returns the target selected window, or NULL if there is none selected.
  WindowSelectorItem* SelectedWindow() const;

  // Returns the WindowSelectorItem if a window is contained in any of the
  // WindowSelectorItems this grid owns. Returns nullptr if no such a
  // WindowSelectorItem exist.
  WindowSelectorItem* GetWindowSelectorItemContaining(
      const aura::Window* window) const;

  // Adds |window| to the grid. Intended to be used by split view. |window|
  // cannot already be on the grid.
  void AddItem(aura::Window* window);

  // Removes |selector_item| from the grid.
  void RemoveItem(WindowSelectorItem* selector_item);

  // Dims the items whose titles do not contain |pattern| and prevents their
  // selection. The pattern has its accents removed and is converted to
  // lowercase in a l10n sensitive context.
  // If |pattern| is empty, no item is dimmed.
  void FilterItems(const base::string16& pattern);

  // Called when |window| is about to get closed. If the |window| is currently
  // selected the implementation fades out |selection_widget_| to transparent
  // opacity, effectively hiding the selector widget.
  void WindowClosing(WindowSelectorItem* window);

  // Sets bounds for the window grid and positions all windows in the grid.
  void SetBoundsAndUpdatePositions(const gfx::Rect& bounds_in_screen);
  void SetBoundsAndUpdatePositionsIgnoringWindow(
      const gfx::Rect& bounds,
      WindowSelectorItem* ignored_item);

  // Shows or hides the selection widget. To be called by a window selector item
  // when it is dragged.
  void SetSelectionWidgetVisibility(bool visible);

  void ShowNoRecentsWindowMessage(bool visible);

  void UpdateCannotSnapWarningVisibility();

  // Called when any WindowSelectorItem on any WindowGrid has started/ended
  // being dragged.
  void OnSelectorItemDragStarted(WindowSelectorItem* item);
  void OnSelectorItemDragEnded();

  // Returns true if the grid has no more windows.
  bool empty() const { return window_list_.empty(); }

  // Returns how many window selector items are in the grid.
  size_t size() const { return window_list_.size(); }

  // Returns true if the selection widget is active.
  bool is_selecting() const { return selection_widget_ != nullptr; }

  // Returns the root window in which the grid displays the windows.
  const aura::Window* root_window() const { return root_window_; }

  const std::vector<std::unique_ptr<WindowSelectorItem>>& window_list() const {
    return window_list_;
  }

  // aura::WindowObserver:
  void OnWindowDestroying(aura::Window* window) override;
  // TODO(flackr): Handle window bounds changed in WindowSelectorItem.
  void OnWindowBoundsChanged(aura::Window* window,
                             const gfx::Rect& old_bounds,
                             const gfx::Rect& new_bounds,
                             ui::PropertyChangeReason reason) override;

  // wm::WindowStateObserver:
  void OnPostWindowStateTypeChange(wm::WindowState* window_state,
                                   mojom::WindowStateType old_type) override;

  bool IsNoItemsIndicatorLabelVisibleForTesting();

  gfx::Rect GetNoItemsIndicatorLabelBoundsForTesting() const;

  WindowSelector* window_selector() { return window_selector_; }

  void set_window_animation_observer(
      base::WeakPtr<OverviewWindowAnimationObserver> observer) {
    window_animation_observer_ = observer;
  }
  base::WeakPtr<OverviewWindowAnimationObserver> window_animation_observer() {
    return window_animation_observer_;
  }

  // Sets |should_animate_when_entering_| and |should_animate_when_exiting_|
  // of the selector items of the windows based on where the first MRU window
  // covering the available workspace is found. Also sets the
  // |should_be_observed_when_exiting_| of the last should-animate item.
  // |selector_item| is not nullptr when |selector_item| is the selected item
  // when exiting overview mode.
  void SetWindowListAnimationStates(
      WindowSelectorItem* selected_item,
      WindowSelector::OverviewTransition transition);

  // Do not animate the entire window list during exiting the overview. It's
  // used when splitview and overview mode are both active, selecting a window
  // will put the window in splitview mode and also end the overview mode. In
  // this case the windows in WindowGrid should not animate when exiting the
  // overivew mode. Instead, OverviewWindowAnimationObserver will observer the
  // snapped window animation and reset all windows transform in WindowGrid
  // directly when the animation is completed.
  void SetWindowListNotAnimatedWhenExiting();

  // Reset |selector_item|'s |should_animate_when_entering_|,
  // |should_animate_when_exiting_| and |should_be_observed_when_exiting_|.
  void ResetWindowListAnimationStates();

 private:
  class ShieldView;
  friend class WindowSelectorTest;

  // Initializes the screen shield widget.
  void InitShieldWidget();

  // Internal function to initialize the selection widget.
  void InitSelectionWidget(WindowSelector::Direction direction);

  // Moves the selection widget to the specified |direction|.
  void MoveSelectionWidget(WindowSelector::Direction direction,
                           bool recreate_selection_widget,
                           bool out_of_bounds,
                           bool animate);

  // Moves the selection widget to the targeted window.
  void MoveSelectionWidgetToTarget(bool animate);

  // Attempts to fit all |out_rects| inside |bounds|. The method ensures that
  // the |out_rects| vector has appropriate size and populates it with the
  // values placing rects next to each other left-to-right in rows of equal
  // |height|. While fitting |out_rects| several metrics are collected that can
  // be used by the caller. |out_max_bottom| specifies the bottom that the rects
  // are extending to. |out_min_right| and |out_max_right| report the right
  // bound of the narrowest and the widest rows respectively. In-values of the
  // |out_max_bottom|, |out_min_right| and |out_max_right| parameters are
  // ignored and their values are always initialized inside this method. Returns
  // true on success and false otherwise.
  bool FitWindowRectsInBounds(const gfx::Rect& bounds,
                              int height,
                              WindowSelectorItem* ignored_item,
                              std::vector<gfx::Rect>* out_rects,
                              int* out_max_bottom,
                              int* out_min_right,
                              int* out_max_right);

  // Sets |selector_item|'s |should_animate_when_entering_|,
  // |should_animate_when_exiting_| and |should_be_observed_when_exiting_|.
  // |selector_item| is not nullptr when |selector_item| is the selected item
  // when exiting overview mode.
  void SetWindowSelectorItemAnimationState(
      WindowSelectorItem* selector_item,
      bool* has_fullscreen_coverred,
      bool selected,
      WindowSelector::OverviewTransition transition);

  // Root window the grid is in.
  aura::Window* root_window_;

  // Pointer to the window selector that spawned this grid.
  WindowSelector* window_selector_;

  // Vector containing all the windows in this grid.
  std::vector<std::unique_ptr<WindowSelectorItem>> window_list_;

  ScopedObserver<aura::Window, WindowGrid> window_observer_;
  ScopedObserver<wm::WindowState, WindowGrid> window_state_observer_;

  // Widget that darkens the screen background.
  std::unique_ptr<views::Widget> shield_widget_;

  // A pointer to |shield_widget_|'s content view.
  ShieldView* shield_view_ = nullptr;

  // Widget that indicates to the user which is the selected window.
  std::unique_ptr<views::Widget> selection_widget_;

  // Shadow around the selector.
  std::unique_ptr<ui::Shadow> selector_shadow_;

  // Current selected window position.
  size_t selected_index_;

  // Number of columns in the grid.
  size_t num_columns_;

  // True only after all windows have been prepared for overview.
  bool prepared_for_overview_;

  // This WindowGrid's total bounds in screen coordinates.
  gfx::Rect bounds_;

  // Weak ptr to the observer monitoring the exit animation of the first MRU
  // window which covers the available workspace. The observer will be deleted
  // by itself when the animation completes.
  base::WeakPtr<OverviewWindowAnimationObserver> window_animation_observer_ =
      nullptr;

  DISALLOW_COPY_AND_ASSIGN(WindowGrid);
};

}  // namespace ash

#endif  // ASH_WM_OVERVIEW_WINDOW_GRID_H_
