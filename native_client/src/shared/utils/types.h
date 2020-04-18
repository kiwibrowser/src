/*
 * Copyright 2009 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_SHARED_UTILS_TYPES_H__
#define NATIVE_CLIENT_SRC_SHARED_UTILS_TYPES_H__

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/portability.h"

EXTERN_C_BEGIN

/* Note: the names Bool, FALSE and TRUE are defined in some directories,
 * which conflict with native_client/src/include/portability.h. Hence,
 * this file has not been encorporated into that header file.
 */

/* Define an implementation for boolean values. */
#if NACL_WINDOWS
typedef BOOL Bool;
#else
typedef enum {
  FALSE,
  TRUE
} Bool;
#endif

EXTERN_C_END

#endif   /* NATIVE_CLIENT_SRC_SHARED_UTILS_TYPES_H__ */
