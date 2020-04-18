// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file contains helper classes for video accelerator unittests.

#ifndef MEDIA_GPU_TEST_VIDEO_ACCELERATOR_UNITTEST_HELPERS_H_
#define MEDIA_GPU_TEST_VIDEO_ACCELERATOR_UNITTEST_HELPERS_H_

#include "base/containers/queue.h"
#include "base/synchronization/condition_variable.h"
#include "base/synchronization/lock.h"

namespace media {

// Helper class allowing one thread to wait on a notification from another.
// If notifications come in faster than they are Wait()'d for, they are
// accumulated (so exactly as many Wait() calls will unblock as Notify() calls
// were made, regardless of order).
template <typename StateEnum>
class ClientStateNotification {
 public:
  ClientStateNotification();
  ~ClientStateNotification();

  // Used to notify a single waiter of a ClientState.
  void Notify(StateEnum state);
  // Used by waiters to wait for the next ClientState Notification.
  StateEnum Wait();

 private:
  base::Lock lock_;
  base::ConditionVariable cv_;
  base::queue<StateEnum> pending_states_for_notification_;
};

template <typename StateEnum>
ClientStateNotification<StateEnum>::ClientStateNotification() : cv_(&lock_) {}

template <typename StateEnum>
ClientStateNotification<StateEnum>::~ClientStateNotification() {}

template <typename StateEnum>
void ClientStateNotification<StateEnum>::Notify(StateEnum state) {
  base::AutoLock auto_lock(lock_);
  pending_states_for_notification_.push(state);
  cv_.Signal();
}

template <typename StateEnum>
StateEnum ClientStateNotification<StateEnum>::Wait() {
  base::AutoLock auto_lock(lock_);
  while (pending_states_for_notification_.empty())
    cv_.Wait();
  StateEnum ret = pending_states_for_notification_.front();
  pending_states_for_notification_.pop();
  return ret;
}

}  // namespace media

#endif  // MEDIA_GPU_TEST_VIDEO_ACCELERATOR_UNITTEST_HELPERS_H_
