/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#ifndef NATIVE_CLIENT_TESTS_TOOLCHAIN_EH_HELPER_H_
#define NATIVE_CLIENT_TESTS_TOOLCHAIN_EH_HELPER_H_ 1

#include <stdio.h>
#include <stdlib.h>

int g_step = 0;

void __attribute__((noinline)) next_step(int n) {
  printf("%d -> %d\n", g_step, n);
  if (g_step != n - 1) {
    printf("ERROR: bad step\n");
    abort();
  }
  g_step = n;
}

#endif  // NATIVE_CLIENT_TESTS_TOOLCHAIN_EH_HELPER_H_
