/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/tests/thread_suspension/suspend_test.h"

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_assert.h"
#include "native_client/src/include/portability_io.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_exit.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"
#include "native_client/src/trusted/service_runtime/include/bits/mman.h"
#include "native_client/src/trusted/service_runtime/include/bits/nacl_syscalls.h"
#include "native_client/src/trusted/service_runtime/load_file.h"
#include "native_client/src/trusted/service_runtime/nacl_all_modules.h"
#include "native_client/src/trusted/service_runtime/nacl_app.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/nacl_copy.h"
#include "native_client/src/trusted/service_runtime/nacl_signal.h"
#include "native_client/src/trusted/service_runtime/nacl_syscall_register.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/sys_memory.h"
#include "native_client/src/trusted/service_runtime/thread_suspension.h"
#include "native_client/tests/common/register_set.h"


/*
 * Some of these tests mirror thread_suspension_test.cc, but they
 * operate on threads running real untrusted code rather than on mock
 * untrusted threads.
 */

static int g_simple_syscall_should_exit;
static volatile int g_simple_syscall_called;

/* This must be called with the mutex nap->threads_mu held. */
static struct NaClAppThread *GetOnlyThread(struct NaClApp *nap) {
  struct NaClAppThread *found_thread = NULL;
  size_t index;
  for (index = 0; index < nap->threads.num_entries; index++) {
    struct NaClAppThread *natp = NaClGetThreadMu(nap, (int) index);
    if (natp != NULL) {
      CHECK(found_thread == NULL);
      found_thread = natp;
    }
  }
  CHECK(found_thread != NULL);
  return found_thread;
}

/*
 * This spins until any previous NaClAppThread has exited to the point
 * where it is removed from the thread array, so that it will not be
 * encountered by a subsequent call to GetOnlyThread().  This is
 * necessary because the threads hosting NaClAppThreads are unjoined.
 */
static void WaitForThreadToExitFully(struct NaClApp *nap) {
  int done;
  do {
    NaClXMutexLock(&nap->threads_mu);
    done = (nap->num_threads == 0);
    NaClXMutexUnlock(&nap->threads_mu);
  } while (!done);
}

static struct SuspendTestShm *StartGuestWithSharedMemory(
    struct NaClApp *nap, char *test_name) {
  char arg_string[32];
  char *args[] = {"prog_name", test_name, arg_string};
  uint32_t mmap_addr;

  /*
   * Allocate some space in untrusted address space.  We pass the address
   * to the guest program so that it can write to it, so that we can
   * observe its writes.
   */
  mmap_addr = NaClSysMmapIntern(
      nap, NULL, sizeof(struct SuspendTestShm),
      NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE,
      NACL_ABI_MAP_PRIVATE | NACL_ABI_MAP_ANONYMOUS,
      -1, 0);
  SNPRINTF(arg_string, sizeof(arg_string), "0x%x", (unsigned int) mmap_addr);

  WaitForThreadToExitFully(nap);

  CHECK(NaClCreateMainThread(nap, 3, args, NULL));
  return (struct SuspendTestShm *) NaClUserToSys(nap, mmap_addr);
}

/*
 * This test checks that after NaClUntrustedThreadsSuspendAll() has
 * returned, untrusted threads are completely suspended.  We test this
 * by running a thread that writes to a memory location.  We check
 * that the memory location does not change after
 * NaClUntrustedThreadsSuspendAll() but does change after
 * NaClUntrustedThreadsResumeAll().
 *
 * This is based on TrySuspendingMutatorThread() in
 * thread_suspension_test.cc, which is a regression test for
 * http://code.google.com/p/nativeclient/issues/detail?id=2557.
 *
 * This is technically a stress test, but it was able to reproduce the
 * problem very reliably, at least on a multicore machine, as long as
 * MutatorThread() makes no NaCl syscalls.
 */
