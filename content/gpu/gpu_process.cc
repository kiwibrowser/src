// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/gpu/gpu_process.h"

namespace content {

GpuProcess::GpuProcess(base::ThreadPriority io_thread_priority)
    : ChildProcess(io_thread_priority) {}

GpuProcess::~GpuProcess() {
}

}  // namespace content
