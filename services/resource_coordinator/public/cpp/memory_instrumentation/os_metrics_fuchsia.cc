// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/public/cpp/memory_instrumentation/os_metrics.h"

#include <vector>

namespace memory_instrumentation {

// static
bool OSMetrics::FillOSMemoryDump(base::ProcessId pid,
                                 mojom::RawOSMemDump* dump) {
  // TODO(fuchsia): Implement this. See crbug.com/750948
  NOTIMPLEMENTED();
  return false;
}

// static
std::vector<mojom::VmRegionPtr> OSMetrics::GetProcessMemoryMaps(
    base::ProcessId) {
  // TODO(fuchsia): Implement this. See crbug.com/750948
  NOTIMPLEMENTED();
  return std::vector<mojom::VmRegionPtr>();
}

}  // namespace memory_instrumentation
