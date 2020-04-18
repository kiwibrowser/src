/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <windows.h>

#include "native_client/src/trusted/debug_stub/thread.h"
#include "native_client/src/trusted/service_runtime/nacl_signal.h"

/*
 * Define the OS specific portions of Thread.
 */

namespace port {

enum PosixSignals {
  SIGINT    = 2,
  SIGQUIT   = 3,
  SIGILL    = 4,
  SIGTRACE  = 5,
  SIGBUS    = 7,
  SIGFPE    = 8,
  SIGKILL   = 9,
  SIGSEGV   = 11,
  SIGSTKFLT = 16,
};

int Thread::ExceptionToSignal(int ex) {
  switch (static_cast<DWORD>(ex)) {
    case EXCEPTION_GUARD_PAGE:
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
    case EXCEPTION_DATATYPE_MISALIGNMENT:
    case EXCEPTION_ACCESS_VIOLATION:
    case EXCEPTION_IN_PAGE_ERROR:
    case EXCEPTION_PRIV_INSTRUCTION:
      return SIGSEGV;

    case EXCEPTION_BREAKPOINT:
    case EXCEPTION_SINGLE_STEP:
      return SIGTRACE;

    case EXCEPTION_FLT_DENORMAL_OPERAND:
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
    case EXCEPTION_FLT_INEXACT_RESULT:
    case EXCEPTION_FLT_INVALID_OPERATION:
    case EXCEPTION_FLT_OVERFLOW:
    case EXCEPTION_FLT_STACK_CHECK:
    case EXCEPTION_FLT_UNDERFLOW:
      return SIGFPE;

    case EXCEPTION_INT_DIVIDE_BY_ZERO:
    case EXCEPTION_INT_OVERFLOW:
    case EXCEPTION_ILLEGAL_INSTRUCTION:
      return SIGILL;

    case EXCEPTION_STACK_OVERFLOW:
      return SIGSTKFLT;

    case CONTROL_C_EXIT:
      return SIGQUIT;

    case EXCEPTION_NONCONTINUABLE_EXCEPTION:
    case EXCEPTION_INVALID_DISPOSITION:
    case EXCEPTION_INVALID_HANDLE:
      return SIGILL;
  }
  return SIGILL;
}

}  // namespace port
