// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/arc/boot_phase_monitor/arc_instance_throttle.h"

#include "ash/shell.h"
#include "ash/wm/window_util.h"
#include "components/arc/arc_util.h"
#include "ui/wm/public/activation_client.h"

namespace arc {

namespace {

void ThrottleInstance(aura::Window* active) {
  SetArcCpuRestriction(!IsArcAppWindow(active));
}

}  // namespace

ArcInstanceThrottle::ArcInstanceThrottle() {
  if (!ash::Shell::HasInstance())  // for unit testing.
    return;
  ash::Shell::Get()->activation_client()->AddObserver(this);
  ThrottleInstance(ash::wm::GetActiveWindow());
}

ArcInstanceThrottle::~ArcInstanceThrottle() {
  if (!ash::Shell::HasInstance())
    return;
  ash::Shell::Get()->activation_client()->RemoveObserver(this);
}

void ArcInstanceThrottle::OnWindowActivated(ActivationReason reason,
                                            aura::Window* gained_active,
                                            aura::Window* lost_active) {
  ThrottleInstance(gained_active);
}

}  // namespace arc
