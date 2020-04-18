/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/include/build_config.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/thread_suspension.h"
#include "native_client/src/trusted/desc/nrd_all_modules.h"

#include "gtest/gtest.h"


// These test cases test suspension of untrusted threads.  They use
// mock untrusted threads, so they do not test interactions between
// thread suspension and context switches between trusted and
// untrusted code.  See tests/thread_suspension for similar tests that
// operate on real untrusted threads.


// These tests work with the Windows and Mac implementations of thread
// suspension, but the Linux implementation (based on asynchronous
// signals) requires more of NaClAppThread to be present than we mock
// out here.  See tests/thread_suspension for a more integrationy test
// that works on Linux.
#if !NACL_LINUX

class ThreadSuspensionTest : public testing::Test {
 protected:
  virtual void SetUp();
  virtual void TearDown();
};

void ThreadSuspensionTest::SetUp() {
  NaClNrdAllModulesInit();
}

void ThreadSuspensionTest::TearDown() {
  NaClNrdAllModulesFini();
}


struct ThreadArgs {
  struct NaClAppThread *natp;
  volatile uint32_t var;
  volatile bool should_exit;
};

// Perform some minimal initialisation of the NaClAppThread based on
// what we observe we need for the test.  Reusing NaClAppThreadMake()
// here is difficult because it launches an untrusted thread.
static void AppThreadInitMinimal(struct NaClAppThread *natp) {
  memset(natp, 0xff, sizeof(*natp));
  natp->suspend_state = NACL_APP_THREAD_UNTRUSTED;
  natp->suspended_registers = NULL;
  ASSERT_EQ(NaClMutexCtor(&natp->suspend_mu), 1);
}


// This test checks that after NaClUntrustedThreadsSuspendAll() has
// returned, untrusted threads are completely suspended.  We test this
// by running a thread that writes to a memory location.  We check
// that the memory location does not change after
// NaClUntrustedThreadsSuspendAll() but does change after
// NaClUntrustedThreadsResumeAll().
//
// This is a regression test for
// http://code.google.com/p/nativeclient/issues/detail?id=2557
// The gotcha in the Windows API is that SuspendThread() can return
// before the thread has really been suspended.
//
// This is technically a stress test, but it was able to reproduce the
// problem very reliably, at least on a multicore machine.

static void WINAPI MutatorThread(void *arg) {
  struct ThreadArgs *args = (struct ThreadArgs *) arg;
  uint32_t next_val = 0;
  while (!args->should_exit) {
    args->var = next_val++;
  }
}

static void TrySuspendingMutatorThread(struct NaClApp *nap,
                                       volatile uint32_t *addr) {
  // Wait for guest program to start writing to the address.
  while (*addr == 0) { /* do nothing */ }

  for (int iteration = 0; iteration < 100; iteration++) {
    NaClUntrustedThreadsSuspendAll(nap, /* save_registers= */ 0);
    uint32_t snapshot = *addr;
    for (int count = 0; count < 100000; count++) {
      uint32_t snapshot2 = *addr;
      if (snapshot2 != snapshot) {
        NaClLog(LOG_FATAL,
                "Read %i but expected %i on try %i of iteration %i\n",
                (int) snapshot2, (int) snapshot, count, iteration);
      }
    }
    NaClUntrustedThreadsResumeAll(nap);
    // Wait for guest program to resume writing.
    while (*addr == snapshot) { /* do nothing */ }
  }
}

TEST_F(ThreadSuspensionTest, TestThreadSuspendsSynchronously) {
  struct NaClApp app;
  ASSERT_EQ(NaClAppCtor(&app), 1);

  struct NaClAppThread app_thread;
  AppThreadInitMinimal(&app_thread);

  struct ThreadArgs thread_args;
  thread_args.natp = NULL;
  thread_args.var = 0;
  thread_args.should_exit = false;
  app_thread.host_thread_is_defined = 1;
  ASSERT_EQ(NaClThreadCreateJoinable(&app_thread.host_thread, MutatorThread,
                                     &thread_args, NACL_KERN_STACK_SIZE), 1);
  ASSERT_EQ(NaClAddThread(&app, &app_thread), 0);
  TrySuspendingMutatorThread(&app, &thread_args.var);
  thread_args.should_exit = true;
  NaClThreadJoin(&app_thread.host_thread);
}


// The test below checks that we do not get a deadlock when using
// NaClUntrustedThreadsSuspendAll() on threads that cross between
// untrusted and trusted code by invoking NaCl syscalls.
//
// This is a stress test.  It is not guaranteed to find a problem, but
// it did reproduce the problem in the original buggy version of the
// code when Sleep() calls were inserted in suitable places.
//
// See http://code.google.com/p/nativeclient/issues/detail?id=2569

static void WINAPI SyscallInvokerThread(void *arg) {
  struct ThreadArgs *args = (struct ThreadArgs *) arg;
  uint32_t next_val = 0;
  while (!args->should_exit) {
    NaClAppThreadSetSuspendState(args->natp, NACL_APP_THREAD_UNTRUSTED,
                                 NACL_APP_THREAD_TRUSTED);
    args->var = next_val++;
    NaClAppThreadSetSuspendState(args->natp, NACL_APP_THREAD_TRUSTED,
                                 NACL_APP_THREAD_UNTRUSTED);
  }
}

static void TrySuspendingSyscallInvokerThread(struct NaClApp *nap,
                                              volatile uint32_t *addr) {
  // Wait for guest program to start writing to the address.
  while (*addr == 0) { /* do nothing */ }

  for (int iteration = 0; iteration < 100; iteration++) {
    NaClUntrustedThreadsSuspendAll(nap, /* save_registers= */ 0);
    NaClUntrustedThreadsResumeAll(nap);

    // Wait for guest program to make some progress.
    uint32_t snapshot = *addr;
    while (*addr == snapshot) { /* do nothing */ }
  }
}

TEST_F(ThreadSuspensionTest, TestNoDeadlockInSyscallCrossing) {
  struct NaClApp app;
  ASSERT_EQ(NaClAppCtor(&app), 1);

  struct NaClAppThread app_thread;
  AppThreadInitMinimal(&app_thread);

  struct ThreadArgs thread_args;
  thread_args.natp = &app_thread;
  thread_args.var = 0;
  thread_args.should_exit = false;
  app_thread.host_thread_is_defined = 1;
  ASSERT_EQ(NaClThreadCreateJoinable(&app_thread.host_thread,
                                     SyscallInvokerThread,
                                     &thread_args, NACL_KERN_STACK_SIZE), 1);
  ASSERT_EQ(NaClAddThread(&app, &app_thread), 0);
  TrySuspendingSyscallInvokerThread(&app, &thread_args.var);
  thread_args.should_exit = true;
  NaClThreadJoin(&app_thread.host_thread);
}

#endif
