/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <string.h>

#include "native_client/src/trusted/interval_multiset/nacl_interval_multiset.h"

void NaClIntervalMultisetDelete(struct NaClIntervalMultiset *obj) {
  (*obj->vtbl->Dtor)(obj);
  free(obj);
}
