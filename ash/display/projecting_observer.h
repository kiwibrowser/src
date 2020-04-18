// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_DISPLAY_PROJECTING_OBSERVER_H_
#define ASH_DISPLAY_PROJECTING_OBSERVER_H_

#include "ash/ash_export.h"
#include "ash/shell_observer.h"
#include "base/macros.h"
#include "ui/display/manager/display_configurator.h"

namespace chromeos {
class PowerManagerClient;
}

namespace ash {

class ASH_EXPORT ProjectingObserver
    : public display::DisplayConfigurator::Observer,
      public ShellObserver {
 public:
  // |display_configurator| must outlive this instance. May be null in tests.
  explicit ProjectingObserver(
      display::DisplayConfigurator* display_configurator);
  ~ProjectingObserver() override;

  // DisplayConfigurator::Observer implementation:
  void OnDisplayModeChanged(
      const display::DisplayConfigurator::DisplayStateList& outputs) override;

  // ash::ShellObserver implementation:
  void OnCastingSessionStartedOrStopped(bool started) override;

 private:
  friend class ProjectingObserverTest;

  void set_power_manager_client_for_test(
      chromeos::PowerManagerClient* power_manager_client) {
    power_manager_client_for_test_ = power_manager_client;
  }

  // Sends the current projecting state to power manager.
  void SetIsProjecting();

  display::DisplayConfigurator* display_configurator_;  // Unowned

  // True if at least one output is internal. This value is updated when
  // |OnDisplayModeChanged| is called.
  bool has_internal_output_ = false;

  // Keeps track of the number of connected outputs.
  int output_count_ = 0;

  // Number of outstanding casting sessions.
  int casting_session_count_ = 0;

  // Weak pointer to the DBusClient PowerManagerClient for testing;
  chromeos::PowerManagerClient* power_manager_client_for_test_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(ProjectingObserver);
};

}  // namespace ash

#endif  // ASH_DISPLAY_PROJECTING_OBSERVER_H_
