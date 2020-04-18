/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Native Client mutex implementation
 */

#include <errno.h>
#include <unistd.h>

#include "native_client/src/include/nacl_compiler_annotations.h"
#include "native_client/src/untrusted/nacl/nacl_irt.h"
#include "native_client/src/untrusted/pthread/pthread.h"
#include "native_client/src/untrusted/pthread/pthread_internal.h"
#include "native_client/src/untrusted/pthread/pthread_types.h"

/*
 * This mutex implementation is based on Ulrich Drepper's paper
 * "Futexes Are Tricky" (dated November 5, 2011; see
 * http://www.akkadia.org/drepper/futex.pdf).  We use the approach
 * from "Mutex, Take 2".
 *
 * Ideally we would use the slightly modified approach from "Mutex,
 * Take 3" instead, for better performance.  For the contended case,
 * this replaces the atomic compare-and-swap ("lock cmpxchg" on x86)
 * with a non-comparing atomic swap ("xchg" on x86 -- the "lock"
 * prefix is redundant for this x86 instruction).
 *
 * However, in NaCl's current x86 GCC, the only atomic swap operation
 * is __sync_lock_test_and_set(), which is documented with the
 * warning:
 *
 *   "Many targets have only minimal support for such locks, and do
 *   not support a full exchange operation.  In this case, a target
 *   may support reduced functionality here by which the only valid
 *   value to store is the immediate constant 1.  The exact value
 *   actually stored in *ptr is implementation defined.
 *
 *   This builtin is not a full barrier, but rather an acquire
 *   barrier."
 *
 * While this GCC and PNaCl/LLVM seem to generate the desired code in
 * this case, the wording is not good enough if we are targeting
 * arbitrary architectures.
 *
 * TODO(mseaborn): Switch to the more modern __atomic_exchange_n()
 * when x86 nacl-gcc supports it.
 */


/*
 * Possible values of mutex_state.  The numeric values are significant
 * because unlocking does an atomic decrement on the mutex_state
 * field.
 */
enum MutexState {
  UNLOCKED = 0,
  LOCKED_WITHOUT_WAITERS = 1,
  LOCKED_WITH_WAITERS = 2
};

int pthread_mutex_init(pthread_mutex_t *mutex,
                       const pthread_mutexattr_t *mutex_attr) {
  mutex->mutex_state = UNLOCKED;
  mutex->owner_thread_id = NACL_PTHREAD_ILLEGAL_THREAD_ID;
  mutex->recursion_counter = 0;
  if (mutex_attr != NULL) {
    mutex->mutex_type = mutex_attr->kind;
  } else {
    mutex->mutex_type = PTHREAD_MUTEX_FAST_NP;
  }
  return 0;
}

int pthread_mutex_destroy(pthread_mutex_t *mutex) {
  if (mutex->mutex_state != UNLOCKED) {
    return EBUSY;
  }
  if (NACL_PTHREAD_ILLEGAL_THREAD_ID != mutex->owner_thread_id) {
    /* The mutex is still locked - cannot destroy. */
    return EBUSY;
  }
  mutex->owner_thread_id = NACL_PTHREAD_ILLEGAL_THREAD_ID;
  mutex->recursion_counter = 0;
  return 0;
}

