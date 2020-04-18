/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>

int main(void) {
  printf("REALOUTPUT: Hello standard output\n");
  fprintf(stderr, "REALOUTPUT: Hello standard error!\n");
  return 0;
}
