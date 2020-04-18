/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_PUBLIC_NONSFI_IRT_RANDOM_H_
#define NATIVE_CLIENT_SRC_PUBLIC_NONSFI_IRT_RANDOM_H_ 1

#include "native_client/src/include/nacl_base.h"

EXTERN_C_BEGIN

/*
 * Initializes the urandom fd with the given |fd|. This function takes
 * ownership of the given |fd|.
 */
void nonsfi_set_urandom_fd(int fd);

EXTERN_C_END

#endif
