/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Stub routine for `getgrgid_r' for porting support.
 */

#include <errno.h>
#include <grp.h>

int getgrgid_r(gid_t gid, struct group *grp,
               char *buf, size_t buflen, struct group **result) {
  errno = ENOSYS;
  return -1;
}
