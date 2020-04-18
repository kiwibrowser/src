/*
 * Copyright 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * A simple test to ensure that dup and dup2 are working.
 */

#include <stdio.h>
#include <unistd.h>

int main(void) {
  FILE  *alt;
  char  buf[1024];
  int   rv;

  printf("Hello world (1)\n");
  fflush(NULL);
  if (-1 == (rv = dup(1))) {
    fprintf(stderr, "dup(1) returned unexpected result: %d\n", rv);
  }
  alt = fdopen(rv, "w");
  fprintf(alt, "Hello world (dup)\n");
  fflush(NULL);
  fclose(alt);
  if (3 != (rv = dup2(0, 3))) {
    fprintf(stderr, "dup2(0, 3) returned unexpected result: %d\n", rv);
  }
  alt = fdopen(3, "r");
  printf("%s", fgets(buf, sizeof buf, alt));
  fclose(alt);
  if (3 != (rv = dup2(1, 3))) {
    fprintf(stderr, "dup2(1, 3) returned unexpected result: %d\n", rv);
  }
  alt = fdopen(3, "w");
  fprintf(alt, "Good bye cruel world! dup2(1, 3)\n");
  fclose(alt);
  if (20 != (rv = dup2(1, 20))) {
    fprintf(stderr, "dup2(1, 3) returned unexpected result: %d\n", rv);
  }
  alt = fdopen(20, "w");
  fprintf(alt, "Good bye cruel world! dup2(1, 20)\n");
  fclose(alt);
  return 0;
}