static void TrySuspendingMutatorThread(struct NaClApp *nap) {
  int iteration;
  struct SuspendTestShm *test_shm;

  test_shm = StartGuestWithSharedMemory(nap, "MutatorThread");

  /* Wait for guest program to start writing to the address. */
  while (test_shm->var == 0) { /* do nothing */ }

  for (iteration = 0; iteration < 100; iteration++) {
    uint32_t snapshot;
    int count;

    NaClUntrustedThreadsSuspendAll(nap, /* save_registers= */ 0);
    snapshot = test_shm->var;
    for (count = 0; count < 100000; count++) {
      uint32_t snapshot2 = test_shm->var;
      if (snapshot2 != snapshot) {
        NaClLog(LOG_FATAL,
                "Read %i but expected %i on try %i of iteration %i\n",
                (int) snapshot2, (int) snapshot, count, iteration);
      }
    }
    NaClUntrustedThreadsResumeAll(nap);
    /* Wait for guest program to resume writing. */
    while (test_shm->var == snapshot) { /* do nothing */ }
  }
  test_shm->should_exit = 1;
  CHECK(NaClWaitForMainThreadToExit(nap) == 0);
}

/*
 * This implements a NaCl syscall.  This syscall will spin until told
 * by the (trusted) test code to return.  This is used for testing
 * suspension of a thread that's inside a syscall.
 */
static int32_t SpinWaitTestSyscall(struct NaClAppThread *natp,
                                   uintptr_t test_shm_uptr) {
  struct SuspendTestShm *test_shm;
  uint32_t next_val = 0;

  test_shm = (struct SuspendTestShm *) NaClUserToSysAddrRange(
      natp->nap, test_shm_uptr, sizeof(*test_shm));

  while (!test_shm->should_exit) {
    /*
     * Assign a new value so that this syscall can be observed to be
     * executing, but avoid small values which have special meanings
     * in this test.
     */
    test_shm->var = 0x100 + (next_val++ & 0xff);
  }
  /* Indicate that we are exiting. */
  test_shm->var = 1;
  return 0;
}

NACL_DEFINE_SYSCALL_1(SpinWaitTestSyscall)

/*
 * This test checks that a thread running a NaCl syscall will not be
 * suspended until the syscall returns to untrusted code.
 */
static void TrySuspendingSyscall(struct NaClApp *nap) {
  int iteration;
  uint32_t snapshot;
  struct SuspendTestShm *test_shm;

  test_shm = StartGuestWithSharedMemory(nap, "SyscallReturnThread");

  /* Wait for the syscall to be entered and stop. */
  while (test_shm->var == 0) { /* do nothing */ }
  NaClUntrustedThreadsSuspendAll(nap, /* save_registers= */ 0);
  /*
   * The syscall should continue to execute, so we should see the
   * value change.
   */
  snapshot = test_shm->var;
  while (test_shm->var == snapshot) { /* do nothing */ }
  /* Tell the syscall to return. */
  test_shm->should_exit = 1;
  /* Wait for the syscall to return. */
  while (test_shm->var != 1) { /* do nothing */ }
  /*
   * Once the syscall has returned, we should see no change to the
   * variable because the thread is suspended.
   */
  for (iteration = 0; iteration < 1000; iteration++) {
    CHECK(test_shm->var == 1);
  }
  /*
   * But once we resume the thread, untrusted code will run and set
   * the value.
   */
  NaClUntrustedThreadsResumeAll(nap);
  while (test_shm->var != 99) { /* do nothing */ }
}

/*
 * The test below checks that we do not get a deadlock when using
 * NaClUntrustedThreadsSuspendAll() on threads that cross between
 * untrusted and trusted code by invoking NaCl syscalls.
 *
 * This is a stress test, based on TrySuspendingSyscallInvokerThread()
 * in thread_suspension_test.cc, which is for testing
 * http://code.google.com/p/nativeclient/issues/detail?id=2569.
 */
