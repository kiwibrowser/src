/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Stub routine for `inet_ntoa' for porting support.
 *
 * NOTE: We return NULL which is not not technically valid and
 * can cause well behaved programs to fail at runtime if they
 * call this function.
 * TODO(sbc): Include and implementation for this function once
 * we include socket headers in newlib toolchain.
 */

#include <stddef.h>
#include <errno.h>

char *inet_ntoa(void *in) {
  errno = ENXIO;
  return NULL;
}
