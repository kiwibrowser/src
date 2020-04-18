/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl test for super simple program not using newlib
 *
 * This test just verifies that we can compile various variable accesses
 * and the resulting relocations.
 * On ARM this not trivial especially when we do not use constant pools.
 *
 * Note, it can be useful to compile this file with arbitrary compilers to
 * study the idioms generated. If such a compiler does not have access to
 * the nacl headers use the -DNO_NACL_STUFF option.
 */

#if !defined(NO_NACL_STUFF)
#include "native_client/tests/barebones/barebones.h"
#endif

/*
 * NOTE: we declare tls vars below. We do not use them but the may
 * compiler may generate references to the symbols below, so we provide
 * some dummy functions below.
 */
void *__aeabi_read_tp(void) { return 0; }
void *__nacl_read_tp(void) { return 0; }
void *__tls_get_addr(void) { return 0; }

volatile __thread int var_tls = 1;
volatile __thread double var_tls_double = 1.0;
/*
 * Note, llc for ARM will merge var_static and var_global into one area
 * c.f. llvm-trunk/lib/Target/ARM/ARMGlobalMerge.cpp
 */

volatile static int var_static = 1;
volatile static double var_static_double = 1.0;

volatile int var_global = 1;
volatile double var_global_double = 1.0;

int main(int argc, char* argv[]) {
  if (argc == 555) {
    /* this should never be executed */
    var_tls = 11;
    var_tls_double = 22.0;

    var_static = 33;
    var_static_double = 44.0;

    var_global = 55;
    var_global_double = 66.0;
  }

  if (argc == 6666) {
    /* this should never be executed */
    return  (int) &var_tls +
            (int) &var_static +
            (int) &var_global +
            (int) &var_tls_double +
            (int) &var_static_double +
            (int) &var_global_double;
  }
#if !defined(NO_NACL_STUFF)
  NACL_SYSCALL(exit)(55);
#endif
  /* UNREACHABLE */
  return 0;
}
