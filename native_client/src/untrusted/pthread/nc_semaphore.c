/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
* Native Client semaphore API
*/

#include <unistd.h>
#include <errno.h>

#include "native_client/src/untrusted/irt/irt_interfaces.h"
#include "native_client/src/untrusted/nacl/nacl_irt.h"

#include "native_client/src/untrusted/pthread/pthread.h"
#include "native_client/src/untrusted/pthread/pthread_internal.h"
#include "native_client/src/untrusted/pthread/pthread_types.h"
#include "native_client/src/untrusted/pthread/semaphore.h"

/*
 * This semaphore implementation is based on the algorithm used by
 * glibc's semaphore implementation.
 *
 * sem_t contains a "count" field which would be sufficient to
 * implement the semaphore by itself.  As an additional optimization,
 * sem_t also contains an "nwaiters" field, which tracks whether any
 * threads are waiting on the semaphore.  This means sem_post() can
 * avoid calling futex_wake() if there are no waiters.  futex_wake()
 * can be relatively expensive because it checks the futex wait queue
 * and may involve doing a syscall or claiming a mutex.
 *
 * For comparison, Bionic (Android's C library) uses a slightly
 * different algorithm.  sem_t contains only a "count" field, and a
 * special count value of -1 is used to indicate that one or more
 * threads are waiting.  The disadvantage of this approach is that
 * sem_post() must wake all waiting threads when count is -1, which
 * creates a thundering herd problem.  If the woken threads wake
 * quickly enough, all but one of them will fail to decrement the
 * semaphore count and will wait again.  Because of this problem, I've
 * chosen not to use this algorithm.  sem_post() below wakes only a
 * single thread.
 */


/* Initialize semaphore  */
int sem_init(sem_t *sem, int pshared, unsigned int value) {
  if (pshared) {
    /* We don't support shared semaphores yet. */
    errno = ENOSYS;
    return -1;
  }
  if (value > SEM_VALUE_MAX) {
    errno = EINVAL;
    return -1;
  }
  sem->count = value;
  sem->nwaiters = 0;
  return 0;
}

int sem_destroy(sem_t *sem) {
  if (sem->nwaiters != 0) {
    errno = EBUSY;
    return -1;
  }
  return 0;
}

static int decrement_if_positive(volatile int *ptr) {
  int old_value;
  do {
    old_value = *ptr;
    if (old_value == 0)
      return 0;
  } while (!__sync_bool_compare_and_swap(ptr, old_value, old_value - 1));
  return 1;
}

int sem_wait(sem_t *sem) {
  if (decrement_if_positive(&sem->count))
    return 0;

  __sync_fetch_and_add(&sem->nwaiters, 1);
  do {
    __libnacl_irt_futex.futex_wait_abs(&sem->count, 0, NULL);
  } while (!decrement_if_positive(&sem->count));
  __sync_fetch_and_sub(&sem->nwaiters, 1);
  return 0;
}

int sem_trywait(sem_t *sem) {
  if (decrement_if_positive(&sem->count))
    return 0;

  errno = EAGAIN;
  return -1;
}

int sem_post(sem_t *sem) {
  /* Increment sem->count, checking for overflow. */
  int old_value;
  do {
    old_value = sem->count;
    if (old_value == SEM_VALUE_MAX) {
      errno = EOVERFLOW;
      return -1;
    }
  } while (!__sync_bool_compare_and_swap(&sem->count, old_value,
                                         old_value + 1));

  /*
   * We only need to call futex_wake() if there are waiters.  Note
   * that this might do an unnecessary call to futex_wake() if the
   * last waiter has already been woken using futex_wake() but has not
   * yet decremented sem->nwaiters.
   */
  if (sem->nwaiters != 0) {
    int woken_count;
    __libnacl_irt_futex.futex_wake(&sem->count, 1, &woken_count);
  }
  return 0;
}

int sem_getvalue(sem_t *sem, int *value) {
  *value = sem->count;
  return 0;
}
