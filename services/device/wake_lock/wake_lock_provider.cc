// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/wake_lock/wake_lock_provider.h"

#include <utility>

#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/device/wake_lock/wake_lock.h"
#include "services/device/wake_lock/wake_lock_for_testing.h"

namespace device {

bool WakeLockProvider::is_in_unittest_ = false;

// static
void WakeLockProvider::Create(
    mojom::WakeLockProviderRequest request,
    scoped_refptr<base::SingleThreadTaskRunner> file_task_runner,
    const WakeLockContextCallback& native_view_getter) {
  mojo::MakeStrongBinding(std::make_unique<WakeLockProvider>(
                              std::move(file_task_runner), native_view_getter),
                          std::move(request));
}

WakeLockProvider::WakeLockProvider(
    scoped_refptr<base::SingleThreadTaskRunner> file_task_runner,
    const WakeLockContextCallback& native_view_getter)
    : file_task_runner_(std::move(file_task_runner)),
      native_view_getter_(native_view_getter) {}

WakeLockProvider::~WakeLockProvider() {}

void WakeLockProvider::GetWakeLockContextForID(
    int context_id,
    mojo::InterfaceRequest<mojom::WakeLockContext> request) {
  DCHECK(context_id >= 0);
  mojo::MakeStrongBinding(
      std::make_unique<WakeLockContext>(context_id, file_task_runner_,
                                        native_view_getter_),
      std::move(request));
}

void WakeLockProvider::GetWakeLockWithoutContext(
    mojom::WakeLockType type,
    mojom::WakeLockReason reason,
    const std::string& description,
    mojom::WakeLockRequest request) {
  if (is_in_unittest_) {
    // Create WakeLockForTesting.
    new WakeLockForTesting(std::move(request), type, reason, description,
                           WakeLockContext::WakeLockInvalidContextId,
                           native_view_getter_, file_task_runner_);
  } else {
    // WakeLock owns itself.
    new WakeLock(std::move(request), type, reason, description,
                 WakeLockContext::WakeLockInvalidContextId, native_view_getter_,
                 file_task_runner_);
  }
}

}  // namespace device
