/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_TESTS_PNACL_SHARED_LIB_TEST_USEFUL_CONSTANTS_H_
#define NATIVE_CLIENT_TESTS_PNACL_SHARED_LIB_TEST_USEFUL_CONSTANTS_H_

/* Import constants like HUGE_VAL. */
#include <math.h>

/* Define a set of constants that fit into 8 bits, 16 bits, etc.,
 * to use as initial values, intermediate values, and expected values. */

/* For library #1. */

#define kINT8_1_INITIAL 0x7e
#define kINT16_1_INITIAL 0x7def
#define kINT32_1_INITIAL 0x7012abcd
#define kINT64_1_INITIAL 0x7012abcd01239876LL
#define kDOUBLE_1_INITIAL 1234567890.0

#define kSTRING_1_LEN 17
#define kSTRING_1_INITIAL "1234567890AbcdEF"

/* For library #2. */

#define kINT8_2_INITIAL -0x7e
#define kINT16_2_INITIAL -0x7def
#define kINT32_2_INITIAL -0x7012abcd
#define kINT64_2_INITIAL -0x7012abcd01239876LL
#define kDOUBLE_2_INITIAL HUGE_VAL

#define kSTRING_2_LEN 17
#define kSTRING_2_INITIAL " 234 edC a56 890"

/* For library #3. */

#define kINT8_3_INITIAL 0x10
#define kINT16_3_INITIAL 0x2000
#define kINT32_3_INITIAL 0x30000000
#define kINT64_3_INITIAL 0x40000000000LL
#define kDOUBLE_3_INITIAL 0.32948975

#define kSTRING_3_LEN 17
#define kSTRING_3_INITIAL "aaa bbb ccc ddd "


#endif  /* NATIVE_CLIENT_TESTS_PNACL_SHARED_LIB_TEST_USEFUL_CONSTANTS_H_ */
