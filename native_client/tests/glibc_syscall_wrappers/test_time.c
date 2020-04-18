/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <time.h>

int main(void) {
  time_t a, b;

  a = time(&b);
  /* Compare with January 1st 2010 */
  assert(1262304000 < a);
  assert(a == b);
  return 0;
}
