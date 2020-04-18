/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>

int __thread a;
int __thread b = 1;
int c = 2;

int main(int argc, char* argv[]) {
  a++;
  b++;
  c++;
  printf("glibc-static %d %d %d", a, b, c);
  return 0;
}

