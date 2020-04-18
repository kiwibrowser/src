/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "native_client/tests/pnacl_shared_lib_test/lib_data1.h"
#include "native_client/tests/pnacl_shared_lib_test/lib_data3.h"
#include "native_client/tests/pnacl_shared_lib_test/useful_constants.h"
#include "native_client/tests/pnacl_shared_lib_test/utils.h"

#ifdef TEST_TLS
#include <pthread.h>
void *test_thread_frobber(void *ignored) {

  fprintf(stderr, "Frobbing lib1's TLS variables in another thread.\n");

  lib_1_int8 = -1;
  lib_1_int16 = -2;
  lib_1_int32 = -3;
  lib_1_int64 = -4;

  fprintf(stderr, "Frobbing lib3's TLS variables in another thread.\n");

  lib_3_int8 = -5;
  lib_3_int16 = -6;
  lib_3_int32 = -7;
  lib_3_int64 = -8;

  return NULL;
}
#endif  /* TEST_TLS */


int main(int argc, char* argv[]) {
  int num_errs = 0;
  int checkpoint_errs = 0;
#ifdef TEST_TLS
  pthread_t thread_id;
#endif

  fprintf(stderr, "*** main checking lib1's values\n");

  CHECK_EQ(lib_1_int8, kINT8_1_INITIAL, "%d", num_errs);
  CHECK_EQ(lib_1_int16, kINT16_1_INITIAL, "%d", num_errs);
  CHECK_EQ(lib_1_int32, kINT32_1_INITIAL, "%d", num_errs);
  CHECK_EQ(lib_1_int64, kINT64_1_INITIAL, "%lld", num_errs);
  CHECK_EQ(lib_1_double, kDOUBLE_1_INITIAL, "%f", num_errs);
  CHECK_STR_EQ(lib_1_string, kSTRING_1_INITIAL, num_errs);

  if (num_errs == 0) {
    fprintf(stderr, "PASS\n");
  } else {
    fprintf(stderr, "FAIL\n");
  }
  checkpoint_errs = num_errs;

  num_errs += foo1();

  fprintf(stderr, "*** main checking lib3's values\n");

  CHECK_EQ(lib_3_int8, kINT8_3_INITIAL, "%d", num_errs);
  CHECK_EQ(lib_3_int16, kINT16_3_INITIAL, "%d", num_errs);
  CHECK_EQ(lib_3_int32, kINT32_3_INITIAL, "%d", num_errs);
  CHECK_EQ(lib_3_int64, kINT64_3_INITIAL, "%lld", num_errs);
  CHECK_EQ(lib_3_double, kDOUBLE_3_INITIAL, "%f", num_errs);
  CHECK_STR_EQ(lib_3_string, kSTRING_3_INITIAL, num_errs);

  if (num_errs == checkpoint_errs) {
    fprintf(stderr, "PASS\n");
  } else {
    fprintf(stderr, "FAIL\n");
  }
  checkpoint_errs = num_errs;

  num_errs += foo3();

  checkpoint_errs = num_errs;

#ifdef TEST_TLS
  fprintf(stderr, "Testing that TLS is really thread-local.\n");

  /* Frob the values in another thread. */
  if (pthread_create(&thread_id, NULL, &test_thread_frobber, NULL) != 0) {
    fprintf(stderr, "Failed to create test thread!\n");
    num_errs++;
  }
  pthread_join(thread_id, NULL);

  /* Check that the main thread is unchanged. */
  CHECK_EQ(lib_1_int8, kINT8_1_INITIAL, "%d", num_errs);
  CHECK_EQ(lib_1_int16, kINT16_1_INITIAL, "%d", num_errs);
  CHECK_EQ(lib_1_int32, kINT32_1_INITIAL, "%d", num_errs);
  CHECK_EQ(lib_1_int64, kINT64_1_INITIAL, "%lld", num_errs);
  CHECK_EQ(lib_1_double, kDOUBLE_1_INITIAL, "%f", num_errs);
  CHECK_STR_EQ(lib_1_string, kSTRING_1_INITIAL, num_errs);

  CHECK_EQ(lib_3_int8, kINT8_3_INITIAL, "%d", num_errs);
  CHECK_EQ(lib_3_int16, kINT16_3_INITIAL, "%d", num_errs);
  CHECK_EQ(lib_3_int32, kINT32_3_INITIAL, "%d", num_errs);
  CHECK_EQ(lib_3_int64, kINT64_3_INITIAL, "%lld", num_errs);
  CHECK_EQ(lib_3_double, kDOUBLE_3_INITIAL, "%f", num_errs);
  CHECK_STR_EQ(lib_3_string, kSTRING_3_INITIAL, num_errs);

  if (num_errs == checkpoint_errs) {
    fprintf(stderr, "PASS\n");
  } else {
    fprintf(stderr, "FAIL\n");
  }
#endif


  return num_errs + 55;
}
