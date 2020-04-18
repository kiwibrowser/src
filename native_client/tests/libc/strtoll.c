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

int test(const char *str, int64_t want, int base) {
  int64_t val;
  errno = 0; /* errno is undefined on success unless initialized */
  val = strtoll(str, 0, base);
  if (errno != 0) {
    fprintf(stderr, "strtoll(\"%s\", 0, %d): %s\n", str, base, strerror(errno));
    return 1;
  }
  if (val == want)
    return 0;
  fprintf(stderr, "%lld != %lld\n", val, want);
  return 1;
}

int main(void) {
  int errors = 0;
  errors += test("-5", -5, 10);
  errors += test("7FFFFFFFFFFFFFFF", 0x7FFFFFFFFFFFFFFFLL, 16);
  return errors;
}
