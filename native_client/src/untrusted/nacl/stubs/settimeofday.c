/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


/*
 * Stub routine for `settimeofday' for porting support.
 */

#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

int settimeofday(const struct timeval *tv, const struct timezone *tz) {
  errno = ENOSYS;
  return -1;
}
