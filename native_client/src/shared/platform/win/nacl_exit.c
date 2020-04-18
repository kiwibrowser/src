/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <intrin.h>
#include <stdlib.h>
#include <stdio.h>

#include "native_client/src/include/portability.h"
#include "native_client/src/shared/platform/nacl_exit.h"
#include "native_client/src/trusted/service_runtime/nacl_signal.h"


/*
 * The MSVC compiler wrongly gives an "unreachable code" warning for
 * the contents of NaClAbort.  Best guess is that it inlines NaClAbort
 * into NaClExit, sees that the call is after the call to ExitProcess
 * and ExitProcess is marked as never returning, and decides the body
 * of NaClAbort is unreachable.  As this is not the only call to
 * NaClAbort, the compiler is just wrong and the only thing to do is
 * just to suppress the warning.
 */
#pragma warning(disable:4702)

void NaClAbort(void) {
  /*
   * We crash the process with a HLT instruction so that the Breakpad
   * crash reporter will be invoked when we are running inside Chrome.
   *
   * This has the disadvantage that an untrusted-code crash will not
   * be distinguishable from a trusted-code NaClAbort() based on the
   * process's exit status alone
   *
   * While we could use the INT3 breakpoint instruction to exit (via
   * __debugbreak()), that does not work if NaCl's debug exception
   * handler is attached, because that always resumes breakpoints (see
   * http://code.google.com/p/nativeclient/issues/detail?id=2772).
   */
  while (1) {
    __halt();
  }
}

void NaClExit(int err_code) {
#ifdef COVERAGE
  /* Give coverage runs a chance to flush coverage data */
  exit(err_code);
#else
  /*
   * We want to exit without running any finalization code, because
   * that could cause currently-running threads to crash.
   *
   * We avoid using exit() because it calls atexit() handlers.  We
   * avoid using _exit() because it is documented as doing some
   * internal C library shutdown.
   *
   * We avoid using TerminateProcess() because it terminates threads
   * in a non-deterministic order.  On Windows, the exit status of a
   * process is taken to be the exit status of the last thread that
   * exited.  Using TerminateProcess() makes the exit status of the
   * process unreliable; this used to cause many tests to be flaky.
   * See https://code.google.com/p/nativeclient/issues/detail?id=2870
   *
   * ExitProcess() has the following properties:
   *
   *  - It first terminates all threads except the calling thread.
   *    This prevents these threads from messing up the process exit
   *    status.
   *
   *  - It calls loaded DLLs' finalization routines.  This is not
   *    ideal, but it is OK because NaClExit() should only be used for
   *    graceful exits (when no internal errors have been detected),
   *    and because there will be no other threads at this point.
   */
  ExitProcess(err_code);
#endif
}
