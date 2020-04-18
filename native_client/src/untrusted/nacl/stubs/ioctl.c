/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


/*
 * Stub routine for `ioctl' for porting support.
 */

#include <errno.h>

int ioctl(int d, int request, ...) {
  errno = ENOSYS;
  return -1;
}
