/*
 * Copyright 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <limits>
#include <errno.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

#include "native_client/src/include/nacl_assert.h"
#include "native_client/src/untrusted/irt/irt.h"
#include "native_client/tests/nonsfi/example_interface.h"

// When we query the interface, we can cache it in this global.
bool g_irt_example_valid = false;
struct nacl_irt_example g_irt_example;

// The querying function built into the untrusted code.
struct nacl_irt_example *get_irt_example() {
  if (!g_irt_example_valid) {
    size_t rc = nacl_interface_query(NACL_IRT_EXAMPLE_v1_0, &g_irt_example,
                                     sizeof(g_irt_example));
    if (rc != sizeof(g_irt_example))
      return NULL;
    else
      g_irt_example_valid = true;
  }
  return &g_irt_example;
}

// All interface functions can be this simple "query then call" format.
int add_one(uint32_t number, uint32_t *result) {
  struct nacl_irt_example *irt_example = get_irt_example();
  if (irt_example == NULL)
    abort();
  // Note: there is no "add_one" specific code -- we just pass the
  // arguments through the IRT function.
  return irt_example->add_one(number, result);
}

int main(int argc, char *argv[]) {
  uint32_t result;
  // Normal add_one call.
  ASSERT_EQ(add_one(1, &result), 0);
  ASSERT_EQ(result, 2);
  // Overflow add_one call, demonstrating how errors can be returned.
  ASSERT_EQ(add_one(std::numeric_limits<uint32_t>::max(), &result), EOVERFLOW);
  puts("PASSED");
  return 0;
}
