/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


/*
 * Stub routine for `llseek' for porting support.
 */

#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

int llseek(unsigned int fd, unsigned long offset_high,
           unsigned long offset_low, long long *result,
           unsigned int whence) {
  errno = ENOSYS;
  return -1;
}
