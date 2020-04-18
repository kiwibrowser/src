/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Stub routine for `recvfrom' for porting support.
 */

#include <errno.h>

struct sockaddr;

int recvfrom(int sockfd, void *buf, size_t len, int flags,
             struct sockaddr *src_addr, unsigned int *addrlen) {
  errno = ENOSYS;
  return -1;
}
