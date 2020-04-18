/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Windows-specific routines for verifying that Data Execution Prevention is
 * functional.
 */

#include "native_client/src/include/portability.h"

#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>

#include "native_client/src/trusted/platform_qualify/nacl_dep_qualify.h"


int NaClAttemptToExecuteDataAtAddr(uint8_t *thunk_buffer, size_t size) {
  int got_fault = 0;
  nacl_void_thunk thunk = NaClGenerateThunk(thunk_buffer, size);
  __try {
    thunk();
  } __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION
              ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
    got_fault = 1;
  }
  return got_fault;
}

/*
 * Returns 1 if Data Execution Prevention is present and working.
 */
int NaClAttemptToExecuteData(void) {
  int result;
  uint8_t *thunk_buffer = malloc(64);
  if (NULL == thunk_buffer) {
    return 0;
  }
  result = NaClAttemptToExecuteDataAtAddr(thunk_buffer, 64);
  free(thunk_buffer);
  return result;
}
