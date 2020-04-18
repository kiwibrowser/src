/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <setjmp.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>

#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/platform_qualify/arch/arm/nacl_arm_qualify.h"

static sigjmp_buf try_state;

static void signal_catch(int sig) {
  siglongjmp(try_state, sig);
}

/*
 * Returns 1 if VFP/NEON instructions work.
 */
int NaClQualifyFpu(void) {
  struct sigaction old_sigaction;
  struct sigaction try_sigaction;
  volatile int success = 0;

  try_sigaction.sa_handler = signal_catch;
  sigemptyset(&try_sigaction.sa_mask);
  try_sigaction.sa_flags = 0;

  if (0 != sigaction(SIGILL, &try_sigaction, &old_sigaction)) {
    NaClLog(LOG_FATAL, "Failed to install handler for SIGILL.\n");
    return 0;
  }

  if (0 == sigsetjmp(try_state, 1)) {
    volatile struct {
      uint64_t v[2] __attribute__((aligned(16)));
    } x = { { 0x0d0e0a0d0b0e0e0fULL, 0x1f1e1e1d1f1a1c1eULL } },
      y = { { 0x1010101010101010ULL, 0x1010101010101010ULL } };
    asm(".fpu neon\n"
        "vldm %3, {d28-d29}\n"  /* q14 = x */
        "vldm %4, {d30-d31}\n"  /* q15 = y */
        "vadd.i64 q15, q14, q15\n"  /* q15 += q14 */
        "vstm %3, {d30-d31}\n"  /* x = q15 */
        : "=m" (x) : "m" (x), "m" (y), "r" (&x), "r" (&y)
        : "q14", "q15");
    success = (x.v[0] == 0x1d1e1a1d1b1e1e1fULL &&
               x.v[1] == 0x2f2e2e2d2f2a2c2eULL);
  }

  if (0 != sigaction(SIGILL, &old_sigaction, NULL)) {
    NaClLog(LOG_FATAL, "Failed to restore handler for SIGILL.\n");
    return 0;
  }

  return success;
}
