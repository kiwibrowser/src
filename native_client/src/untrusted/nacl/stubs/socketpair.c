/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Stub routine for `socketpair' for porting support.
 */

#include <errno.h>

int socketpair(int domain, int type, int protocol, int sv[2]) {
  errno = ENOSYS;
  return -1;
}
