/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Native Client condition variable API
 */

#include <errno.h>
#include <limits.h>
#include <unistd.h>

#include "native_client/src/untrusted/nacl/nacl_irt.h"
#include "native_client/src/untrusted/pthread/pthread.h"
#include "native_client/src/untrusted/pthread/pthread_internal.h"
#include "native_client/src/untrusted/pthread/pthread_types.h"

/*
 * This implementation is based on the condvar implementation in
 * Android's C library, Bionic.  See libc/bionic/pthread.c in Bionic:
 *
 * https://android.googlesource.com/platform/bionic.git/+/7a34ed2bb36fcbe6967d8b670f4d70ada1dcef49/libc/bionic/pthread.c
 *
 * Technically there is a race condition in our pthread_cond_wait():
 * It could miss a wakeup if pthread_cond_signal() or
 * pthread_cond_broadcast() is called an exact multiple of 2^32 times
 * between pthread_cond_wait()'s fetching of cond->sequence_number and
 * its call to futex_wait().  That is very unlikely to happen,
 * however.
 *
 * Unlike glibc's more complex condvar implementation, we do not
 * attempt to optimize pthread_cond_signal/broadcast() to avoid a
 * futex_wake() call in the case where there are no waiting threads.
 */


/*
 * Initialize condition variable COND using attributes ATTR, or use
 * the default values if later is NULL.
 */
int pthread_cond_init(pthread_cond_t *cond,
                      const pthread_condattr_t *cond_attr) {
  cond->sequence_number = 0;
  return 0;
}

/*
 * Destroy condition variable COND.
 */
int pthread_cond_destroy(pthread_cond_t *cond) {
  return 0;
}

static int pulse(pthread_cond_t *cond, int count) {
  /*
   * This atomic increment executes the full memory barrier that
   * pthread_cond_signal/broadcast() are required to execute.
   */
  __sync_fetch_and_add(&cond->sequence_number, 1);

  int unused_woken_count;
  __libnacl_irt_futex.futex_wake(&cond->sequence_number, count,
                                 &unused_woken_count);
  return 0;
}

/*
 * Wake up one thread waiting for condition variable COND.
 */
int pthread_cond_signal(pthread_cond_t *cond) {
  return pulse(cond, 1);
}

int pthread_cond_broadcast(pthread_cond_t *cond) {
  return pulse(cond, INT_MAX);
}

int pthread_cond_wait(pthread_cond_t *cond,
                      pthread_mutex_t *mutex) {
  return pthread_cond_timedwait_abs(cond, mutex, NULL);
}

int pthread_cond_timedwait_abs(pthread_cond_t *cond,
                               pthread_mutex_t *mutex,
                               const struct timespec *abstime) {
  int old_value = cond->sequence_number;

  int err = pthread_mutex_unlock(mutex);
  if (err != 0)
    return err;

  int status = __libnacl_irt_futex.futex_wait_abs(&cond->sequence_number,
                                                  old_value, abstime);

  err = pthread_mutex_lock(mutex);
  if (err != 0)
    return err;

  /*
   * futex_wait() can return EWOULDBLOCK but pthread_cond_wait() is
   * not allowed to return that.
   */
  if (status == ETIMEDOUT) {
    return ETIMEDOUT;
  } else {
    return 0;
  }
}

int pthread_condattr_init(pthread_condattr_t *attr) {
  return 0;
}

int pthread_condattr_destroy(pthread_condattr_t *attr) {
  return 0;
}

int pthread_condattr_getpshared(const pthread_condattr_t *attr, int *pshared) {
  *pshared = PTHREAD_PROCESS_PRIVATE;
  return 0;
}

int pthread_condattr_setpshared(pthread_condattr_t *attr, int pshared) {
  switch (pshared) {
    case PTHREAD_PROCESS_PRIVATE:
      return 0;
    case PTHREAD_PROCESS_SHARED:
      return ENOTSUP;
    default:
      return EINVAL;
  }
}

int pthread_condattr_getclock(const pthread_condattr_t *attr,
                              clockid_t *clock_id) {
  *clock_id = CLOCK_REALTIME;
  return 0;
}

int pthread_condattr_setclock(pthread_condattr_t *attr, clockid_t clock_id) {
  switch (clock_id) {
    case CLOCK_REALTIME:  /* Only one really supported.  */
      return 0;

    case CLOCK_MONOTONIC: /* A "known clock", but not supported for this.  */
      return ENOTSUP;

    default:
      /*
       * Anything else is either not a "known clock", or is a CPU-time clock.
       */
      return EINVAL;
  }
}
