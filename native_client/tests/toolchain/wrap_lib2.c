/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * See wrap_main.c.
 */

#include <stdio.h>

extern int __real_foo(void);

int __wrap_foo(void) {
  printf("__wrap_foo()\n");
  return 5 + __real_foo();
}
