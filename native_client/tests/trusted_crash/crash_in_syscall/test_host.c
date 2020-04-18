/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <string.h>

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_exit.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/service_runtime/include/bits/nacl_syscalls.h"
#include "native_client/src/trusted/service_runtime/load_file.h"
#include "native_client/src/trusted/service_runtime/nacl_all_modules.h"
#include "native_client/src/trusted/service_runtime/nacl_app.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/nacl_signal.h"
#include "native_client/src/trusted/service_runtime/nacl_syscall_register.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/win/debug_exception_handler.h"

#if NACL_WINDOWS
# include <windows.h>
# include <dbghelp.h>
#elif NACL_LINUX
# include <signal.h>
#endif

/*
 * This test case checks that an exception handler registered via
 * Windows' SetUnhandledExceptionFilter() API gets called if a crash
 * occurs inside a NaCl syscall handler.  On x86-64, we have to ensure
 * that the stack is set up in a way that Windows likes, otherwise
 * this exception handler does not get called.  For background, see
 * http://code.google.com/p/nativeclient/issues/detail?id=2237.
 */


static const char *g_crash_type;


static int TestWithUntrustedExceptionHandling(void) {
  return strcmp(g_crash_type, "NACL_TEST_CRASH_JUMP_TO_ZERO") == 0 ||
         strcmp(g_crash_type, "NACL_TEST_CRASH_JUMP_INTO_SANDBOX") == 0;
}


#if NACL_WINDOWS

#define MAX_SYMBOL_NAME_LENGTH 100

static void PrintSymbolForAddress(DWORD64 addr) {
  /*
   * Code adapted from Chromium's stack_trace_win.cc, in turn adapted
   * from an MSDN example:
   * http://msdn.microsoft.com/en-us/library/ms680578(VS.85).aspx
   */
  ULONG64 buffer[(sizeof(SYMBOL_INFO) +
                  MAX_SYMBOL_NAME_LENGTH * sizeof(wchar_t) +
                  sizeof(ULONG64) - 1) /
                 sizeof(ULONG64)];
  DWORD64 sym_displacement = 0;
  PSYMBOL_INFO symbol = (PSYMBOL_INFO) buffer;
  BOOL has_symbol;
  memset(buffer, 0, sizeof(buffer));

  SymInitialize(GetCurrentProcess(), NULL, TRUE);

  symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
  symbol->MaxNameLen = MAX_SYMBOL_NAME_LENGTH - 1;
  has_symbol = SymFromAddr(GetCurrentProcess(),
                           addr, &sym_displacement, symbol);
  if (has_symbol) {
    fprintf(stderr, "%s + 0x%x\n", symbol->Name, sym_displacement);
  } else {
    fprintf(stderr, "<no symbol>\n");
  }
}

/*
 * On x86-64 Windows, we expect the backtrace to contain the
 * following, with no gaps:
 *
 *   NaClSyscallCSegHook
 *   NaClSwitchSavingStackPtr
 *   NaClStartThreadInApp
 *
 * We could check for those names, but symbols are not always
 * available.  Instead we check for bogus stack frames below, which
 * the stack unwinder lets through.
 */
