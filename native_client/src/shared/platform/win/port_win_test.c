/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#include <stdio.h>

#include "native_client/src/include/portability.h"
uint64_t test_tls_asm(void);

int loop_ffs(int v) {
  int rv = 1;
  int mask = 1;

  while (0 != mask) {
    if (v & mask) {
      return rv;
    }
    mask <<= 1;
    ++rv;
  }
  return 0;
}

/*
 * Selectively test FFS by checking every bit while enabling every other bit.
 */
int TestFFS(void) {
  unsigned int errors = 0;
  uint32_t x;
  uint32_t bits;

  bits = 0;
  for (x = 0; x <= 32; ++x) {
    bits <<= 1;
    if (x & 1) bits |= 1;
    if (loop_ffs(x) != ffs(x)) {
      printf("ERROR: differs at %d (0x%x)\n", x, x);
      errors = 1;  /* if fail everywhere, errors would be UINT_MAX */
    }
  }
  if (loop_ffs(0) != ffs(0)) {
    printf("ERROR: differs at 0\n");
    errors = 1;
  }
  return errors;
}

/*
 * Since the Win64 version of nacl_syscall.S uses handcoded assembly to
 * retrieve TLS values, we need to test that assembly to ensure we don't
 * get surprised by undocumented changes.
 *
 * This small test is only adequate to verify that the linker trick we use in
 * nacl_syscal_64.S is still valid. See
 * src/trusted/service_runtime/nacl_tls_unittest.c for a full (and
 * cross-platform) test of TLS behavior.
 */
#ifdef _WIN64
THREAD uint64_t tlsValue;

uint64_t test_tls_c(void) {
  return tlsValue;
}

int TestTlsAccess(void) {
  int errors = 0;
  const uint64_t kFoo = 0xF000F000F000F000;
  const uint64_t kBar = 0xBAAABAAABAAABAAA;
  uint64_t testValue;

  tlsValue = kFoo;

  testValue = test_tls_c();
  if (kFoo != testValue) {
    ++errors;
  }

  tlsValue = kBar;

  testValue = test_tls_asm();
  if (kBar != testValue) {
    ++errors;
  }
  return errors;
}
#endif
