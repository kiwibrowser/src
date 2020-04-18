// Copyright 2018 The Crashpad Authors. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "util/fuchsia/scoped_task_suspend.h"

#include <zircon/process.h>
#include <zircon/syscalls.h>
#include <zircon/syscalls/debug.h>

#include <vector>

#include "base/fuchsia/fuchsia_logging.h"
#include "base/fuchsia/scoped_zx_handle.h"
#include "base/logging.h"
#include "util/fuchsia/koid_utilities.h"

namespace crashpad {

namespace {

zx_obj_type_t GetHandleType(zx_handle_t handle) {
  zx_info_handle_basic_t basic;
  zx_status_t status = zx_object_get_info(
      handle, ZX_INFO_HANDLE_BASIC, &basic, sizeof(basic), nullptr, nullptr);
  if (status != ZX_OK) {
    ZX_LOG(ERROR, status) << "zx_object_get_info";
    return ZX_OBJ_TYPE_NONE;
  }
  return basic.type;
}

enum class SuspensionResult {
  FailedSuspendCall,
  FailedToSuspendInTimelyFashion,
  Succeeded,
};

SuspensionResult SuspendThread(zx_handle_t thread) {
  zx_status_t status = zx_task_suspend(thread);
  ZX_LOG_IF(ERROR, status != ZX_OK, status) << "zx_task_suspend";
  if (status != ZX_OK)
    return SuspensionResult::FailedSuspendCall;
  // zx_task_suspend() suspends the thread "sometime soon", but it's hard to
  // use when it's not guaranteed to be suspended after return. Try reading the
  // thread state until the registers are retrievable, which means that the
  // thread is actually suspended. Don't wait forever in case the suspend
  // failed for whatever reason, but try a few times.
  for (int i = 0; i < 5; ++i) {
    zx_thread_state_general_regs_t regs;
    status = zx_thread_read_state(
        thread, ZX_THREAD_STATE_GENERAL_REGS, &regs, sizeof(regs));
    if (status == ZX_OK) {
      return SuspensionResult::Succeeded;
    }
    zx_nanosleep(zx_deadline_after(ZX_MSEC(10)));
  }
  LOG(ERROR) << "thread failed to suspend";
  return SuspensionResult::FailedToSuspendInTimelyFashion;
}

bool ResumeThread(zx_handle_t thread) {
  zx_status_t status = zx_task_resume(thread, 0);
  ZX_LOG_IF(ERROR, status != ZX_OK, status) << "zx_task_resume";
  return status == ZX_OK;
}

}  // namespace

ScopedTaskSuspend::ScopedTaskSuspend(zx_handle_t task) : task_(task) {
  DCHECK_NE(task_, zx_process_self());
  DCHECK_NE(task_, zx_thread_self());

  zx_obj_type_t type = GetHandleType(task_);
  if (type == ZX_OBJ_TYPE_THREAD) {
    // Note that task_ is only marked invalid if the zx_task_suspend() call
    // completely fails, otherwise the suspension might just not have taken
    // effect yet, so avoid leaving it suspended forever by still resuming on
    // destruction.
    if (SuspendThread(task_) == SuspensionResult::FailedSuspendCall) {
      task_ = ZX_HANDLE_INVALID;
    }
  } else if (type == ZX_OBJ_TYPE_PROCESS) {
    for (const auto& thread : GetChildHandles(task_, ZX_INFO_PROCESS_THREADS)) {
      SuspendThread(thread.get());
    }
  } else {
    LOG(ERROR) << "unexpected handle type";
    task_ = ZX_HANDLE_INVALID;
  }
}

ScopedTaskSuspend::~ScopedTaskSuspend() {
  if (task_ != ZX_HANDLE_INVALID) {
    zx_obj_type_t type = GetHandleType(task_);
    if (type == ZX_OBJ_TYPE_THREAD) {
      ResumeThread(task_);
    } else if (type == ZX_OBJ_TYPE_PROCESS) {
      for (const auto& thread :
           GetChildHandles(task_, ZX_INFO_PROCESS_THREADS)) {
        ResumeThread(thread.get());
      }
    } else {
      LOG(ERROR) << "unexpected handle type";
    }
  }
}

}  // namespace crashpad
