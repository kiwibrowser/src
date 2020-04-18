/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


// ConditionVariable is a reasonable attempt at simulating
// the newer Posix and Vista-only construct for condition variable
// synchronization.  This functionality is very helpful for having several
// threads wait for an event, as is common with a thread pool
// managed by a master.  The meaning of such an event in the
// (worker) thread pool scenario is that additional tasks are
// now available for processing.  It is used in Chrome in the
// DNS prefetching system to notify worker threads that a queue
// now has items (tasks) which need to be tended to.
// A related use would have a pool manager waiting on a
// ConditionVariable, waiting for a thread in the pool to announce
// (signal) that there is now more room in a (bounded size) communications
// queue for the manager to deposit tasks, or, as a second example, that
// the queue of tasks is completely empty and all workers are waiting.

// USAGE NOTE 1: spurious signal events are possible with this and
// most implementations of condition variables.  As a result, be
// *sure* to retest your condition before proceeding.  The following
// is a good example of doing this correctly:

// while (!work_to_be_done()) Wait(...);

// In contrast do NOT do the following:

// if (!work_to_be_done()) Wait(...);  // Don't do this.

// Especially avoid the above if you are relying on some other thread only
// issuing a signal up *if* there is work-to-do.  There can/will
// be spurious signals.  Recheck state on waiting thread before
// assuming the signal was intentional. Caveat caller ;-).

// USAGE NOTE 2: Broadcast() frees up all waiting threads at once,
// which leads to contention for the locks they all held when they
// called Wait().  This results in POOR performance.  A much better
// approach to getting a lot of threads out of Wait() is to have each
// thread (upon exiting Wait()) call Signal() to free up another
// Wait'ing thread.  Look at condition_variable_unittest.cc for
// both examples.

// Broadcast() can be used nicely during teardown, as it gets the job
// done, and leaves no sleeping threads... and performance is less
// critical at that point.

// The semantics of Broadcast() are carefully crafted so that *all*
// threads that were waiting when the request was made will indeed
// get signaled.  Some implementations mess up, and don't signal them
// all, while others allow the wait to be effectively turned off (for
// for a while while waiting threads come around).  This implementation
// appears correct, as it will not "lose" any signals, and will guarantee
// that all threads get signaled by Broadcast().

// This implementation offers support for "performance" in its selection of
// which thread to revive.  Performance, in direct contrast with "fairness,"
// assures that the thread that most recently began to Wait() is selected by
// Signal to revive.  Fairness would (if publicly supported) assure that the
// thread that has Wait()ed the longest is selected. The default policy
// may improve performance, as the selected thread may have a greater chance of
// having some of its stack data in various CPU caches.  Although not publicly
// available, internal support for "fairness" is used by the UnitTest harness
// only on Windows to more thoroughly test the implementation.

// For many very subtle implementation details, see the condition_variable.cc.

// NOTE(gregoryd): changed the interface to allow providing the Lock reference
// in Wait functions instead of the constructor.

#ifndef NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_WIN_CONDITION_VARIABLE_H_
#define NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_WIN_CONDITION_VARIABLE_H_


#include "native_client/src/shared/platform/win/condition_variable_events.h"
#include "native_client/src/shared/platform/win/lock.h"

namespace NaCl {

class Lock;

class ConditionVariable {
 public:
  // Construct a cv (our version allows to use the cv with different locks).
  ConditionVariable();

  ~ConditionVariable();

  // Wait() releases the caller's critical section atomically as it starts to
  // sleep, and the re-acquires it when it is signaled.
  // We provide two variants of TimedWait:
  // the first one takes relative time in microseconds as an argument.
  int TimedWaitRel(Lock& user_lock, int64_t max_usec);
  // The second TimedWait takes absolute time in microseconds.
  int TimedWaitAbs(Lock& user_lock, int64_t abs_usec);
  void Wait(Lock& user_lock);

  // Broadcast() revives all waiting threads.
  void Broadcast();
  // Signal() revives one waiting thread.
  void Signal();

 private:
  enum RunState { RUNNING = 64213, SHUTDOWN = 0 };

  // Internal implementation methods supporting Wait().
  ConditionVariableEvent* GetEventForWaiting();
  void RecycleEvent(ConditionVariableEvent* used_event);

  // Note that RUNNING is an unlikely number to have in RAM by accident.
  // This helps with defensive destructor coding in the face of user error.
  RunState run_state_;

  // Private critical section for access to member data.
  Lock internal_lock_;
  // Lock that is acquired before calling Wait().
  // NOTE(gregoryd): Lock& user_lock_;-removed, see the note at top of file

  // Events that threads are blocked on.
  ConditionVariableEvent waiting_list_;

  // Free list for old events.
  ConditionVariableEvent recycling_list_;
  int recycling_list_size_;

  // The number of allocated, but not yet deleted events.
  int allocation_counter_;

  NACL_DISALLOW_COPY_AND_ASSIGN(ConditionVariable);
};

}  // namespace NaCl

#endif  // NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_WIN_CONDITION_VARIABLE_H_
