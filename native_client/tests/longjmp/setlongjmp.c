/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <setjmp.h>
#include <stdio.h>

#include "native_client/src/include/nacl_assert.h"

static jmp_buf buf;

int trysetjmp(int longjmp_arg) {
  volatile int result = -1;
  int setjmp_ret = -1;

  setjmp_ret = setjmp(buf);
  if (!setjmp_ret) {
    /* Check that setjmp() doesn't return 0 multiple times */
    ASSERT_EQ(result, -1);

    result = 55;
    printf("setjmp was invoked\n");
    longjmp(buf, longjmp_arg);
    printf("this print statement is not reached\n");
    return -1;
  } else {
    int expected_ret = longjmp_arg != 0 ? longjmp_arg : 1;
    ASSERT_EQ(setjmp_ret, expected_ret);
    printf("longjmp was invoked\n");
    return result;
  }
}

int main(void) {
  if (trysetjmp(1) != 55 ||
      trysetjmp(0) != 55 ||
      trysetjmp(-1) != 55)
    return -1;
  return 55;
}
