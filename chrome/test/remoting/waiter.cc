// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/remoting/waiter.h"

#include "content/public/test/test_utils.h"

namespace remoting {

TimeoutWaiter::TimeoutWaiter(base::TimeDelta timeout)
    : timeout_(timeout) {
  DCHECK(timeout > base::TimeDelta::FromSeconds(0));
}

TimeoutWaiter::~TimeoutWaiter() {}

bool TimeoutWaiter::Wait() {
  DCHECK(!timeout_timer_.IsRunning());

  timeout_timer_.Start(
      FROM_HERE,
      timeout_,
      base::Bind(&TimeoutWaiter::CancelWaitCallback, base::Unretained(this)));

  message_loop_runner_ = new content::MessageLoopRunner;
  message_loop_runner_->Run();

  return true;
}

void TimeoutWaiter::CancelWait() {
  message_loop_runner_->Quit();
}

void TimeoutWaiter::CancelWaitCallback() {
  CancelWait();
}

ConditionalTimeoutWaiter::ConditionalTimeoutWaiter(base::TimeDelta timeout,
                                                   base::TimeDelta interval,
                                                   const Predicate& callback)
    : TimeoutWaiter(timeout),
      interval_(interval),
      callback_(callback),
      success_(false) {
  DCHECK(timeout > interval);
}

ConditionalTimeoutWaiter::~ConditionalTimeoutWaiter() {}

bool ConditionalTimeoutWaiter::Wait() {
  DCHECK(!condition_timer_.IsRunning());

  condition_timer_.Start(
      FROM_HERE,
      interval_,
      base::Bind(&ConditionalTimeoutWaiter::CancelWaitCallback,
                 base::Unretained(this)));

  // Also call the base class Wait() to start the timeout timer.
  TimeoutWaiter::Wait();

  return success_;
}

void ConditionalTimeoutWaiter::CancelWait() {
  condition_timer_.Stop();

  // Also call the base class CancelWait() to stop the timeout timer.
  TimeoutWaiter::CancelWait();
}

void ConditionalTimeoutWaiter::CancelWaitCallback() {
  if (callback_.Run()) {
    success_ = true;
    CancelWait();
  }
}

}  // namespace remoting
