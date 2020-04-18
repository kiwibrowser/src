/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


/*
 * Stub routine for `setegid' for porting support.
 */

#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

int setegid(uid_t euid) {
  errno = ENOSYS;
  return -1;
}
