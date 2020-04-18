/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int test(const char *str, uint64_t want, int base) {
  uint64_t val;
  errno = 0; /* errno is undefined on success unless initialized */
  val = strtoull(str, 0, base);
  if (errno != 0) {
    fprintf(stderr, "strtoull(\"%s\", 0, %d): %s\n",
        str, base, strerror(errno));
    return 1;
  }
  if (val == want)
    return 0;
  fprintf(stderr, "%llu != %llu\n", val, want);
  return 1;
}

int main(void) {
  int errors = 0;
  errors += test("5", 5, 10);
  errors += test("FFFFFFFFFFFFFFFF", 0xFFFFFFFFFFFFFFFFLL, 16);
  return errors;
}
