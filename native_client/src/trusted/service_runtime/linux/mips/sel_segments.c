/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdint.h>
#include "native_client/src/trusted/service_runtime/sel_util.h"

uint32_t NaClGetStackPtr(void) {
  uint32_t sp;

  asm("move %0, $sp" : "=r" (sp));

  return sp;
}

