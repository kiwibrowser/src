/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdlib.h>
#include <unistd.h>

/*
 * This replaces newlib's definition of abort(), which calls _exit(1),
 * which tends not to be helpful because it produces no diagnostics
 * and won't trigger a crash reporter or a debugger.
 * See: https://code.google.com/p/nativeclient/issues/detail?id=3248
 */
void abort(void) {
  static const char message[] = "** abort() called\n";
  write(2, message, sizeof(message) - 1);

  /* Crash, so that this can trigger a crash reporter or a debugger. */
  __builtin_trap();
}
