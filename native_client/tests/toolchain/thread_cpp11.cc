/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This test ensures that the PNaCl backends can deal with C++11's
 * <thread> header.
 */

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <thread>


#define STR_(A) #A
#define STR(A) STR_(A)

#define CHECK_NE(LHS, RHS, MSG) do {            \
    printf("\t" MSG ":\t" STR(LHS) "=%" PRIu64  \
           " and " STR(RHS) "=%" PRIu64 "\n",   \
           (uint64_t)(LHS), (uint64_t)(RHS));   \
    if ((LHS) == (RHS)) {                       \
      fprintf(stderr, "ERROR: " MSG ": `"       \
              STR(LHS) " == " STR(RHS) "` "     \
              "\n");                            \
      exit(1);                                  \
    }                                           \
  } while (0)

// std::thread::hardware_concurrency returns 0 if not computable or not
// well defined, and a hint about the number of hardware thread contexts
// otherwise.
void test_hardware_concurrency() {
  CHECK_NE(std::thread::hardware_concurrency(), 0,
           "NaCl should know the platform's hardware concurrency");
}

// TODO(jfb) Test the rest of C++11 <thread>.

int main() {
  test_hardware_concurrency();
  return 0;
}
