// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/overview/window_selector_controller.h"

#include <vector>

#include "ash/public/cpp/window_properties.h"
#include "ash/root_window_controller.h"
#include "ash/session/session_controller.h"
#include "ash/shell.h"
#include "ash/wallpaper/wallpaper_controller.h"
#include "ash/wallpaper/wallpaper_widget_controller.h"
#include "ash/wm/mru_window_tracker.h"
#include "ash/wm/overview/overview_utils.h"
#include "ash/wm/overview/window_grid.h"
#include "ash/wm/overview/window_selector.h"
#include "ash/wm/overview/window_selector_item.h"
#include "ash/wm/root_window_finder.h"
#include "ash/wm/screen_pinning_controller.h"
#include "ash/wm/splitview/split_view_controller.h"
#include "ash/wm/window_state.h"
#include "ash/wm/window_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/user_metrics.h"
#include "ui/gfx/animation/animation_delegate.h"
#include "ui/gfx/animation/slide_animation.h"
#include "ui/wm/core/window_util.h"

namespace ash {

namespace {

// Amount of blur to apply on the wallpaper when we enter or exit overview mode.
constexpr double kWallpaperBlurSigma = 10.f;
constexpr double kWallpaperClearBlurSigma = 0.f;
constexpr int kBlurSlideDurationMs = 250;

// Returns true if |window| should be hidden when entering overview.
bool ShouldHideWindowInOverview(const aura::Window* window) {
  return !window->GetProperty(ash::kShowInOverviewKey);
}

// Returns true if |window| should be excluded from overview.
bool ShouldExcludeWindowFromOverview(const aura::Window* window) {
  if (ShouldHideWindowInOverview(window))
    return true;

  // Other non-selectable windows will be ignored in overview.
  if (!WindowSelector::IsSelectable(window))
    return true;

  // Remove the default snapped window from the window list. The default
  // snapped window occupies one side of the screen, while the other windows
  // occupy the other side of the screen in overview mode. The default snap
  // position is the position where the window was first snapped. See
  // |default_snap_position_| in SplitViewController for more detail.
  if (Shell::Get()->IsSplitViewModeActive() &&
      window ==
          Shell::Get()->split_view_controller()->GetDefaultSnappedWindow()) {
    return true;
  }

  // The window that is currently in tab-dragging process should be ignored in
  // overview grid.
  if (ash::wm::IsDraggingTabs(window))
    return true;

  return false;
}

bool IsBlurEnabled() {
  return Shell::Get()->wallpaper_controller()->IsBlurEnabled();
}

}  // namespace

// Class that handles of blurring wallpaper upon entering and exiting overview
// mode. Blurs the wallpaper automatically if the wallpaper is not visible
// prior to entering overview mode (covered by a window), otherwise animates
// the blur.
class WindowSelectorController::OverviewBlurController
    : public gfx::AnimationDelegate,
      public aura::WindowObserver {
 public:
  OverviewBlurController() : animation_(this) {
    animation_.SetSlideDuration(kBlurSlideDurationMs);
  }

  ~OverviewBlurController() override {
    animation_.Stop();
    for (aura::Window* root : roots_to_animate_)
      root->RemoveObserver(this);
  }

  void Blur() {
    state_ = WallpaperAnimationState::kAddingBlur;
    OnBlurChange();
  }

  void Unblur() {
    state_ = WallpaperAnimationState::kRemovingBlur;
    OnBlurChange();
  }

 private:
  enum class WallpaperAnimationState {
    kAddingBlur,
    kRemovingBlur,
    kNormal,
  };

  // gfx::AnimationDelegate:
  void AnimationEnded(const gfx::Animation* animation) override {
    if (state_ == WallpaperAnimationState::kNormal)
      return;

    double value = state_ == WallpaperAnimationState::kAddingBlur
                       ? kWallpaperBlurSigma
                       : kWallpaperClearBlurSigma;
    for (aura::Window* root : roots_to_animate_)
      ApplyBlur(root, value);
    state_ = WallpaperAnimationState::kNormal;
  }

  void AnimationProgressed(const gfx::Animation* animation) override {
    double value = animation_.CurrentValueBetween(kWallpaperClearBlurSigma,
                                                  kWallpaperBlurSigma);
    for (aura::Window* root : roots_to_animate_)
      ApplyBlur(root, value);
  }

  void AnimationCanceled(const gfx::Animation* animation) override {
    for (aura::Window* root : roots_to_animate_)
      ApplyBlur(root, kWallpaperClearBlurSigma);
    state_ = WallpaperAnimationState::kNormal;
  }

  // aura::WindowObserver:
  void OnWindowDestroying(aura::Window* window) override {
    window->RemoveObserver(this);
    auto it =
        std::find(roots_to_animate_.begin(), roots_to_animate_.end(), window);
    if (it != roots_to_animate_.end())
      roots_to_animate_.erase(it);
  }

  void ApplyBlur(aura::Window* root, float blur_sigma) {
    RootWindowController::ForWindow(root)
        ->wallpaper_widget_controller()
        ->SetWallpaperBlur(static_cast<float>(blur_sigma));
  }

  // Called when the wallpaper is to be changed. Checks to see which root
  // windows should have their wallpaper blurs animated and fills
  // |roots_to_animate_| accordingly. Applys blur or unblur immediately if
  // the wallpaper does not need blur animation.
  void OnBlurChange() {
    bool should_blur = state_ == WallpaperAnimationState::kAddingBlur;
    double value = should_blur ? kWallpaperBlurSigma : kWallpaperClearBlurSigma;
    for (aura::Window* root : roots_to_animate_)
      root->RemoveObserver(this);
    roots_to_animate_.clear();

    WindowSelector* window_selector =
        Shell::Get()->window_selector_controller()->window_selector();
    DCHECK(window_selector);
    for (aura::Window* root : Shell::Get()->GetAllRootWindows()) {
      if (!window_selector->ShouldAnimateWallpaper(root)) {
        ApplyBlur(root, value);
      } else {
        root->AddObserver(this);
        roots_to_animate_.push_back(root);
      }
    }

    // Run the animation if one of the roots needs to be aniamted.
    if (roots_to_animate_.empty()) {
      state_ = WallpaperAnimationState::kNormal;
    } else if (should_blur) {
      animation_.Show();
    } else {
      animation_.Hide();
    }
  }

  WallpaperAnimationState state_ = WallpaperAnimationState::kNormal;
  gfx::SlideAnimation animation_;
  // Vector which contains the root windows, if any, whose wallpaper should have
  // blur animated after Blur or Unblur is called.
  std::vector<aura::Window*> roots_to_animate_;

  DISALLOW_COPY_AND_ASSIGN(OverviewBlurController);
};

WindowSelectorController::WindowSelectorController()
    : overview_blur_controller_(std::make_unique<OverviewBlurController>()) {}

WindowSelectorController::~WindowSelectorController() {
  overview_blur_controller_.reset();

  // Destroy widgets that may be still animating if shell shuts down soon after
  // exiting overview mode.
  for (std::unique_ptr<DelayedAnimationObserver>& animation_observer :
       delayed_animations_) {
    animation_observer->Shutdown();
  }

  if (window_selector_.get()) {
    window_selector_->Shutdown();
    window_selector_.reset();
  }
}

// static
bool WindowSelectorController::CanSelect() {
  // Don't allow a window overview if the user session is not active (e.g.
  // locked or in user-adding screen) or a modal dialog is open or running in
  // kiosk app session.
  SessionController* session_controller = Shell::Get()->session_controller();
  return session_controller->GetSessionState() ==
             session_manager::SessionState::ACTIVE &&
         !Shell::IsSystemModalWindowOpen() &&
         !Shell::Get()->screen_pinning_controller()->IsPinned() &&
         !session_controller->IsRunningInAppMode();
}

bool WindowSelectorController::ToggleOverview() {
  auto windows = Shell::Get()->mru_window_tracker()->BuildMruWindowList();

  // Hidden windows will be removed by ShouldExcludeWindowFromOverview so we
  // must copy them out first.
  std::vector<aura::Window*> hide_windows(windows.size());
  auto end = std::copy_if(windows.begin(), windows.end(), hide_windows.begin(),
                          ShouldHideWindowInOverview);
  hide_windows.resize(end - hide_windows.begin());

  end = std::remove_if(windows.begin(), windows.end(),
                       ShouldExcludeWindowFromOverview);
  windows.resize(end - windows.begin());

  if (IsSelecting()) {
    // Do not allow ending overview if we're in single split mode.
    if (windows.empty() && Shell::Get()->IsSplitViewModeActive())
      return true;
    OnSelectionEnded();
  } else {
    // Don't start overview if window selection is not allowed.
    if (!CanSelect())
      return false;

    window_selector_.reset(new WindowSelector(this));
    Shell::Get()->NotifyOverviewModeStarting();
    window_selector_->Init(windows, hide_windows);
    if (IsBlurEnabled())
      overview_blur_controller_->Blur();
    OnSelectionStarted();
  }
  return true;
}

bool WindowSelectorController::IsSelecting() const {
  return window_selector_.get() != NULL;
}

void WindowSelectorController::IncrementSelection(int increment) {
  DCHECK(IsSelecting());
  window_selector_->IncrementSelection(increment);
}

bool WindowSelectorController::AcceptSelection() {
  DCHECK(IsSelecting());
  return window_selector_->AcceptSelection();
}

bool WindowSelectorController::IsRestoringMinimizedWindows() const {
  return window_selector_.get() != NULL &&
         window_selector_->restoring_minimized_windows();
}

void WindowSelectorController::OnOverviewButtonTrayLongPressed(
    const gfx::Point& event_location) {
  // Do nothing if split view is not enabled.
  if (!SplitViewController::ShouldAllowSplitView())
    return;

  // Depending on the state of the windows and split view, a long press has many
  // different results.
  // 1. Already in split view - exit split view. Activate the left window if it
  // is snapped left or both sides. Activate the right window if it is snapped
  // right.
  // 2. Not in overview mode - enter split view iff
  //     a) there is an active window
  //     b) there are at least two windows in the mru list
  //     c) the active window is snappable
  // 3. In overview mode - enter split view iff
  //     a) there are at least two windows in the mru list
  //     b) the first window in the mru list is snappable

  auto* split_view_controller = Shell::Get()->split_view_controller();
  // Exit split view mode if we are already in it.
  if (split_view_controller->IsSplitViewModeActive()) {
    // In some cases the window returned by wm::GetActiveWindow will be an item
    // in overview mode (maybe the overview mode text selection widget). The
    // active window may also be a transient descendant of the left or right
    // snapped window, in which we want to activate the transient window's
    // ancestor (left or right snapped window). Manually set |active_window| as
    // either the left or right window.
    aura::Window* active_window = wm::GetActiveWindow();
    while (::wm::GetTransientParent(active_window))
      active_window = ::wm::GetTransientParent(active_window);
    if (active_window != split_view_controller->left_window() &&
        active_window != split_view_controller->right_window()) {
      active_window = split_view_controller->GetDefaultSnappedWindow();
    }
    DCHECK(active_window);
    split_view_controller->EndSplitView();
    if (IsSelecting())
      ToggleOverview();
    ::wm::ActivateWindow(active_window);
    base::RecordAction(
        base::UserMetricsAction("Tablet_LongPressOverviewButtonExitSplitView"));
    return;
  }

  WindowSelectorItem* item_to_snap = nullptr;
  if (!IsSelecting()) {
    // The current active window may be a transient child.
    aura::Window* active_window = wm::GetActiveWindow();
    while (active_window && ::wm::GetTransientParent(active_window))
      active_window = ::wm::GetTransientParent(active_window);

    // Do nothing if there are no active windows or less than two windows to
    // work with.
    if (!active_window ||
        Shell::Get()->mru_window_tracker()->BuildWindowForCycleList().size() <
            2u) {
      return;
    }

    // Show a toast if the window cannot be snapped.
    if (!split_view_controller->CanSnap(active_window)) {
      split_view_controller->ShowAppCannotSnapToast();
      return;
    }

    // If we are not in overview mode, enter overview mode and then find the
    // window item to snap.
    ToggleOverview();
    DCHECK(window_selector_);
    WindowGrid* current_grid =
        window_selector_->GetGridWithRootWindow(active_window->GetRootWindow());
    if (current_grid) {
      item_to_snap =
          current_grid->GetWindowSelectorItemContaining(active_window);
    }
  } else {
    // Currently in overview mode, with no snapped windows. Retrieve the first
    // window selector item and attempt to snap that window.
    DCHECK(window_selector_);
    WindowGrid* current_grid = window_selector_->GetGridWithRootWindow(
        wm::GetRootWindowAt(event_location));
    if (current_grid) {
      const auto& windows = current_grid->window_list();
      if (windows.size() > 1)
        item_to_snap = windows[0].get();
    }
  }

  // Do nothing if no item was retrieved, or if the retrieved item is
  // unsnappable.
  // TODO(sammiequon): Bounce the window if it is not snappable.
  if (!item_to_snap ||
      !split_view_controller->CanSnap(item_to_snap->GetWindow())) {
    return;
  }

  split_view_controller->SnapWindow(item_to_snap->GetWindow(),
                                    SplitViewController::LEFT);
  base::RecordAction(
      base::UserMetricsAction("Tablet_LongPressOverviewButtonEnterSplitView"));
}

std::vector<aura::Window*>
WindowSelectorController::GetWindowsListInOverviewGridsForTesting() {
  std::vector<aura::Window*> windows;
  for (const std::unique_ptr<WindowGrid>& grid :
       window_selector_->grid_list_for_testing()) {
    for (const auto& window_selector_item : grid->window_list())
      windows.push_back(window_selector_item->GetWindow());
  }
  return windows;
}

// TODO(flackr): Make WindowSelectorController observe the activation of
// windows, so we can remove WindowSelectorDelegate.
void WindowSelectorController::OnSelectionEnded() {
  if (is_shutting_down_)
    return;

  if (IsBlurEnabled())
    overview_blur_controller_->Unblur();
  is_shutting_down_ = true;
  Shell::Get()->NotifyOverviewModeEnding();
  window_selector_->Shutdown();
  // Don't delete |window_selector_| yet since the stack is still using it.
  base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE,
                                                  window_selector_.release());
  last_selection_time_ = base::Time::Now();
  Shell::Get()->NotifyOverviewModeEnded();
  is_shutting_down_ = false;
}

void WindowSelectorController::AddDelayedAnimationObserver(
    std::unique_ptr<DelayedAnimationObserver> animation_observer) {
  animation_observer->SetOwner(this);
  delayed_animations_.push_back(std::move(animation_observer));
}

void WindowSelectorController::RemoveAndDestroyAnimationObserver(
    DelayedAnimationObserver* animation_observer) {
  class IsEqual {
   public:
    explicit IsEqual(DelayedAnimationObserver* animation_observer)
        : animation_observer_(animation_observer) {}
    bool operator()(const std::unique_ptr<DelayedAnimationObserver>& other) {
      return (other.get() == animation_observer_);
    }

   private:
    const DelayedAnimationObserver* animation_observer_;
  };
  delayed_animations_.erase(
      std::remove_if(delayed_animations_.begin(), delayed_animations_.end(),
                     IsEqual(animation_observer)),
      delayed_animations_.end());
}

void WindowSelectorController::OnSelectionStarted() {
  if (!last_selection_time_.is_null()) {
    UMA_HISTOGRAM_LONG_TIMES("Ash.WindowSelector.TimeBetweenUse",
                             base::Time::Now() - last_selection_time_);
  }
}

}  // namespace ash
