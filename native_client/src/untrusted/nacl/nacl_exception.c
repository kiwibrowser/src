/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/include/nacl/nacl_exception.h"

#include <errno.h>
#include <stdlib.h>

#include "native_client/src/untrusted/irt/irt.h"


static struct nacl_irt_exception_handling irt_exception_handling;

/*
 * We don't do any locking here, but simultaneous calls are harmless enough.
 * They'll all be writing the same values to the same words.
 */
static int set_up_irt_exception_handling(void) {
  return nacl_interface_query(NACL_IRT_EXCEPTION_HANDLING_v0_1,
                              &irt_exception_handling,
                              sizeof(irt_exception_handling))
         == sizeof(irt_exception_handling);
}

int nacl_exception_set_handler(nacl_exception_handler_t handler) {
  return nacl_exception_get_and_set_handler(handler, NULL);
}

int nacl_exception_get_and_set_handler(nacl_exception_handler_t handler,
                                       nacl_exception_handler_t *old) {
  if (irt_exception_handling.exception_handler == NULL) {
    if (!set_up_irt_exception_handling())
      return ENOSYS;
  }
  return irt_exception_handling.exception_handler(handler, old);
}

int nacl_exception_set_stack(void *stack, size_t size) {
  if (irt_exception_handling.exception_stack == NULL) {
    if (!set_up_irt_exception_handling())
      return ENOSYS;
  }
  return irt_exception_handling.exception_stack(stack, size);
}

int nacl_exception_clear_flag() {
  if (irt_exception_handling.exception_clear_flag == NULL) {
    if (!set_up_irt_exception_handling())
      return ENOSYS;
  }
  return irt_exception_handling.exception_clear_flag();
}
