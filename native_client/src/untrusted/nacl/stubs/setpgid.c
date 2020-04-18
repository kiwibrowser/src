/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Stub routine for `setpgid' for porting support.
 */

#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

int setpgid(pid_t pid, pid_t pgid) {
  errno = ENOSYS;
  return -1;
}