static void TrySuspendingSyscallInvokerThread(struct NaClApp *nap,
                                              int reset_registers) {
  int iteration;
  struct SuspendTestShm *test_shm;

  test_shm = StartGuestWithSharedMemory(nap, "SyscallInvokerThread");

  /* Wait for guest program to start writing to the address. */
  while (test_shm->var == 0) { /* do nothing */ }

  for (iteration = 0; iteration < 1000; iteration++) {
    uint32_t snapshot;

    NaClUntrustedThreadsSuspendAll(nap, /* save_registers= */ reset_registers);
    if (reset_registers) {
      /*
       * Additionally, we can stress-test setting the registers to
       * their current state, since the debug stub does this.  This is
       * supposed to be idempotent, but it was failing on Mac (see
       * https://code.google.com/p/nativeclient/issues/detail?id=3243)
       * because of the workaround we use for restoring segment
       * registers on x86-32.
       */
      struct NaClAppThread *natp = GetOnlyThread(nap);
      struct NaClSignalContext regs;
      NaClAppThreadGetSuspendedRegisters(natp, &regs);
      NaClAppThreadSetSuspendedRegisters(natp, &regs);
    }
    NaClUntrustedThreadsResumeAll(nap);

    /* Wait for guest program to make some progress. */
    snapshot = test_shm->var;
    while (test_shm->var == snapshot) { /* do nothing */ }
  }
  test_shm->should_exit = 1;
  CHECK(NaClWaitForMainThreadToExit(nap) == 0);
}

static void TestGettingRegisterSnapshot(struct NaClApp *nap) {
  struct SuspendTestShm *test_shm;
  struct NaClAppThread *natp;
  struct NaClSignalContext regs;
  struct NaClSignalContext regs_copy;

  test_shm = StartGuestWithSharedMemory(nap, "RegisterSetterThread");
  /*
   * Wait for the guest program to reach untrusted code and set
   * registers to known test values.
   */
  while (test_shm->var == 0) { /* do nothing */ }

  /*
   * Check that registers are not saved unless this is requested.
   * Currently this must come before calling
   * NaClUntrustedThreadsSuspendAll() with save_registers=1.
   */
  NaClUntrustedThreadsSuspendAll(nap, /* save_registers= */ 0);
  natp = GetOnlyThread(nap);
  CHECK(natp->suspended_registers == NULL);
  NaClUntrustedThreadsResumeAll(nap);

  NaClUntrustedThreadsSuspendAll(nap, /* save_registers= */ 1);
  /*
   * The previous natp is valid only while holding nap->threads_mu,
   * which NaClUntrustedThreadsSuspendAll() claims for us.  Re-get
   * natp in case the thread exited.
   */
  natp = GetOnlyThread(nap);
  CHECK(natp->suspended_registers != NULL);
  NaClAppThreadGetSuspendedRegisters(natp, &regs);

  RegsAssertEqual(&regs, &test_shm->expected_regs);

  /*
   * Test that we can also modify the registers of a suspended thread.
   *
   * We change the registers so that the thread calls
   * continue_after_suspension(), which checks the registers and
   * indicates success by exiting.
   */
#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 32
  regs.eax = 0x12340001;
  regs.ecx = 0x12340002;
  regs.edx = 0x12340003;
  regs.ebx = 0x12340004;
  /* Leave %esp and %ebp alone for now. */
  regs.esi = 0x12340005;
  regs.edi = 0x12340006;
  regs.prog_ctr = test_shm->continue_after_suspension_func;
#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 64
  regs.rax = 0x1234567800000001;
  regs.rcx = 0x1234567800000002;
  regs.rdx = 0x1234567800000003;
  regs.rbx = 0x1234567800000004;
  /* Leave %rsp and %rbp alone for now. */
  regs.rsi = 0x1234567800000005;
  regs.rdi = 0x1234567800000006;
  regs.r8 = 0x1234567800000007;
  regs.r9 = 0x1234567800000008;
  regs.r10 = 0x1234567800000009;
  regs.r11 = 0x123456780000000a;
  regs.r12 = 0x123456780000000b;
  regs.r13 = 0x123456780000000c;
  regs.r14 = 0x123456780000000d;
  /* The x86-64 sandbox requires %r15 to stay the same. */
  regs.prog_ctr = regs.r15 + test_shm->continue_after_suspension_func;
#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_arm
  regs.r0 = 0x12340001;
  regs.r1 = 0x12340002;
  regs.r2 = 0x12340003;
  regs.r3 = 0x12340004;
  regs.r4 = 0x12340005;
  regs.r5 = 0x12340006;
  regs.r6 = 0x12340007;
  regs.r7 = 0x12340008;
  regs.r8 = 0x12340009;
  /*
   * Skip r9 because it is not supposed to be settable or readable by
   * untrusted code.
   */
  regs.r10 = 0x1234000a;
  regs.r11 = 0x1234000b;
  regs.r12 = 0x1234000c;
  /* Leave sp (r13) and lr (r14) alone for now. */
  regs.prog_ctr = test_shm->continue_after_suspension_func;
#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_mips
  /* Skip setting zero register because it's read-only. */
  regs.at = 0x12340001;
  regs.v0 = 0x12340002;
  regs.v1 = 0x12340003;
  regs.a0 = 0x12340004;
  regs.a1 = 0x12340005;
  regs.a2 = 0x12340006;
  regs.a3 = 0x12340007;
  regs.t0 = 0x12340008;
  regs.t1 = 0x12340009;
  regs.t2 = 0x12340010;
  regs.t3 = 0x12340011;
  regs.t4 = 0x12340012;
  regs.t5 = 0x12340013;
  /* Skip setting t6 and t7 because those are mask registers. */
  regs.s0 = 0x12340014;
  regs.s1 = 0x12340015;
  regs.s2 = 0x12340016;
  regs.s3 = 0x12340017;
  regs.s4 = 0x12340018;
  regs.s5 = 0x12340019;
  regs.s6 = 0x12340020;
  regs.s7 = 0x12340021;
  /* Skip setting t8 because it holds thread pointer. */
  regs.t9 = 0x12340022;
  /* Skip setting k0 and k1 registers, they are used by kernel. */
  regs.global_ptr  = 0x12340023;
  /* Skip setting sp also. */
  regs.frame_ptr   = 0x12340025;
  regs.return_addr = 0x12340026;
  regs.prog_ctr = test_shm->continue_after_suspension_func;
#else
# error Unsupported architecture
#endif

  NaClAppThreadSetSuspendedRegisters(natp, &regs);
  test_shm->expected_regs = regs;

  /*
   * Sanity check: After setting registers, getting registers should
   * read back what we just set.  (We have to be careful to ensure
   * this on x86-32 Mac, where we modify the register values that are
   * really set.)
   */
  NaClAppThreadGetSuspendedRegisters(natp, &regs_copy);
  RegsAssertEqual(&regs_copy, &regs);

  NaClUntrustedThreadsResumeAll(nap);
  CHECK(NaClWaitForMainThreadToExit(nap) == 0);
}

