// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_REMOTING_WAITER_H_
#define CHROME_TEST_REMOTING_WAITER_H_

#include "base/macros.h"
#include "base/timer/timer.h"

namespace content {
class MessageLoopRunner;
}

namespace remoting {

// Block the execution of the test code for the specified number of
// milliseconds while keeping the message loop running. The browser instance
// will be responsive during the wait and test actions initiated before
// the wait will keep running.
class TimeoutWaiter {
 public:
  explicit TimeoutWaiter(base::TimeDelta timeout);
  virtual ~TimeoutWaiter();

  // Returns true in case of success.
  // For TimeoutWaiter it should always be true.
  virtual bool Wait();

 protected:
  virtual void CancelWait();

 private:
  // Callback used to cancel the TimeoutWaiter::Wait.
  void CancelWaitCallback();

  base::OneShotTimer timeout_timer_;
  base::TimeDelta timeout_;
  scoped_refptr<content::MessageLoopRunner> message_loop_runner_;

  DISALLOW_COPY_AND_ASSIGN(TimeoutWaiter);
};

// With a message loop running, keep calling callback in the specified
// interval until true is returned.
class ConditionalTimeoutWaiter : public TimeoutWaiter {
 public:
  typedef base::Callback<bool(void)> Predicate;

  ConditionalTimeoutWaiter(base::TimeDelta timeout,
                           base::TimeDelta interval,
                           const Predicate& callback);
  ~ConditionalTimeoutWaiter() override;

  // Returns true if |callback_| returned true and false in case of timeout.
  bool Wait() override;

 protected:
  void CancelWait() override;

 private:
  // Callback used to cancel the ConditionalTimeoutWaiter::Wait.
  void CancelWaitCallback();

  base::TimeDelta interval_;
  Predicate callback_;
  base::RepeatingTimer condition_timer_;
  bool success_;

  DISALLOW_COPY_AND_ASSIGN(ConditionalTimeoutWaiter);
};

}  // namespace remoting

#endif  // CHROME_TEST_REMOTING_WAITER_H_
