/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


/*
 * Stub routine for `times' for porting support.
 */

#include <sys/times.h>
#include <errno.h>

clock_t times(struct tms *buf) {
  errno = ENOSYS;
  return -1;
}
