/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Stub routine for `setsockopt' for porting support.
 */

#include <errno.h>

int setsockopt(int sockfd, int level, int optname,
               const void *optval, unsigned int optlen) {
  errno = ENOSYS;
  return -1;
}