static void TestGettingRegisterSnapshotInSyscall(struct NaClApp *nap) {
  struct SuspendTestShm *test_shm;
  struct NaClAppThread *natp;
  struct NaClSignalContext regs;

  test_shm = StartGuestWithSharedMemory(nap, "SyscallRegisterSetterThread");
  /* Wait for the syscall to be entered and stop. */
  while (test_shm->var == 0) { /* do nothing */ }

  NaClUntrustedThreadsSuspendAll(nap, /* save_registers= */ 1);
  natp = GetOnlyThread(nap);
  NaClAppThreadGetSuspendedRegisters(natp, &regs);
  NaClUntrustedThreadsResumeAll(nap);
  test_shm->should_exit = 1;
  CHECK(NaClWaitForMainThreadToExit(nap) == 0);

  RegsAssertEqual(&regs, &test_shm->expected_regs);
}

/*
 * This implements a NaCl syscall.  This is called in an infinite loop
 * in order to test getting the register state of a thread during
 * trusted/untrusted context switches.
 */
static int32_t SimpleTestSyscall(struct NaClAppThread *natp) {
  g_simple_syscall_called = 1;
  if (g_simple_syscall_should_exit) {
    NaClAppThreadTeardown(natp);
  }
  return 0;
}

NACL_DEFINE_SYSCALL_0(SimpleTestSyscall)

