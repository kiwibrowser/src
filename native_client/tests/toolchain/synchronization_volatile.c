/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This test ensures that the PNaCl backends can deal with volatile
 * operations as would be written by regular users, including PNaCl's
 * ABI stabilization and target lowering.
 *
 * See other synchronization_* tests in this directory.
 *
 * This is a syntactical check as we do not run this multithreaded. Some
 * real testing is done here: tests/threads/thread_test.c
 *
 * PNaCl's atomic support is based on C11/C++11's. PNaCl only supports
 * 8, 16, 32 and 64-bit atomics, and should be able to handle any type
 * that can map to these (e.g. 32- and 64-bit atomic floating point
 * load/store).
 */

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


#define STR_(A) #A
#define STR(A) STR_(A)

#define CHECK_EQ(LHS, RHS, MSG) do {            \
    printf("\t" MSG ":\t" STR(LHS) "=%" PRIu64  \
           " and " STR(RHS) "=%" PRIu64 "\n",   \
           (uint64_t)(LHS), (uint64_t)(RHS));   \
    if ((LHS) != (RHS)) {                       \
      fprintf(stderr, "ERROR: " MSG ": `"       \
              STR(LHS) " != " STR(RHS) "` "     \
              "\n");                            \
      exit(1);                                  \
    }                                           \
  } while (0)


void test_volatile_operations(void) {
# define VOLATILE_OPERATION_TEST(OPERATION_NAME, OPERATION, TYPE) do {  \
    volatile TYPE loc = 5;                                              \
    TYPE loc_ = 5;                                                      \
    TYPE value = 41;                                                    \
    printf("Testing volatile " STR(OPERATION_NAME)                      \
           " with " STR(TYPE) "\n");                                    \
    loc = loc OPERATION value;                                          \
    loc_ = loc_ OPERATION value;                                        \
    CHECK_EQ(loc, loc_, "volatile " STR(TYPE) " " STR(OPERATION_NAME)); \
  } while (0)

# define VOLATILE_OPERATION_TEST_ALL_TYPES(OPERATION_NAME, OPERATION)   \
  VOLATILE_OPERATION_TEST(OPERATION_NAME, OPERATION, uint8_t);          \
  VOLATILE_OPERATION_TEST(OPERATION_NAME, OPERATION, uint16_t);         \
  VOLATILE_OPERATION_TEST(OPERATION_NAME, OPERATION, uint32_t);         \
  VOLATILE_OPERATION_TEST(OPERATION_NAME, OPERATION, uint64_t);

  VOLATILE_OPERATION_TEST_ALL_TYPES(add, +);
  VOLATILE_OPERATION_TEST_ALL_TYPES(sub, -);
  VOLATILE_OPERATION_TEST_ALL_TYPES(or,  |);
  VOLATILE_OPERATION_TEST_ALL_TYPES(and, &);
  VOLATILE_OPERATION_TEST_ALL_TYPES(xor, ^);
}

void test_volatile_load_store(void) {
# define VOLATILE_LOAD_STORE_TEST(TYPE) do {            \
    volatile TYPE loc = 5;                              \
    TYPE res = loc;                                     \
    CHECK_EQ(res, 5, "volatile " STR(TYPE) " load");    \
    loc = 41;                                           \
    CHECK_EQ(loc, 41, "volatile " STR(TYPE) " store");  \
  } while (0)

  VOLATILE_LOAD_STORE_TEST(uint8_t);
  VOLATILE_LOAD_STORE_TEST(uint16_t);
  VOLATILE_LOAD_STORE_TEST(uint32_t);
  VOLATILE_LOAD_STORE_TEST(uint64_t);
  VOLATILE_LOAD_STORE_TEST(float);
  VOLATILE_LOAD_STORE_TEST(double);
}


int main(void) {
  test_volatile_operations();
  test_volatile_load_store();
  return 0;
}
