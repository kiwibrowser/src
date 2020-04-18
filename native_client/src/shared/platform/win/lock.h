/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#ifndef NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_WIN_LOCK_H_
#define NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_WIN_LOCK_H_

#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/shared/platform/win/lock_impl.h"

// A convenient wrapper for a critical section.
//
// NOTE: A thread may acquire the same lock multiple times, but it must call
// Release for each call to Acquire in order to finally release the lock.
//
// Complication: UnitTest for DeathTests catch DCHECK exceptions, so we need
// to write code assuming DCHECK will throw.  This means we need to save any
// assertable value in a local until we can safely throw.

namespace NaCl {

class Lock {
 public:
  Lock();
  ~Lock();
  void Acquire();
  void Release();
  bool Try();

 private:
  NaCl::LockImpl lock_;  // User-supplied underlying lock implementation.

  // All private data is implicitly protected by spin_lock_.
  // Be VERY careful to only access under that lock.
  int32_t recursion_count_shadow_;

  // Allow access to GetCurrentThreadRecursionCount()
  friend class AutoUnlock;
  int32_t GetCurrentThreadRecursionCount();

#ifndef NDEBUG
  // Even in Debug mode, the expensive tallies won't be calculated by default.
  bool recursion_used_;
  int32_t acquisition_count_;

  int32_t contention_count_;
#endif  // NDEBUG

  NACL_DISALLOW_COPY_AND_ASSIGN(Lock);
};

// A helper class that acquires the given Lock while the AutoLock is in scope.
class AutoLock {
 public:
  explicit AutoLock(Lock& lock) : lock_(lock) {
    lock_.Acquire();
  }

  ~AutoLock() {
    lock_.Release();
  }

 private:
  Lock& lock_;
  NACL_DISALLOW_COPY_AND_ASSIGN(AutoLock);
};

// A helper macro to perform a single operation (expressed by expr)
// in a lock
#define LOCKED_EXPRESSION(lock, expr) \
  do { \
    NaCl::AutoLock _auto_lock(lock); \
    (expr); \
  } while (0)

// AutoUnlock is a helper class for ConditionVariable instances
// that is analogous to AutoLock.  It provides for nested Releases
// of a lock for the Wait functionality of a ConditionVariable class.
// The destructor automatically does the corresponding Acquire
// calls (to return to the initial nested lock state).

// Instances of AutoUnlock can ***ONLY*** validly be constructed if the
// caller currently holds the lock provided as the constructor's argument.
// If that ***REQUIREMENT*** is violated in debug mode, a DCHECK will
// be generated in the Lock class.  In production (non-debug),
// the results are undefined (and probably bad) if the caller
// is not already holding the indicated lock.
class ConditionVariable;
class AutoUnlock {
 private:  // Everything is private, so only our friend can use us.
  friend class ConditionVariable;  // The only user of this class.
  explicit AutoUnlock(Lock& lock);
  ~AutoUnlock();

  Lock* lock_;
  int release_count_;
};

}  // namespace NaCl

#endif  // NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_WIN_LOCK_H_
