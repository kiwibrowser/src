/*
 * Copyright 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <string.h>
#include <sys/mman.h>

#include "native_client/src/include/nacl_assert.h"
#include "native_client/src/untrusted/irt/irt.h"

typedef int (*TestFunc)(void);

void test_clear_cache() {
  struct nacl_irt_icache irt_icache;
  ASSERT_EQ(sizeof(irt_icache), nacl_interface_query(NACL_IRT_ICACHE_v0_1,
                                                     &irt_icache,
                                                     sizeof(irt_icache)));

  static const uint32_t code_template[] = {
    0xe3000001,  // movw r0, #0x1
    0xe12fff1e,  // bx lr
  };
  void *start =
      mmap(NULL, sizeof(code_template), PROT_READ | PROT_WRITE | PROT_EXEC,
           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  ASSERT_NE(MAP_FAILED, start);
  memcpy(start, code_template, sizeof(code_template));
  size_t size = sizeof(code_template);
  ASSERT_EQ(0, irt_icache.clear_cache(start, size));
  TestFunc func = reinterpret_cast<TestFunc>(
      reinterpret_cast<uintptr_t>(start));
  ASSERT_EQ(0x1, func());
  // Modify the function to return 0x11.
  *reinterpret_cast<uint32_t*>(start) = 0xe3000011;  // movw r0, #0x11
  // In most cases 0x1 is still returned because the cached code is executed
  // although the CPU is not obliged to do so.
  // Uncomment the following line to see if the cached code is executed.
  // ASSERT_EQ(0x1, func());
  ASSERT_EQ(0, irt_icache.clear_cache(start, size));
  // Now it is ensured that 0x11 is returned because I-cache was invalidated
  // and updated with the new code.
  ASSERT_EQ(0x11, func());
  ASSERT_EQ(0, munmap(start, sizeof(code_template)));
}

int main(int argc, char *argv[]) {
  test_clear_cache();
  return 0;
}
