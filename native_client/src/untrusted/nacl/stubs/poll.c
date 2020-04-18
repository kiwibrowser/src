/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Stub routine for `poll' for porting support.
 */

#include <errno.h>

struct pollfd;

int poll(struct pollfd *fds, int nfds, int timeout) {
  errno = ENOSYS;
  return -1;
}
