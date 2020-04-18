// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/memory/memory_monitor_linux.h"

#include "base/process/process_metrics.h"

namespace content {

namespace {

// The number of bits to shift to convert KiB to MiB.
const int kShiftKiBtoMiB = 10;

}  // namespace

MemoryMonitorLinux::MemoryMonitorLinux(MemoryMonitorDelegate* delegate)
    : delegate_(delegate) {}

MemoryMonitorLinux::~MemoryMonitorLinux() {}

int MemoryMonitorLinux::GetFreeMemoryUntilCriticalMB() {
  base::SystemMemoryInfoKB mem_info = {};
  delegate_->GetSystemMemoryInfo(&mem_info);

  // According to kernel commit 34e431b0ae398fc54ea69ff85ec700722c9da773,
  // "available" is the "amount of memory that is available for a new workload
  // without pushing the system into swap"; return that value if it is valid.
  // Old linux kernels (before 3.14) don't support "available" and show zero
  // instead.
  if (mem_info.available > 0)
    return mem_info.available >> kShiftKiBtoMiB;

  // If there is no "available" value, guess at it based on free memory.
  // Though there will be easily discardable memory (buffers and caches), we
  // don't count them because discarding them will affect the overall
  // performance of the OS.
  return mem_info.free >> kShiftKiBtoMiB;
}

// static
std::unique_ptr<MemoryMonitorLinux> MemoryMonitorLinux::Create(
    MemoryMonitorDelegate* delegate) {
  return std::make_unique<MemoryMonitorLinux>(delegate);
}

// Implementation of factory function defined in memory_monitor.h.
std::unique_ptr<MemoryMonitor> CreateMemoryMonitor() {
  return MemoryMonitorLinux::Create(MemoryMonitorDelegate::GetInstance());
}

}  // namespace content
