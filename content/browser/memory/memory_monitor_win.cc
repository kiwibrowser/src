// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/memory/memory_monitor_win.h"

#include "base/process/process_metrics.h"

// TODO(chrisha): Implement a mechanism for observing swapping, and updating the
// memory threshold on a per machine basis.

namespace content {

namespace {

const int kKBperMB = 1024;

// A global static instance of the default delegate. Used by default by
// MemoryMonitorWin.
MemoryMonitorDelegate g_memory_monitor_win_delegate;

}  // namespace

// A system is considered 'large memory' if it has more than 1.5GB of system
// memory available for use by the memory manager (not reserved for hardware
// and drivers). This is a fuzzy version of the ~2GB discussed below.
const int MemoryMonitorWin::kLargeMemoryThresholdMB = 1536;

// This is the target free memory used for systems with < ~2GB of physical
// memory. Such systems have been observed to always maintain ~100MB of
// available memory, paging until that is the case. To try to avoid paging a
// threshold slightly above this is chosen.
const int MemoryMonitorWin::kSmallMemoryTargetFreeMB = 200;

// This is the target free memory used for systems with >= ~2GB of physical
// memory. Such systems have been observed to always maintain ~300MB of
// available memory, paging until that is the case.
const int MemoryMonitorWin::kLargeMemoryTargetFreeMB = 400;

MemoryMonitorWin::MemoryMonitorWin(MemoryMonitorDelegate* delegate,
                                   int target_free_mb)
    : delegate_(delegate), target_free_mb_(target_free_mb) {}

int MemoryMonitorWin::GetFreeMemoryUntilCriticalMB() {
  base::SystemMemoryInfoKB mem_info = {};
  delegate_->GetSystemMemoryInfo(&mem_info);
  int free_mb = mem_info.avail_phys / kKBperMB;
  free_mb -= target_free_mb_;
  return free_mb;
}

// static
std::unique_ptr<MemoryMonitorWin> MemoryMonitorWin::Create(
    MemoryMonitorDelegate* delegate) {
  return std::unique_ptr<MemoryMonitorWin>(new MemoryMonitorWin(
      delegate, GetTargetFreeMB(delegate)));
}

// static
bool MemoryMonitorWin::IsLargeMemory(MemoryMonitorDelegate* delegate) {
  base::SystemMemoryInfoKB mem_info = {};
  delegate->GetSystemMemoryInfo(&mem_info);
  return (mem_info.total / kKBperMB) >=
      MemoryMonitorWin::kLargeMemoryThresholdMB;
}

// static
int MemoryMonitorWin::GetTargetFreeMB(MemoryMonitorDelegate* delegate) {
  if (IsLargeMemory(delegate))
    return MemoryMonitorWin::kLargeMemoryTargetFreeMB;
  return MemoryMonitorWin::kSmallMemoryTargetFreeMB;
}

// Implementation of factory function defined in memory_monitor.h.
std::unique_ptr<MemoryMonitor> CreateMemoryMonitor() {
  return MemoryMonitorWin::Create(&g_memory_monitor_win_delegate);
}

}  // namespace content
