/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

void far_away(void);

int main(void) {
  /*
   * This is useful test only if the branch distance here is more than
   * 32MB, which is the limit for ARM branches.
   */
  assert((uintptr_t) &far_away > (uintptr_t) &main);
  assert((uintptr_t) &far_away - (uintptr_t) &main > (32 << 20));
  far_away();
  return 1;
}
