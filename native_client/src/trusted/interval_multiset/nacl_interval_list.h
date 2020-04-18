/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_INTERVAL_MULTISET_NACL_INTERVAL_LIST_H_
#define NATIVE_CLIENT_SRC_TRUSTED_INTERVAL_MULTISET_NACL_INTERVAL_LIST_H_

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/trusted/interval_multiset/nacl_interval_multiset.h"

EXTERN_C_BEGIN

struct NaClIntervalListMultiset;

int NaClIntervalListMultisetCtor(struct NaClIntervalListMultiset *self);

EXTERN_C_END

#endif
