/*
 * Copyright (c) 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <string.h>

/*
 * Refer to: https://code.google.com/p/nativeclient/issues/detail?id=4049
 * For PNaCl, we internalize used globals, since we have the whole, single
 * program.
 */
__attribute__((visibility("hidden")))
__attribute__((used))
unsigned foo(const char *bar) {
  /* foo should not be called anywhere. We want to validate it will
   * still be present in the output binary.
   */

  // CHECK: <{{.*}}foo{{.*}}>:
  //
  unsigned len = strlen(bar);
  return len;
}

int main() {
  return 0;
}
