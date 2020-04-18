// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_CLIENT_PLUGIN_PEPPER_MAIN_THREAD_TASK_RUNNER_H_
#define REMOTING_CLIENT_PLUGIN_PEPPER_MAIN_THREAD_TASK_RUNNER_H_

#include <stdint.h>

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "base/time/time.h"

namespace pp {
class Core;
}  // namespace pp

namespace remoting {

// SingleThreadTaskRunner implementation for the main plugin thread.
class PepperMainThreadTaskRunner : public base::SingleThreadTaskRunner {
 public:
  PepperMainThreadTaskRunner();

  // base::SingleThreadTaskRunner interface.
  bool PostDelayedTask(const base::Location& from_here,
                       base::OnceClosure task,
                       base::TimeDelta delay) override;
  bool PostNonNestableDelayedTask(const base::Location& from_here,
                                  base::OnceClosure task,
                                  base::TimeDelta delay) override;
  bool RunsTasksInCurrentSequence() const override;

 protected:
  ~PepperMainThreadTaskRunner() override;

 private:
  // Helper that allows a base::Closure to be used as a pp::CompletionCallback,
  // by ignoring the completion result.
  void RunTask(base::OnceClosure task);

  pp::Core* core_;

  base::WeakPtr<PepperMainThreadTaskRunner> weak_ptr_;
  base::WeakPtrFactory<PepperMainThreadTaskRunner> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(PepperMainThreadTaskRunner);
};

}  // namespace remoting

#endif  // REMOTING_CLIENT_PLUGIN_PEPPER_MAIN_THREAD_TASK_RUNNER_H_
