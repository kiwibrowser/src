// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEMORY_BROWSER_MEMORY_MONITOR_WIN_H_
#define CONTENT_BROWSER_MEMORY_BROWSER_MEMORY_MONITOR_WIN_H_

#include "content/browser/memory/memory_monitor.h"

namespace base {
struct SystemMemoryInfoKB;
}  // namespace base

namespace content {

// A memory monitor for the Windows platform. After much experimentation this
// class uses a very simple heuristic to anticipate paging (critical memory
// pressure). When the amount of memory available dips below a provided
// threshold, it is assumed that paging is inevitable.
class CONTENT_EXPORT MemoryMonitorWin : public MemoryMonitor {
 public:
  // Default constants governing the amount of free memory that the memory
  // manager attempts to maintain.
  static const int kLargeMemoryThresholdMB;
  static const int kSmallMemoryTargetFreeMB;
  static const int kLargeMemoryTargetFreeMB;

  MemoryMonitorWin(MemoryMonitorDelegate* delegate, int target_free_mb);
  ~MemoryMonitorWin() override {}

  // MemoryMonitor:
  int GetFreeMemoryUntilCriticalMB() override;

  // Returns the current free memory target.
  int target_free_mb() const { return target_free_mb_; }

  // Factory function. Automatically sizes |target_free_mb| based on the
  // system.
  static std::unique_ptr<MemoryMonitorWin> Create(
      MemoryMonitorDelegate* delegate);

 protected:
  // Determines if the system is in large memory mode. Exposed so that this
  // function can be tested.
  static bool IsLargeMemory(MemoryMonitorDelegate* delegate);

  // Determines the default target free MB value. Exposed so that this function
  // can be tested.
  static int GetTargetFreeMB(MemoryMonitorDelegate* delegate);

 private:
  // The delegate to be used for retrieving system memory information. Used as a
  // testing seam.
  MemoryMonitorDelegate* delegate_;

  // The amount of memory that the memory manager (MM) attempts to keep in a
  // free state. When less than this amount of physical memory is free, it is
  // assumed that the MM will start paging things out.
  int target_free_mb_;
};

// A delegate that wraps functions used by MemoryMonitorWin. Used as a testing
// seam.
class CONTENT_EXPORT MemoryMonitorWinDelegate {
 public:
  MemoryMonitorWinDelegate() {}
  virtual ~MemoryMonitorWinDelegate() {}

  // Returns system memory information.
  virtual void GetSystemMemoryInfo(base::SystemMemoryInfoKB* mem_info) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(MemoryMonitorWinDelegate);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEMORY_BROWSER_MEMORY_MONITOR_WIN_H_
