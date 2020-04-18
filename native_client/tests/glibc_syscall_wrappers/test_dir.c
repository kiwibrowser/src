/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char** argv) {
  DIR* dir = NULL;
  struct dirent* entry;
  int found = 0;

  if (2 != argc) {
    printf("Usage: sel_ldr test_dir.nexe directory/containing/test_dir.nexe\n");
    return 1;
  }
  dir = opendir(argv[1]);
  assert(NULL != dir);
  for (; NULL != (entry = readdir(dir)); ) {
    if (0 == strcmp("test_dir.nexe", entry->d_name)) {
      found = 1;
    }
  }
  assert(1 == found);
  assert(0 == closedir(dir));
  return 0;
}
