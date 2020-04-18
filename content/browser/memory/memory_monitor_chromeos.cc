// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/memory/memory_monitor_chromeos.h"

#include "base/process/process_metrics.h"

namespace content {

namespace {

// The number of bits to shift to convert KiB to MiB.
const int kShiftKiBtoMiB = 10;

}  // namespace

MemoryMonitorChromeOS::MemoryMonitorChromeOS(MemoryMonitorDelegate* delegate)
    : delegate_(delegate) {}

MemoryMonitorChromeOS::~MemoryMonitorChromeOS() {}

int MemoryMonitorChromeOS::GetFreeMemoryUntilCriticalMB() {
  base::SystemMemoryInfoKB mem_info = {};
  delegate_->GetSystemMemoryInfo(&mem_info);

  // Use available free memory provided by the OS if the OS supports it.
  if (mem_info.available > 0)
    return mem_info.available >> kShiftKiBtoMiB;

  // The kernel internally uses 50MB.
  const int kMinFileMemory = 50 * 1024;

  // Most file memory can be easily reclaimed.
  int file_memory = mem_info.active_file + mem_info.inactive_file;
  // unless it is dirty or it's a minimal portion which is required.
  file_memory -= mem_info.dirty + kMinFileMemory;

  // Available memory is the sum of free and easy reclaimable memory.
  return (mem_info.free + file_memory) >> kShiftKiBtoMiB;
}

// static
std::unique_ptr<MemoryMonitorChromeOS> MemoryMonitorChromeOS::Create(
    MemoryMonitorDelegate* delegate) {
  return std::make_unique<MemoryMonitorChromeOS>(delegate);
}

// Implementation of factory function defined in memory_monitor.h.
std::unique_ptr<MemoryMonitor> CreateMemoryMonitor() {
  return MemoryMonitorChromeOS::Create(MemoryMonitorDelegate::GetInstance());
}

}  // namespace content
