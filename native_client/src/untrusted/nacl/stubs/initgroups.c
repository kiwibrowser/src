/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Stub routine for `initgroups' for porting support.
 */

#include <errno.h>
#include <grp.h>

int initgroups(const char *user, gid_t group) {
  errno = ENOSYS;
  return -1;
}
