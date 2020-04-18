/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


// See header file for description of class.
// There are numerous subtle implementation details in this class.  See the FAQ
// comments at the end of this file for that discussion.

#include "native_client/src/include/portability.h"
#include <stack>

#include "native_client/src/shared/platform/nacl_sync.h"
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_time.h"
#include "native_client/src/shared/platform/win/condition_variable.h"
#include "native_client/src/shared/platform/win/lock.h"
#include "native_client/src/trusted/service_runtime/include/sys/time.h"

static const int64_t kMillisecondsPerSecond = 1000;
static const int64_t kMicrosecondsPerMillisecond = 1000;
static const int64_t kMicrosecondsPerSecond =
    kMicrosecondsPerMillisecond * kMillisecondsPerSecond;

NaCl::ConditionVariable::ConditionVariable()
  : run_state_(RUNNING),
    recycling_list_size_(0),
    allocation_counter_(0) {
}

NaCl::ConditionVariable::~ConditionVariable() {
  AutoLock auto_lock(internal_lock_);
  run_state_ = SHUTDOWN;  // Prevent any more waiting.

  // DCHECK(recycling_list_size_ == allocation_counter_) ;
  if (recycling_list_size_ != allocation_counter_) {  // Rare shutdown problem.
    // User left threads Wait()ing, and is destructing us.
    // Note: waiting_list_ *might* be empty, but recycling is still pending.
    AutoUnlock auto_unlock(internal_lock_);
    Broadcast();  // Make sure all waiting threads have been signaled.
    Sleep(10);  // Give threads a chance to grab internal_lock_.
    // All contained threads should be blocked on user_lock_ by now :-).
  }  // Re-Acquire internal_lock_

  // DCHECK(recycling_list_size_ == allocation_counter_); //We couldn't fix it.

  while (!recycling_list_.IsEmpty()) {
    ConditionVariableEvent* cv_event;
    cv_event = recycling_list_.PopFront();
    recycling_list_size_--;
    delete cv_event;
    allocation_counter_--;
  }
  // DCHECK(0 == recycling_list_size_);
  // DCHECK(0 == allocation_counter_);
}

// Wait() atomically releases the caller's lock as it starts to Wait, and then
// re-acquires it when it is signaled.
int NaCl::ConditionVariable::TimedWaitRel(Lock& user_lock, int64_t max_usec) {
  ConditionVariableEvent* waiting_event;
  HANDLE  handle;
  DWORD   result;
  { /* SCOPE */
    AutoLock auto_lock(internal_lock_);
    if (RUNNING != run_state_) return 0;  // Destruction in progress.
    waiting_event = GetEventForWaiting();
    handle = waiting_event->handle();
    // DCHECK(0 != handle);
  }  // Release internal_lock.

  { /* SCOPE */
    AutoUnlock unlock(user_lock);  // Release caller's lock
    result = WaitForSingleObject(
        handle, static_cast<DWORD>(max_usec / kMicrosecondsPerMillisecond));
    // Minimize spurious signal creation window by recycling asap.
    AutoLock auto_lock(internal_lock_);
    RecycleEvent(waiting_event);
    // Release internal_lack_
  }  // Re-Acquire callers lock to depth at entry.
  if (WAIT_OBJECT_0 == result)
    return 1;  // the object was signaled
  return 0;
}

static int64_t NaClWinUnixTimeBaseNow() {
  struct nacl_abi_timeval unix_time;
  (void) NaClGetTimeOfDay(&unix_time);
  return unix_time.nacl_abi_tv_sec * kMicrosecondsPerSecond +
         unix_time.nacl_abi_tv_usec;
}

