/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Stub routine for `getsockopt' for porting support.
 */

#include <errno.h>

int getsockopt(int sockfd, int level, int optname, void *optval,
               unsigned int *optlen) {
  errno = ENOSYS;
  return -1;
}
