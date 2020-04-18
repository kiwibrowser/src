/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Stub routine for `inet_ntop' for porting support.
 */

#include <errno.h>

const char *inet_ntop(int af, const void *src,
                      char *dst, unsigned int size) {
  errno = ENOSYS;
  return NULL;
}