static void Backtrace(CONTEXT *initial_context) {
  int machine_type;
  CONTEXT context_for_frame = *initial_context;
  STACKFRAME64 frame = { 0 };
  int frame_number = 0;
  int failed = 0;

  fprintf(stderr, "Stack backtrace:\n");
#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 64
  machine_type = IMAGE_FILE_MACHINE_AMD64;
  frame.AddrPC.Offset = initial_context->Rip;
  frame.AddrFrame.Offset = initial_context->Rbp;
  frame.AddrStack.Offset = initial_context->Rsp;
#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 32
  machine_type = IMAGE_FILE_MACHINE_I386;
  frame.AddrPC.Offset = initial_context->Eip;
  frame.AddrFrame.Offset = initial_context->Ebp;
  frame.AddrStack.Offset = initial_context->Esp;
#else
# error Unknown architecture
#endif
  frame.AddrPC.Mode = AddrModeFlat;
  frame.AddrFrame.Mode = AddrModeFlat;
  frame.AddrStack.Mode = AddrModeFlat;
  frame.Virtual = 0;

  while (1) {
    STACKFRAME64 previous_frame = frame;
    if (!StackWalk64(machine_type, GetCurrentProcess(), GetCurrentThread(),
                     &frame, &context_for_frame,
                     NULL, /* use ReadMemory() */
                     SymFunctionTableAccess64, SymGetModuleBase64, NULL)) {
      break;
    }
    fprintf(stderr, "#%i: ip=%p stack=%llx frame=%llx ",
            frame_number,
            frame.AddrPC.Offset,
            frame.AddrStack.Offset,
            frame.AddrFrame.Offset);
    PrintSymbolForAddress(frame.AddrPC.Offset);
    /*
     * Perform some sanity checks.  Windows' x86-64 stack unwinder
     * applies a fallback rule when it sees return addresses without
     * unwind info, but the fallback rule is for leaf functions.  This
     * causes the stack unwinder to report stack frames that cannot
     * possibly be valid in the Windows x86-64 ABI.  We check for such
     * frames here.  An error here would suggest that the unwind info
     * for NaClSwitchSavingStackPtr is wrong.
     *
     * The frame for a non-leaf function Foo() looks like this:
     *     32 bytes   shadow space (scratch space for Foo())
     *   ---- Foo()'s caller's AddrStack points here
     *      8 bytes   return address (points into Foo()'s caller)
     *      8 bytes   scratch space for Foo()
     *   ---- Foo()'s AddrFrame points here (unless the hardware exception
     *        occurred inside Foo(), in which case this is less well-defined)
     *   16*n bytes   scratch space for Foo() (for some n >= 0)
     *     32 bytes   shadow space (scratch space for Foo()'s callees)
     *   ---- Foo()'s rsp and AddrStack point here
     *
     * The frame for a leaf function Bar() that never adjusts rsp
     * looks like this:
     *     32 bytes   shadow space (scratch space for Bar())
     *      8 bytes   return address (points into Bar()'s caller)
     *   ---- Bar()'s rsp and AddrStack points here
     *      8 bytes   not usable by Bar() at all!
     *   ---- Bar()'s AddrFrame points here
     */
    if (NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 64) {
      /* frame_size must be signed for the check to be useful. */
      long long frame_size = frame.AddrFrame.Offset - frame.AddrStack.Offset;
      if (frame_number > 0 && frame_size < 32) {
        fprintf(stderr, "Error: frame_size=%i, which is too small\n",
                frame_size);
        failed = 1;
      }
      if (frame_number > 1 &&
          frame.AddrStack.Offset != previous_frame.AddrFrame.Offset + 16) {
        fprintf(stderr, "Error: stack does not fit with previous frame\n");
        failed = 1;
      }
    }
    frame_number++;
  }
  if (failed) {
    _exit(1);
  }
}

static LONG WINAPI ExceptionHandler(EXCEPTION_POINTERS *exc_info) {
  printf("Inside exception handler, as expected\n");
  Backtrace(exc_info->ContextRecord);
  printf("Stack backtrace passed sanity checks\n");

  if (strcmp(g_crash_type, "NACL_TEST_CRASH_MEMORY") == 0 ||
      strcmp(g_crash_type, "NACL_TEST_CRASH_JUMP_TO_ZERO") == 0 ||
      strcmp(g_crash_type, "NACL_TEST_CRASH_JUMP_INTO_SANDBOX") == 0) {
    /*
     * STATUS_ACCESS_VIOLATION is 0xc0000005 but we deliberately
     * convert this to a signed number since Python's wrapper for
     * GetExitCodeProcess() treats the STATUS_* values as negative,
     * although the unsigned values are used in headers and are more
     * widely recognised
     */
    fprintf(stderr, "** intended_exit_status=%i\n", STATUS_ACCESS_VIOLATION);
  } else if (strcmp(g_crash_type, "NACL_TEST_CRASH_LOG_FATAL") == 0 ||
             strcmp(g_crash_type, "NACL_TEST_CRASH_CHECK_FAILURE") == 0) {
    fprintf(stderr, "** intended_exit_status=trusted_sigabrt\n");
  } else {
    NaClLog(LOG_FATAL, "Unknown crash type: \"%s\"\n", g_crash_type);
  }
  /*
   * Continuing is what Breakpad does, but this should cause the
   * process to exit with an exit status that is appropriate for the
   * type of exception.  We want to test that ExceptionHandler() does
   * not get called twice, since that does not work with Chrome's
   * embedding of Breakpad.
   */
  return EXCEPTION_CONTINUE_SEARCH;
}

