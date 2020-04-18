// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEMORY_BROWSER_MEMORY_MONITOR_H_
#define CONTENT_BROWSER_MEMORY_BROWSER_MEMORY_MONITOR_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "content/common/content_export.h"

namespace base {
struct SystemMemoryInfoKB;
}

namespace content {

// A simple class that monitors the amount of free memory available on a system.
// This is an interface to facilitate dependency injection for testing.
class CONTENT_EXPORT MemoryMonitor {
 public:
  MemoryMonitor() {}
  virtual ~MemoryMonitor() {}

  // Returns the amount of free memory available on the system until the system
  // will be in a critical state. Critical is as defined by the OS (swapping
  // will occur, or physical memory will run out, etc). It is possible for this
  // to return negative values, in which case that much memory would have to be
  // freed in order to exit a critical memory state.
  virtual int GetFreeMemoryUntilCriticalMB() = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(MemoryMonitor);
};

// Factory function for creating a monitor for the current platform.
CONTENT_EXPORT std::unique_ptr<MemoryMonitor> CreateMemoryMonitor();

// A class for fetching system information used by a memory monitor. This can
// be subclassed for testing or if a particular MemoryMonitor implementation
// needs additional functionality.
class CONTENT_EXPORT MemoryMonitorDelegate {
 public:
  static MemoryMonitorDelegate* GetInstance();

  MemoryMonitorDelegate() {}
  virtual ~MemoryMonitorDelegate();

  // Returns system memory information.
  virtual void GetSystemMemoryInfo(base::SystemMemoryInfoKB* mem_info);

 private:
  friend struct base::DefaultSingletonTraits<MemoryMonitorDelegate>;

  DISALLOW_COPY_AND_ASSIGN(MemoryMonitorDelegate);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEMORY_BROWSER_MEMORY_MONITOR_H_
