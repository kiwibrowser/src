/*
 * Copyright 2009 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "native_client/src/include/portability.h"
#include "native_client/src/include/nacl_macros.h"

int main(void) {
  char buffer[4096];
  char *bufptr = buffer;
  size_t ix;

  NACL_ASSERT_IS_POINTER(bufptr);
  NACL_ASSERT_IS_ARRAY(buffer);

  /*
   * NACL_ASSERT_IS_ARRAY(bufptr);
   */

  printf("#buffer = %"NACL_PRIuS"\n", NACL_ARRAY_SIZE(buffer));

  /*
   * printf("#bufptr = %lu\n", ARRAY_SIZE(bufptr));
   */

  /*
   * for checking that the store to gNaClArrayCheck is moved out of
   * the loop.
   */
  for (ix = 0; ix < NACL_ARRAY_SIZE(buffer); ++ix) {
    buffer[ix] = (char) ix;
  }
  return (buffer[10] + buffer[4095] == 0);  /* loop was not dead code! */
}
