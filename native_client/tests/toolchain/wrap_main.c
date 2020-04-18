/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Ensure that linker symbol name rewriting works.
 *
 * When using ``-Wl,--wrap=sym``, the ``sym`` definition is rewritten to
 * ``__real_sym`` and all calls/declarations of ``sym`` are rewritten to
 * `__wrap_sym``. Note that this only applies across object files,
 * direct calls within an object file are unaffected.
 *
 * The wrap_lib{1,2}.c files are built as an archive and linked with
 * wrap_main.c. They contain the ``foo`` and ``bar`` definitions as well
 * as their wrapped equivalents. wrap_main.c calls ``foo`` and ``bar``,
 * which are rewritten to calls to the ``__wrap_*`` functions
 * instead. The ``__wrap_*`` functions then call the ``__real_*`` ones,
 * which are the now-renamed ``foo`` and ``bar``.
 *
 * The golden file expects the four functions to get called, and as an
 * extra check we expect the sum of the return value of the functions to
 * be 2+3+5+7 (the return value of each of the four functions).
 */

#include <stdlib.h>

extern int foo(void);
extern int bar(void);

int main(void) {
  int f = foo();
  int b = bar();
  return f + b == 17 ? EXIT_SUCCESS : EXIT_FAILURE;
}
