// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/chromeos/loader_controller.h"

#include "base/callback_helpers.h"

namespace drive {
namespace internal {

LoaderController::LoaderController()
    : lock_count_(0), weak_ptr_factory_(this) {}

LoaderController::~LoaderController() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
}

std::unique_ptr<base::ScopedClosureRunner> LoaderController::GetLock() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  ++lock_count_;
  return std::make_unique<base::ScopedClosureRunner>(base::BindOnce(
      &LoaderController::Unlock, weak_ptr_factory_.GetWeakPtr()));
}

void LoaderController::ScheduleRun(const base::RepeatingClosure& task) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(task);

  if (lock_count_ > 0) {
    pending_tasks_.push_back(task);
  } else {
    task.Run();
  }
}

void LoaderController::Unlock() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK_LT(0, lock_count_);

  if (--lock_count_ > 0)
    return;

  std::vector<base::RepeatingClosure> tasks;
  tasks.swap(pending_tasks_);
  for (auto& task : tasks)
    task.Run();
}

}  // namespace internal
}  // namespace drive
