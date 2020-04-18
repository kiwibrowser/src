// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_PUBLIC_GLOBAL_STATE_TASK_SCHEDULER_INIT_PARAMS_CALLBACK_H_
#define IOS_WEB_PUBLIC_GLOBAL_STATE_TASK_SCHEDULER_INIT_PARAMS_CALLBACK_H_

#include "base/callback_forward.h"
#include "base/task_scheduler/task_scheduler.h"

namespace web {

// Callback which returns a pointer to InitParams for base::TaskScheduler.
typedef base::OnceCallback<std::unique_ptr<base::TaskScheduler::InitParams>()>
    TaskSchedulerInitParamsCallback;

}  // namespace web

#endif  // IOS_WEB_PUBLIC_GLOBAL_STATE_TASK_SCHEDULER_INIT_PARAMS_CALLBACK_H_