/*
 * Test getting the register state of a thread suspended during a
 * trusted/untrusted context switch.  In this test, untrusted code
 * calls a syscall in an infinite loop; we should get the same
 * register state regardless of when this thread is interrupted.  This
 * is a stress test; see tests/signal_handler_single_step for a
 * non-stress test which covers the unwinding logic.
 */
static void TestGettingRegisterSnapshotInSyscallContextSwitch(
    struct NaClApp *nap) {
  struct SuspendTestShm *test_shm;
  struct NaClAppThread *natp;
  struct NaClSignalContext regs;
  int iteration;

  g_simple_syscall_should_exit = 0;
  g_simple_syscall_called = 0;
  test_shm = StartGuestWithSharedMemory(nap, "SyscallRegisterSetterLoopThread");
  /*
   * Wait until the syscall is called, otherwise
   * test_shm->expected_regs won't have been filled out by untrusted
   * code yet.
   */
  while (!g_simple_syscall_called) { /* do nothing */ }

  for (iteration = 0; iteration < 10000; iteration++) {
    NaClUntrustedThreadsSuspendAll(nap, /* save_registers= */ 1);
    natp = GetOnlyThread(nap);
    NaClAppThreadGetSuspendedRegisters(natp, &regs);

    /*
     * Only compare prog_ctr if the thread is inside the syscall,
     * otherwise there is a small set of instructions that untrusted
     * code executes.
     */
    if (!NaClAppThreadIsSuspendedInSyscall(natp)) {
      regs.prog_ctr = test_shm->expected_regs.prog_ctr;
      RegsUnsetNonCalleeSavedRegisters(&regs);
    }
    RegsAssertEqual(&regs, &test_shm->expected_regs);

    NaClUntrustedThreadsResumeAll(nap);
  }

  g_simple_syscall_should_exit = 1;
  WaitForThreadToExitFully(nap);
}

int main(int argc, char **argv) {
  struct NaClApp app;

  NaClHandleBootstrapArgs(&argc, &argv);

  /* Turn off buffering to aid debugging. */
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);

  NaClAllModulesInit();

#if NACL_LINUX
  NaClSignalHandlerInit();
#endif

  if (argc != 2) {
    NaClLog(LOG_FATAL, "Expected 1 argument: executable filename\n");
  }

  CHECK(NaClAppCtor(&app));
  CHECK(NaClAppLoadFileFromFilename(&app, argv[1]) == LOAD_OK);
  NaClAppInitialDescriptorHookup(&app);

  NACL_REGISTER_SYSCALL(&app, SpinWaitTestSyscall, NACL_sys_test_syscall_1);
  NACL_REGISTER_SYSCALL(&app, SimpleTestSyscall, NACL_sys_test_syscall_2);

  /*
   * We reuse the same sandbox for both tests.
   *
   * TODO(mseaborn): It would be cleaner to create a new sandbox for
   * each test, but if we are to do that in a single process without
   * running out of address space on x86-32, we would need to
   * reinstate the code for deallocating a sandbox's address space.
   */

  printf("Running TrySuspendingMutatorThread...\n");
  TrySuspendingMutatorThread(&app);

  printf("Running TrySuspendingSyscall...\n");
  TrySuspendingSyscall(&app);

  printf("Running TrySuspendingSyscallInvokerThread"
         " (reset_registers=0)...\n");
  TrySuspendingSyscallInvokerThread(&app, /* reset_registers= */ 0);

  printf("Running TrySuspendingSyscallInvokerThread"
         " (reset_registers=1)...\n");
  TrySuspendingSyscallInvokerThread(&app, /* reset_registers= */ 1);

  printf("Running TestGettingRegisterSnapshot...\n");
  TestGettingRegisterSnapshot(&app);

  printf("Running TestGettingRegisterSnapshotInSyscall...\n");
  TestGettingRegisterSnapshotInSyscall(&app);

  printf("Running TestGettingRegisterSnapshotInSyscallContextSwitch...\n");
  TestGettingRegisterSnapshotInSyscallContextSwitch(&app);

  /*
   * Avoid calling exit() because it runs process-global destructors
   * which might break code that is running in our unjoined threads.
   */
  NaClExit(0);
}
