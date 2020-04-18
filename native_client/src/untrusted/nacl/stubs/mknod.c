/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Stub routine for `mknod' for porting support.
 */

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

int mknod(const char *pathname, mode_t mode, dev_t dev) {
  errno = ENOSYS;
  return -1;
}
