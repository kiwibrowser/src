/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * POSIX-specific routines for verifying that Data Execution Prevention is
 * functional.
 */

#include <setjmp.h>
#include <stdlib.h>
#include <signal.h>

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_compiler_annotations.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/trusted/platform_qualify/nacl_dep_qualify.h"

#if NACL_OSX
#include <mach/mach.h>
#endif

#if (NACL_OSX && NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && \
     NACL_BUILD_SUBARCH == 64)
# define EXPECTED_SIGNAL SIGBUS
#else
# define EXPECTED_SIGNAL SIGSEGV
#endif

static struct sigaction previous_sigaction;
static struct sigaction try_sigaction;
static sigjmp_buf try_state;

static void signal_catch(int sig) {
  siglongjmp(try_state, sig);
}

static void setup_signals(void) {
  try_sigaction.sa_handler = signal_catch;
  sigemptyset(&try_sigaction.sa_mask);
  try_sigaction.sa_flags = SA_RESETHAND;

  (void) sigaction(EXPECTED_SIGNAL, &try_sigaction, &previous_sigaction);
}

static void restore_signals(void) {
  (void) sigaction(EXPECTED_SIGNAL, &previous_sigaction, 0);
}

#if NACL_OSX

/*
 * If present, Breakpad's handler will swallow the hardware exception that
 * the test triggers before its signal handler can respond to it. Temporarily
 * disable Breakpad's handler to allow the system's in-kernel handler to
 * transform the exception into the signal that's expected here.
 */

/*
 * <mach/mach_types.defs> says that these arrays have room for 32 elements,
 * not EXC_TYPES_COUNT. task_swap_exception_ports expects there to be room for
 * 32.
 */
#define MAX_EXCEPTION_PORTS 32

typedef struct {
  exception_mask_t masks[MAX_EXCEPTION_PORTS];
  exception_handler_t ports[MAX_EXCEPTION_PORTS];
  exception_behavior_t behaviors[MAX_EXCEPTION_PORTS];
  thread_state_flavor_t flavors[MAX_EXCEPTION_PORTS];
  mach_msg_type_number_t count;
} MachExceptionHandlerData;

static void DisableMachExceptionHandler(
    MachExceptionHandlerData *saved_handlers) {
  kern_return_t kr = task_swap_exception_ports(
      mach_task_self(),
      EXC_MASK_BAD_ACCESS,
      MACH_PORT_NULL,
      EXCEPTION_DEFAULT,
      THREAD_STATE_NONE,
      (exception_mask_array_t) &saved_handlers->masks,
      &saved_handlers->count,
      (exception_handler_array_t) &saved_handlers->ports,
      (exception_behavior_array_t) &saved_handlers->behaviors,
      (exception_flavor_array_t) &saved_handlers->flavors);
  CHECK(kr == KERN_SUCCESS);
}

static void EnableMachExceptionHandler(
    MachExceptionHandlerData *saved_handlers) {
  for (size_t index = 0; index < saved_handlers->count; ++index) {
    if (saved_handlers->ports[index] != MACH_PORT_NULL) {
      kern_return_t kr = task_set_exception_ports(
          mach_task_self(),
          saved_handlers->masks[index],
          saved_handlers->ports[index],
          saved_handlers->behaviors[index],
          saved_handlers->flavors[index]);
      CHECK(kr == KERN_SUCCESS);
    }
  }
}

#else

typedef int MachExceptionHandlerData;

static void DisableMachExceptionHandler(
    MachExceptionHandlerData *saved_handlers) {
  UNREFERENCED_PARAMETER(saved_handlers);
}

static void EnableMachExceptionHandler(
    MachExceptionHandlerData *saved_handlers) {
  UNREFERENCED_PARAMETER(saved_handlers);
}

#endif

int NaClAttemptToExecuteDataAtAddr(uint8_t *thunk_buffer, size_t size) {
  int result;
  MachExceptionHandlerData saved_handlers;
  nacl_void_thunk thunk = NaClGenerateThunk(thunk_buffer, size);

  setup_signals();
  DisableMachExceptionHandler(&saved_handlers);

  if (0 == sigsetjmp(try_state, 1)) {
    thunk();
    result = 0;
  } else {
    result = 1;
  }

  EnableMachExceptionHandler(&saved_handlers);
  restore_signals();

  return result;
}

/*
 * Returns 1 if Data Execution Prevention is present and working.
 */
int NaClAttemptToExecuteData(void) {
  int result;
  uint8_t *thunk_buffer = malloc(64);
  if (NULL == thunk_buffer) {
    return 0;
  }
  result = NaClAttemptToExecuteDataAtAddr(thunk_buffer, 64);
  free(thunk_buffer);
  return result;
}
