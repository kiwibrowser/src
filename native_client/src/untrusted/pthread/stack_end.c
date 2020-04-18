/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>

#include "native_client/src/untrusted/nacl/start.h"
#include "native_client/src/untrusted/pthread/pthread_internal.h"
#include "native_client/src/untrusted/pthread/pthread_types.h"


int pthread_get_stack_end_np(pthread_t tid, void **stack_end) {
  if (tid == __nc_initial_thread_id) {
    *stack_end = __nacl_initial_thread_stack_end;
    return 0;
  }
  /*
   * Potential race condition: If the thread exits while we are
   * querying it, tdb will be freed and this could crash.  We could
   * address that by locking __nc_thread_management_lock, but:
   *  * This function is intended to be used by a crash reporter, and
   *    we don't want to claim mutexes in a crash handler in case
   *    doing so deadlocks or crashes.
   *  * It is not very useful to have a stack address for a thread that
   *    has exited anyway.
   */
  if (tid->tdb == NULL) {
    return EINVAL;
  }
  nc_thread_memory_block_t *stack_node = tid->tdb->stack_node;
  char *stack_addr = nc_memory_block_to_payload(stack_node);
  *stack_end = stack_addr + stack_node->size;
  return 0;
}
