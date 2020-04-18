/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Server Runtime mutex and condition variable abstraction layer.
 * This is the host-OS-independent interface.
 */
#ifndef NATIVE_CLIENT_SRC_SHARED_PLATFORM_NACL_SYNC_H_
#define NATIVE_CLIENT_SRC_SHARED_PLATFORM_NACL_SYNC_H_

#include "native_client/src/include/build_config.h"

#if defined(__native_client__) || NACL_LINUX || NACL_OSX
# include <pthread.h>
# include "native_client/src/shared/platform/posix/nacl_fast_mutex.h"
#elif NACL_WINDOWS
# include "native_client/src/shared/platform/win/nacl_fast_mutex.h"
#else
# error "What OS?"
#endif
#include "native_client/src/include/nacl_compiler_annotations.h"
#include "native_client/src/include/nacl_base.h"
#include "native_client/src/trusted/service_runtime/include/machine/_types.h"

EXTERN_C_BEGIN

// It is very difficult to get a definition of nacl_abi_timespec inside of a
// native client compile.  Sidestep this issue for now.
// TODO(bsy,sehr): change the include guards on time.h to allow both defines.
#ifdef __native_client__
// In a native client module timespec and nacl_abi_timespec are the same.
typedef struct timespec NACL_TIMESPEC_T;
#else
// In trusted code time.h is not derived from
// src/trusted/service_runtime/include/sys/time.h, so there is no conflict.
typedef struct nacl_abi_timespec NACL_TIMESPEC_T;
#endif

struct NaClMutex {
#if defined(__native_client__) || NACL_LINUX || NACL_OSX
  pthread_mutex_t mu;
  int held;
#elif NACL_WINDOWS
  void* lock;
  int held;
  /*
   * Windows lock implementation is recursive -- use a boolean to
   * convert to a binary mutex.
   */
#else
# error "What OS?"
#endif
};

struct NaClCondVar {
#if defined(__native_client__) || NACL_LINUX || NACL_OSX
  pthread_cond_t cv;
#elif NACL_WINDOWS
  void* cv;
#else
# error "What OS?"
#endif
};

struct nacl_abi_timespec;

typedef enum NaClSyncStatus {
  NACL_SYNC_OK,
  NACL_SYNC_INTERNAL_ERROR,
  NACL_SYNC_BUSY,
  NACL_SYNC_MUTEX_INVALID,
  NACL_SYNC_MUTEX_DEADLOCK,
  NACL_SYNC_MUTEX_PERMISSION,
  NACL_SYNC_MUTEX_INTERRUPTED,
  NACL_SYNC_CONDVAR_TIMEDOUT,
  NACL_SYNC_CONDVAR_INTR,
  NACL_SYNC_SEM_INTERRUPTED,
  NACL_SYNC_SEM_RANGE_ERROR
} NaClSyncStatus;

int NaClMutexCtor(struct NaClMutex *mp) NACL_WUR;  /* bool success/fail */

void NaClMutexDtor(struct NaClMutex *mp);

NaClSyncStatus NaClMutexLock(struct NaClMutex *mp) NACL_WUR;

NaClSyncStatus NaClMutexTryLock(struct NaClMutex *mp) NACL_WUR;

NaClSyncStatus NaClMutexUnlock(struct NaClMutex *mp) NACL_WUR;


int NaClCondVarCtor(struct NaClCondVar *cvp) NACL_WUR;

void NaClCondVarDtor(struct NaClCondVar *cvp);

NaClSyncStatus NaClCondVarSignal(struct NaClCondVar *cvp) NACL_WUR;

NaClSyncStatus NaClCondVarBroadcast(struct NaClCondVar *cvp) NACL_WUR;

NaClSyncStatus NaClCondVarWait(struct NaClCondVar *cvp,
                               struct NaClMutex   *mp) NACL_WUR;

NaClSyncStatus NaClCondVarTimedWaitRelative(
    struct NaClCondVar              *cvp,
    struct NaClMutex                *mp,
    NACL_TIMESPEC_T const           *reltime) NACL_WUR;

NaClSyncStatus NaClCondVarTimedWaitAbsolute(
    struct NaClCondVar              *cvp,
    struct NaClMutex                *mp,
    NACL_TIMESPEC_T const           *abstime) NACL_WUR;


/*
 * "Fast" binary mutex lock that does not work with condition
 * variables.  This may be implemented using spinlocks, normal POSIX
 * mutexes (where futex is used so uncontended locks are fast), or
 * CRITICAL_SECTION (on Windows).
 *
 * NOT FOR USE WHEN THE LOCK MIGHT BE HELD FOR AN EXTENDED PERIOD.
 *
 * Consider what might happen if this were implemented with a spinlock
 * and the lock is held for a large fraction of a scheduling quanta.
 * The probability of the thread holding the lock getting interrupted
 * by the scheduler becomes non-negligible.  If there are other
 * runnable threads, then the interrupted thread might be descheduled
 * while holding the spinlock to let some other thread run.  What
 * would happen -- especially if there is lock contention -- when
 * another thread (possibly the very one that was just scheduled to
 * run) tries to take the spinlock?  That the second (and any other
 * subsequent) thread will spin for its entire scheduling quantum,
 * wasting all the CPU cycles in the time quantum.  On a multicore /
 * multiprocessor machine, if the ratio of the number of runnable
 * threads to the number of available cores is greater than 1.0 -- so
 * that scheduling quanta expiration often leads to thread switch -- a
 * contended lock that is held for a long time would cause noticeable
 * performance degradation due to cores spinning doing essentially
 * nothing -- waiting for the spinlock to be free.  In that case, a
 * blocking lock should be used instead.
 *
 * The probability of a thread being descheduled while holding a
 * spinlock depends on many factors, such as the size of the
 * scheduling quantum.  Performing a syscall while holding a spinlock
 * is generally a bad idea, since syscalls often block, and even if
 * they are guaranteed to be nonblocking, some kernel designs like to
 * context switch to other threads since most of the context switch
 * overhead has already been paid (e.g., if the thread is close to the
 * end of its scheduling quantum).
 */
struct NaClFastMutex;

int NaClFastMutexCtor(struct NaClFastMutex *flp) NACL_WUR;

void NaClFastMutexDtor(struct NaClFastMutex *flp);

void NaClFastMutexLock(struct NaClFastMutex *flp);

int NaClFastMutexTryLock(struct NaClFastMutex *flp) NACL_WUR;

void NaClFastMutexUnlock(struct NaClFastMutex *flp);

EXTERN_C_END


#endif  /* NATIVE_CLIENT_SRC_SHARED_PLATFORM_NACL_SYNC_H_ */