static int mutex_lock_nonrecursive(pthread_mutex_t *mutex, int try_only,
                                   const struct timespec *abstime) {
  /*
   * Try to claim the mutex.  This compare-and-swap executes the full
   * memory barrier that pthread_mutex_lock() is required to execute.
   */
  int old_state = __sync_val_compare_and_swap(&mutex->mutex_state, UNLOCKED,
                                              LOCKED_WITHOUT_WAITERS);
  if (NACL_UNLIKELY(old_state != UNLOCKED)) {
    if (try_only) {
      return EBUSY;
    }
    if (abstime != NULL &&
        (abstime->tv_nsec < 0 || 1000000000 <= abstime->tv_nsec)) {
      return EINVAL;
    }
    do {
      /*
       * If the state shows there are already waiters, or we can
       * update it to indicate that there are waiters, then wait.
       */
      if (old_state == LOCKED_WITH_WAITERS ||
          __sync_val_compare_and_swap(&mutex->mutex_state,
                                      LOCKED_WITHOUT_WAITERS,
                                      LOCKED_WITH_WAITERS) != UNLOCKED) {
        int rc = __libnacl_irt_futex.futex_wait_abs(&mutex->mutex_state,
                                                    LOCKED_WITH_WAITERS,
                                                    abstime);
        if (abstime != NULL && rc == ETIMEDOUT)
          return ETIMEDOUT;
      }
      /*
       * Try again to claim the mutex.  On this try, we must set
       * mutex_state to LOCKED_WITH_WAITERS rather than
       * LOCKED_WITHOUT_WAITERS.  We could have been woken up when
       * many threads are in the wait queue for the mutex.
       */
      old_state = __sync_val_compare_and_swap(&mutex->mutex_state, UNLOCKED,
                                              LOCKED_WITH_WAITERS);
    } while (old_state != UNLOCKED);
  }
  return 0;
}

static int mutex_lock(pthread_mutex_t *mutex, int try_only,
                      const struct timespec *abstime) {
  if (NACL_LIKELY(mutex->mutex_type == PTHREAD_MUTEX_FAST_NP)) {
    return mutex_lock_nonrecursive(mutex, try_only, abstime);
  }

  /*
   * Reading owner_thread_id here must be done atomically, because
   * this read may be concurrent with pthread_mutex_unlock()'s write.
   * PNaCl's memory model requires these accesses to be declared as
   * atomic, which under PNaCl is achieved by declaring
   * owner_thread_id as "volatile".
   *
   * Checking the mutex's owner_thread_id without further
   * synchronization is safe.  We are checking whether the owner's id
   * is equal to the current thread id, and this can happen only if
   * the current thread is actually the owner, otherwise the owner id
   * will hold an illegal value or an id of a different thread.
   */
  pthread_t self = pthread_self();
  if (mutex->owner_thread_id == self) {
    if (mutex->mutex_type == PTHREAD_MUTEX_ERRORCHECK_NP) {
      return EDEADLK;
    } else {
      /* This thread already owns the mutex. */
      ++mutex->recursion_counter;
      return 0;
    }
  }
  int err = mutex_lock_nonrecursive(mutex, try_only, abstime);
  if (err != 0)
    return err;
  mutex->owner_thread_id = self;
  mutex->recursion_counter = 1;
  return 0;
}

int pthread_mutex_trylock(pthread_mutex_t *mutex) {
  return mutex_lock(mutex, 1, NULL);
}

int pthread_mutex_lock(pthread_mutex_t *mutex) {
  return mutex_lock(mutex, 0, NULL);
}

int pthread_mutex_timedlock(pthread_mutex_t *mutex,
                            const struct timespec *abstime) {
  return mutex_lock(mutex, 0, abstime);
}

