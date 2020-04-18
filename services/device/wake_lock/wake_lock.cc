// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/wake_lock/wake_lock.h"

#include <utility>


namespace device {

WakeLock::WakeLock(mojom::WakeLockRequest request,
                   mojom::WakeLockType type,
                   mojom::WakeLockReason reason,
                   const std::string& description,
                   int context_id,
                   WakeLockContextCallback native_view_getter,
                   scoped_refptr<base::SingleThreadTaskRunner> file_task_runner)
    : num_lock_requests_(0),
      type_(type),
      reason_(reason),
      description_(std::make_unique<std::string>(description)),
#if defined(OS_ANDROID)
      context_id_(context_id),
      native_view_getter_(native_view_getter),
#endif
      main_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      file_task_runner_(std::move(file_task_runner)) {
  AddClient(std::move(request));
  binding_set_.set_connection_error_handler(
      base::Bind(&WakeLock::OnConnectionError, base::Unretained(this)));
}

WakeLock::~WakeLock() {}

void WakeLock::AddClient(mojom::WakeLockRequest request) {
  DCHECK(main_task_runner_->RunsTasksInCurrentSequence());
  binding_set_.AddBinding(this, std::move(request),
                          std::make_unique<bool>(false));
}

void WakeLock::RequestWakeLock() {
  DCHECK(main_task_runner_->RunsTasksInCurrentSequence());
  DCHECK(binding_set_.dispatch_context());

  // Uses the Context to get the outstanding status of current binding.
  // Two consecutive requests from the same client should be coalesced
  // as one request.
  if (*binding_set_.dispatch_context())
    return;

  *binding_set_.dispatch_context() = true;
  num_lock_requests_++;
  UpdateWakeLock();
}

void WakeLock::CancelWakeLock() {
  DCHECK(main_task_runner_->RunsTasksInCurrentSequence());
  DCHECK(binding_set_.dispatch_context());

  if (!(*binding_set_.dispatch_context()))
    return;

  DCHECK(num_lock_requests_ > 0);
  *binding_set_.dispatch_context() = false;
  num_lock_requests_--;
  UpdateWakeLock();
}

void WakeLock::ChangeType(mojom::WakeLockType type,
                          ChangeTypeCallback callback) {
  DCHECK(main_task_runner_->RunsTasksInCurrentSequence());

#if defined(OS_ANDROID)
  LOG(ERROR) << "WakeLock::ChangeType() has no effect on Android.";
  std::move(callback).Run(false);
  return;
#endif
  if (binding_set_.size() > 1) {
    LOG(ERROR) << "WakeLock::ChangeType() is not allowed when the current wake "
                  "lock is shared by more than one clients.";
    std::move(callback).Run(false);
    return;
  }

  mojom::WakeLockType old_type = type_;
  type_ = type;

  if (type_ != old_type && wake_lock_)
    SwapWakeLock();

  std::move(callback).Run(true);
}

void WakeLock::HasWakeLockForTests(HasWakeLockForTestsCallback callback) {
  std::move(callback).Run(!!wake_lock_);
}

void WakeLock::UpdateWakeLock() {
  DCHECK(num_lock_requests_ >= 0);

  if (num_lock_requests_) {
    if (!wake_lock_)
      CreateWakeLock();
  } else {
    if (wake_lock_)
      RemoveWakeLock();
  }
}

void WakeLock::CreateWakeLock() {
  DCHECK(!wake_lock_);

  wake_lock_ = std::make_unique<PowerSaveBlocker>(
      type_, reason_, *description_, main_task_runner_, file_task_runner_);

  if (type_ != mojom::WakeLockType::kPreventDisplaySleep)
    return;

#if defined(OS_ANDROID)
  if (context_id_ == WakeLockContext::WakeLockInvalidContextId) {
    LOG(ERROR) << "Client must pass a valid context_id when requests wake lock "
                  "on Android.";
    return;
  }

  gfx::NativeView native_view = native_view_getter_.Run(context_id_);
  if (native_view)
    wake_lock_.get()->InitDisplaySleepBlocker(native_view);
#endif
}

void WakeLock::RemoveWakeLock() {
  DCHECK(wake_lock_);
  wake_lock_.reset();
}

void WakeLock::SwapWakeLock() {
  DCHECK(wake_lock_);

  auto new_wake_lock = std::make_unique<PowerSaveBlocker>(
      type_, reason_, *description_, main_task_runner_, file_task_runner_);

  // Do a swap to ensure that there isn't a brief period where the old
  // PowerSaveBlocker is unblocked while the new PowerSaveBlocker is not
  // created.
  wake_lock_.swap(new_wake_lock);
}

void WakeLock::OnConnectionError() {
  // If this client has an outstanding wake lock request, decrease the
  // num_lock_requests and call UpdateWakeLock().
  if (*binding_set_.dispatch_context() && num_lock_requests_ > 0) {
    num_lock_requests_--;
    UpdateWakeLock();
  }

  if (binding_set_.empty()) {
    base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, this);
  }
}

}  // namespace device
