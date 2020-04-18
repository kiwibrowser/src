/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Stub routine for `getrlimit' for porting support.
 */

#include <errno.h>
#include <sys/resource.h>

struct rlimit;

int getrlimit(int resource, struct rlimit *rlim) {
  errno = ENOSYS;
  return -1;
}
