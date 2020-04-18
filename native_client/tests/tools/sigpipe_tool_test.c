/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

int main(void) {
  int p[2];

  if (-1 == pipe(p)) {
    perror("pipe");
    return 1;
  }
  if (-1 == close(p[0])) {
    perror("close read end");
    return 2;
  }
  printf("Hello world foo bar\n");
  fprintf(stderr, "something for you to read.\n");
  fflush(NULL);
  if (3 == write(p[1], "foo", 3)) {
    fprintf(stderr, "Write to pipe with no readers wrote all bytes!\n");
  }
  fprintf(stderr,
          "Write to write end of pipe with no readers did not"
          " result in SIGPIPE?!?\n");
  return 1;
}
