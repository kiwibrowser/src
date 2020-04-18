/*
 * Copyright 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Native Client rwlock implementation
 *
 * This implementation is a 'write-preferring' reader-writer lock which
 * avoids writer starvation by preventing readers from acquiring the lock
 * while there are waiting writers (with an exception to prevent deadlocks
 * in the case of recursive read lock (see read_lock_available)). See:
 * http://en.wikipedia.org/wiki/Readers%E2%80%93writer_lock
 *
 * The thundering herd problem is avoided (at least for waiting writers)
 * by only waking a single writer at a time.
 */

#include <errno.h>

#include "native_client/src/untrusted/pthread/pthread.h"
#include "native_client/src/untrusted/pthread/pthread_internal.h"
#include "native_client/src/untrusted/pthread/pthread_types.h"

/* Number of rdlocks that the current thread holds. */
static __thread unsigned int thread_rdlock_count = 0;

int pthread_rwlockattr_init(pthread_rwlockattr_t *attr) {
  return 0;
}

int pthread_rwlockattr_destroy(pthread_rwlockattr_t *attr) {
  return 0;
}

int pthread_rwlockattr_getpshared(const pthread_rwlockattr_t *attr,
                                  int *pshared) {
  *pshared = PTHREAD_PROCESS_PRIVATE;
  return 0;
}

int pthread_rwlockattr_setpshared(pthread_rwlockattr_t *attr, int pshared) {
  if (pshared != PTHREAD_PROCESS_PRIVATE)
    return EINVAL;
  return 0;
}

int pthread_rwlock_init(pthread_rwlock_t *rwlock,
                        const pthread_rwlockattr_t *attr) {
  rwlock->writer_thread_id = NACL_PTHREAD_ILLEGAL_THREAD_ID;
  rwlock->writers_waiting = 0;
  rwlock->reader_count = 0;
  int rc = pthread_mutex_init(&rwlock->mutex, NULL);
  if (rc != 0)
    return rc;
  rc = pthread_cond_init(&rwlock->write_possible, NULL);
  if (rc != 0)
    return rc;
  return pthread_cond_init(&rwlock->read_possible, NULL);
}

/*
 * Helper function used by waiting writers to determine if they can take
 * the lock. The rwlock->mutex must be held when calling this function.
 * Returns 1 if the write lock can be taken, 0 if it can't.
 */
static inline int write_lock_available(pthread_rwlock_t *rwlock) {
  /*
   * Write lock is available if there is no current writer and no current
   * readers.
   */
  if (rwlock->writer_thread_id != NACL_PTHREAD_ILLEGAL_THREAD_ID)
    return 0;
  if (rwlock->reader_count > 0)
    return 0;
  return 1;
}

/*
 * Helper function used by waiting readers to determine if they can take
 * the lock. The rwlock->mutex must be held when calling this function.
 * Returns 1 if the write lock can be taken, 0 if it can't.
 */
static inline int read_lock_available(pthread_rwlock_t *rwlock) {
  /*
   * Read lock is unavailable if there is a current writer.
   */
  if (rwlock->writer_thread_id != NACL_PTHREAD_ILLEGAL_THREAD_ID)
    return 0;

  /*
   * Attempt to reduce writer starvation by blocking readers when there
   * is a waiting writer.  However don't do this if the current thread
   * already holds one or more rdlocks in order to allow for recursive
   * rdlocks. See: http://stackoverflow.com/questions/2190090/
   * how-to-prevent-writer-starvation-in-a-read-write-lock-in-pthreads
   */
  if (rwlock->writers_waiting > 0 && thread_rdlock_count == 0)
    return 0;
  return 1;
}

/*
 * Internal function used to acquire the read lock.
 * This operates in three different ways in order to implement the three public
 * functions:
 *  pthread_rwlock_rdlock
 *  pthread_rwlock_tryrdlock
 *  pthread_rwlock_timedrdlock
 */
static int rdlock_internal(pthread_rwlock_t *rwlock,
                           const struct timespec *abs_timeout,
                           int try_only) {
  int rc2;
  int rc = pthread_mutex_lock(&rwlock->mutex);
  if (rc != 0)
    return rc;

  /*
   * Wait repeatedly until the write preconditions are met.
   * In theory this loop should only execute once because the preconditions
   * should always be true when the condition is signaled.
   */
  while (!read_lock_available(rwlock)) {
    if (try_only) {
      rc = EBUSY;
    } else if (abs_timeout != NULL) {
      rc = pthread_cond_timedwait(&rwlock->read_possible,
                                  &rwlock->mutex,
                                  abs_timeout);
    } else {
      rc = pthread_cond_wait(&rwlock->read_possible, &rwlock->mutex);
    }
    if (rc != 0)
      goto done;
  }

  /* Acquire the read lock. */
  rwlock->reader_count++;
  thread_rdlock_count++;
done:
  rc2 = pthread_mutex_unlock(&rwlock->mutex);
  return rc == 0 ? rc2 : rc;
}

