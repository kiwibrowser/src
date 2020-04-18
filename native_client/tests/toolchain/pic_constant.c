/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <string.h>

#include "native_client/tests/toolchain/pic_constant_lib.h"

/*
 * This is called by pic_constant_lib.c, which see.
 */
const char *g(const char *s, size_t n) {
  return s;
}

int main(void) {
  int result = 0;

  const char *s = i();
  if (strcmp(s, "string constant") != 0)
    result = 1;

  char buf[4];
  sprint_nybble(9, buf);
  if (memcmp(buf, "1001", 4) != 0)
    result = 1;

  return result;
}
