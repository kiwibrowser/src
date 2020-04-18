/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Stub routine for `msync' for porting support.
 */

#include <errno.h>

int msync(void *addr, size_t length, int flags) {
  errno = ENOSYS;
  return -1;
}
