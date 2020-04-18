/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>

/* Prototypes to disable inlining. */
int inner(const char *, const char *) __attribute__((noinline));
int middle(const char *) __attribute__((noinline));
int outer(char *) __attribute__((noinline));
int alloca_size(void) __attribute__((noinline));


int inner(const char *outer_local, const char *middle_frame) {
  const char local = 0;

  fprintf(stderr,
          "inner: outer_local = %p, &local = %p, "
          "middle_frame = %p\n",
          (void *) outer_local, (void *) &local, (void *) middle_frame);
  if (outer_local >= &local) {
    int inner_below_middle = middle_frame >= &local;
    int middle_below_outer = outer_local >= middle_frame;
    fprintf(stderr, "inner: stack grows downwards, "
            " middle_below_outer %d, inner_below_middle %d\n",
            middle_below_outer, inner_below_middle);
    return middle_below_outer && inner_below_middle;
  }
  else {
    int inner_above_middle = middle_frame <= &local;
    int middle_above_outer = outer_local <= middle_frame;
    fprintf(stderr, "inner: stack grows upwards, "
            " middle_above_outer %d, inner_above_middle %d\n",
            middle_above_outer, inner_above_middle);
    return middle_above_outer && inner_above_middle;
  }
}

int middle(const char *outer_local) {
  const char *frame = __builtin_frame_address (0);
  int retval;

  fprintf(stderr, "middle: outer_local = %p, frame = %p\n",
          (void *) outer_local, (void *) frame);
  retval = inner(outer_local, frame);
  /* fprintf also disables tail call optimization. */
  fprintf(stderr, "middle: inner returned %d\n", retval);
  return retval != 0;
}

int outer(char *dummy) {
  const char local = 0;
  int retval;

  fprintf(stderr, "outer: &local = %p\n", (void *) &local);
  retval = middle(&local);
  /* fprintf also disables tail call optimization. */
  fprintf(stderr, "outer: middle returned %d\n", retval);
  return retval != 0;
}

int alloca_size(void) {
  return 8;
}

int main (void) {
  char *dummy = __builtin_alloca(alloca_size());
  int retval;

  fprintf(stderr, "main: dummy = %p\n", (void *) dummy);
  retval = outer(dummy);
  fprintf(stderr, "main: outer returned %d\n", retval);
  if (retval == 0)
    abort();
  return 0;
}
