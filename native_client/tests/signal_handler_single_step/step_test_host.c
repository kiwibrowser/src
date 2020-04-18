/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdarg.h>
#include <signal.h>

#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_exit.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/service_runtime/include/bits/nacl_syscalls.h"
#include "native_client/src/trusted/service_runtime/load_file.h"
#include "native_client/src/trusted/service_runtime/nacl_all_modules.h"
#include "native_client/src/trusted/service_runtime/nacl_app.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/nacl_copy.h"
#include "native_client/src/trusted/service_runtime/nacl_signal.h"
#include "native_client/src/trusted/service_runtime/nacl_syscall_register.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/tests/signal_handler_single_step/step_test_common.h"
#include "native_client/tests/signal_handler_single_step/step_test_syscalls.h"


/*
 * This test case checks that NaCl's Linux signal handler can be
 * entered at any point during the context switches between trusted
 * and untrusted code.
 *
 * In particular, this tests that the signal handler correctly
 * restores %gs on x86-32.  This is tricky because NaCl's context
 * switch code must set %cs and %gs separately, so there is a small
 * window during which %gs is set to the untrusted-code value but %cs
 * is not.  The signal handler needs to work at this point if we are
 * to handle asynchronous signals correctly (such as for implementing
 * thread suspension).
 */

/* This should be at least 2 so that we test the syscall return path. */
static const int kNumberOfCallsToTest = 5;

static int g_call_count = 0;
static int g_in_untrusted_code = 0;
static int g_context_switch_count = 0;

static int g_instruction_count = 0;
static int g_instruction_byte_count = 0;
static int g_jump_count = 0;
static nacl_reg_t g_last_prog_ctr = 0;

static const char* g_description = NULL;


static void SignalSafePrintf(const char *format, ...) {
  va_list args;
  char buf[200];
  int len;
  va_start(args, format);
  len = vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);
  SignalSafeWrite(buf, len);
}

/*
 * We use a custom NaCl syscall here partly because we need to avoid
 * making any Linux syscalls while the trap flag is set.  On x86-32
 * Linux, doing a syscall with the trap flag set will sometimes kill
 * the process with SIGTRAP rather than entering the signal handler.
 * This might be a kernel bug.  x86-64 processes do not have the same
 * problem.
 */
static int32_t TestSyscall(struct NaClAppThread *natp) {
  /* Check that the trap flag has not been unset by anything unexpected. */
  CHECK(GetTrapFlag());

  if (++g_call_count == kNumberOfCallsToTest) {
    UnsetTrapFlag();
    NaClReportExitStatus(natp->nap, 0);
    NaClAppThreadTeardown(natp);
  }
  return 0;
}

NACL_DEFINE_SYSCALL_0(TestSyscall)

static void TrapSignalHandler(int signal,
                              const struct NaClSignalContext *context,
                              int is_untrusted) {
  if (signal == SIGTRAP) {
    g_instruction_count++;
    /*
     * This is a heuristic for detecting jumps, based on the maximum
     * length of an x86 instruction being 15 bytes.  We would miss
     * short forward jumps.
     */
    if (context->prog_ctr - g_last_prog_ctr > 15) {
      g_jump_count++;
    } else {
      /* Measure total size of instructions, except for taken branches. */
      g_instruction_byte_count += context->prog_ctr - g_last_prog_ctr;
    }
    g_last_prog_ctr = context->prog_ctr;

    if (g_in_untrusted_code != is_untrusted) {
      g_context_switch_count++;
      g_in_untrusted_code = is_untrusted;

      SignalSafePrintf("Switching to %s: since previous switch: "
                       "%i instructions, %i instruction bytes, %i jumps\n",
                       is_untrusted ? "untrusted" : "trusted",
                       g_instruction_count,
                       g_instruction_byte_count,
                       g_jump_count);
      if (is_untrusted && g_call_count == kNumberOfCallsToTest - 1) {
        SignalSafePrintf("RESULT InstructionsPerSyscall: %s= "
                         "%i count\n", g_description, g_instruction_count);
        SignalSafePrintf("RESULT InstructionBytesPerSyscall: %s= "
                         "%i count\n",
                         g_description, g_instruction_byte_count);
        SignalSafePrintf("RESULT JumpsPerSyscall: %s= "
                         "%i count\n", g_description, g_jump_count);
      }
      g_instruction_count = 0;
      g_instruction_byte_count = 0;
      g_jump_count = 0;
    }
  } else {
    SignalSafeLogStringLiteral("Error: Received unexpected signal\n");
    _exit(1);
  }
}

static void ThreadCreateHook(struct NaClAppThread *natp) {
  UNREFERENCED_PARAMETER(natp);

  SetTrapFlag();
}

static void ThreadExitHook(struct NaClAppThread *natp) {
  UNREFERENCED_PARAMETER(natp);
}

static void ProcessExitHook(void) {
}

static const struct NaClDebugCallbacks debug_callbacks = {
  ThreadCreateHook,
  ThreadExitHook,
  ProcessExitHook
};

int main(int argc, char **argv) {
  struct NaClApp app;

  NaClAllModulesInit();

  if (argc != 3) {
    NaClLog(LOG_FATAL, "Expected 2 arguments: <executable filename> "
                       "<description-string>\n");
  }
  g_description = argv[2];

  CHECK(NaClAppWithEmptySyscallTableCtor(&app));
  NACL_REGISTER_SYSCALL(&app, TestSyscall, SINGLE_STEP_TEST_SYSCALL);

  app.debug_stub_callbacks = &debug_callbacks;
  NaClSignalHandlerInit();
  NaClSignalHandlerSet(TrapSignalHandler);

  CHECK(NaClAppLoadFileFromFilename(&app, argv[1]) == LOAD_OK);
  CHECK(NaClCreateMainThread(&app, 0, NULL, NULL));
  CHECK(NaClWaitForMainThreadToExit(&app) == 0);

  CHECK(!g_in_untrusted_code);
  CHECK(g_context_switch_count == kNumberOfCallsToTest * 2);

  /*
   * Avoid calling exit() because it runs process-global destructors
   * which might break code that is running in our unjoined threads.
   */
  NaClExit(0);
  return 0;
}
