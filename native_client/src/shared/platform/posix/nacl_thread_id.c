/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <pthread.h>

#include "native_client/src/shared/platform/nacl_threads.h"

/*
 * This file is separate from nacl_threads.c mostly for the benefit
 * of the untrusted build, so we didn't bother separating it out in
 * the Windows implementation.  There NaClThreadId is in nacl_threads.c.
 */

uint32_t NaClThreadId(void) {
  return (uintptr_t) pthread_self();
}
