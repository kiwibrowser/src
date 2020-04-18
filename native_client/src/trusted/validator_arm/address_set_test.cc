/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * A simple test for AddressSet.
 */

#ifndef NACL_TRUSTED_BUT_NOT_TCB
#error This file is not meant for use in the TCB
#endif

#include <stdio.h>
#include <stdlib.h>

#include "native_client/src/include/portability.h"
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/trusted/validator_arm/address_set.h"

using nacl_arm_val::AddressSet;

static void test_mutation() {
  uint32_t base = 0x1234;
  uint32_t size = 0x1000;
  AddressSet as(base, size);

  as.add(0x1200);  // Becomes a no-op
  as.add(base + (31 * 4));  // Added
  as.add(0x1240);  // Added
  as.add(0x1230);  // No-op
  as.add(base+size);  // No-op
  as.add(0x1235);  // Added as 1234
  as.add(0x1238);  // Added
  as.add(0x2000);  // Added
  as.add(base+size + 100);  // No-op
  as.add(0x1400);  // Added

  // Successful additions in ascending order:
  uint32_t expected[] = { 0x1234, 0x1238, 0x1240, base+(31*4), 0x1400, 0x2000 };
  for (uint32_t i = 0; i < NACL_ARRAY_SIZE(expected); i++) {
    if (!as.contains(expected[i])) {
      fprintf(stderr, "Set should contain %08X, does not.\n", expected[i]);
      abort();
    }
  }

  uint32_t x = 0;
  for (AddressSet::Iterator it = as.begin(); it != as.end(); ++it, ++x) {
    if (*it != expected[x]) {
      fprintf(stderr, "At %" NACL_PRIu32 ": expecting %08X, got %08X\n",
              x, expected[x], *it);
      abort();
    }
  }
  if (x != NACL_ARRAY_SIZE(expected)) {
    fprintf(stderr, "Expected iterator to step %" NACL_PRIuS
            " times, got %" NACL_PRIu32 "\n",
            NACL_ARRAY_SIZE(expected), x);
    abort();
  }

  // Unsuccessful additions:
  uint32_t unexpected[] = { 0x1200, 0x1230, base+size, base+size+100 };
  for (uint32_t i = 0; i < NACL_ARRAY_SIZE(unexpected); i++) {
    if (as.contains(unexpected[i])) {
      fprintf(stderr, "Set should not contain %08X.\n", unexpected[i]);
      abort();
    }
  }
}

int main() {
  test_mutation();
  puts("Tests successful.");
  return 0;
}
