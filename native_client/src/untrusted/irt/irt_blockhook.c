/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>

#include "native_client/src/untrusted/irt/irt.h"
#include "native_client/src/untrusted/irt/irt_private.h"

/*
 * This module calls callbacks in user code whenever a thread enters
 * or leaves a NaCl syscall that blocks (or might block).  This is
 * intended for use by garbage collectors that need to snapshot the
 * state of user threads.
 *
 * The current implementation tends to generate more callbacks than
 * are necessary for syscalls used internally by the PPAPI proxy.  An
 * alternative approach would be for the PPAPI proxy to call the block
 * hooks itself around PPAPI interfaces that block.  In that case, we
 * would not need g_is_irt_internal_thread below, and sys_private.c
 * would not use NACL_GC_WRAP_SYSCALL().
 * See http://code.google.com/p/nativeclient/issues/detail?id=3040
 */

static void (*pre_block_hook)(void);
static void (*post_block_hook)(void);

/*
 * We use a thread-local variable to mark IRT-internal threads.  GC
 * block hooks are callbacks provided by user code, and user code
 * should never be run on IRT-internal threads because these threads
 * do not have user thread state set up.
 */
__thread int g_is_irt_internal_thread;

void IRT_pre_irtcall_hook(void) {
  if (pre_block_hook != NULL && !g_is_irt_internal_thread) {
    pre_block_hook();
  }
}

void IRT_post_irtcall_hook(void) {
  if (post_block_hook != NULL && !g_is_irt_internal_thread) {
    post_block_hook();
  }
}

static int irt_register_block_hooks(void (*pre)(void), void (*post)(void)) {
  if (pre == NULL || post == NULL)
    return EINVAL;
  if (pre_block_hook != NULL || post_block_hook != NULL)
    return EBUSY;
  pre_block_hook = pre;
  post_block_hook = post;
  return 0;
}

const struct nacl_irt_blockhook nacl_irt_blockhook = {
  irt_register_block_hooks,
};
