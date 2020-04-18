/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_TESTS_PNACL_SHARED_LIB_TEST_LIB_DATA1_H_
#define NATIVE_CLIENT_TESTS_PNACL_SHARED_LIB_TEST_LIB_DATA1_H_

#include <stdint.h>
#include "native_client/tests/pnacl_shared_lib_test/useful_constants.h"
#include "native_client/tests/pnacl_shared_lib_test/utils.h"

extern TLS_OR_NOT int8_t lib_1_int8;
extern TLS_OR_NOT int16_t lib_1_int16;
extern TLS_OR_NOT int32_t lib_1_int32;
extern TLS_OR_NOT int64_t lib_1_int64;
extern TLS_OR_NOT double lib_1_double;
extern TLS_OR_NOT const char lib_1_string[kSTRING_1_LEN]
__attribute__((aligned(32)));

extern int foo1(void);

#endif  /* NATIVE_CLIENT_TESTS_PNACL_SHARED_LIB_TEST_LIB_DATA1_H_ */
