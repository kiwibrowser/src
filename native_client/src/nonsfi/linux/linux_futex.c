/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <sys/time.h>
#include <time.h>

#include "native_client/src/nonsfi/linux/abi_conversion.h"
#include "native_client/src/nonsfi/linux/linux_syscall_defines.h"
#include "native_client/src/nonsfi/linux/linux_syscall_wrappers.h"
#include "native_client/src/public/linux_syscalls/sys/syscall.h"
#include "native_client/src/trusted/service_runtime/include/sys/time.h"
#include "native_client/src/untrusted/irt/irt_interfaces.h"
#include "native_client/src/untrusted/pthread/pthread_internal.h"

/*
 * Converts a pair of timespec of absolute time and host's timespec of
 * the current time to host's timespec of the relative time between them.
 * Note that assuming tv_nsec for the both input structs are non-negative,
 * the returned reltime's tv_nsec is also always non-negative.
 * (I.e., for -0.1 sec, {tv_sec: -1, tv_nsec: 900000000} will be returned).
 */
static void abs_time_to_linux_rel_time(const struct timespec *nacl_abstime,
                                       const struct timespec *now,
                                       struct linux_abi_timespec *reltime) {
  reltime->tv_sec = nacl_abstime->tv_sec - now->tv_sec;
  reltime->tv_nsec = nacl_abstime->tv_nsec - now->tv_nsec;
  if (reltime->tv_nsec < 0) {
    reltime->tv_sec -= 1;
    reltime->tv_nsec += 1000000000;
  }
}

static int nacl_irt_futex_wait_abs(volatile int *addr, int value,
                                   const struct timespec *abstime) {
  struct linux_abi_timespec timeout;
  struct linux_abi_timespec *timeout_ptr = NULL;
  if (abstime) {
    /*
     * futex syscall takes relative timeout, but the ABI for IRT's
     * futex_wait_abs is absolute timeout. So, here we convert it.
     */
    struct timespec now;
    if (clock_gettime(CLOCK_REALTIME, &now) < 0)
      return errno;
    abs_time_to_linux_rel_time(abstime, &now, &timeout);

    /*
     * Linux's FUTEX_WAIT returns EINVAL for negative timeout, but an absolute
     * time that is in the past is a valid argument to irt_futex_wait_abs(),
     * and a caller expects ETIMEDOUT.
     * Here check only tv_sec since we assume time_t is signed. See also
     * the comment for abs_time_to_rel_time.
     */
    if (timeout.tv_sec < 0)
      return ETIMEDOUT;
    timeout_ptr = &timeout;
  }

  int result = linux_syscall6(__NR_futex, (uintptr_t) addr, FUTEX_WAIT_PRIVATE,
                              value, (uintptr_t) timeout_ptr, 0, 0);
  if (result < 0)
    return -result;
  return 0;
}

static int nacl_irt_futex_wake(volatile int *addr, int nwake, int *count) {
  int result = linux_syscall6(__NR_futex, (uintptr_t) addr, FUTEX_WAKE_PRIVATE,
                              nwake, 0, 0, 0);
  if (result < 0)
    return -result;
  *count = result;
  return 0;
}

const struct nacl_irt_futex nacl_irt_futex = {
  nacl_irt_futex_wait_abs,
  nacl_irt_futex_wake,
};

extern struct nacl_irt_futex __libnacl_irt_futex
  __attribute__((alias("nacl_irt_futex")));