int NaCl::ConditionVariable::TimedWaitAbs(Lock& user_lock, int64_t abs_usec) {
  int64_t relative_usec = abs_usec - NaClWinUnixTimeBaseNow();
  NaClLog(4,
          "TimedWaitAbs: req'd abs_time %" NACL_PRId64 " ticks (uS)\n",
          abs_usec);
  NaClLog(4,
          "TimedWaitAbs: relative_usec  %" NACL_PRId64 " ticks (uS)\n",
          relative_usec);
  if (relative_usec < 0) {
    NaClLog(4, "TimedWaitAbs: time already passed!\n");
    // we check to see if the object has been signaled anyway
    relative_usec = 0;
  }
  return TimedWaitRel(user_lock, relative_usec);
}

void NaCl::ConditionVariable::Wait(Lock& user_lock) {
  // Default to "wait forever" timing, which means have to get a Signal()
  // or Broadcast() to come out of this wait state.
  TimedWaitRel(user_lock, INT64_MAX);
}

// Broadcast() is guaranteed to signal all threads that were waiting (i.e., had
// a cv_event internally allocated for them) before Broadcast() was called.
void NaCl::ConditionVariable::Broadcast() {
  std::stack<HANDLE> handles;  // See FAQ-question-10.
  { /* SCOPE */
    AutoLock auto_lock(internal_lock_);
    if (waiting_list_.IsEmpty())
      return;
    while (!waiting_list_.IsEmpty())
      // This is not a leak from waiting_list_.  See FAQ-question 12.
      handles.push(waiting_list_.PopBack()->handle());
  }  // Release internal_lock_.
  while (handles.size() > 0) {
    SetEvent(handles.top());
    handles.pop();
  }
}

// Signal() will select one of the waiting threads, and signal it (signal its
// cv_event).  For better performance we signal the thread that went to sleep
// most recently (LIFO).  If we want fairness, then we wake the thread that has
// been sleeping the longest (FIFO).
void NaCl::ConditionVariable::Signal() {
  HANDLE handle;
  { /* SCOPE */
    AutoLock auto_lock(internal_lock_);
    if (waiting_list_.IsEmpty())
      return;  // No one to signal.

#ifndef FAIRNESS
    // Only performance option should be used.
    // This is not a leak from waiting_list.  See FAQ-question 12.
     handle = waiting_list_.PopBack()->handle();  // LIFO.
#else
     handle = waiting_list_.PopFront()->handle();  // FIFO.
#endif  // FAIRNESS
  }  // Release internal_lock_.
  SetEvent(handle);
}

// GetEventForWaiting() provides a unique cv_event for any caller that needs to
// wait.  This means that (worst case) we may over time create as many cv_event
// objects as there are threads simultaneously using this instance's Wait()
// functionality .
NaCl::ConditionVariableEvent* NaCl::ConditionVariable::GetEventForWaiting() {
  // We hold internal_lock, courtesy of Wait().
  ConditionVariableEvent* cv_event;
  if (0 == recycling_list_size_) {
    // DCHECK(recycling_list_.IsEmpty());
    cv_event = new ConditionVariableEvent(true);
    // CHECK(cv_event);
    allocation_counter_++;
    // CHECK(0 != cv_event->handle());
  } else {
    cv_event = recycling_list_.PopFront();
    recycling_list_size_--;
  }
  waiting_list_.PushBack(cv_event);
  return cv_event;
}

// RecycleEvent() takes a cv_event that was previously used for Wait()ing, and
// recycles it for use in future Wait() calls for this or other threads.
// Note that there is a tiny chance that the cv_event is still signaled when we
// obtain it, and that can cause spurious signals (if/when we re-use the
// cv_event), but such is quite rare (see FAQ-question-5).
void NaCl::ConditionVariable::RecycleEvent(ConditionVariableEvent* used_event) {
  // We hold internal_lock, courtesy of Wait().
  // If the cv_event timed out, then it is necessary to remove it from
  // waiting_list_.  If it was selected by Broadcast() or Signal(), then it is
  // already gone.
  used_event->Extract();  // Possibly redundant
  recycling_list_.PushBack(used_event);
  recycling_list_size_++;
}

