/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/service_runtime/sys_futex.h"

#include "native_client/src/include/build_config.h"
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/nacl_copy.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"

#if NACL_LINUX

#include <errno.h>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/shared/platform/nacl_clock.h"
#include "native_client/src/shared/platform/nacl_host_desc.h"

#if defined(__ANDROID__) && !defined(FUTEX_PRIVATE_FLAG)
/*
 * Android's Linux headers currently don't define these flags.
 * Note: nacl_irt_futex implementation should always use PRIVATE futexes.
 * See irt.h for more details.
 */
# define FUTEX_PRIVATE_FLAG 128
#endif

static void AbsTimeToRelTime(const struct nacl_abi_timespec *abstime,
                             const struct nacl_abi_timespec *now,
                             struct timespec *host_reltime) {
  host_reltime->tv_sec = abstime->tv_sec - now->tv_sec;
  host_reltime->tv_nsec = abstime->tv_nsec - now->tv_nsec;
  if (host_reltime->tv_nsec < 0) {
    host_reltime->tv_sec -= 1;
    host_reltime->tv_nsec += 1000000000;
  }
}

int32_t NaClSysFutexWaitAbs(struct NaClAppThread *natp, uint32_t addr,
                            uint32_t value, uint32_t abstime_ptr) {
  int result;
  struct NaClApp *nap = natp->nap;
  struct nacl_abi_timespec abstime;
  struct nacl_abi_timespec now;
  struct timespec host_rel_timeout;

  uintptr_t sysaddr = NaClUserToSysAddrRange(nap, addr, sizeof(uint32_t));
  if (kNaClBadAddress == sysaddr) {
    NaClLog(1, "NaClSysFutexWaitAbs: address out of range\n");
    return -NACL_ABI_EFAULT;
  }

  if (abstime_ptr != 0) {
    if (!NaClCopyInFromUser(nap, &abstime, abstime_ptr, sizeof(abstime))) {
      return -NACL_ABI_EFAULT;
    }
    result = NaClClockGetTime(NACL_CLOCK_REALTIME, &now);
    if (result != 0) {
      return result;
    }
    AbsTimeToRelTime(&abstime, &now, &host_rel_timeout);
    /*
     * Linux's FUTEX_WAIT returns EINVAL for negative timeout, but an absolute
     * time that is in the past is a valid argument to irt_futex_wait_abs(),
     * and a caller expects ETIMEDOUT.
     */
    if (host_rel_timeout.tv_sec < 0) {
      return -NACL_ABI_ETIMEDOUT;
    }
  }

  if (syscall(__NR_futex,
              sysaddr,
              FUTEX_WAIT | FUTEX_PRIVATE_FLAG,
              value,
              (abstime_ptr != 0 ? (uintptr_t) &host_rel_timeout : 0),
              0,
              0)) {
    /*
     * The non-Linux implementation will crash on a bad address here,
     * so ensure the Linux implementation behaves consistently.
     */
    if (errno == EFAULT) {
      NaClLog(LOG_FATAL,
              "NaClSysFutexWaitAbs: Futex syscall returned EFAULT; "
              "aborting for consistency\n");
    }
    return -NaClXlateErrno(errno);
  }
  return 0;
}

int32_t NaClSysFutexWake(struct NaClAppThread *natp, uint32_t addr,
                         uint32_t nwake) {
  int woken_count;
  struct NaClApp *nap = natp->nap;

  uintptr_t sysaddr = NaClUserToSysAddrRange(nap, addr, sizeof(uint32_t));
  if (kNaClBadAddress == sysaddr) {
    NaClLog(1, "NaClSysFutexWake: address out of range\n");
    return 0;
  }
  woken_count = syscall(__NR_futex,
                        sysaddr,
                        FUTEX_WAKE | FUTEX_PRIVATE_FLAG,
                        nwake,
                        0,
                        0,
                        0);
  if (woken_count < 0) {
    return -NaClXlateErrno(errno);
  }
  return woken_count;
}

#else

/*
 * This is a simple futex implementation that is based on the
 * untrusted-code futex implementation from the NaCl IRT
 * (irt_futex.c), which in turn was based on futex_emulation.c from
 * nacl-glibc.
 *
 * The main way the performance of this implementation could be improved
 * is as follows:
 *
 * The current futex_wake() implementation does a linear search while
 * holding a global lock, which could perform poorly if there are large
 * numbers of waiting threads.
 *
 * We could use a hash table rather than a single linked list for looking
 * up wait addresses.  Furthermore, to reduce lock contention, we could
 * use one lock per hash bucket rather than a single lock.
 */


static void ListAddNodeAtEnd(struct NaClListNode *new_node,
                             struct NaClListNode *head) {
  head->prev->next = new_node;
  new_node->prev = head->prev;
  new_node->next = head;
  head->prev = new_node;
}

static void ListRemoveNode(struct NaClListNode *node) {
  node->next->prev = node->prev;
  node->prev->next = node->next;
}

