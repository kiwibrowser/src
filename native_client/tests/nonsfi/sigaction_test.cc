/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <setjmp.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include "native_client/src/include/nacl_assert.h"
#include "native_client/src/nonsfi/linux/linux_sys_private.h"
#include "native_client/src/nonsfi/linux/linux_syscall_defines.h"
#include "native_client/src/nonsfi/linux/linux_syscall_structs.h"

static jmp_buf g_jmp_buf;

static void return_from_signal_handler(int signo, linux_siginfo_t *info,
                                       void *data) {
  ASSERT_EQ(signo, SIGSEGV);
  ASSERT_NE(info, NULL);
  ASSERT_EQ(info->si_signo, SIGSEGV);
  ASSERT_NE(data, NULL);
  longjmp(g_jmp_buf, 42);
}

int main(int argc, char *argv[]) {
  struct linux_sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_sigaction = return_from_signal_handler;
  sa.sa_flags = LINUX_SA_SIGINFO;
  sigset_t *mask = (sigset_t *)&sa.sa_mask;
  sigemptyset(mask);
  sigaddset(mask, LINUX_SIGSEGV);

  int rc = linux_sigaction(LINUX_SIGSEGV, &sa, NULL);
  ASSERT_EQ(rc, 0);

  rc = setjmp(g_jmp_buf);
  if (rc == 0) {
    /* Crash. */
    *(volatile int *) 0 = 0;
    _exit(1); /* Shouldn't reach here. */
  }

  ASSERT_EQ(rc, 42);

  /*
   * We do not test oldact (the third argument of sigaction) as user
   * mode qemu seems not to support it and crash with it.
   */

  puts("PASSED");
  return 0;
}
