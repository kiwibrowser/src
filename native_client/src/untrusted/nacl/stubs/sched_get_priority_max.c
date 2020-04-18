/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Stub routine for `sched_get_priority_max' for porting support.
 */

#include <errno.h>
#include <sched.h>

int sched_get_priority_max(int policy) {
  errno = ENOSYS;
  return -1;
}
