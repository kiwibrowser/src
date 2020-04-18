/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/tests/pnacl_shared_lib_test/lib_data3.h"

#include <stdint.h>
#include <stdio.h>

#include "native_client/tests/pnacl_shared_lib_test/useful_constants.h"
#include "native_client/tests/pnacl_shared_lib_test/utils.h"

TLS_OR_NOT const char lib_3_string[kSTRING_3_LEN] = kSTRING_3_INITIAL;
TLS_OR_NOT double lib_3_double = kDOUBLE_3_INITIAL;
TLS_OR_NOT int64_t lib_3_int64 = kINT64_3_INITIAL;
TLS_OR_NOT int32_t lib_3_int32 = kINT32_3_INITIAL;
TLS_OR_NOT int16_t lib_3_int16 = kINT16_3_INITIAL;
TLS_OR_NOT int8_t lib_3_int8 = kINT8_3_INITIAL;

int foo3(void) {
  int num_errs = 0;

  fprintf(stderr, "*** lib3 checking own values\n");

  CHECK_EQ(lib_3_int8, kINT8_3_INITIAL, "%d", num_errs);
  CHECK_EQ(lib_3_int16, kINT16_3_INITIAL, "%d", num_errs);
  CHECK_EQ(lib_3_int32, kINT32_3_INITIAL, "%d", num_errs);
  CHECK_EQ(lib_3_int64, kINT64_3_INITIAL, "%lld", num_errs);
  CHECK_EQ(lib_3_double, kDOUBLE_3_INITIAL, "%f", num_errs);
  CHECK_STR_EQ(lib_3_string, kSTRING_3_INITIAL, num_errs);

  if (num_errs == 0) {
    fprintf(stderr, "PASS\n");
  } else {
    fprintf(stderr, "FAIL\n");
  }

  return num_errs;
}
