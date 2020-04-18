/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Stub routine for `sched_setparam' for porting support.
 */

#include <errno.h>
#include <sched.h>

int sched_setparam(pid_t pid, const struct sched_param *param) {
  errno = ENOSYS;
  return -1;
}
