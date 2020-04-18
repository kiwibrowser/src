// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_OVERVIEW_SCOPED_HIDE_OVERVIEW_WINDOWS_
#define ASH_WM_OVERVIEW_SCOPED_HIDE_OVERVIEW_WINDOWS_

#include <map>
#include <vector>

#include "ash/ash_export.h"
#include "base/macros.h"
#include "ui/aura/window_observer.h"

namespace aura {
class Window;
}  // namespace aura

namespace ash {

// ScopedHideOverviewWindows hides the list of windows in overview mode,
// remembers their visibility and recovers the visibility after overview mode.
class ASH_EXPORT ScopedHideOverviewWindows : public aura::WindowObserver {
 public:
  // |windows| the list of windows to hide in overview mode.
  explicit ScopedHideOverviewWindows(const std::vector<aura::Window*>& windows);
  ~ScopedHideOverviewWindows() override;

  // WindowObserver overrides.
  void OnWindowDestroying(aura::Window* window) override;
  void OnWindowVisibilityChanged(aura::Window* window, bool visible) override;

 private:
  std::map<aura::Window*, bool> window_visibility_;
  DISALLOW_COPY_AND_ASSIGN(ScopedHideOverviewWindows);
};

}  // namespace ash

#endif  // ASH_WM_OVERVIEW_SCOPED_HIDE_OVERVIEW_WINDOWS_