int pthread_mutex_unlock(pthread_mutex_t *mutex) {
  if (NACL_UNLIKELY(mutex->mutex_type != PTHREAD_MUTEX_FAST_NP)) {
    if ((PTHREAD_MUTEX_RECURSIVE_NP == mutex->mutex_type) &&
        (0 != (--mutex->recursion_counter))) {
      /*
       * We assume that this thread owns the lock
       * (no verification for recursive locks),
       * so just decrement the counter, this thread is still the owner.
       */
      return 0;
    }
    if ((PTHREAD_MUTEX_ERRORCHECK_NP == mutex->mutex_type) &&
        (pthread_self() != mutex->owner_thread_id)) {
      /* Error - releasing a mutex that's free or owned by another thread. */
      return EPERM;
    }
    /* Writing to owner_thread_id here must be done atomically. */
    mutex->owner_thread_id = NACL_PTHREAD_ILLEGAL_THREAD_ID;
    mutex->recursion_counter = 0;
  }

  /*
   * Release the mutex.  This atomic decrement executes the full
   * memory barrier that pthread_mutex_unlock() is required to
   * execute.
   */
  int old_state = __sync_fetch_and_sub(&mutex->mutex_state, 1);
  if (NACL_UNLIKELY(old_state != LOCKED_WITHOUT_WAITERS)) {
    if (old_state == UNLOCKED) {
      /*
       * The mutex was not locked.  mutex_state is now -1 and the
       * mutex is likely unusable, but that is the caller's fault for
       * using the mutex interface incorrectly.
       */
      return EPERM;
    }
    /*
     * We decremented mutex_state from LOCKED_WITH_WAITERS to
     * LOCKED_WITHOUT_WAITERS.  We must now release the mutex fully.
     *
     * No further memory barrier is required for the following
     * modification of mutex_state.  The full memory barrier from the
     * atomic decrement acts as a release memory barrier for the
     * following modification.
     *
     * TODO(mseaborn): Change the following store to use an atomic
     * store builtin when this is available in all the NaCl
     * toolchains.  For now, PNaCl converts the volatile store to an
     * atomic store.
     */
    mutex->mutex_state = UNLOCKED;
    int woken;
    __libnacl_irt_futex.futex_wake(&mutex->mutex_state, 1, &woken);
  }
  return 0;
}

/*
 * NOTE(sehr): pthread_once needs to be defined in the same module as contains
 * the mutex definitions, so that it overrides the weak symbol in the libstdc++
 * library.  Otherwise we get calls through address zero.
 */
int pthread_once(pthread_once_t *once_control,
                 void (*init_routine)(void)) {
  /*
   * NOTE(gregoryd): calling pthread_once from init_routine providing
   * the same once_control argument is an error and will cause a
   * deadlock
   */
  volatile AtomicInt32 *pdone = &once_control->done;
  if (*pdone == 0) {
    /* Not done yet. */
    pthread_mutex_lock(&once_control->lock);
    if (*pdone == 0) {
      /* Still not done - but this time we own the lock. */
      (*init_routine)();

      /*
       * GCC intrinsic; see:
       * http://gcc.gnu.org/onlinedocs/gcc/Atomic-Builtins.html.
       * The x86-{32,64} compilers generate inline code.  The ARM
       * implementation is external: stubs/intrinsics_arm.S.
       */
      __sync_fetch_and_add(pdone, 1);
    }
    pthread_mutex_unlock(&once_control->lock);
  }
  return 0;
}

int pthread_mutexattr_init(pthread_mutexattr_t *attr) {
  /* The default mutex type is non-recursive. */
  attr->kind = PTHREAD_MUTEX_FAST_NP;
  return 0;
}

int pthread_mutexattr_destroy(pthread_mutexattr_t *attr) {
  /* Nothing to do. */
  return 0;
}

int pthread_mutexattr_settype(pthread_mutexattr_t *attr,
                              int kind) {
  if ((kind == PTHREAD_MUTEX_FAST_NP) ||
      (kind == PTHREAD_MUTEX_RECURSIVE_NP) ||
      (kind == PTHREAD_MUTEX_ERRORCHECK_NP)) {
    attr->kind = kind;
  } else {
    /* Unknown kind. */
    return -1;
  }
  return 0;
}

int pthread_mutexattr_gettype(const pthread_mutexattr_t *attr,
                              int *kind) {
  *kind = attr->kind;
  return 0;
}

int pthread_mutexattr_setpshared(pthread_mutexattr_t *attr,
                                 int pshared) {
  switch (pshared) {
    case PTHREAD_PROCESS_PRIVATE:
      return 0;
    case PTHREAD_PROCESS_SHARED:
      return ENOTSUP;
    default:
      return EINVAL;
  }
}

int pthread_mutexattr_getpshared(const pthread_mutexattr_t *attr,
                                 int *pshared) {
  *pshared = PTHREAD_PROCESS_PRIVATE;
  return 0;
}
