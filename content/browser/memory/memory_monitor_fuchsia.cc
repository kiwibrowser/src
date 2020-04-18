// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/memory/memory_monitor.h"

#include "base/logging.h"

namespace content {

// TODO(crbug.com/707031): Implement this.
std::unique_ptr<MemoryMonitor> CreateMemoryMonitor() {
  NOTIMPLEMENTED();
  return nullptr;
}

}  // namespace content
