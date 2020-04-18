/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>


/* Test that we can't write to the read-only data segment. */

/* We must initialise this for it to be put into .rodata. */
const char buf[] = "x";

int main(void) {
  fprintf(stderr, "** intended_exit_status=untrusted_segfault\n");
  /* This should fault. */
  *(char *) buf = 'y';
  fprintf(stderr, "We're still running. This is not good.\n");
  return 1;
}
