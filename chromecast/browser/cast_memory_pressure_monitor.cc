// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/browser/cast_memory_pressure_monitor.h"

#include <algorithm>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/location.h"
#include "base/metrics/histogram_macros.h"
#include "base/process/process_metrics.h"
#include "base/strings/string_number_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chromecast/base/chromecast_switches.h"
#include "chromecast/base/metrics/cast_metrics_helper.h"

namespace chromecast {
namespace {

// Memory thresholds (as fraction of total memory) for memory pressure levels.
// See more detailed description of pressure heuristic in PollPressureLevel.
// TODO(halliwell): tune thresholds based on data.
constexpr float kCriticalMemoryFraction = 0.25f;
constexpr float kModerateMemoryFraction = 0.4f;

// Memory thresholds in MB for the simple heuristic based on 'free' memory.
constexpr int kCriticalFreeMemoryKB = 20 * 1024;
constexpr int kModerateFreeMemoryKB = 30 * 1024;

constexpr int kPollingIntervalMS = 5000;

int GetSystemReservedKb() {
  int rtn_kb_ = 0;
  const base::CommandLine* command_line(base::CommandLine::ForCurrentProcess());
  base::StringToInt(
      command_line->GetSwitchValueASCII(switches::kMemPressureSystemReservedKb),
      &rtn_kb_);
  DCHECK(rtn_kb_ >= 0);
  return std::max(rtn_kb_, 0);
}

}  // namespace

CastMemoryPressureMonitor::CastMemoryPressureMonitor()
    : current_level_(base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_NONE),
      system_reserved_kb_(GetSystemReservedKb()),
      dispatch_callback_(
          base::Bind(&base::MemoryPressureListener::NotifyMemoryPressure)),
      weak_ptr_factory_(this) {
  PollPressureLevel();
}

CastMemoryPressureMonitor::~CastMemoryPressureMonitor() {}

CastMemoryPressureMonitor::MemoryPressureLevel
CastMemoryPressureMonitor::GetCurrentPressureLevel() {
  return current_level_;
}

void CastMemoryPressureMonitor::PollPressureLevel() {
  MemoryPressureLevel level =
      base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_NONE;

  base::SystemMemoryInfoKB info;
  if (!base::GetSystemMemoryInfo(&info)) {
    LOG(ERROR) << "GetSystemMemoryInfo failed";
  } else if (system_reserved_kb_ != 0 || info.available != 0) {
    // Preferred memory pressure heuristic:
    // 1. Use /proc/meminfo's MemAvailable if possible, fall back to estimate
    // of free + buffers + cached otherwise.
    const int total_available =
        (info.available != 0) ? info.available
        : (info.free + info.buffers + info.cached);

    // 2. Allow some memory to be 'reserved' on command line.
    const int available = total_available - system_reserved_kb_;
    const int total = info.total - system_reserved_kb_;
    DCHECK_GT(total, 0);
    const float ratio = available / static_cast<float>(total);

    if (ratio < kCriticalMemoryFraction)
      level = base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_CRITICAL;
    else if (ratio < kModerateMemoryFraction)
      level = base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_MODERATE;
  } else {
    // Backup method purely using 'free' memory.  It may generate more
    // pressure events than necessary, since more memory may actually be free.
    if (info.free < kCriticalFreeMemoryKB)
      level = base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_CRITICAL;
    else if (info.free < kModerateFreeMemoryKB)
      level = base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_MODERATE;
  }

  UpdateMemoryPressureLevel(level);

  UMA_HISTOGRAM_PERCENTAGE("Platform.MeminfoMemFree",
                           (info.free * 100.0) / info.total);
  UMA_HISTOGRAM_CUSTOM_COUNTS("Platform.Cast.MeminfoMemFreeDerived",
                              (info.free + info.buffers + info.cached) / 1024,
                              1, 500, 100);
  if (info.available != 0) {
    UMA_HISTOGRAM_CUSTOM_COUNTS(
        "Platform.Cast.MeminfoMemAvailable",
        info.available / 1024,
        1, 500, 100);
  }

  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&CastMemoryPressureMonitor::PollPressureLevel,
                     weak_ptr_factory_.GetWeakPtr()),
      base::TimeDelta::FromMilliseconds(kPollingIntervalMS));
}

void CastMemoryPressureMonitor::UpdateMemoryPressureLevel(
    MemoryPressureLevel new_level) {
  if (new_level != base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_NONE)
    dispatch_callback_.Run(new_level);

  if (new_level == current_level_)
    return;

  current_level_ = new_level;
  metrics::CastMetricsHelper::GetInstance()->RecordApplicationEventWithValue(
      "Memory.Pressure.LevelChange", new_level);
}

void CastMemoryPressureMonitor::SetDispatchCallback(
    const DispatchCallback& callback) {
  dispatch_callback_ = callback;
}

}  // namespace chromecast
