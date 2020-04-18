// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/overview/window_grid.h"

#include <algorithm>
#include <functional>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "ash/public/cpp/shelf_types.h"
#include "ash/public/cpp/shell_window_ids.h"
#include "ash/public/cpp/wallpaper_types.h"
#include "ash/public/cpp/window_state_type.h"
#include "ash/root_window_controller.h"
#include "ash/screen_util.h"
#include "ash/shelf/shelf.h"
#include "ash/shell.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/wallpaper/wallpaper_controller.h"
#include "ash/wallpaper/wallpaper_widget_controller.h"
#include "ash/wm/overview/cleanup_animation_observer.h"
#include "ash/wm/overview/overview_utils.h"
#include "ash/wm/overview/overview_window_animation_observer.h"
#include "ash/wm/overview/rounded_rect_view.h"
#include "ash/wm/overview/scoped_overview_animation_settings.h"
#include "ash/wm/overview/window_selector.h"
#include "ash/wm/overview/window_selector_delegate.h"
#include "ash/wm/overview/window_selector_item.h"
#include "ash/wm/window_state.h"
#include "base/i18n/string_search.h"
#include "base/strings/string_number_conversions.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/compositor/layer_animation_observer.h"
#include "ui/compositor/scoped_layer_animation_settings.h"
#include "ui/compositor_extra/shadow.h"
#include "ui/gfx/animation/tween.h"
#include "ui/gfx/color_analysis.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/geometry/safe_integer_conversions.h"
#include "ui/gfx/geometry/vector2d.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"
#include "ui/wm/core/coordinate_conversion.h"
#include "ui/wm/core/window_animations.h"

namespace ash {
namespace {

// Time it takes for the selector widget to move to the next target. The same
// time is used for fading out shield widget when the overview mode is opened
// or closed.
constexpr int kOverviewSelectorTransitionMilliseconds = 250;

// The color and opacity of the screen shield in overview.
constexpr SkColor kShieldColor = SkColorSetARGB(255, 0, 0, 0);
constexpr float kShieldOpacity = 0.6f;

// The color and opacity of the overview selector.
constexpr SkColor kWindowSelectionColor = SkColorSetARGB(36, 255, 255, 255);

// Corner radius and shadow applied to the overview selector border.
constexpr int kWindowSelectionRadius = 9;
constexpr int kWindowSelectionShadowElevation = 24;

// The base color which is mixed with the dark muted color from wallpaper to
// form the shield widgets color.
constexpr SkColor kShieldBaseColor = SkColorSetARGB(179, 0, 0, 0);

// In the conceptual overview table, the window margin is the space reserved
// around the window within the cell. This margin does not overlap so the
// closest distance between adjacent windows will be twice this amount.
constexpr int kWindowMargin = 5;

// Windows are not allowed to get taller than this.
constexpr int kMaxHeight = 512;

// Margins reserved in the overview mode.
constexpr float kOverviewInsetRatio = 0.05f;

// Additional vertical inset reserved for windows in overview mode.
constexpr float kOverviewVerticalInset = 0.1f;

// Values for the no items indicator which appears when opening overview mode
// with no opened windows.
constexpr int kNoItemsIndicatorHeightDp = 32;
constexpr int kNoItemsIndicatorHorizontalPaddingDp = 16;
constexpr int kNoItemsIndicatorRoundingDp = 16;
constexpr int kNoItemsIndicatorVerticalPaddingDp = 8;
constexpr SkColor kNoItemsIndicatorBackgroundColor = SK_ColorBLACK;
constexpr SkColor kNoItemsIndicatorTextColor = SK_ColorWHITE;
constexpr float kNoItemsIndicatorBackgroundOpacity = 0.8f;

// Returns the vector for the fade in animation.
gfx::Vector2d GetSlideVectorForFadeIn(WindowSelector::Direction direction,
                                      const gfx::Rect& bounds) {
  gfx::Vector2d vector;
  switch (direction) {
    case WindowSelector::UP:
    case WindowSelector::LEFT:
      vector.set_x(-bounds.width());
      break;
    case WindowSelector::DOWN:
    case WindowSelector::RIGHT:
      vector.set_x(bounds.width());
      break;
  }
  return vector;
}

}  // namespace

// ShieldView contains the background for overview mode. It also contains text
// which is shown if there are no windows to be displayed.
class WindowGrid::ShieldView : public views::View {
 public:
  ShieldView() {
    background_view_ = new views::View();
    background_view_->SetPaintToLayer(ui::LAYER_SOLID_COLOR);
    background_view_->layer()->SetColor(kShieldBaseColor);
    background_view_->layer()->SetOpacity(kShieldOpacity);

    label_ = new views::Label(
        l10n_util::GetStringUTF16(IDS_ASH_OVERVIEW_NO_RECENT_ITEMS),
        views::style::CONTEXT_LABEL);
    label_->SetHorizontalAlignment(gfx::ALIGN_CENTER);
    label_->SetEnabledColor(kNoItemsIndicatorTextColor);
    label_->SetBackgroundColor(kNoItemsIndicatorBackgroundColor);

    // |label_container_| is the parent of |label_| which allows the text to
    // have padding and rounded edges.
    label_container_ = new RoundedRectView(kNoItemsIndicatorRoundingDp,
                                           kNoItemsIndicatorBackgroundColor);
    label_container_->SetLayoutManager(std::make_unique<views::BoxLayout>(
        views::BoxLayout::kVertical,
        gfx::Insets(kNoItemsIndicatorVerticalPaddingDp,
                    kNoItemsIndicatorHorizontalPaddingDp)));
    label_container_->AddChildView(label_);
    label_container_->SetPaintToLayer();
    label_container_->layer()->SetFillsBoundsOpaquely(false);
    label_container_->layer()->SetOpacity(kNoItemsIndicatorBackgroundOpacity);
    label_container_->SetVisible(false);

    AddChildView(background_view_);
    AddChildView(label_container_);
  }

