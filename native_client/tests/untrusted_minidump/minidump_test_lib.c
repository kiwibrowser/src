/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

__attribute__((noinline))
void lib_crash(void) {
  *(volatile int *) 1 = 1;
}
