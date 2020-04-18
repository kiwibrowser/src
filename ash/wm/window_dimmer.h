// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_WINDOW_DIMMER_H_
#define ASH_WM_WINDOW_DIMMER_H_

#include "ash/ash_export.h"
#include "base/macros.h"
#include "ui/aura/window_observer.h"

namespace ash {

// WindowDimmer creates a window whose opacity is animated by way of
// SetDimOpacity() and whose size matches that of its parent. WindowDimmer is
// intended to be used in cases where a certain set of windows need to appear
// partially obscured. This is achieved by creating WindowDimmer, setting the
// opacity, and then stacking window() above the windows that are to appear
// obscured.
//
// WindowDimmer owns the window it creates, but supports having that window
// deleted out from under it (this generally happens if the parent of the
// window is deleted). If WindowDimmer is deleted and the window it created is
// still valid, then WindowDimmer deletes the window.
class ASH_EXPORT WindowDimmer : public aura::WindowObserver {
 public:
  // Creates a new WindowDimmer. The window() created by WindowDimmer is added
  // to |parent| and stacked above all other child windows.
  explicit WindowDimmer(aura::Window* parent);
  ~WindowDimmer() override;

  void SetDimOpacity(float target_opacity);

  aura::Window* parent() { return parent_; }
  aura::Window* window() { return window_; }

  // NOTE: WindowDimmer is an observer for both |parent_| and |window_|.
  // aura::WindowObserver:
  void OnWindowBoundsChanged(aura::Window* window,
                             const gfx::Rect& old_bounds,
                             const gfx::Rect& new_bounds,
                             ui::PropertyChangeReason reason) override;
  void OnWindowDestroying(aura::Window* window) override;
  void OnWindowHierarchyChanging(const HierarchyChangeParams& params) override;

 private:
  aura::Window* parent_;
  // See class description for details on ownership.
  aura::Window* window_;

  DISALLOW_COPY_AND_ASSIGN(WindowDimmer);
};

}  // namespace ash

#endif  // ASH_WM_WINDOW_DIMMER_H_
