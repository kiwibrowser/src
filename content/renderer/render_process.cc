// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "content/renderer/render_process.h"

namespace content {

RenderProcess::RenderProcess(
    const std::string& task_scheduler_name,
    std::unique_ptr<base::TaskScheduler::InitParams> task_scheduler_init_params)
    : ChildProcess(base::ThreadPriority::NORMAL,
                   task_scheduler_name,
                   std::move(task_scheduler_init_params)) {}

}  // namespace content
