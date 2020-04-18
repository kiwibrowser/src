/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
  if (strstr(argv[0], "/argv0_test.nexe") == NULL) {
    fprintf(stderr, "unexpected argv[0]: %s\n", argv[0]);
    return 1;
  }
  return 0;
}