  ~ShieldView() override = default;

  void SetBackgroundColor(SkColor color) {
    background_view_->layer()->SetColor(color);
  }

  void SetLabelVisibility(bool visible) {
    label_container_->SetVisible(visible);
  }

  gfx::Rect GetLabelBounds() const {
    return label_container_->GetBoundsInScreen();
  }

  // ShieldView takes up the whole workspace since it changes opacity of the
  // whole wallpaper. The bounds of the grid may be smaller in some cases of
  // splitview. The label should be centered in the bounds of the grid.
  void SetGridBounds(const gfx::Rect& bounds) {
    const int label_width = label_->GetPreferredSize().width() +
                            2 * kNoItemsIndicatorHorizontalPaddingDp;
    gfx::Rect label_container_bounds = bounds;
    label_container_bounds.ClampToCenteredSize(
        gfx::Size(label_width, kNoItemsIndicatorHeightDp));
    label_container_->SetBoundsRect(label_container_bounds);
  }

  bool IsLabelVisible() const { return label_container_->visible(); }

 protected:
  // views::View:
  void Layout() override { background_view_->SetBoundsRect(GetLocalBounds()); }

 private:
  // Owned by views heirarchy.
  views::View* background_view_ = nullptr;
  RoundedRectView* label_container_ = nullptr;
  views::Label* label_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(ShieldView);
};

WindowGrid::WindowGrid(aura::Window* root_window,
                       const std::vector<aura::Window*>& windows,
                       WindowSelector* window_selector,
                       const gfx::Rect& bounds_in_screen)
    : root_window_(root_window),
      window_selector_(window_selector),
      window_observer_(this),
      window_state_observer_(this),
      selected_index_(0),
      num_columns_(0),
      prepared_for_overview_(false),
      bounds_(bounds_in_screen) {
  aura::Window::Windows windows_in_root;
  for (auto* window : windows) {
    if (window->GetRootWindow() == root_window)
      windows_in_root.push_back(window);
  }

  for (auto* window : windows_in_root) {
    // TODO(https://crbug.com/812496): Investigate why we need to keep target
    // transform instead of using identity when exiting.
    // Stop ongoing animations before entering overview mode. Because we are
    // deferring SetTransform of the windows beneath the window covering the
    // available workspace, we need to set the correct transforms of these
    // windows before entering overview mode again in the
    // OnImplicitAnimationsCompleted() of the observer of the
    // available-workspace-covering window's animation.
    auto* animator = window->layer()->GetAnimator();
    if (animator->is_animating())
      window->layer()->GetAnimator()->StopAnimating();
    window_observer_.Add(window);
    window_state_observer_.Add(wm::GetWindowState(window));
    window_list_.push_back(
        std::make_unique<WindowSelectorItem>(window, window_selector_, this));
  }
}

WindowGrid::~WindowGrid() = default;

void WindowGrid::Shutdown() {
  for (const auto& window : window_list_)
    window->Shutdown();

  if (shield_widget_) {
    // Fade out the shield widget. This animation continues past the lifetime
    // of |this|.
    FadeOutWidgetOnExit(std::move(shield_widget_),
                        OVERVIEW_ANIMATION_RESTORE_WINDOW);
  }
}

void WindowGrid::PrepareForOverview() {
  InitShieldWidget();
  for (const auto& window : window_list_)
    window->PrepareForOverview();
  prepared_for_overview_ = true;
}

void WindowGrid::PositionWindows(bool animate,
                                 WindowSelectorItem* ignored_item) {
  if (window_selector_->IsShuttingDown())
    return;

  DCHECK(shield_widget_.get());
  // Keep the background shield widget covering the whole screen. A grid without
  // any windows still needs the shield widget bounds updated.
  aura::Window* widget_window = shield_widget_->GetNativeWindow();
  const gfx::Rect bounds = widget_window->parent()->bounds();
  widget_window->SetBounds(bounds);

  ShowNoRecentsWindowMessage(window_list_.empty());

  if (window_list_.empty())
    return;

  gfx::Rect total_bounds = bounds_;
  // Windows occupy vertically centered area with additional vertical insets.
  int horizontal_inset =
      gfx::ToFlooredInt(std::min(kOverviewInsetRatio * total_bounds.width(),
                                 kOverviewInsetRatio * total_bounds.height()));
  int vertical_inset =
      horizontal_inset +
      kOverviewVerticalInset * (total_bounds.height() - 2 * horizontal_inset);
  total_bounds.Inset(std::max(0, horizontal_inset - kWindowMargin),
                     std::max(0, vertical_inset - kWindowMargin));
  std::vector<gfx::Rect> rects;

  // Keep track of the lowest coordinate.
  int max_bottom = total_bounds.y();

  // Right bound of the narrowest row.
  int min_right = total_bounds.right();
  // Right bound of the widest row.
  int max_right = total_bounds.x();

  // Keep track of the difference between the narrowest and the widest row.
  // Initially this is set to the worst it can ever be assuming the windows fit.
  int width_diff = total_bounds.width();

  // Initially allow the windows to occupy all available width. Shrink this
  // available space horizontally to find the breakdown into rows that achieves
  // the minimal |width_diff|.
  int right_bound = total_bounds.right();

  // Determine the optimal height bisecting between |low_height| and
  // |high_height|. Once this optimal height is known, |height_fixed| is set to
  // true and the rows are balanced by repeatedly squeezing the widest row to
  // cause windows to overflow to the subsequent rows.
  int low_height = 2 * kWindowMargin;
  int high_height =
      std::max(low_height, static_cast<int>(total_bounds.height() + 1));
  int height = 0.5 * (low_height + high_height);
  bool height_fixed = false;

  // Repeatedly try to fit the windows |rects| within |right_bound|.
  // If a maximum |height| is found such that all window |rects| fit, this
  // fitting continues while shrinking the |right_bound| in order to balance the
  // rows. If the windows fit the |right_bound| would have been decremented at
  // least once so it needs to be incremented once before getting out of this
  // loop and one additional pass made to actually fit the |rects|.
  // If the |rects| cannot fit (e.g. there are too many windows) the bisection
  // will still finish and we might increment the |right_bound| once pixel extra
  // which is acceptable since there is an unused margin on the right.
  bool make_last_adjustment = false;
  while (true) {
    gfx::Rect overview_bounds(total_bounds);
    overview_bounds.set_width(right_bound - total_bounds.x());
    bool windows_fit = FitWindowRectsInBounds(
        overview_bounds, std::min(kMaxHeight + 2 * kWindowMargin, height),
        ignored_item, &rects, &max_bottom, &min_right, &max_right);

    if (height_fixed) {
      if (!windows_fit) {
        // Revert the previous change to |right_bound| and do one last pass.
        right_bound++;
        make_last_adjustment = true;
        break;
      }
      // Break if all the windows are zero-width at the current scale.
      if (max_right <= total_bounds.x())
        break;
    } else {
      // Find the optimal row height bisecting between |low_height| and
      // |high_height|.
      if (windows_fit)
        low_height = height;
      else
        high_height = height;
      height = 0.5 * (low_height + high_height);
      // When height can no longer be improved, start balancing the rows.
      if (height == low_height)
        height_fixed = true;
    }

    if (windows_fit && height_fixed) {
      if (max_right - min_right <= width_diff) {
        // Row alignment is getting better. Try to shrink the |right_bound| in
        // order to squeeze the widest row.
        right_bound = max_right - 1;
        width_diff = max_right - min_right;
      } else {
        // Row alignment is getting worse.
        // Revert the previous change to |right_bound| and do one last pass.
        right_bound++;
        make_last_adjustment = true;
        break;
      }
    }
  }
  // Once the windows in |window_list_| no longer fit, the change to
  // |right_bound| was reverted. Perform one last pass to position the |rects|.
  if (make_last_adjustment) {
    gfx::Rect overview_bounds(total_bounds);
    overview_bounds.set_width(right_bound - total_bounds.x());
    FitWindowRectsInBounds(
        overview_bounds, std::min(kMaxHeight + 2 * kWindowMargin, height),
        ignored_item, &rects, &max_bottom, &min_right, &max_right);
  }
  // Position the windows centering the left-aligned rows vertically. Do not
  // position |ignored_item| if it is not nullptr and matches a item in
  // |window_list_|.
  gfx::Vector2d offset(0, (total_bounds.bottom() - max_bottom) / 2);
  for (size_t i = 0; i < window_list_.size(); ++i) {
    if (ignored_item != nullptr && window_list_[i].get() == ignored_item)
      continue;

    const bool should_animate = window_list_[i]->ShouldAnimateWhenEntering();
    window_list_[i]->SetBounds(
        rects[i] + offset,
        animate && should_animate
            ? OverviewAnimationType::OVERVIEW_ANIMATION_LAY_OUT_SELECTOR_ITEMS
            : OverviewAnimationType::OVERVIEW_ANIMATION_NONE);
  }

  // If the selection widget is active, reposition it without any animation.
  if (selection_widget_)
    MoveSelectionWidgetToTarget(animate);
}

bool WindowGrid::Move(WindowSelector::Direction direction, bool animate) {
  if (empty())
    return true;

  bool recreate_selection_widget = false;
  bool out_of_bounds = false;
  bool changed_selection_index = false;
  gfx::Rect old_bounds;
  if (SelectedWindow()) {
    old_bounds = SelectedWindow()->target_bounds();
    // Make the old selected window header non-transparent first.
    SelectedWindow()->set_selected(false);
  }

  // [up] key is equivalent to [left] key and [down] key is equivalent to
  // [right] key.
  if (!selection_widget_) {
    switch (direction) {
      case WindowSelector::UP:
      case WindowSelector::LEFT:
        selected_index_ = window_list_.size() - 1;
        break;
      case WindowSelector::DOWN:
      case WindowSelector::RIGHT:
        selected_index_ = 0;
        break;
    }
    changed_selection_index = true;
  }
  while (!changed_selection_index ||
         (!out_of_bounds && window_list_[selected_index_]->dimmed())) {
    switch (direction) {
      case WindowSelector::UP:
      case WindowSelector::LEFT:
        if (selected_index_ == 0)
          out_of_bounds = true;
        selected_index_--;
        break;
      case WindowSelector::DOWN:
      case WindowSelector::RIGHT:
        if (selected_index_ >= window_list_.size() - 1)
          out_of_bounds = true;
        selected_index_++;
        break;
    }
    if (!out_of_bounds && SelectedWindow()) {
      if (SelectedWindow()->target_bounds().y() != old_bounds.y())
        recreate_selection_widget = true;
    }
    changed_selection_index = true;
  }
  MoveSelectionWidget(direction, recreate_selection_widget, out_of_bounds,
                      animate);

  // Make the new selected window header fully transparent.
  if (SelectedWindow())
    SelectedWindow()->set_selected(true);
  return out_of_bounds;
}

WindowSelectorItem* WindowGrid::SelectedWindow() const {
  if (!selection_widget_)
    return nullptr;
  CHECK(selected_index_ < window_list_.size());
  return window_list_[selected_index_].get();
}

WindowSelectorItem* WindowGrid::GetWindowSelectorItemContaining(
    const aura::Window* window) const {
  for (const auto& window_item : window_list_) {
    if (window_item && window_item->Contains(window))
      return window_item.get();
  }
  return nullptr;
}

void WindowGrid::AddItem(aura::Window* window) {
  DCHECK(!GetWindowSelectorItemContaining(window));

  window_observer_.Add(window);
  window_state_observer_.Add(wm::GetWindowState(window));
  window_list_.push_back(
      std::make_unique<WindowSelectorItem>(window, window_selector_, this));
  window_list_.back()->PrepareForOverview();

  PositionWindows(/*animate=*/true);
}

void WindowGrid::RemoveItem(WindowSelectorItem* selector_item) {
  auto iter =
      std::find_if(window_list_.begin(), window_list_.end(),
                   [selector_item](std::unique_ptr<WindowSelectorItem>& item) {
                     return (item.get() == selector_item);
                   });
  if (iter != window_list_.end()) {
    window_observer_.Remove(selector_item->GetWindow());
    window_state_observer_.Remove(
        wm::GetWindowState(selector_item->GetWindow()));
    window_list_.erase(iter);
  }
}

void WindowGrid::FilterItems(const base::string16& pattern) {
  base::i18n::FixedPatternStringSearchIgnoringCaseAndAccents finder(pattern);
  for (const auto& window : window_list_) {
    if (finder.Search(window->GetWindow()->GetTitle(), nullptr, nullptr)) {
      window->SetDimmed(false);
    } else {
      window->SetDimmed(true);
      if (selection_widget_ && SelectedWindow() == window.get()) {
        SelectedWindow()->set_selected(false);
        selection_widget_.reset();
        selector_shadow_.reset();
      }
    }
  }
}

void WindowGrid::WindowClosing(WindowSelectorItem* window) {
  if (!selection_widget_ || SelectedWindow() != window)
    return;
  aura::Window* selection_widget_window = selection_widget_->GetNativeWindow();
  ScopedOverviewAnimationSettings animation_settings_label(
      OverviewAnimationType::OVERVIEW_ANIMATION_CLOSING_SELECTOR_ITEM,
      selection_widget_window);
  selection_widget_->SetOpacity(0.f);
}

void WindowGrid::SetBoundsAndUpdatePositions(const gfx::Rect& bounds) {
  SetBoundsAndUpdatePositionsIgnoringWindow(bounds, nullptr);
}

void WindowGrid::SetBoundsAndUpdatePositionsIgnoringWindow(
    const gfx::Rect& bounds,
    WindowSelectorItem* ignored_item) {
  bounds_ = bounds;
  if (shield_view_)
    shield_view_->SetGridBounds(bounds_);
  PositionWindows(/*animate=*/true, ignored_item);
}

void WindowGrid::SetSelectionWidgetVisibility(bool visible) {
  if (!selection_widget_)
    return;

  if (visible)
    selection_widget_->Show();
  else
    selection_widget_->Hide();
}

void WindowGrid::ShowNoRecentsWindowMessage(bool visible) {
  // Only show the warning on the grid associated with primary root.
  if (root_window_ != Shell::GetPrimaryRootWindow())
    return;

  if (shield_view_)
    shield_view_->SetLabelVisibility(visible);
}

void WindowGrid::UpdateCannotSnapWarningVisibility() {
  for (auto& window_selector_item : window_list_)
    window_selector_item->UpdateCannotSnapWarningVisibility();
}

void WindowGrid::OnSelectorItemDragStarted(WindowSelectorItem* item) {
  for (auto& window_selector_item : window_list_)
    window_selector_item->OnSelectorItemDragStarted(item);
}

void WindowGrid::OnSelectorItemDragEnded() {
  for (auto& window_selector_item : window_list_)
    window_selector_item->OnSelectorItemDragEnded();
}

void WindowGrid::OnWindowDestroying(aura::Window* window) {
  window_observer_.Remove(window);
  window_state_observer_.Remove(wm::GetWindowState(window));
  auto iter = std::find_if(window_list_.begin(), window_list_.end(),
                           [window](std::unique_ptr<WindowSelectorItem>& item) {
                             return item->GetWindow() == window;
                           });
  DCHECK(iter != window_list_.end());

  size_t removed_index = iter - window_list_.begin();
  window_list_.erase(iter);

  if (empty()) {
    selection_widget_.reset();
    // If the grid is now empty, notify the window selector so that it erases us
    // from its grid list.
    window_selector_->OnGridEmpty(this);
    return;
  }

  // If selecting, update the selection index.
  if (selection_widget_) {
    bool send_focus_alert = selected_index_ == removed_index;
    if (selected_index_ >= removed_index && selected_index_ != 0)
      selected_index_--;
    SelectedWindow()->set_selected(true);
    if (send_focus_alert)
      SelectedWindow()->SendAccessibleSelectionEvent();
  }

  PositionWindows(true);
}

void WindowGrid::OnWindowBoundsChanged(aura::Window* window,
                                       const gfx::Rect& old_bounds,
                                       const gfx::Rect& new_bounds,
                                       ui::PropertyChangeReason reason) {
  // During preparation, window bounds can change. Ignore bounds
  // change notifications in this case; we'll reposition soon.
  if (!prepared_for_overview_)
    return;

  auto iter = std::find_if(window_list_.begin(), window_list_.end(),
                           [window](std::unique_ptr<WindowSelectorItem>& item) {
                             return item->GetWindow() == window;
                           });
  DCHECK(iter != window_list_.end());

  // Immediately finish any active bounds animation.
  window->layer()->GetAnimator()->StopAnimatingProperty(
      ui::LayerAnimationElement::BOUNDS);
  (*iter)->UpdateWindowDimensionsType();
  PositionWindows(false);
}

void WindowGrid::OnPostWindowStateTypeChange(wm::WindowState* window_state,
                                             mojom::WindowStateType old_type) {
  // During preparation, window state can change, e.g. updating shelf
  // visibility may show the temporarily hidden (minimized) panels.
  if (!prepared_for_overview_)
    return;

  mojom::WindowStateType new_type = window_state->GetStateType();
  if (IsMinimizedWindowStateType(old_type) ==
      IsMinimizedWindowStateType(new_type)) {
    return;
  }

  auto iter =
      std::find_if(window_list_.begin(), window_list_.end(),
                   [window_state](std::unique_ptr<WindowSelectorItem>& item) {
                     return item->Contains(window_state->window());
                   });
  if (iter != window_list_.end()) {
    (*iter)->OnMinimizedStateChanged();
    PositionWindows(false);
  }
}

bool WindowGrid::IsNoItemsIndicatorLabelVisibleForTesting() {
  return shield_view_ && shield_view_->IsLabelVisible();
}

gfx::Rect WindowGrid::GetNoItemsIndicatorLabelBoundsForTesting() const {
  if (!shield_view_)
    return gfx::Rect();

  return shield_view_->GetLabelBounds();
}

void WindowGrid::SetWindowListAnimationStates(
    WindowSelectorItem* selected_item,
    WindowSelector::OverviewTransition transition) {
  // |selected_item| is nullptr during entering animation.
  DCHECK(transition == WindowSelector::OverviewTransition::kExit ||
         selected_item == nullptr);

  bool has_covered_available_workspace = false;
  bool has_checked_selected_item = false;
  if (!selected_item ||
      !wm::GetWindowState(selected_item->GetWindow())->IsFullscreen()) {
    // Check the always on top window first if |selected_item| is nullptr or the
    // |selected_item|'s window is not fullscreen. Because always on top windows
    // are visible and may have a window which can cover available workspace.
    // If the |selected_item| is fullscreen, we will depromote all always on top
    // windows.
    aura::Window* always_on_top_container =
        RootWindowController::ForWindow(root_window_)
            ->GetContainer(kShellWindowId_AlwaysOnTopContainer);
    aura::Window::Windows top_windows = always_on_top_container->children();
    for (aura::Window::Windows::const_reverse_iterator
             it = top_windows.rbegin(),
             rend = top_windows.rend();
         it != rend; ++it) {
      aura::Window* top_window = *it;
      WindowSelectorItem* container_item =
          GetWindowSelectorItemContaining(top_window);
      if (!container_item)
        continue;

      const bool is_selected_item = (selected_item == container_item);
      if (!has_checked_selected_item && is_selected_item)
        has_checked_selected_item = true;
      SetWindowSelectorItemAnimationState(
          container_item, &has_covered_available_workspace,
          /*selected=*/is_selected_item, transition);
    }
  }

  if (!has_checked_selected_item) {
    SetWindowSelectorItemAnimationState(selected_item,
                                        &has_covered_available_workspace,
                                        /*selected=*/true, transition);
  }
  for (const auto& item : window_list_) {
    // Has checked the |selected_item|.
    if (selected_item == item.get())
      continue;
    // Has checked all always on top windows.
    if (item->GetWindow()->GetProperty(aura::client::kAlwaysOnTopKey))
      continue;
    SetWindowSelectorItemAnimationState(item.get(),
                                        &has_covered_available_workspace,
                                        /*selected=*/false, transition);
  }
}

void WindowGrid::SetWindowListNotAnimatedWhenExiting() {
  for (const auto& item : window_list_) {
    item->set_should_animate_when_exiting(false);
    item->set_should_be_observed_when_exiting(false);
  }
}

void WindowGrid::ResetWindowListAnimationStates() {
  for (const auto& selector_item : window_list_)
    selector_item->ResetAnimationStates();
}

void WindowGrid::InitShieldWidget() {
  // TODO(varkha): The code assumes that SHELF_BACKGROUND_MAXIMIZED is
  // synonymous with a black shelf background. Update this code if that
  // assumption is no longer valid.
  const float initial_opacity =
      (Shelf::ForWindow(root_window_)->GetBackgroundType() ==
       SHELF_BACKGROUND_MAXIMIZED)
          ? 1.f
          : 0.f;
  SkColor shield_color = kShieldColor;
  // Extract the dark muted color from the wallpaper and mix it with
  // |kShieldBaseColor|. Just use |kShieldBaseColor| if the dark muted color
  // could not be extracted.
  SkColor dark_muted_color =
      Shell::Get()->wallpaper_controller()->GetProminentColor(
          color_utils::ColorProfile());
  if (dark_muted_color != ash::kInvalidWallpaperColor) {
    shield_color =
        color_utils::GetResultingPaintColor(kShieldBaseColor, dark_muted_color);
  }
  shield_widget_ = CreateBackgroundWidget(
      root_window_, ui::LAYER_SOLID_COLOR, SK_ColorTRANSPARENT, 0, 0,
      SK_ColorTRANSPARENT, initial_opacity, /*parent=*/nullptr,
      /*stack_on_top=*/true);
  aura::Window* widget_window = shield_widget_->GetNativeWindow();
  const gfx::Rect bounds = widget_window->parent()->bounds();
  widget_window->SetBounds(bounds);
  widget_window->SetName("OverviewModeShield");

  // Create |shield_view_| and animate its background and label if needed.
  shield_view_ = new ShieldView();
  shield_view_->SetBackgroundColor(shield_color);
  shield_view_->SetGridBounds(bounds_);
  shield_widget_->SetContentsView(shield_view_);
  shield_widget_->SetOpacity(initial_opacity);
  ui::ScopedLayerAnimationSettings animation_settings(
      widget_window->layer()->GetAnimator());
  animation_settings.SetTransitionDuration(base::TimeDelta::FromMilliseconds(
      kOverviewSelectorTransitionMilliseconds));
  animation_settings.SetTweenType(gfx::Tween::EASE_OUT);
  animation_settings.SetPreemptionStrategy(
      ui::LayerAnimator::IMMEDIATELY_ANIMATE_TO_NEW_TARGET);
  shield_widget_->SetOpacity(1.f);
}

void WindowGrid::InitSelectionWidget(WindowSelector::Direction direction) {
  selection_widget_ = CreateBackgroundWidget(
      root_window_, ui::LAYER_TEXTURED, kWindowSelectionColor, 0,
      kWindowSelectionRadius, SK_ColorTRANSPARENT, 0.f, /*parent=*/nullptr,
      /*stack_on_top=*/true);
  aura::Window* widget_window = selection_widget_->GetNativeWindow();
  gfx::Rect target_bounds = SelectedWindow()->target_bounds();
  ::wm::ConvertRectFromScreen(root_window_, &target_bounds);
  gfx::Vector2d fade_out_direction =
      GetSlideVectorForFadeIn(direction, target_bounds);
  widget_window->SetBounds(target_bounds - fade_out_direction);
  widget_window->SetName("OverviewModeSelector");

  selector_shadow_ = std::make_unique<ui::Shadow>();
  selector_shadow_->Init(kWindowSelectionShadowElevation);
  selector_shadow_->layer()->SetVisible(true);
  selection_widget_->GetLayer()->SetMasksToBounds(false);
  selection_widget_->GetLayer()->Add(selector_shadow_->layer());
  selector_shadow_->SetContentBounds(gfx::Rect(target_bounds.size()));
}

void WindowGrid::MoveSelectionWidget(WindowSelector::Direction direction,
                                     bool recreate_selection_widget,
                                     bool out_of_bounds,
                                     bool animate) {
  // If the selection widget is already active, fade it out in the selection
  // direction.
  if (selection_widget_ && (recreate_selection_widget || out_of_bounds)) {
    // Animate the old selection widget and then destroy it.
    views::Widget* old_selection = selection_widget_.get();
    aura::Window* old_selection_window = old_selection->GetNativeWindow();
    gfx::Vector2d fade_out_direction =
        GetSlideVectorForFadeIn(direction, old_selection_window->bounds());

    ui::ScopedLayerAnimationSettings animation_settings(
        old_selection_window->layer()->GetAnimator());
    animation_settings.SetTransitionDuration(base::TimeDelta::FromMilliseconds(
        kOverviewSelectorTransitionMilliseconds));
    animation_settings.SetPreemptionStrategy(
        ui::LayerAnimator::IMMEDIATELY_ANIMATE_TO_NEW_TARGET);
    animation_settings.SetTweenType(gfx::Tween::FAST_OUT_LINEAR_IN);
    // CleanupAnimationObserver will delete itself (and the widget) when the
    // motion animation is complete.
    // Ownership over the observer is passed to the window_selector_->delegate()
    // which has longer lifetime so that animations can continue even after the
    // overview mode is shut down.
    std::unique_ptr<CleanupAnimationObserver> observer(
        new CleanupAnimationObserver(std::move(selection_widget_)));
    animation_settings.AddObserver(observer.get());
    window_selector_->delegate()->AddDelayedAnimationObserver(
        std::move(observer));
    old_selection->SetOpacity(0.f);
    old_selection_window->SetBounds(old_selection_window->bounds() +
                                    fade_out_direction);
    old_selection->Hide();
  }
  if (out_of_bounds)
    return;

  if (!selection_widget_)
    InitSelectionWidget(direction);
  // Send an a11y alert so that if ChromeVox is enabled, the item label is
  // read.
  SelectedWindow()->SendAccessibleSelectionEvent();
  // The selection widget is moved to the newly selected item in the same
  // grid.
  MoveSelectionWidgetToTarget(animate);
}

void WindowGrid::MoveSelectionWidgetToTarget(bool animate) {
  gfx::Rect bounds = SelectedWindow()->target_bounds();
  ::wm::ConvertRectFromScreen(root_window_, &bounds);
  if (animate) {
    aura::Window* selection_widget_window =
        selection_widget_->GetNativeWindow();
    ui::ScopedLayerAnimationSettings animation_settings(
        selection_widget_window->layer()->GetAnimator());
    animation_settings.SetTransitionDuration(base::TimeDelta::FromMilliseconds(
        kOverviewSelectorTransitionMilliseconds));
    animation_settings.SetTweenType(gfx::Tween::EASE_IN_OUT);
    animation_settings.SetPreemptionStrategy(
        ui::LayerAnimator::IMMEDIATELY_ANIMATE_TO_NEW_TARGET);
    selection_widget_->SetBounds(bounds);
    selection_widget_->SetOpacity(1.f);

    if (selector_shadow_) {
      ui::ScopedLayerAnimationSettings animation_settings_shadow(
          selector_shadow_->shadow_layer()->GetAnimator());
      animation_settings_shadow.SetTransitionDuration(
          base::TimeDelta::FromMilliseconds(
              kOverviewSelectorTransitionMilliseconds));
      animation_settings_shadow.SetTweenType(gfx::Tween::EASE_IN_OUT);
      animation_settings_shadow.SetPreemptionStrategy(
          ui::LayerAnimator::IMMEDIATELY_ANIMATE_TO_NEW_TARGET);
      bounds.Inset(1, 1);
      selector_shadow_->SetContentBounds(
          gfx::Rect(gfx::Point(1, 1), bounds.size()));
    }
    return;
  }
  selection_widget_->SetBounds(bounds);
  selection_widget_->SetOpacity(1.f);
  if (selector_shadow_) {
    bounds.Inset(1, 1);
    selector_shadow_->SetContentBounds(
        gfx::Rect(gfx::Point(1, 1), bounds.size()));
  }
}

bool WindowGrid::FitWindowRectsInBounds(const gfx::Rect& bounds,
                                        int height,
                                        WindowSelectorItem* ignored_item,
                                        std::vector<gfx::Rect>* out_rects,
                                        int* out_max_bottom,
                                        int* out_min_right,
                                        int* out_max_right) {
  out_rects->resize(window_list_.size());
  bool windows_fit = true;

  // Start in the top-left corner of |bounds|.
  int left = bounds.x();
  int top = bounds.y();

  // Keep track of the lowest coordinate.
  *out_max_bottom = bounds.y();

  // Right bound of the narrowest row.
  *out_min_right = bounds.right();
  // Right bound of the widest row.
  *out_max_right = bounds.x();

  // All elements are of same height and only the height is necessary to
  // determine each item's scale.
  const gfx::Size item_size(0, height);
  size_t i = 0;
  for (const auto& window : window_list_) {
    if (ignored_item && ignored_item == window.get()) {
      // Increment the index anyways. PositionWindows will handle skipping this
      // entry.
      ++i;
      continue;
    }

    const gfx::Rect target_bounds = window->GetTargetBoundsInScreen();
    int width = std::max(1, gfx::ToFlooredInt(target_bounds.width() *
                                              window->GetItemScale(item_size)) +
                                2 * kWindowMargin);
    switch (window->GetWindowDimensionsType()) {
      case ScopedTransformOverviewWindow::GridWindowFillMode::kLetterBoxed:
        width = ScopedTransformOverviewWindow::kExtremeWindowRatioThreshold *
                height;
        break;
      case ScopedTransformOverviewWindow::GridWindowFillMode::kPillarBoxed:
        width = height /
                ScopedTransformOverviewWindow::kExtremeWindowRatioThreshold;
        break;
      default:
        break;
    }

    if (left + width > bounds.right()) {
      // Move to the next row if possible.
      if (*out_min_right > left)
        *out_min_right = left;
      if (*out_max_right < left)
        *out_max_right = left;
      top += height;

      // Check if the new row reaches the bottom or if the first item in the new
      // row does not fit within the available width.
      if (top + height > bounds.bottom() ||
          bounds.x() + width > bounds.right()) {
        windows_fit = false;
        // If the |ignored_item| is the last item, update |out_max_bottom|
        // before breaking the loop, but no need to add the height, as the last
        // item does not contribute to the grid bounds.
        if (ignored_item && ignored_item == window_list_.back().get())
          *out_max_bottom = top;
        break;
      }
      left = bounds.x();
    }

    // Position the current rect.
    (*out_rects)[i].SetRect(left, top, width, height);

    // Increment horizontal position using sanitized positive |width()|.
    left += (*out_rects)[i].width();

    if (++i == out_rects->size()) {
      // Update the narrowest and widest row width for the last row.
      if (*out_min_right > left)
        *out_min_right = left;
      if (*out_max_right < left)
        *out_max_right = left;
    }
    *out_max_bottom = top + height;
  }
  return windows_fit;
}

void WindowGrid::SetWindowSelectorItemAnimationState(
    WindowSelectorItem* selector_item,
    bool* has_covered_available_workspace,
    bool selected,
    WindowSelector::OverviewTransition transition) {
  if (!selector_item)
    return;

  aura::Window* window = selector_item->GetWindow();
  // |selector_item| should be contained in the |window_list_|.
  DCHECK(GetWindowSelectorItemContaining(window));

  bool can_cover_available_workspace = CanCoverAvailableWorkspace(window);
  const bool should_animate = selected || !(*has_covered_available_workspace);
  if (transition == WindowSelector::OverviewTransition::kEnter)
    selector_item->set_should_animate_when_entering(should_animate);
  if (transition == WindowSelector::OverviewTransition::kExit)
    selector_item->set_should_animate_when_exiting(should_animate);

  if (!(*has_covered_available_workspace) && can_cover_available_workspace) {
    if (transition == WindowSelector::OverviewTransition::kExit) {
      selector_item->set_should_be_observed_when_exiting(true);
      auto* observer = new OverviewWindowAnimationObserver();
      set_window_animation_observer(observer->GetWeakPtr());
    }
    *has_covered_available_workspace = true;
  }
}

}  // namespace ash
