/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Stub routine for `setgroups' for porting support.
 */

#include <errno.h>
#include <unistd.h>

int setgroups(size_t size, const gid_t *list) {
  errno = ENOSYS;
  return -1;
}
