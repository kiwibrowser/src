// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_IMMERSIVE_FOCUS_WATCHER_CLASSIC_H_
#define ASH_WM_IMMERSIVE_FOCUS_WATCHER_CLASSIC_H_

#include "ash/ash_export.h"
#include "ash/public/cpp/immersive/immersive_focus_watcher.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/widget/widget_observer.h"
#include "ui/wm/core/transient_window_observer.h"

namespace ash {

class ImmersiveFullscreenController;
class ImmersiveRevealedLock;

// ImmersiveFocusWatcher is responsible for grabbing a reveal lock based on
// activation and/or focus. This implementation grabs a lock if views focus is
// in the top view, or a bubble is showing that is anchored to the top view.
class ASH_EXPORT ImmersiveFocusWatcherClassic
    : public ImmersiveFocusWatcher,
      public views::FocusChangeListener,
      public views::WidgetObserver,
      public ::wm::TransientWindowObserver {
 public:
  explicit ImmersiveFocusWatcherClassic(
      ImmersiveFullscreenController* controller);
  ~ImmersiveFocusWatcherClassic() override;

  // ImmersiveFocusWatcher:
  void UpdateFocusRevealedLock() override;
  void ReleaseLock() override;

 private:
  class BubbleObserver;

  views::Widget* GetWidget();
  aura::Window* GetWidgetWindow();

  // Recreate |bubble_observer_| and start observing any bubbles anchored to a
  // child of |top_container_|.
  void RecreateBubbleObserver();

  // views::FocusChangeObserver overrides:
  void OnWillChangeFocus(views::View* focused_before,
                         views::View* focused_now) override;
  void OnDidChangeFocus(views::View* focused_before,
                        views::View* focused_now) override;

  // views::WidgetObserver overrides:
  void OnWidgetActivationChanged(views::Widget* widget, bool active) override;

  // ::wm::TransientWindowObserver overrides:
  void OnTransientChildAdded(aura::Window* window,
                             aura::Window* transient) override;
  void OnTransientChildRemoved(aura::Window* window,
                               aura::Window* transient) override;

  ImmersiveFullscreenController* immersive_fullscreen_controller_;

  // Lock which keeps the top-of-window views revealed based on the focused view
  // and the active widget. Acquiring the lock never triggers a reveal because
  // a view is not focusable till a reveal has made it visible.
  std::unique_ptr<ImmersiveRevealedLock> lock_;

  // Manages bubbles which are anchored to a child of
  // |ImmersiveFullscreenController::top_container_|.
  std::unique_ptr<BubbleObserver> bubble_observer_;

  DISALLOW_COPY_AND_ASSIGN(ImmersiveFocusWatcherClassic);
};

}  // namespace ash

#endif  // ASH_WM_IMMERSIVE_FOCUS_WATCHER_CLASSIC_H_
