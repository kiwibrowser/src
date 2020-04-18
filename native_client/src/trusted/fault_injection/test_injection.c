/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/fault_injection/test_injection.h"

static struct NaClTestInjectionTable const g_null_test_injection;
/*
 * Zero filled.  This structure contains function pointers for test
 * injection, and this file's definition is for tests where no test
 * injection should occur.  The structure will (slowly) grow in size
 * as the number of tests that need test injection junctures grow, and
 * the header dependency will cause the recompilation to occur.
 */

struct NaClTestInjectionTable const *g_nacl_test_injection_functions =
    &g_null_test_injection;

void NaClTestInjectionSetInjectionTable(
    struct NaClTestInjectionTable const *new_table) {
  g_nacl_test_injection_functions = new_table;
}