/*
FAQ On subtle implementation details:

1) What makes this problem subtle?  Please take a look at "Strategies
for Implementing POSIX Condition Variables on Win32" by Douglas
C. Schmidt and Irfan Pyarali.
http://www.cs.wustl.edu/~schmidt/win32-cv-1.html It includes
discussions of numerous flawed strategies for implementing this
functionality.  I'm not convinced that even the final proposed
implementation has semantics that are as nice as this implementation
(especially with regard to Broadcast() and the impact on threads that
try to Wait() after a Broadcast() has been called, but before all the
original waiting threads have been signaled).

2) Why can't you use a single wait_event for all threads that call
Wait()?  See FAQ-question-1, or consider the following: If a single
event were used, then numerous threads calling Wait() could release
their cs locks, and be preempted just before calling
WaitForSingleObject().  If a call to Broadcast() was then presented on
a second thread, it would be impossible to actually signal all
waiting(?) threads.  Some number of SetEvent() calls *could* be made,
but there could be no guarantee that those led to to more than one
signaled thread (SetEvent()'s may be discarded after the first!), and
there could be no guarantee that the SetEvent() calls didn't just
awaken "other" threads that hadn't even started waiting yet (oops).
Without any limit on the number of requisite SetEvent() calls, the
system would be forced to do many such calls, allowing many new waits
to receive spurious signals.

3) How does this implementation cause spurious signal events?  The
cause in this implementation involves a race between a signal via
time-out and a signal via Signal() or Broadcast().  The series of
actions leading to this are:

a) Timer fires, and a waiting thread exits the line of code:

    WaitForSingleObject(waiting_event, max_time.InMilliseconds());

b) That thread (in (a)) is randomly pre-empted after the above line,
leaving the waiting_event reset (unsignaled) and still in the
waiting_list_.

c) A call to Signal() (or Broadcast()) on a second thread proceeds, and
selects the waiting cv_event (identified in step (b)) as the event to revive
via a call to SetEvent().

d) The Signal() method (step c) calls SetEvent() on waiting_event (step b).

e) The waiting cv_event (step b) is now signaled, but no thread is
waiting on it.

f) When that waiting_event (step b) is reused, it will immediately
be signaled (spuriously).


4) Why do you recycle events, and cause spurious signals?  First off,
the spurious events are very rare.  They can only (I think) appear
when the race described in FAQ-question-3 takes place.  This should be
very rare.  Most(?)  uses will involve only timer expiration, or only
Signal/Broadcast() actions.  When both are used, it will be rare that
the race will appear, and it would require MANY Wait() and signaling
activities.  If this implementation did not recycle events, then it
would have to create and destroy events for every call to Wait().
That allocation/deallocation and associated construction/destruction
would be costly (per wait), and would only be a rare benefit (when the
race was "lost" and a spurious signal took place). That would be bad
(IMO) optimization trade-off.  Finally, such spurious events are
allowed by the specification of condition variables (such as
implemented in Vista), and hence it is better if any user accommodates
such spurious events (see usage note in condition_variable.h).

5) Why don't you reset events when you are about to recycle them, or
about to reuse them, so that the spurious signals don't take place?
The thread described in FAQ-question-3 step c may be pre-empted for an
arbitrary length of time before proceeding to step d.  As a result,
the wait_event may actually be re-used *before* step (e) is reached.
As a result, calling reset would not help significantly.

6) How is it that the callers lock is released atomically with the
entry into a wait state?  We commit to the wait activity when we
allocate the wait_event for use in a given call to Wait().  This
allocation takes place before the caller's lock is released (and
actually before our internal_lock_ is released).  That allocation is
the defining moment when "the wait state has been entered," as that
thread *can* now be signaled by a call to Broadcast() or Signal().
Hence we actually "commit to wait" before releasing the lock, making
the pair effectively atomic.

8) Why do you need to lock your data structures during waiting, as the
caller is already in possession of a lock?  We need to Acquire() and
Release() our internal lock during Signal() and Broadcast().  If we tried
to use a callers lock for this purpose, we might conflict with their
external use of the lock.  For example, the caller may use to consistently
hold a lock on one thread while calling Signal() on another, and that would
block Signal().

9) Couldn't a more efficient implementation be provided if you
preclude using more than one external lock in conjunction with a
single ConditionVariable instance?  Yes, at least it could be viewed
as a simpler API (since you don't have to reiterate the lock argument
in each Wait() call).  One of the constructors now takes a specific
lock as an argument, and a there are corresponding Wait() calls that
don't specify a lock now.  It turns that the resulting implmentation
can't be made more efficient, as the internal lock needs to be used by
Signal() and Broadcast(), to access internal data structures.  As a
result, I was not able to utilize the user supplied lock (which is
being used by the user elsewhere presumably) to protect the private
member access.

9) Since you have a second lock, how can be be sure that there is no
possible deadlock scenario?  Our internal_lock_ is always the last
lock acquired, and the first one released, and hence a deadlock (due
to critical section problems) is impossible as a consequence of our
lock.

10) When doing a Broadcast(), why did you copy all the events into
an STL queue, rather than making a linked-loop, and iterating over it?
The iterating during Broadcast() is done so outside the protection
of the internal lock. As a result, other threads, such as the thread
wherein a related event is waiting, could asynchronously manipulate
the links around a cv_event.  As a result, the link structure cannot
be used outside a lock.  Broadcast() could iterate over waiting
events by cycling in-and-out of the protection of the internal_lock,
but that appears more expensive than copying the list into an STL
stack.

11) Why did the lock.h file need to be modified so much for this
change?  Central to a Condition Variable is the atomic release of a
lock during a Wait().  This places Wait() functionality exactly
mid-way between the two classes, Lock and Condition Variable.  Given
that there can be nested Acquire()'s of locks, and Wait() had to
Release() completely a held lock, it was necessary to augment the Lock
class with a recursion counter. Even more subtle is the fact that the
recursion counter (in a Lock) must be protected, as many threads can
access it asynchronously.  As a positive fallout of this, there are
now some DCHECKS to be sure no one Release()s a Lock more than they
Acquire()ed it, and there is ifdef'ed functionality that can detect
nested locks (legal under windows, but not under Posix).

12) Why is it that the cv_events removed from list in Broadcast() and Signal()
are not leaked?  How are they recovered??  The cv_events that appear to leak are
taken from the waiting_list_.  For each element in that list, there is currently
a thread in or around the WaitForSingleObject() call of Wait(), and those
threads have references to these otherwise leaked events. They are passed as
arguments to be recycled just aftre returning from WaitForSingleObject().

13) Why did you use a custom container class (the linked list), when STL has
perfectly good containers, such as an STL list?  The STL list, as with any
container, does not guarantee the utility of an iterator across manipulation
(such as insertions and deletions) of the underlying container.  The custom
double-linked-list container provided that assurance.  I don't believe any
combination of STL containers provided the services that were needed at the same
O(1) efficiency as the custom linked list.  The unusual requirement
for the container class is that a reference to an item within a container (an
iterator) needed to be maintained across an arbitrary manipulation of the
container.  This requirement exposes itself in the Wait() method, where a
waiting_event must be selected prior to the WaitForSingleObject(), and then it
must be used as part of recycling to remove the related instance from the
waiting_list.  A hash table (STL map) could be used, but I was embarrased to
use a complex and relatively low efficiency container when a doubly linked list
provided O(1) performance in all required operations.  Since other operations
to provide performance-and/or-fairness required queue (FIFO) and list (LIFO)
containers, I would also have needed to use an STL list/queue as well as an STL
map.  In the end I decided it would be "fun" to just do it right, and I
put so many assertions (DCHECKs) into the container class that it is trivial to
code review and validate its correctness.

*/
