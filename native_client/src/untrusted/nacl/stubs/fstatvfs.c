/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Stub routine for `fstatvfs' for porting support.
 */

#include <errno.h>

struct statvfs;

int fstatvfs(int fd, struct statvfs *buf) {
  errno = ENOSYS;
  return -1;
}
