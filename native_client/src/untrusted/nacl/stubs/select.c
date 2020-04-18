/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


/*
 * Stub routine for `select' for porting support.
 */

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

int select(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
           struct timeval *timeout) {
  /* NOTE(sehr): NOT IMPLEMENTED: select */
  errno = ENOSYS;
  return -1;
}
