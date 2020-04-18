// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_MRU_WINDOW_TRACKER_H_
#define ASH_WM_MRU_WINDOW_TRACKER_H_

#include <list>
#include <vector>

#include "ash/ash_export.h"
#include "base/macros.h"
#include "ui/aura/window_observer.h"
#include "ui/wm/public/activation_change_observer.h"

namespace ash {

// Maintains a most recently used list of windows. This is used for window
// cycling using Alt+Tab and overview mode.
class ASH_EXPORT MruWindowTracker : public ::wm::ActivationChangeObserver,
                                    public aura::WindowObserver {
 public:
  using WindowList = std::vector<aura::Window*>;

  MruWindowTracker();
  ~MruWindowTracker() override;

  // Returns the set of windows which can be cycled through using the tracked
  // list of most recently used windows.
  WindowList BuildMruWindowList() const;

  // This does the same thing as the above, but ignores the system modal dialog
  // state and hence the returned list could contain more windows if a system
  // modal dialog window is present.
  WindowList BuildWindowListIgnoreModal() const;

  // This does the same thing as |BuildMruWindowList()| but with some
  // exclusions. This list is used for cycling through by the keyboard via
  // alt-tab.
  WindowList BuildWindowForCycleList() const;

  // Starts or stops ignoring window activations. If no longer ignoring
  // activations the currently active window is moved to the front of the
  // MRU window list. Used by WindowCycleList to avoid adding all cycled
  // windows to the front of the MRU window list.
  void SetIgnoreActivations(bool ignore);

 private:
  // Updates the mru_windows_ list to insert/move |active_window| at/to the
  // front.
  void SetActiveWindow(aura::Window* active_window);

  // Overridden from wm::ActivationChangeObserver:
  void OnWindowActivated(ActivationReason reason,
                         aura::Window* gained_active,
                         aura::Window* lost_active) override;

  // Overridden from aura::WindowObserver:
  void OnWindowDestroyed(aura::Window* window) override;

  // List of windows that have been activated in containers that we cycle
  // through, sorted by most recently used.
  std::list<aura::Window*> mru_windows_;

  bool ignore_window_activations_;

  DISALLOW_COPY_AND_ASSIGN(MruWindowTracker);
};

}  // namespace ash

#endif  // ASH_WM_MRU_WINDOW_TRACKER_H_