static void RegisterHandlers(void) {
  /*
   * The UnhandledExceptionFilter does not get run if a debugger
   * process is attached, so use vectored exception handling instead
   * in that case.
   */
  if (TestWithUntrustedExceptionHandling()) {
    CHECK(AddVectoredExceptionHandler(1, ExceptionHandler) != NULL);
  } else {
    SetUnhandledExceptionFilter(ExceptionHandler);
  }
}

#else

static const char exit_message[] = "** intended_exit_status=0\n";

static void SignalHandler(int sig) {
  if (strcmp(g_crash_type, "NACL_TEST_CRASH_MEMORY") == 0 ||
      strcmp(g_crash_type, "NACL_TEST_CRASH_JUMP_TO_ZERO") == 0 ||
      strcmp(g_crash_type, "NACL_TEST_CRASH_JUMP_INTO_SANDBOX") == 0) {
    CHECK(sig == SIGSEGV);
  } else if (strcmp(g_crash_type, "NACL_TEST_CRASH_LOG_FATAL") == 0 ||
             strcmp(g_crash_type, "NACL_TEST_CRASH_CHECK_FAILURE") == 0) {
    CHECK(sig == SIGABRT);
  } else {
    NaClLog(LOG_FATAL, "Unknown crash type: \"%s\"\n", g_crash_type);
  }
  /* Avoid printf() because it uses a lot of stack space. */
  if (write(2, exit_message, sizeof(exit_message) - 1) != 0) {
    /* This conditional suppresses a compiler warning. */
  }
  _exit(0);
}

static void RegisterHandlers(void) {
  int signals[] = { SIGSEGV, SIGABRT };
  size_t index;
  for (index = 0; index < NACL_ARRAY_SIZE(signals); index++) {
    CHECK(signal(signals[index], SignalHandler) == 0);
  }
  NaClSignalHandlerInit();
}

#endif

int32_t JumpToZeroCrashSyscall(struct NaClAppThread *natp) {
  void (*null_func_ptr)(void) = NULL;

  UNREFERENCED_PARAMETER(natp);

  null_func_ptr();

  NaClLog(LOG_FATAL, "JumpToZeroCrashSyscall: Should not reach here\n");
  return 1;
}

int32_t JumpIntoSandboxCrashSyscall(struct NaClAppThread *natp) {
  void (*bad_func_ptr)(void) = (void (*)(void)) natp->nap->mem_start;
  bad_func_ptr();

  NaClLog(LOG_FATAL, "JumpIntoSandboxCrashSyscall: Should not reach here\n");
  return 1;
}

NACL_DEFINE_SYSCALL_0(JumpToZeroCrashSyscall)
NACL_DEFINE_SYSCALL_0(JumpIntoSandboxCrashSyscall)

int main(int argc, char **argv) {
  struct NaClApp app;

  NaClHandleBootstrapArgs(&argc, &argv);
  NaClDebugExceptionHandlerStandaloneHandleArgs(argc, argv);

  /* Turn off buffering to aid debugging. */
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);

  NaClAllModulesInit();

  if (argc != 3) {
    NaClLog(LOG_FATAL,
            "Expected 2 arguments: <executable-filename> <crash-type>\n");
  }

  g_crash_type = argv[2];

  CHECK(NaClAppCtor(&app));
  CHECK(NaClAppLoadFileFromFilename(&app, argv[1]) == LOAD_OK);
  NaClAppInitialDescriptorHookup(&app);

  if (TestWithUntrustedExceptionHandling()) {
    app.enable_exception_handling = 1;
#if NACL_WINDOWS
    app.attach_debug_exception_handler_func =
        NaClDebugExceptionHandlerStandaloneAttach;
#endif
  }

  NACL_REGISTER_SYSCALL(&app, JumpToZeroCrashSyscall, NACL_sys_test_syscall_1);
  NACL_REGISTER_SYSCALL(&app, JumpIntoSandboxCrashSyscall,
                        NACL_sys_test_syscall_2);

  RegisterHandlers();

  CHECK(NaClCreateMainThread(&app, argc - 1, argv + 1, NULL));
  NaClWaitForMainThreadToExit(&app);

  NaClLog(LOG_ERROR, "We did not expect the test program to exit cleanly\n");
  return 1;
}
