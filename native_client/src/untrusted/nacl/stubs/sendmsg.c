/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Stub routine for `sendmsg' for porting support.
 */

#include <errno.h>

struct msghdr;

int sendmsg(int sockfd, const struct msghdr *msg, int flags) {
  errno = ENOSYS;
  return -1;
}
