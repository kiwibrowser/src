/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "native_client/src/include/portability.h"
#include "native_client/src/include/nacl_macros.h"

/*
 * This file is compiled by the test infrastructure as both a C and a
 * C++ program, since the nacl_macros used below have different
 * definitions depending on the language.
 */

int main(void) {
  char buffer[4096];

  printf("#buffer = %"NACL_PRIuS"\n", NACL_ARRAY_SIZE(buffer));

  return 0;
}
