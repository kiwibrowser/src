/*
 * Copyright (c) 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>

/*
 * A simple program that makes it simple to test that all the arguments
 * were passed correctly.
 */

int main(int argc, char **argv) {
  for (int i = 1; i < argc; ++i) {
    puts(argv[i]);
  }
  return 0;
}

/*
 * The linker's -dynamic-linker command-line flag doesn't cause a PT_INTERP
 * to be generated unless there are some dynamic objects in the link.  But
 * the magic ".interp" section name does cause a PT_INTERP, so we use that
 * for these tests.
 */
#if defined(INTERPRETER)
# define STRINGIFY(x) STRINGIFY_1(x)
# define STRINGIFY_1(x) #x
static const char pt_interp[] __attribute__((used, section(".interp"))) =
    STRINGIFY(INTERPRETER);
#endif
