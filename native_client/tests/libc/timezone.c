/*
 * Copyright 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "native_client/src/include/nacl_assert.h"

/*
 * Test that the C library defines the variables timezone, daylight and
 * tzname.  This is a regression test for this issue with nacl-newlib:
 * https://bugs.chromium.org/p/nativeclient/issues/detail?id=3737
 *
 * Also test that tzset() sets these variables correctly based on the "TZ"
 * environment variable.
 */

int main(void) {
  int rc = setenv("TZ", "BLAH+5:30", 1);
  ASSERT_EQ(rc, 0);

  tzset();

  ASSERT_EQ(timezone, 60 * (60 * 5 + 30)); /* 5h30m in seconds */
  ASSERT_EQ(daylight, 0);
  ASSERT_EQ(strcmp(tzname[0], "BLAH"), 0);
  ASSERT_EQ(strcmp(tzname[1], "BLAH"), 0);

  return 0;
}
