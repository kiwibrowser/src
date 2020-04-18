/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


/*
 * Stub routine for `ttyname_r' for porting support.
 */

#include <unistd.h>
#include <errno.h>

int ttyname_r(int fd, char *buf, size_t buflen) {
  return ENOSYS;
}
