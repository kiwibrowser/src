/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/tests/pnacl_shared_lib_test/lib_data1.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "native_client/tests/pnacl_shared_lib_test/lib_data2.h"
#include "native_client/tests/pnacl_shared_lib_test/useful_constants.h"
#include "native_client/tests/pnacl_shared_lib_test/utils.h"

TLS_OR_NOT int8_t lib_1_int8 = kINT8_1_INITIAL;
TLS_OR_NOT int16_t lib_1_int16 = kINT16_1_INITIAL;
TLS_OR_NOT int32_t lib_1_int32 = kINT32_1_INITIAL;
TLS_OR_NOT int64_t lib_1_int64 = kINT64_1_INITIAL;
TLS_OR_NOT double lib_1_double = kDOUBLE_1_INITIAL;
TLS_OR_NOT const char lib_1_string[kSTRING_1_LEN] = kSTRING_1_INITIAL;

int foo1(void) {
  int num_errs = 0;
  int checkpoint_errs = 0;

  fprintf(stderr, "*** lib1 checking own values\n");

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

  fprintf(stderr, "*** lib1 checking lib2's values\n");

  CHECK_EQ(lib_2_int8, kINT8_2_INITIAL, "%d", num_errs);
  CHECK_EQ(lib_2_int16, kINT16_2_INITIAL, "%d", num_errs);
  CHECK_EQ(lib_2_int32, kINT32_2_INITIAL, "%d", num_errs);
  CHECK_EQ(lib_2_int64, kINT64_2_INITIAL, "%lld", num_errs);
  CHECK_EQ(lib_2_double, kDOUBLE_2_INITIAL, "%f", num_errs);
  CHECK_STR_EQ(lib_2_string, kSTRING_2_INITIAL, num_errs);

  if (num_errs == checkpoint_errs) {
    fprintf(stderr, "PASS\n");
  } else {
    fprintf(stderr, "FAIL\n");
  }

  num_errs += foo2();

  return num_errs;
}
