/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <string.h>

#include "native_client/src/trusted/service_runtime/sel_ldr.h"

/*
 * This function handles the nacl_helper_bootstrap arguments r_debug
 * and reserved_at_zero and updates argc and argv to act as though
 * these arguments were not passed.  For example, the command
 *
 * nacl_helper_bootstrap program --r_debug=0xXXXXXXXXXXXXXXXX \
 * --reserved_at_zero=0xXXXXXXXXXXXXXXXX program_arg
 *
 * will result in the following argc and argv structure for program after
 * running throughg the bootstrapper:
 *
 * argc: 4
 * argv[0]: program
 * argv[1]: --r_debug=0xXXXXXXXXXXXXXXXX
 * argv[2]: --reserved_at_zero=0xXXXXXXXXXXXXXXXX
 * argv[3]: program_arg
 *
 * After program calls NaClHandleBootstrapArgs, the argc and argv structure
 * will be:
 *
 * argc: 2
 * argv[0]: program
 * argv[1]: program_arg
 *
 * argc_p is the pointer to the argc variable passed to main.
 * argv_p is the pointer to the argv variable passed to main.
 */
void NaClHandleBootstrapArgs(int *argc_p, char ***argv_p) {
  static const char kRDebugSwitch[] = "--r_debug=";
  static const char kAtZeroSwitch[] = "--reserved_at_zero=";
  int argc = *argc_p;
  char **argv = *argv_p;
  char *argv0;

  if (argc == 0)
    return;
  argv0 = argv[0];

  if (argc > 1 &&
      0 == strncmp(argv[1], kRDebugSwitch, sizeof(kRDebugSwitch) - 1)) {
    NaClHandleRDebug(&argv[1][sizeof(kRDebugSwitch) - 1], argv[0]);
    --argc;
    ++argv;
  }

  if (argc > 1 &&
      0 == strncmp(argv[1], kAtZeroSwitch, sizeof(kAtZeroSwitch) - 1)) {
    NaClHandleReservedAtZero(&argv[1][sizeof(kAtZeroSwitch) - 1]);
    --argc;
    ++argv;
  }

  *argc_p = argc;
  *argv_p = argv;
  (*argv_p)[0] = argv0;
}
