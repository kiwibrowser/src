/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/shared/platform/aligned_malloc.h"


void *NaClAlignedMalloc(size_t size, size_t alignment) {
  return _aligned_malloc(size, alignment);
}

void NaClAlignedFree(void *block) {
  _aligned_free(block);
}
