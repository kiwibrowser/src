/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/include/build_config.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"


#if NACL_OSX
        /*
         * This entry point is used on Mac OS X for resuming a
         * suspended thread with registers modified, because OS X's
         * thread_set_state() ignores non-trusted-code segment
         * register values.
         *
         * This point will be entered with most registers restored via
         * thread_set_state(): we want to minimize the number of
         * instructions executed before reaching untrusted code
         * because, if the trap flag is set, the exception handler
         * will have to single-step through them.
         */
DEFINE_GLOBAL_HIDDEN_FUNCTION(NaClSwitchRemainingRegsViaECX):
        /* Restore segment registers to values for untrusted code. */
        movw 0x16(%ecx), %gs
        movw 0x14(%ecx), %fs
        movw 0x12(%ecx), %es

        /* Jump to springboard, restoring %cs. */
        ljmp *8(%ecx)
DEFINE_GLOBAL_HIDDEN_LOCATION(NaClSwitchRemainingRegsAsmEnd):
#endif
