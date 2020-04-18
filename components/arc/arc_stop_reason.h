// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_ARC_STOP_REASON_H_
#define COMPONENTS_ARC_ARC_STOP_REASON_H_

#include <ostream>

namespace arc {

// Describes the reason the ARC instance is stopped.
enum class ArcStopReason {
  // ARC instance has been gracefully shut down.
  SHUTDOWN,

  // Errors occurred during the ARC instance boot. This includes any failures
  // before the instance is actually attempted to be started (e.g.
  // session_manager failed to fork/exec the instance), and also a "clean"
  // (non crashy) but unexpected container shutdown.
  GENERIC_BOOT_FAILURE,

  // The device is critically low on disk space.
  LOW_DISK_SPACE,

  // ARC instance has crashed.
  CRASH,
};

// Defines "<<" operator for LOGging purpose.
std::ostream& operator<<(std::ostream& os, ArcStopReason reason);

}  // namespace arc

#endif  // COMPONENTS_ARC_ARC_STOP_REASON_H_
