// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_DISPLAY_PERSISTENT_WINDOW_CONTROLLER_H_
#define ASH_DISPLAY_PERSISTENT_WINDOW_CONTROLLER_H_

#include "ash/ash_export.h"
#include "ash/display/window_tree_host_manager.h"
#include "ash/session/session_observer.h"
#include "base/callback.h"
#include "base/macros.h"
#include "ui/display/display_observer.h"

namespace ash {

// Observes display changes and saves/restores window bounds persistently in
// multi-displays scenario.
class ASH_EXPORT PersistentWindowController
    : public display::DisplayObserver,
      public SessionObserver,
      public WindowTreeHostManager::Observer {
 public:
  // Public so it can be used by unit tests.
  constexpr static char kNumOfWindowsRestoredHistogramName[] =
      "Ash.PersistentWindow.NumOfWindowsRestored";

  PersistentWindowController();
  ~PersistentWindowController() override;

 private:
  // display::DisplayObserver:
  void OnWillProcessDisplayChanges() override;
  void OnDisplayAdded(const display::Display& new_display) override;

  // SessionObserver:
  void OnSessionStateChanged(session_manager::SessionState state) override;

  // WindowTreeHostManager::Observer:
  void OnDisplayConfigurationChanged() override;

  // Called when restoring persistent window placement is wanted.
  void MaybeRestorePersistentWindowBounds();

  // Callback binded on display added and run on display configuration changed.
  base::OnceCallback<void()> restore_callback_;

  DISALLOW_COPY_AND_ASSIGN(PersistentWindowController);
};

}  // namespace ash

#endif  // ASH_DISPLAY_PERSISTENT_WINDOW_CONTROLLER_H_
