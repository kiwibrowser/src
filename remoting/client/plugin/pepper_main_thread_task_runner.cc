// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/client/plugin/pepper_main_thread_task_runner.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "ppapi/cpp/completion_callback.h"
#include "ppapi/cpp/core.h"
#include "ppapi/cpp/module.h"

namespace remoting {
namespace {

void RunAndDestroy(void* task_ptr, int32_t) {
  std::unique_ptr<base::OnceClosure> task(
      static_cast<base::OnceClosure*>(task_ptr));
  std::move(*task).Run();
}

}  // namespace

PepperMainThreadTaskRunner::PepperMainThreadTaskRunner()
    : core_(pp::Module::Get()->core()), weak_ptr_factory_(this) {
  DCHECK(core_->IsMainThread());
  weak_ptr_ = weak_ptr_factory_.GetWeakPtr();
}

bool PepperMainThreadTaskRunner::PostDelayedTask(
    const base::Location& from_here,
    base::OnceClosure task,
    base::TimeDelta delay) {
  auto task_ptr = std::make_unique<base::OnceClosure>(base::Bind(
      &PepperMainThreadTaskRunner::RunTask, weak_ptr_, base::Passed(&task)));
  core_->CallOnMainThread(
      delay.InMillisecondsRoundedUp(),
      pp::CompletionCallback(&RunAndDestroy, task_ptr.release()));
  return true;
}

bool PepperMainThreadTaskRunner::PostNonNestableDelayedTask(
    const base::Location& from_here,
    base::OnceClosure task,
    base::TimeDelta delay) {
  return PostDelayedTask(from_here, std::move(task), delay);
}

bool PepperMainThreadTaskRunner::RunsTasksInCurrentSequence() const {
  return core_->IsMainThread();
}

PepperMainThreadTaskRunner::~PepperMainThreadTaskRunner() {}

void PepperMainThreadTaskRunner::RunTask(base::OnceClosure task) {
  std::move(task).Run();
}

}  // namespace remoting
