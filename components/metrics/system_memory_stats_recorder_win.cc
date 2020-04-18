// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/metrics/system_memory_stats_recorder.h"

#include <windows.h>

#include "base/metrics/histogram_macros.h"
#include "base/process/process_metrics.h"

namespace metrics {
namespace {
enum { kMBytes = 1024 * 1024 };
}

void RecordMemoryStats(RecordMemoryStatsType type) {
  MEMORYSTATUSEX mem_status;
  mem_status.dwLength = sizeof(mem_status);
  if (!::GlobalMemoryStatusEx(&mem_status))
    return;

  switch (type) {
    case RECORD_MEMORY_STATS_TAB_DISCARDED: {
      UMA_HISTOGRAM_CUSTOM_COUNTS("Memory.Stats.Win.MemoryLoad",
                                  mem_status.dwMemoryLoad, 1, 100, 101);
      UMA_HISTOGRAM_LARGE_MEMORY_MB("Memory.Stats.Win.TotalPhys2",
                                    mem_status.ullTotalPhys / kMBytes);
      UMA_HISTOGRAM_LARGE_MEMORY_MB("Memory.Stats.Win.AvailPhys2",
                                    mem_status.ullAvailPhys / kMBytes);
      UMA_HISTOGRAM_LARGE_MEMORY_MB("Memory.Stats.Win.TotalPageFile2",
                                    mem_status.ullTotalPageFile / kMBytes);
      UMA_HISTOGRAM_LARGE_MEMORY_MB("Memory.Stats.Win.AvailPageFile2",
                                    mem_status.ullAvailPageFile / kMBytes);
      UMA_HISTOGRAM_LARGE_MEMORY_MB("Memory.Stats.Win.TotalVirtual2",
                                    mem_status.ullTotalVirtual / kMBytes);
      UMA_HISTOGRAM_LARGE_MEMORY_MB("Memory.Stats.Win.AvailVirtual2",
                                    mem_status.ullAvailVirtual / kMBytes);
      break;
    }
    default:
      NOTREACHED() << "Received unexpected notification";
      break;
  }
}

}  // namespace metrics
