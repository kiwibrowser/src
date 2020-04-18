/*
 * Copyright (c) 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * It can't get much simpler than this (uh, except for noop.c).
 */

#include <stdio.h>
#include <time.h>
#include <sys/time.h>

void loop() {
  int count = 0;
  timespec t = {1, 0};
  while (1) {
    printf("Running");
    int i = 0;
    for (i = 0; i < count % 5; ++i) {
      printf(".");
    }
    for (; i < 5; ++i) {
      printf(" ");
    }
    printf("\r");
    fflush(stdout);
    ++count;
    nanosleep(&t, 0);
  }
}

int main(int argc, char* argv[]) {
  loop();
  return 0;
}
