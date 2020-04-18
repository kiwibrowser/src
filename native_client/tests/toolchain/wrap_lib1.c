/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * See wrap_main.c.
 */

#include <stdio.h>

extern int __real_bar(void);

int foo(void) {
  printf("foo()\n");
  return 2;
}

int bar(void) {
  printf("bar()\n");
  return 3;
}

int __wrap_bar(void) {
  printf("__wrap_bar()\n");
  return 7 + __real_bar();
}
