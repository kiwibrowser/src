/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/stat.h>

int main(int argc, char** argv) {
  struct stat st;
  FILE *file;
  int fd;
  if (2 != argc) {
    printf("Usage: sel_ldr test_fstat.nexe test_stat_data\n");
    return 1;
  }
  file = fopen(argv[1], "r");
  if (file == NULL)
    return 2;
  fd = fileno(file);
  errno = 0;
  if (fstat(fd, &st))
    return 3;
  printf("%d\n", (int)st.st_size);
  if (errno != 0)
    return 4;
  if (fclose(file))
    return 5;
  errno = 0;
  if (fstat(-1, &st) != -1)
    return 6;
  if (errno != EBADF)
    return 7;
  return 0;
}
