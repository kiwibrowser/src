/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "native_client/tests/toolchain/utils.h"

int main(int argc, char *argv[]);
void recurse(int n, int is_first);

#define MAIN_ADDR       ((uintptr_t) &main)
#define RECURSE_ADDR    ((uintptr_t) &recurse)

static bool inside_main(uintptr_t addr) {
  if (MAIN_ADDR > RECURSE_ADDR)
    return addr >= MAIN_ADDR;
  return addr < RECURSE_ADDR;
}

static bool inside_recurse(uintptr_t addr) {
  if (MAIN_ADDR > RECURSE_ADDR)
    return addr < MAIN_ADDR;
  return addr >= RECURSE_ADDR;
}

__attribute__((noinline)) void recurse(int n, int is_first) {
  /* c.f.  http://gcc.gnu.org/onlinedocs/gcc/Return-Address.html */
  uintptr_t ra = (uintptr_t) __builtin_return_address(0);
  printf("ra: %#x\n", ra);

  if (is_first) {
    ASSERT(inside_main(ra), "ERROR: ra to main is off\n");
  } else {
    ASSERT(inside_recurse(ra), "ERROR: ra to recurse is off\n");
  }

  if (n != 0) {
    recurse(n - 1, 0);
    /* NOTE: this print statement also prevents this function
     * from tail recursing into itself.
     * On gcc this behavior can also be controlled using
     *   -foptimize-sibling-calls
     */
    printf("recurse <- %d\n", n);
  }
}

int main(int argc, char *argv[]) {
  /* NOTE: confuse optimizer, argc is never 5555 */
  if (argc != 5555) {
    argc = 10;
  }
  printf("main %#x recurse %#x\n", (uintptr_t) main, (uintptr_t) recurse);

  recurse(argc, 1);
  return 55;
}