/*
 * Internal function used to acquire the write lock.
 * This operates in three different ways in order to implement the three public
 * functions:
 *  pthread_rwlock_wrlock
 *  pthread_rwlock_trywrlock
 *  pthread_rwlock_timedwrlock
 */
static int rwlock_internal(pthread_rwlock_t *rwlock,
                           const struct timespec *abs_timeout,
                           int try_only) {
  int rc2;
  int rc = pthread_mutex_lock(&rwlock->mutex);
  if (rc != 0)
    return rc;

  /* Wait repeatedly until the write preconditions are met */
  while (!write_lock_available(rwlock)) {
    if (try_only) {
      rc = EBUSY;
    } else {
      /*
       * Before waiting (and releasing the lock) we increment the
       * waiting_writers count so the unlocking code knows to wake
       * a writer first (before any waiting readers).
       */
      rwlock->writers_waiting++;
      if (abs_timeout != NULL) {
        rc = pthread_cond_timedwait(&rwlock->write_possible,
                                    &rwlock->mutex,
                                    abs_timeout);
      } else {
        rc = pthread_cond_wait(&rwlock->write_possible,
                               &rwlock->mutex);
      }
      rwlock->writers_waiting--;
    }
    if (rc != 0)
      goto done;
  }

  /* Acquire the write lock. */
  rwlock->writer_thread_id = pthread_self();
done:
  rc2 = pthread_mutex_unlock(&rwlock->mutex);
  return rc == 0 ? rc2 : rc;
}

int pthread_rwlock_timedrdlock(pthread_rwlock_t *rwlock,
                               const struct timespec *abs_timeout) {
  return rdlock_internal(rwlock, abs_timeout, 0);
}

int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock) {
  return rdlock_internal(rwlock, NULL, 0);
}

int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock) {
  return rdlock_internal(rwlock, NULL, 1);
}

int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock) {
  return rwlock_internal(rwlock, NULL, 0);
}

int pthread_rwlock_timedwrlock(pthread_rwlock_t *rwlock,
                               const struct timespec *abs_timeout) {
  return rwlock_internal(rwlock, abs_timeout, 0);
}

int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock) {
  return rwlock_internal(rwlock, NULL, 1);
}

int pthread_rwlock_unlock(pthread_rwlock_t *rwlock) {
  int rc2;
  int rc = pthread_mutex_lock(&rwlock->mutex);
  if (rc != 0)
    return rc;

  if (rwlock->writer_thread_id != NACL_PTHREAD_ILLEGAL_THREAD_ID) {
    /* The write lock is held. Ensure it's the current thread that holds it. */
    if (rwlock->writer_thread_id != pthread_self()) {
      rc = EPERM;
      goto done;
    }

    /* Release write lock. */
    rwlock->writer_thread_id = NACL_PTHREAD_ILLEGAL_THREAD_ID;
    if (rwlock->writers_waiting > 0) {
      /* Wake a waiting writer if there is one. */
      rc = pthread_cond_signal(&rwlock->write_possible);
    } else {
      /*
       * Otherwise wake all waiting readers.  All of them should be able
       * to make progress now that the write lock is no longer held.
       */
      rc = pthread_cond_broadcast(&rwlock->read_possible);
    }
  } else {
    if (rwlock->reader_count == 0) {
      rc = EPERM;
      goto done;
    }

    /* Release read lock. */
    rwlock->reader_count--;
    thread_rdlock_count--;
    if (rwlock->reader_count == 0 && rwlock->writers_waiting > 0) {
      /* Wake a waiting writer. */
      rc = pthread_cond_signal(&rwlock->write_possible);
    }
  }

done:
  rc2 = pthread_mutex_unlock(&rwlock->mutex);
  return rc == 0 ? rc2 : rc;
}

int pthread_rwlock_destroy(pthread_rwlock_t *rwlock) {
  /* Return EBUSY if another thread holds the mutex. */
  int rc = pthread_mutex_trylock(&rwlock->mutex);
  if (rc != 0) {
    return rc;
  }

  /* Return EBUSY if there are active readers or an active writer. */
  if (rwlock->reader_count != 0) {
    pthread_mutex_unlock(&rwlock->mutex);
    return EBUSY;
  }
  if (rwlock->writer_thread_id != NACL_PTHREAD_ILLEGAL_THREAD_ID) {
    pthread_mutex_unlock(&rwlock->mutex);
    return EBUSY;
  }

  int rc1 = pthread_cond_destroy(&rwlock->write_possible);
  int rc2 = pthread_cond_destroy(&rwlock->read_possible);

  /* Finally unlock the mutex and destroy it. */
  pthread_mutex_unlock(&rwlock->mutex);
  int rc3 = pthread_mutex_destroy(&rwlock->mutex);
  if (rc1 != 0)
    return rc1;
  if (rc2 != 0)
    return rc2;
  return rc3;
}
