/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>

#include "native_client/src/include/nacl_assert.h"

void call_with_misaligned_stack(void (*func)(void));

volatile uintptr_t addrf;

/*
 * Test that locals are properly aligned, even if functions are called with a
 * misaligned stack. This tests the effect of LLC's -force-align-stack (and also
 * the fact that doubles must be 8-byte aligned). This test should only pass
 * when using a flag that forces stack realignment.
*/
void testfunc(void) {
  double f;
  /*
   * Smart compiler will optimize away the test (it assumes the alignment is
   * correct) unless we write it to a volatile.
   */
  addrf = (uintptr_t) &f;
  ASSERT_EQ(addrf % 8, 0);
}

int main(void) {
  printf("Calling testfunc with properly aligned stack\n");
  testfunc();
  printf("Calling testfunc with misaligned stack\n");
  call_with_misaligned_stack(testfunc);
  return 0;
}
