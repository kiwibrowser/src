/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Stub routine for `statvfs' for porting support.
 */

#include <errno.h>

struct statvfs;

int statvfs(const char *path, struct statvfs *buf) {
  errno = ENOSYS;
  return -1;
}
