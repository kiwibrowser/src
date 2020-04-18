// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/wake_lock/wake_lock_context.h"

#include <utility>

#include "services/device/wake_lock/wake_lock.h"

namespace device {

const int WakeLockContext::WakeLockInvalidContextId = -1;

WakeLockContext::WakeLockContext(
    int context_id,
    scoped_refptr<base::SingleThreadTaskRunner> file_task_runner,
    const WakeLockContextCallback& native_view_getter)
    : file_task_runner_(std::move(file_task_runner)),
      context_id_(context_id),
      native_view_getter_(native_view_getter) {}

WakeLockContext::~WakeLockContext() {}

void WakeLockContext::GetWakeLock(mojom::WakeLockType type,
                                  mojom::WakeLockReason reason,
                                  const std::string& description,
                                  mojom::WakeLockRequest request) {
  // WakeLock owns itself.
  new WakeLock(std::move(request), type, reason, description, context_id_,
               native_view_getter_, file_task_runner_);
}

}  // namespace device