/*
 * Given a pointer to a NaClAppThread's futex_wait_list_node, this
 * returns a pointer to the NaClAppThread.
 */
static struct NaClAppThread *GetNaClAppThreadFromListNode(
    struct NaClListNode *node) {
  return (struct NaClAppThread *)
         ((uintptr_t) node -
          offsetof(struct NaClAppThread, futex_wait_list_node));
}

int32_t NaClSysFutexWaitAbs(struct NaClAppThread *natp, uint32_t addr,
                            uint32_t value, uint32_t abstime_ptr) {
  struct NaClApp *nap = natp->nap;
  struct nacl_abi_timespec abstime;
  uint32_t read_value;
  int32_t result;
  NaClSyncStatus sync_status;

  if (abstime_ptr != 0) {
    if (!NaClCopyInFromUser(nap, &abstime, abstime_ptr, sizeof(abstime))) {
      return -NACL_ABI_EFAULT;
    }
  }

  NaClXMutexLock(&nap->futex_wait_list_mu);

  /*
   * Note about lock ordering: NaClCopyInFromUser() can claim the
   * mutex nap->mu.  nap->mu may be claimed after
   * nap->futex_wait_list_mu but never before it.
   */
  if (!NaClCopyInFromUser(nap, &read_value, addr, sizeof(uint32_t))) {
    result = -NACL_ABI_EFAULT;
    goto cleanup;
  }
  if (read_value != value) {
    result = -NACL_ABI_EWOULDBLOCK;
    goto cleanup;
  }

  /* Add the current thread onto the futex wait list. */
  natp->futex_wait_addr = addr;
  ListAddNodeAtEnd(&natp->futex_wait_list_node, &nap->futex_wait_list_head);

  if (abstime_ptr == 0) {
    sync_status = NaClCondVarWait(
        &natp->futex_condvar, &nap->futex_wait_list_mu);
  } else {
    sync_status = NaClCondVarTimedWaitAbsolute(
        &natp->futex_condvar, &nap->futex_wait_list_mu, &abstime);
  }
  result = -NaClXlateNaClSyncStatus(sync_status);

  if (natp->futex_wait_list_node.next == NULL) {
    /*
     * This thread was woken by NaClSysFutexWake(), which removed this
     * thread from the wait queue.  It might also be the case that
     * NaClCondVarTimedWaitAbsolute() timed out and returned a timeout
     * error status value -- the timeout can race with NaClSysFutexWake().
     * If that happened, we still want to return a success status.
     *
     * Here is an example of what could go wrong if we returned ETIMEDOUT
     * here: Suppose the FutexWake() call came from pthread_mutex_unlock()
     * and the FutexWait() call came from pthread_mutex_timedlock().
     * Suppose we have a typical implementation of
     * pthread_mutex_timedlock() which returns immediately without trying
     * again to claim the mutex if FutexWait() returns ETIMEDOUT.  If
     * another thread were waiting on the mutex, it wouldn't get woken --
     * the wakeup from the FutexWake() call would have got lost.
     *
     * See https://bugs.chromium.org/p/nativeclient/issues/detail?id=4373
     */
    result = 0;
  } else {
    /*
     * A timeout or spurious wakeup occurred, so NaClSysFutexWake() did not
     * remove this thread from the wait queue, so we must remove it from
     * the wait queue ourselves.
     */
    ListRemoveNode(&natp->futex_wait_list_node);
  }
  /* Clear these fields to prevent their accidental use. */
  natp->futex_wait_list_node.next = NULL;
  natp->futex_wait_list_node.prev = NULL;
  natp->futex_wait_addr = 0;

cleanup:
  NaClXMutexUnlock(&nap->futex_wait_list_mu);
  return result;
}

int32_t NaClSysFutexWake(struct NaClAppThread *natp, uint32_t addr,
                         uint32_t nwake) {
  struct NaClApp *nap = natp->nap;
  struct NaClListNode *entry;
  uint32_t woken_count = 0;

  NaClXMutexLock(&nap->futex_wait_list_mu);

  /* We process waiting threads in FIFO order. */
  entry = nap->futex_wait_list_head.next;
  while (nwake > 0 && entry != &nap->futex_wait_list_head) {
    struct NaClListNode *next = entry->next;
    struct NaClAppThread *waiting_thread = GetNaClAppThreadFromListNode(entry);

    if (waiting_thread->futex_wait_addr == addr) {
      ListRemoveNode(entry);
      /*
       * Mark the thread as having been removed from the wait queue:
       * tell it not to try to remove itself from the queue.
       */
      entry->next = NULL;

      /* Also clear these fields to prevent their accidental use. */
      entry->prev = NULL;
      waiting_thread->futex_wait_addr = 0;

      NaClXCondVarSignal(&waiting_thread->futex_condvar);
      woken_count++;
      nwake--;
    }
    entry = next;
  }

  NaClXMutexUnlock(&nap->futex_wait_list_mu);

  return woken_count;
}

#endif
