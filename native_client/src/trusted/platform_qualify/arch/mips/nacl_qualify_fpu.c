/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <setjmp.h>
#include <signal.h>
#include <stdlib.h>

#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/platform_qualify/arch/mips/nacl_mips_qualify.h"

static sigjmp_buf g_try_state;

static void SignalCatch(int sig) {
  siglongjmp(g_try_state, sig);
}

/*
 * Returns 1 if the CPU has FPU unit and floating point registers are in
 * 32-bit mode.
 */
int NaClQualifyFpu(void) {
  struct sigaction old_sigaction_ill;
  struct sigaction old_sigaction_fpe;
  struct sigaction try_sigaction;
  double result = 0;

  try_sigaction.sa_handler = SignalCatch;
  if (sigemptyset(&try_sigaction.sa_mask)) {
    NaClLog(LOG_FATAL, "Failed to initialise and empty a signal set\n");
    return 0;
  }
  try_sigaction.sa_flags = 0;

  if (sigaction(SIGILL, &try_sigaction, &old_sigaction_ill)) {
    NaClLog(LOG_FATAL, "Failed to install handler for SIGILL.\n");
    return 0;
  }

  if (sigaction(SIGFPE, &try_sigaction, &old_sigaction_fpe)) {
    NaClLog(LOG_FATAL, "Failed to install handler for SIGFPE.\n");
    return 0;
  }

  if (0 == sigsetjmp(g_try_state, 1)) {
    /* Bit representation of (double)1 is 0x3FF0000000000000 */
    asm("lui   $t0, 0x3FF0\n"
        "ldc1  $f0, %0\n"
        "mtc1  $t0, $f1\n"
        "sdc1  $f0, %0\n"
        : "+m" (result)
        : : "t0", "$f0", "$f1", "memory");
  }

  if (sigaction(SIGILL, &old_sigaction_ill, NULL)) {
    NaClLog(LOG_FATAL, "Failed restoring handler for SIGILL.\n");
    return 0;
  }
  if (sigaction(SIGFPE, &old_sigaction_fpe, NULL)) {
    NaClLog(LOG_FATAL, "Failed restoring handler for SIGFPE.\n");
    return 0;
  }

  return result == (double) 1;
}
