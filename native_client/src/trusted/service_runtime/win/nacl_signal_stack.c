/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/service_runtime/nacl_signal.h"


/*
 * On Windows, we do not have an ALT stack available, so we do not
 * allocate one.  This makes signal handling in Windows unsafe.
 */
int NaClSignalStackAllocate(void **result) {
  UNREFERENCED_PARAMETER(result);
  return 1; /* Success */
}

void NaClSignalStackFree(void *stack) {
  UNREFERENCED_PARAMETER(stack);
}

void NaClSignalStackRegister(void *stack) {
  UNREFERENCED_PARAMETER(stack);
}

void NaClSignalStackUnregister(void) {
}
