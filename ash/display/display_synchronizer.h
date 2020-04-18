// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_DISPLAY_DISPLAY_SYNCHRONIZER_H_
#define ASH_DISPLAY_DISPLAY_SYNCHRONIZER_H_

#include "ash/display/window_tree_host_manager.h"
#include "base/macros.h"
#include "ui/display/display_observer.h"

namespace aura {
class WindowManagerClient;
}

namespace ash {

// DisplaySynchronizer keeps the display state in mus in sync with ash's display
// state. As ash controls the overall display state this synchronization is one
// way (from ash to mus).
class DisplaySynchronizer : public WindowTreeHostManager::Observer,
                            public display::DisplayObserver {
 public:
  explicit DisplaySynchronizer(
      aura::WindowManagerClient* window_manager_client);
  ~DisplaySynchronizer() override;

 private:
  void SendDisplayConfigurationToServer();

  // WindowTreeHostManager::Observer:
  void OnDisplaysInitialized() override;
  void OnDisplayConfigurationChanged() override;
  void OnWindowTreeHostReusedForDisplay(
      AshWindowTreeHost* window_tree_host,
      const display::Display& display) override;
  void OnWindowTreeHostsSwappedDisplays(AshWindowTreeHost* host1,
                                        AshWindowTreeHost* host2) override;

  // display::DisplayObserver:
  void OnWillProcessDisplayChanges() override;
  void OnDidProcessDisplayChanges() override;
  void OnDisplayMetricsChanged(const display::Display& display,
                               uint32_t changed_metrics) override;

  aura::WindowManagerClient* window_manager_client_;

  bool sent_initial_config_ = false;

  // Set to true when OnWillProcessDisplayChanges() is called and false in
  // OnDidProcessDisplayChanges(). DisplayManager calls out while the list of
  // displays contains both newly added displays and displays that have been
  // removed. This means if we attempt to access the list of displays during
  // this time we may get the wrong state (SendDisplayConfigurationToServer()
  // would send a bogus display to the window server). By only processing the
  // change after DisplayManager has updated its internal state we ensure we
  // don't send a bad config.
  bool processing_display_changes_ = false;

  DISALLOW_COPY_AND_ASSIGN(DisplaySynchronizer);
};

}  // namespace ash

#endif  // ASH_DISPLAY_DISPLAY_SYNCHRONIZER_H_
