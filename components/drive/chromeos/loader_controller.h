// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DRIVE_CHROMEOS_LOADER_CONTROLLER_H_
#define COMPONENTS_DRIVE_CHROMEOS_LOADER_CONTROLLER_H_

#include <memory>
#include <vector>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"

namespace base {
class ScopedClosureRunner;
}

namespace drive {
namespace internal {

// Delays execution of tasks as long as more than one lock is alive.
// Used to ensure that ChangeListLoader does not cause race condition by adding
// new entries created by sync tasks before they do.
// All code which may add entries found on the server to the local metadata
// should use this class.
// TODO(slangley): Consider specific unit tests for LoaderController. Currently
// it is tested as an artifact of other unit tests.
class LoaderController {
 public:
  LoaderController();
  ~LoaderController();

  // Increments the lock count and returns an object which decrements the count
  // on its destruction.
  // While the lock count is positive, tasks will be pending.
  // TODO(slangley): Return ScopedClosureRunner directly, rather than one that
  // is wrapped in a unique_ptr.
  std::unique_ptr<base::ScopedClosureRunner> GetLock();

  // Runs the task if the lock count is 0, otherwise it will be pending.
  // TODO(slangley): Change to using a OnceClosure.
  void ScheduleRun(const base::Closure& task);

 private:
  // Decrements the lock count.
  void Unlock();

  int lock_count_;
  std::vector<base::RepeatingClosure> pending_tasks_;

  THREAD_CHECKER(thread_checker_);

  base::WeakPtrFactory<LoaderController> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(LoaderController);
};

}  // namespace internal
}  // namespace drive

#endif  // COMPONENTS_DRIVE_CHROMEOS_LOADER_CONTROLLER_H_
