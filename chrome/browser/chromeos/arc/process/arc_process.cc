// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/arc/process/arc_process.h"

#include <utility>

#include "base/strings/string_util.h"

namespace arc {

constexpr char kCloudDpcrocessName[] =
    "com.google.android.apps.work.clouddpc.arc";

ArcProcess::ArcProcess(base::ProcessId nspid,
                       base::ProcessId pid,
                       const std::string& process_name,
                       mojom::ProcessState process_state,
                       bool is_focused,
                       int64_t last_activity_time)
    : nspid_(nspid),
      pid_(pid),
      process_name_(process_name),
      process_state_(process_state),
      is_focused_(is_focused),
      last_activity_time_(last_activity_time) {}

ArcProcess::~ArcProcess() = default;

// Sort by (process_state, last_activity_time) pair.
// Smaller process_state value means higher priority as defined in Android.
// Larger last_activity_time means more recently used.
bool ArcProcess::operator<(const ArcProcess& rhs) const {
  return std::make_pair(process_state(), -last_activity_time()) <
         std::make_pair(rhs.process_state(), -rhs.last_activity_time());
}

ArcProcess::ArcProcess(ArcProcess&& other) = default;
ArcProcess& ArcProcess::operator=(ArcProcess&& other) = default;

bool ArcProcess::IsImportant() const {
  return process_state() <= mojom::ProcessState::IMPORTANT_FOREGROUND ||
         IsArcProtected();
}

bool ArcProcess::IsKernelKillable() const {
  // Protect PERSISTENT, PERSISTENT_UI, our HOME and custom set of ARC processes
  // since they should never be killed even by the kernel. Returning false for
  // them allows their OOM adjustment scores to remain negative.
  return process_state() > arc::mojom::ProcessState::PERSISTENT_UI &&
         !IsArcProtected();
}

bool ArcProcess::IsArcProtected() const {
  return process_name() == kCloudDpcrocessName;
}

std::ostream& operator<<(std::ostream& out, const ArcProcess& arc_process) {
  out << "process_name: " << arc_process.process_name()
      << ", pid: " << arc_process.pid()
      << ", process_state: " << arc_process.process_state()
      << ", is_focused: " << arc_process.is_focused()
      << ", last_activity_time: " << arc_process.last_activity_time()
      << ", packages: " << base::JoinString(arc_process.packages(), ",");
  return out;
}

}  // namespace arc
