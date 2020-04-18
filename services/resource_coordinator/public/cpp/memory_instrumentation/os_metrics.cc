// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/public/cpp/memory_instrumentation/os_metrics.h"

#include "build/build_config.h"

namespace memory_instrumentation {

// static
bool OSMetrics::FillProcessMemoryMaps(base::ProcessId pid,
                                      mojom::MemoryMapOption mmap_option,
                                      mojom::RawOSMemDump* dump) {
  DCHECK_NE(mmap_option, mojom::MemoryMapOption::NONE);

  std::vector<mojom::VmRegionPtr> results;

#if defined(OS_MACOSX)
  // TODO: Consider implementing this optimization for other platforms as well,
  // if performance becomes an issue. https://crbug.com/826913.
  if (mmap_option == mojom::MemoryMapOption::MODULES)
    results = GetProcessModules(pid);
#endif

  if (results.empty())
    results = GetProcessMemoryMaps(pid);

  if (results.empty())
    return false;

  dump->memory_maps = std::move(results);

  return true;
}

}  // namespace memory_instrumentation
