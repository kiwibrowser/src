/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/mman.h>

#include "native_client/src/include/build_config.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"
#include "native_client/src/trusted/service_runtime/nacl_signal.h"


/*
 * NaCl uses POSIX signal handling on Linux, but not on Mac OS X.  We
 * register a signal stack on Mac OS X anyway, though, for safety, to
 * prevent a signal handler from running on an untrusted stack just in
 * case a signal handler gets left registered.
 */

/* Use 4K more than the minimum to allow breakpad to run. */
static uint32_t g_signal_stack_size = SIGSTKSZ + 4096;

#define STACK_GUARD_SIZE NACL_PAGESIZE

void NaClSignalStackSetSize(uint32_t size) {
  g_signal_stack_size = size;
}

int NaClSignalStackAllocate(void **result) {
  /*
   * We use mmap() to allocate the signal stack for two reasons:
   *
   * 1) By page-aligning the memory allocation (which malloc() does
   * not do for small allocations), we avoid allocating any real
   * memory in the common case in which the signal handler is never
   * run.
   *
   * 2) We get to create a guard page, to guard against the unlikely
   * occurrence of the signal handler both overrunning and doing so in
   * an exploitable way.
   */
  uint8_t *stack = mmap(NULL, g_signal_stack_size + STACK_GUARD_SIZE,
                        PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON,
                        -1, 0);
  if (stack == MAP_FAILED) {
    return 0;
  }
  /* We assume that the stack grows downwards. */
  if (mprotect(stack, STACK_GUARD_SIZE, PROT_NONE) != 0) {
    NaClLog(LOG_FATAL, "Failed to mprotect() the stack guard page:\n\t%s\n",
            strerror(errno));
  }
  *result = stack;
  return 1;
}

void NaClSignalStackFree(void *stack) {
  CHECK(stack != NULL);
  if (munmap(stack, g_signal_stack_size + STACK_GUARD_SIZE) != 0) {
    NaClLog(LOG_FATAL, "Failed to munmap() signal stack:\n\t%s\n",
            strerror(errno));
  }
}

void NaClSignalStackRegister(void *stack) {
  /*
   * If we set up signal handlers, we must ensure that any thread that
   * runs untrusted code has an alternate signal stack set up.  The
   * default for a new thread is to use the stack pointer from the
   * point at which the fault occurs, but it would not be safe to use
   * untrusted code's %esp/%rsp value.
   */
  stack_t st;
  st.ss_size = g_signal_stack_size;
  st.ss_sp = ((uint8_t *) stack) + STACK_GUARD_SIZE;
  st.ss_flags = 0;
  if (sigaltstack(&st, NULL) != 0) {
    NaClLog(LOG_FATAL, "Failed to register signal stack:\n\t%s\n",
            strerror(errno));
  }
}

void NaClSignalStackUnregister(void) {
  /*
   * Unregister the signal stack in case a fault occurs between the
   * thread deallocating the signal stack and exiting.  Such a fault
   * could be unsafe if the address space were reallocated before the
   * fault, although that is unlikely.
   */
  stack_t st;
#if NACL_OSX
  /*
   * This is a workaround for a bug in Mac OS X's libc, in which new
   * versions of the sigaltstack() wrapper return ENOMEM if ss_size is
   * less than MINSIGSTKSZ, even when ss_size should be ignored
   * because we are unregistering the signal stack.
   * See http://code.google.com/p/nativeclient/issues/detail?id=1053
   */
  st.ss_size = MINSIGSTKSZ;
#else
  st.ss_size = 0;
#endif
  st.ss_sp = NULL;
  st.ss_flags = SS_DISABLE;
  if (sigaltstack(&st, NULL) != 0) {
    NaClLog(LOG_FATAL, "Failed to unregister signal stack:\n\t%s\n",
            strerror(errno));
  }
}
