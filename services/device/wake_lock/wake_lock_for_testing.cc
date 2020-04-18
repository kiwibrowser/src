// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/wake_lock/wake_lock_for_testing.h"

#include <utility>

namespace device {

WakeLockForTesting::WakeLockForTesting(
    mojom::WakeLockRequest request,
    mojom::WakeLockType type,
    mojom::WakeLockReason reason,
    const std::string& description,
    int context_id,
    WakeLockContextCallback native_view_getter,
    scoped_refptr<base::SingleThreadTaskRunner> file_task_runner)
    : WakeLock(std::move(request),
               type,
               reason,
               description,
               context_id,
               native_view_getter,
               file_task_runner),
      has_wake_lock_(false) {}

WakeLockForTesting::~WakeLockForTesting() {}

void WakeLockForTesting::HasWakeLockForTests(
    HasWakeLockForTestsCallback callback) {
  std::move(callback).Run(has_wake_lock_);
}

void WakeLockForTesting::UpdateWakeLock() {
  DCHECK(num_lock_requests_ >= 0);

  if (num_lock_requests_) {
    if (!has_wake_lock_)
      CreateWakeLock();
  } else {
    if (has_wake_lock_)
      RemoveWakeLock();
  }
}

void WakeLockForTesting::CreateWakeLock() {
  DCHECK(!has_wake_lock_);
  has_wake_lock_ = true;
}

void WakeLockForTesting::RemoveWakeLock() {
  DCHECK(has_wake_lock_);
  has_wake_lock_ = false;
}

void WakeLockForTesting::SwapWakeLock() {
  DCHECK(has_wake_lock_);
}

}  // namespace device
