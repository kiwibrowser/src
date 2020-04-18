/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


/*
 * Stub routine for `getuid' for porting support.
 */

#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

uid_t getuid(void) {
  errno = ENOSYS;
  return -1;
}
