/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>


typedef void (*FUN_PTR) (void);

#define ATTR_SEC(sec)\
  __attribute__ ((__used__, section(sec), aligned(sizeof(FUN_PTR))))

#define MAKE_FUN(name) void name(void) { printf(#name "\n");}

MAKE_FUN(main_preinit1)
MAKE_FUN(main_preinit2)
MAKE_FUN(main_preinit3)
static const FUN_PTR array_preinit[] ATTR_SEC(".preinit_array") =
    { main_preinit1, main_preinit2, main_preinit3 };


MAKE_FUN(main_init1)
MAKE_FUN(main_init2)
MAKE_FUN(main_init3)
static const FUN_PTR array_init[] ATTR_SEC(".init_array") =
    { main_init1, main_init2, main_init3 };

MAKE_FUN(main_fini1)
MAKE_FUN(main_fini2)
MAKE_FUN(main_fini3)
static const FUN_PTR array_fini[] ATTR_SEC(".fini_array") =
    { main_fini1, main_fini2, main_fini3 };

/* NOTE: there is NO .prefini_array */

int main(void) {
  return 0;
}
