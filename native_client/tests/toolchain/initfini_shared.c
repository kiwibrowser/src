/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>

typedef void (*FUN_PTR) (void);

#define ATTR_SEC(sec)\
  __attribute__ ((__used__, section(sec), aligned(sizeof(FUN_PTR))))

#define MAKE_FUN(name) void name(void) { printf(#name "\n"); }

/* NOTE: .preinit_array is not allowed in shared object */

MAKE_FUN(so_init1)
MAKE_FUN(so_init2)
MAKE_FUN(so_init3)
static const FUN_PTR array_init[] ATTR_SEC(".init_array") =
    { so_init1, so_init2, so_init3 };

MAKE_FUN(so_fini1)
MAKE_FUN(so_fini2)
MAKE_FUN(so_fini3)
static const FUN_PTR array_fini[] ATTR_SEC(".fini_array") =
    { so_fini1, so_fini2, so_fini3 };

/* NOTE: there is NO .prefini_array */
