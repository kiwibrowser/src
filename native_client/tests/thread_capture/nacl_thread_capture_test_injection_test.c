/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/ucontext.h>

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"
#include "native_client/src/trusted/service_runtime/sel_main.h"
#include "native_client/src/trusted/fault_injection/test_injection.h"
#include "native_client/tests/thread_capture/thread_capture_test_injection.h"

#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_arm
static const int kExpectedSignal = SIGILL;
#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_mips
static const int kExpectedSignal = SIGTRAP;
#else
static const int kExpectedSignal = SIGSEGV;
#endif

uintptr_t g_nacl_syscall_thread_capture_fault_addr;

void NaClSignalHandler(int signum, siginfo_t *info, void *other) {
  uintptr_t faulting_pc;
  ucontext_t *ucp;

  UNREFERENCED_PARAMETER(info);
  ucp = (ucontext_t *) other;
#if NACL_LINUX
# if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86
#  if NACL_BUILD_SUBARCH == 32
  faulting_pc = ucp->uc_mcontext.gregs[REG_EIP];
#  elif NACL_BUILD_SUBARCH == 64
  faulting_pc = ucp->uc_mcontext.gregs[REG_RIP];
#  else
#   error "128-bit version of x86?!?"
#  endif
# elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_arm
  faulting_pc = ucp->uc_mcontext.arm_pc;
# elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_mips
  faulting_pc = ucp->uc_mcontext.pc;
# else
#  error "how do i get the PC on this cpu architecture?"
# endif
#elif NACL_OSX
# if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86
#  if NACL_BUILD_SUBARCH == 32
  /* typeof(ucp->uc_mcontext) is struct __darwin_mcontext32 *. */
  faulting_pc = ucp->uc_mcontext->__ss.__eip;
#  else
  faulting_pc = ucp->uc_mcontext->__ss.__rip;
#  endif
# else
#  error "fix this for non-x86 OSX"
# endif
#else
# error "signals on Windows?!?"
#endif

  /*
   * Normally printf is unsafe in signal handlers, but we should only
   * get here due to hitting an explicit hlt instruction and we
   * shouldn't be in the middle of an I/O routine.
   */
  printf("signal %d\n", signum);
  printf("faulting_pc 0x%"NACL_PRIxPTR"\n", faulting_pc);
  printf("NaClSyscallThreadCaptureFault 0x%"NACL_PRIxPTR"\n",
         g_nacl_syscall_thread_capture_fault_addr);

  CHECK(signum == kExpectedSignal);
  CHECK(g_nacl_syscall_thread_capture_fault_addr == faulting_pc);
  exit(0);
}

static char g_nacl_altstack[SIGSTKSZ + 4096];

void NaClSetSignalHandler(void) {
  struct sigaction action;
  stack_t stack;

  stack.ss_sp = g_nacl_altstack;
  stack.ss_flags = 0;
  stack.ss_size = NACL_ARRAY_SIZE(g_nacl_altstack);
  sigaltstack(&stack, (stack_t *) NULL);

  memset(&action, 0, sizeof action);
  action.sa_sigaction = NaClSignalHandler;
  sigfillset(&action.sa_mask);
  action.sa_flags = SA_ONSTACK | SA_SIGINFO;

  CHECK(0 == sigaction(kExpectedSignal, &action, (struct sigaction *) NULL));
}

static struct NaClTestInjectionTable const g_test_injection_functions = {
  NaClInjectThreadCaptureSyscall,  /* ChangeTrampolines */
  NaClSetSignalHandler,  /* BeforeMainThreadLaunches */
};

int main(int argc, char **argv) {
  NaClTestInjectionSetInjectionTable(&g_test_injection_functions);
  (void) NaClSelLdrMain(argc, argv);
  return 1;  /* fail! */
}
