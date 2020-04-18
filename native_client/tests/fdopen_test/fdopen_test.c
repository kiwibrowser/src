/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>


int main(int argc, char *argv[]) {
  assert(argc == 2);
  int fd = open(argv[1], O_RDONLY);
  assert(fd > 0);
  FILE *file = fdopen(fd, "r");
  assert(fd == fileno(file));
  char buf[100];
  int count = fread(buf, 1, sizeof buf, file);
  const char expected[] = "Testing Data!";
  assert(count == strlen(expected));
  assert(memcmp(buf, "Testing Data!", count) == 0);
  fclose(file);
  return 0;
}
