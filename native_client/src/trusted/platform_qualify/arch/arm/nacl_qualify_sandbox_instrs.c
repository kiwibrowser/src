/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <setjmp.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>

#include "native_client/src/include/arm_sandbox.h"
#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/platform_qualify/arch/arm/nacl_arm_qualify.h"

static sigjmp_buf try_state;

static void signal_catch(int sig) {
  siglongjmp(try_state, sig);
}

/*
 * Returns 1 if special sandbox instructions trap as expected.
 */
int NaClQualifySandboxInstrs(void) {
  struct sigaction old_sigaction_trap;
  struct sigaction old_sigaction_ill;
#if NACL_ANDROID
  struct sigaction old_sigaction_bus;
#endif
  struct sigaction try_sigaction;
  volatile int fell_through = 0;

  try_sigaction.sa_handler = signal_catch;
  sigemptyset(&try_sigaction.sa_mask);
  try_sigaction.sa_flags = 0;

  if (0 != sigaction(SIGTRAP, &try_sigaction, &old_sigaction_trap)) {
    NaClLog(LOG_FATAL, "Failed to install handler for SIGTRAP.\n");
    return 0;
  }
  if (0 != sigaction(SIGILL, &try_sigaction, &old_sigaction_ill)) {
    NaClLog(LOG_FATAL, "Failed to install handler for SIGILL.\n");
    return 0;
  }
#if NACL_ANDROID
  /*
   * Android yields a SIGBUS for the breakpoint instruction used to mark
   * literal pools heads.
   */
  if (0 != sigaction(SIGBUS, &try_sigaction, &old_sigaction_bus)) {
    NaClLog(LOG_FATAL, "Failed to install handler for SIGBUS.\n");
    return 0;
  }
#endif

  /* Each of the following should trap, successively executing
     each else clause, never the fallthrough. */
  if (0 == sigsetjmp(try_state, 1)) {
    asm(".word " NACL_TO_STRING(NACL_INSTR_ARM_LITERAL_POOL_HEAD) "\n");
    fell_through = 1;
  } else if (0 == sigsetjmp(try_state, 1)) {
    asm(".word " NACL_TO_STRING(NACL_INSTR_ARM_BREAKPOINT) "\n");
    fell_through = 1;
  } else if (0 == sigsetjmp(try_state, 1)) {
    asm(".word " NACL_TO_STRING(NACL_INSTR_ARM_HALT_FILL) "\n");
    fell_through = 1;
  } else if (0 == sigsetjmp(try_state, 1)) {
    asm(".word " NACL_TO_STRING(NACL_INSTR_ARM_ABORT_NOW) "\n");
    fell_through = 1;
  } else if (0 == sigsetjmp(try_state, 1)) {
    asm(".word " NACL_TO_STRING(NACL_INSTR_ARM_FAIL_VALIDATION) "\n");
    fell_through = 1;
  }

#if NACL_ANDROID
  if (0 != sigaction(SIGBUS, &old_sigaction_bus, NULL)) {
    NaClLog(LOG_FATAL, "Failed to restore handler for SIGBUS.\n");
    return 0;
  }
#endif
  if (0 != sigaction(SIGILL, &old_sigaction_ill, NULL)) {
    NaClLog(LOG_FATAL, "Failed to restore handler for SIGILL.\n");
    return 0;
  }
  if (0 != sigaction(SIGTRAP, &old_sigaction_trap, NULL)) {
    NaClLog(LOG_FATAL, "Failed to restore handler for SIGTRAP.\n");
    return 0;
  }

  return !fell_through;
}
