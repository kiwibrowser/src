/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define BUF_SIZE 64

int main(int argc, char** argv) {
  FILE* file;
  char buf[BUF_SIZE];
  int res;
  int y;

  if (2 != argc) {
    fprintf(stderr, "Usage: sel_ldr test_rewind.nexe "
            "path/to/test_rewind_data\n");
    return 1;
  }
  file = fopen(argv[1], "r");
  assert(NULL != file);
  for (y = 0; y < 8; ++y) {
    res = fread(buf, 1, BUF_SIZE, file);
    assert(0 < res);
    buf[res] = '\0';
    res = strcmp(buf, "test");
    assert(0 == res);
    rewind(file);
  }
  return 0;
}
