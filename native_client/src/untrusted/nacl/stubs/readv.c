/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Stub routine for `readv' for porting support.
 */

#include <errno.h>
#include <sys/types.h>

struct iovec;

ssize_t readv(int fd, const struct iovec *iov, int iovcnt) {
  errno = ENOSYS;
  return -1;
}
