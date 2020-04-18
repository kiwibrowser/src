/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_INTERVAL_MULTISET_NACL_INTERVAL_LIST_INTERN_H_
#define NATIVE_CLIENT_SRC_TRUSTED_INTERVAL_MULTISET_NACL_INTERVAL_LIST_INTERN_H_

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/trusted/interval_multiset/nacl_interval_multiset.h"

EXTERN_C_BEGIN

/*
 * Object size needed for placement-new style construction.  Internals
 * should be treated as opaque.
 */

struct NaClIntervalNode;

struct NaClIntervalListMultiset {
  struct NaClIntervalMultiset base;
  struct NaClIntervalNode *intervals;
};

EXTERN_C_END

#endif
