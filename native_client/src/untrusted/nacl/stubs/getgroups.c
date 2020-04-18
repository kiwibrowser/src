/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Stub routine for `getgroups' for porting support.
 */

#include <errno.h>
#include <grp.h>

int getgroups(int size, gid_t list[]) {
  errno = ENOSYS;
  return -1;
}
