/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>

extern int SyscallReturnIsSandboxed(void);

int main(void) {
  if (SyscallReturnIsSandboxed()) {
    printf("PASSED\n");
    return 0;
  } else {
    printf("FAILED\n");
    return 1;
  }
}
