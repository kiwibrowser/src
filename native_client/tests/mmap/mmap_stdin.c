/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

int main(void) {
  const size_t pagesize = sysconf(_SC_PAGESIZE);
  void *ptr = mmap(NULL, pagesize, PROT_READ, MAP_PRIVATE, STDIN_FILENO, 0);
  if (ptr == MAP_FAILED) {
    perror("mmap");
    return 0;
  }
  printf("mmap succeeded (%p) on stdin!\n", ptr);
  return 1;
}
