/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This exists only because Windows 64 doesn't support trivial inline assembly.
 */

#include "native_client/src/trusted/service_runtime/nacl_config.h"

        .text

DEFINE_GLOBAL_HIDDEN_FUNCTION(NaClDoFnstcw):
        fnstcw (%rcx)
        ret
