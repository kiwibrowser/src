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

#ifndef CRASHPAD_UTIL_FUCHSIA_SCOPED_TASK_SUSPEND_H_
#define CRASHPAD_UTIL_FUCHSIA_SCOPED_TASK_SUSPEND_H_

#include <zircon/types.h>

#include "base/macros.h"

namespace crashpad {

//! \brief Manages the suspension of another task.
//!
//! Currently, suspends and resumes are not counted on Fuchsia, so while this
//! class attempts to manage suspension of a task, if another caller or process
//! is simultaneously suspending or resuming this task, the results may not be
//! as expected.
//!
//! Additionally, the underlying API only supports suspending threads (despite
//! its name) not entire tasks. As a result, it's possible some threads may not
//! be correctly suspended/resumed as their creation might race enumeration.
//!
//! Because of these limitations, this class is limited to being a best-effort,
//! and correct suspension/resumption cannot be relied upon.
//!
//! Callers should not attempt to suspend the current task as obtained via
//! `zx_process_self()`.
class ScopedTaskSuspend {
 public:
  explicit ScopedTaskSuspend(zx_handle_t task);
  ~ScopedTaskSuspend();

 private:
  zx_handle_t task_;  // weak

  DISALLOW_COPY_AND_ASSIGN(ScopedTaskSuspend);
};

}  // namespace crashpad

#endif  // CRASHPAD_UTIL_FUCHSIA_SCOPED_TASK_SUSPEND_H_
