// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/memory/test_memory_monitor.h"

namespace content {

TestMemoryMonitorDelegate::~TestMemoryMonitorDelegate() {}

void TestMemoryMonitorDelegate::GetSystemMemoryInfo(
    base::SystemMemoryInfoKB* mem_info) {
  *mem_info = mem_info_;
  ++calls_;
}

}  // namespace content
