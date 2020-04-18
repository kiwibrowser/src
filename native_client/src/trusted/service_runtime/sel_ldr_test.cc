/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/include/build_config.h"
#include "native_client/src/shared/platform/aligned_malloc.h"
#include "native_client/src/shared/platform/nacl_host_desc.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/nacl_text.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/desc/nacl_desc_io.h"
#include "native_client/src/trusted/desc/nrd_all_modules.h"

#include "gtest/gtest.h"

//
// There are several problems in how these tests are set up.
//
// 1. NaCl modules such as the Log module are supposed to be
// initialized at process startup and finalized at shutdown.  In
// particular, there should not be any threads other than the main
// thread running when the Log module initializes, since the verbosity
// level is set then -- and thereafter it is assumed to be invariant
// and read without acquring locks.  If any threads are left running
// (e.g., NaClApp internal service threads), then race detectors would
// legitimately report an error which is inappropriate because the
// test is ignoring the API contract.
//
// 2. NaClApp objects, while they don't have a Dtor, are expected to
// have a lifetime equal to that of the process that contain them.  In
// particular, when the untrusted thread invokes the exit syscall, it
// expects to be able to use _exit to exit, killing all other
// untrusted threads as a side effect.  Furthermore, once a NaClApp
// object is initialized and NaClAppLaunchServiceThreads invoked,
// system service threads are running holding references to the
// NaClApp object.  If the NaClApp object goes out of scope or is
// otherwise destroyed and its memory freed, then these system thread
// may access memory that is no longer valid.  Tests cannot readily be
// written to cleanly exercise the state space of a NaClApp after
// NaClAppLaunchServiceThreads unless the test process exits---thereby
// killing the service threads as a side-effect---when each individual
// test is complete.
//
// These tests do not invoke NaClAppLaunchServiceThreads, so there
// should be no service threads left running between tests.

class SelLdrTest : public testing::Test {
 protected:
  virtual void SetUp();
  virtual void TearDown();
};

void SelLdrTest::SetUp() {
  NaClNrdAllModulesInit();
}

void SelLdrTest::TearDown() {
  NaClNrdAllModulesFini();
}

// set, get, setavail operations on the descriptor table
TEST_F(SelLdrTest, DescTable) {
  struct NaClApp app;
  struct NaClHostDesc *host_desc;
  struct NaClDesc* io_desc;
  struct NaClDesc* ret_desc;
  int ret_code;

  ret_code = NaClAppCtor(&app);
  ASSERT_EQ(1, ret_code);

  host_desc = (struct NaClHostDesc *) malloc(sizeof *host_desc);
  if (NULL == host_desc) {
    fprintf(stderr, "No memory\n");
  }
  ASSERT_TRUE(NULL != host_desc);

  io_desc = (struct NaClDesc *) NaClDescIoDescMake(host_desc);

  // 1st pos available is 0
  ret_code = NaClAppSetDescAvail(&app, io_desc);
  ASSERT_EQ(0, ret_code);
  // valid desc at pos 0
  ret_desc = NaClAppGetDesc(&app, 0);
  ASSERT_TRUE(NULL != ret_desc);

  // next pos available is 1
  ret_code = NaClAppSetDescAvail(&app, NULL);
  ASSERT_EQ(1, ret_code);
  // no desc at pos 1
  ret_desc = NaClAppGetDesc(&app, 1);
  ASSERT_TRUE(NULL == ret_desc);

  // no desc at pos 1 -> pos 1 is available
  ret_code = NaClAppSetDescAvail(&app, io_desc);
  ASSERT_EQ(1, ret_code);

  // valid desc at pos 1
  ret_desc = NaClAppGetDesc(&app, 1);
  ASSERT_TRUE(NULL != ret_desc);

  // set no desc at pos 3
  NaClAppSetDesc(&app, 3, NULL);

  // valid desc at pos 4
  NaClAppSetDesc(&app, 4, io_desc);
  ret_desc = NaClAppGetDesc(&app, 4);
  ASSERT_TRUE(NULL != ret_desc);

  // never set a desc at pos 10
  ret_desc = NaClAppGetDesc(&app, 10);
  ASSERT_TRUE(NULL == ret_desc);
}

// add and remove operations on the threads table
// Remove thread from an empty table is tested in a death test.
// TODO(tuduce): specify the death test name when checking in.
TEST_F(SelLdrTest, ThreadTableTest) {
  struct NaClApp app;
  struct NaClAppThread nat, *appt = &nat;
  int ret_code;

  ret_code = NaClAppCtor(&app);
  ASSERT_EQ(1, ret_code);

  // 1st pos available is 0
  ASSERT_EQ(0, app.num_threads);
  ret_code = NaClAddThread(&app, appt);
  ASSERT_EQ(0, ret_code);
  ASSERT_EQ(1, app.num_threads);

  // next pos available is 1
  ret_code = NaClAddThread(&app, NULL);
  ASSERT_EQ(1, ret_code);
  ASSERT_EQ(2, app.num_threads);

  // no thread at pos 1 -> pos 1 is available
  ret_code = NaClAddThread(&app, appt);
  ASSERT_EQ(1, ret_code);
  ASSERT_EQ(3, app.num_threads);

  NaClRemoveThread(&app, 0);
  ASSERT_EQ(2, app.num_threads);
}

