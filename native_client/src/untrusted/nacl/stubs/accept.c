/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Stub routine for `accept' for porting support.
 */

#include <errno.h>

struct sockaddr;

int accept(int sockfd, struct sockaddr *addr, void *addrlen) {
  errno = ENOSYS;
  return -1;
}
