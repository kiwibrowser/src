/*
 * Copyright (c) 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

// Testing NativeClient cross-platfom memory management functions

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/include/nacl_platform.h"
#include "native_client/src/shared/platform/nacl_log.h"

// TODO(robertm): eliminate need for this
#if NACL_WINDOWS
#include "native_client/src/include/win/mman.h"
#endif
#include "native_client/src/trusted/service_runtime/sel_memory.h"

#include "gtest/gtest.h"

class SelMemoryBasic : public testing::Test {
 protected:
  virtual void SetUp();
  virtual void TearDown();
  // TODO(gregoryd) - do we need a destructor here?
};

void SelMemoryBasic::SetUp() {
  NaClLogModuleInit();
}

void SelMemoryBasic::TearDown() {
  NaClLogModuleFini();
}


TEST_F(SelMemoryBasic, AllocationTests) {
  int res = 0;
  void *p = NULL;
  int size;

  size = 0x2001;  // not power of two - should be supported

  res = NaClPageAlloc(&p, size);
  EXPECT_EQ(0, res);
  EXPECT_NE(static_cast<void *>(NULL), p);

  NaClPageFree(p, size);
  p = NULL;

  // Try to allocate large memory block
  size = 256 * 1024 * 1024;  // 256M

  res = NaClPageAlloc(&p, size);
  EXPECT_EQ(0, res);
  EXPECT_NE(static_cast<void *>(NULL), p);

  NaClPageFree(p, size);
}

TEST_F(SelMemoryBasic, mprotect) {
  int res = 0;
  void *p = NULL;
  int size;
  char *addr;

  size = 0x100000;

  res = NaClPageAlloc(&p, size);
  EXPECT_EQ(0, res);
  EXPECT_NE(static_cast<void *>(NULL), p);

  // TODO(gregoryd) - since ASSERT_DEATH is not supported for client projects,
  // we cannot use gUnit to test the protection. We might want to add some
  // internal processing (based on SEH/signals) at some stage

  res = NaClMprotect(p, size, PROT_READ |PROT_WRITE);
  EXPECT_EQ(0, res);
  addr = reinterpret_cast<char*>(p);
  addr[0] = '5';

  res = NaClMprotect(p, size, PROT_READ);
  EXPECT_EQ(0, res);
  EXPECT_EQ('5', addr[0]);

  res = NaClMprotect(p, size, PROT_READ|PROT_WRITE|PROT_EXEC);

  NaClPageFree(p, size);
}
