/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


/*
 * Stub routine for `getpwuid' for porting support.
 */

#include <sys/types.h>
#include <pwd.h>
#include <errno.h>

struct passwd *getpwuid(uid_t uid) {
  errno = ENOSYS;
  return NULL;
}
