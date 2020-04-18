/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


/*
 * Stub routine for `waitpid' for porting support.
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

pid_t waitpid(pid_t pid, int *status, int options) {
  errno = ENOSYS;
  return -1;
}
