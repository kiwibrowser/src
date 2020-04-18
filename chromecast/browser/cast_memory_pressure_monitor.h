// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_BROWSER_CAST_MEMORY_PRESSURE_MONITOR_H_
#define CHROMECAST_BROWSER_CAST_MEMORY_PRESSURE_MONITOR_H_

#include "base/macros.h"
#include "base/memory/memory_pressure_monitor.h"
#include "base/memory/weak_ptr.h"

namespace chromecast {

// Memory pressure monitor for Cast: polls for current memory
// usage periodically and sends memory pressure notifications.
class CastMemoryPressureMonitor : public base::MemoryPressureMonitor {
 public:
  CastMemoryPressureMonitor();
  ~CastMemoryPressureMonitor() override;

  // base::MemoryPressureMonitor implementation:
  MemoryPressureLevel GetCurrentPressureLevel() override;
  void SetDispatchCallback(const DispatchCallback& callback) override;

 private:
  void PollPressureLevel();
  void UpdateMemoryPressureLevel(MemoryPressureLevel new_level);

  MemoryPressureLevel current_level_;
  const int system_reserved_kb_;
  DispatchCallback dispatch_callback_;
  base::WeakPtrFactory<CastMemoryPressureMonitor> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(CastMemoryPressureMonitor);
};

}  // namespace chromecast

#endif  // CHROMECAST_BROWSER_CAST_MEMORY_PRESSURE_MONITOR_H_
