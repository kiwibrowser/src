/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


/*
 * Stub routine for `umask' for porting support.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

mode_t umask(mode_t mask) {
  errno = ENOSYS;
  return 0777;
}
