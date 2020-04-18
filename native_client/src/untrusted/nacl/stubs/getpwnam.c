/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


/*
 * Stub routine for `getpwnam' for porting support.
 */

#include <sys/types.h>
#include <pwd.h>
#include <errno.h>

struct passwd *getpwnam(const char *name) {
  errno = ENOSYS;
  return NULL;
}
