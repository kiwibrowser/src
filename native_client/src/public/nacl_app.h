/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_PUBLIC_NACL_APP_H_
#define NATIVE_CLIENT_SRC_PUBLIC_NACL_APP_H_

#include "native_client/src/include/nacl_base.h"

EXTERN_C_BEGIN

struct NaClApp;
struct NaClDesc;

/* Creates and returns a newly-initialized NaClApp. */
struct NaClApp *NaClAppCreate(void);

/*
 * Sets entry number |fd| in the NaClApp's descriptor table to |desc|,
 * removing any existing entry under |fd|.  Note that this takes
 * ownership of |desc|.
 */
void NaClAppSetDesc(struct NaClApp *nap, int fd, struct NaClDesc *desc);

EXTERN_C_END

#endif