TEST_F(SelLdrTest, MinimumThreadGenerationTest) {
  struct NaClApp app;
  ASSERT_EQ(1, NaClAppCtor(&app));
  ASSERT_EQ(INT_MAX, NaClMinimumThreadGeneration(&app));

  struct NaClAppThread thread1;
  struct NaClAppThread thread2;
  // Perform some minimal initialisation of our NaClAppThreads based
  // on what we know NaClMinimumThreadGeneration() does.  Reusing
  // NaClAppThreadMake() here is difficult because it launches an
  // untrusted thread.
  memset(&thread1, 0xff, sizeof(thread1));
  memset(&thread2, 0xff, sizeof(thread2));
  ASSERT_EQ(1, NaClMutexCtor(&thread1.mu));
  ASSERT_EQ(1, NaClMutexCtor(&thread2.mu));
  thread1.dynamic_delete_generation = 200;
  thread2.dynamic_delete_generation = 100;

  ASSERT_EQ(0, NaClAddThread(&app, &thread1));
  ASSERT_EQ(200, NaClMinimumThreadGeneration(&app));
  ASSERT_EQ(1, NaClAddThread(&app, &thread2));
  ASSERT_EQ(100, NaClMinimumThreadGeneration(&app));

  thread2.dynamic_delete_generation = 300;
  ASSERT_EQ(200, NaClMinimumThreadGeneration(&app));

  // This is a regression test for
  // http://code.google.com/p/nativeclient/issues/detail?id=2190.
  // The thread array can contain NULL entries where threads have
  // exited and been removed.  NaClMinimumThreadGeneration() should
  // know to skip those.  Also, if it wrongly uses num_threads instead
  // of threads.num_entries it will miss thread2 and not return 300.
  NaClRemoveThread(&app, 0);
  ASSERT_EQ(300, NaClMinimumThreadGeneration(&app));
}

TEST_F(SelLdrTest, NaClUserToSysAddrRangeTest) {
  struct NaClApp app;

  ASSERT_EQ(1, NaClAppCtor(&app));
  /*
   * addr_bits set appropriately.  mem_start is 0, which is bogus but
   * doesn't matter wrt to what this is testing.
   */
  uintptr_t addr_test;
  size_t obj_size;

  obj_size = 16;

  /*
   * small object placement
   */
  addr_test = 65536;
  ASSERT_EQ(addr_test,
            NaClUserToSysAddrRange(&app, addr_test, obj_size));

  addr_test = ((uintptr_t) 1U << app.addr_bits) - obj_size;
  ASSERT_EQ(addr_test,
            NaClUserToSysAddrRange(&app, addr_test, obj_size));

  addr_test = ((uintptr_t) 1U << app.addr_bits) - obj_size + 1;
  ASSERT_EQ(kNaClBadAddress,
            NaClUserToSysAddrRange(&app, addr_test, obj_size));

  /* size-based exceed range */
  addr_test = 65536;
  obj_size = ((uintptr_t) 1U << app.addr_bits) - addr_test;
  ASSERT_EQ(addr_test,
            NaClUserToSysAddrRange(&app, addr_test, obj_size));

  addr_test = 65536;
  obj_size = ((uintptr_t) 1U << app.addr_bits) - addr_test + 1;
  ASSERT_EQ(kNaClBadAddress,
            NaClUserToSysAddrRange(&app, addr_test, obj_size));

  /*
   * wraparound; assumes ~(uintptr_t) 0 is greater than
   * ((uintptr_t) 1U) << app.addr_bits
   */

  addr_test = 65536;
  obj_size = ~(uintptr_t) 0U - addr_test;
  ASSERT_EQ(kNaClBadAddress,
            NaClUserToSysAddrRange(&app, addr_test, obj_size));

  addr_test = 65536;
  obj_size = ~(uintptr_t) 0U - addr_test + 1;
  ASSERT_EQ(kNaClBadAddress,
            NaClUserToSysAddrRange(&app, addr_test, obj_size));
}

// On Intel Atom CPUs, memory accesses through the %gs segment are
// slow unless the start of the %gs segment is 64-byte-aligned.  This
// is a sanity check to ensure our alignment declarations work.
#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 32
TEST_F(SelLdrTest, GsSegmentAlignmentTest) {
  struct NaClAppThread *natp =
      (struct NaClAppThread *)
      NaClAlignedMalloc(sizeof(*natp), __alignof(struct NaClAppThread));
  ASSERT_TRUE(natp);
  // We use "volatile" in an attempt to prevent the compiler from
  // optimizing away our assertion based on the compiler's own
  // knowledge of the alignment of the struct it allocated.
  volatile uintptr_t addr = (uintptr_t) &natp->user.gs_segment;
  ASSERT_EQ((int) (addr % 64), 0);
  NaClAlignedFree(natp);
}
#endif
