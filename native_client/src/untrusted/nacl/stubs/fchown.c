/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


/*
 * Stub routine for `fchown' for porting support.
 */

#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

int fchown(int fd, uid_t owner, gid_t group) {
  errno = ENOSYS;
  return -1;
}
