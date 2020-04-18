/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


/*
 * Stub routine for `getlogin' for porting support.
 */

#include <unistd.h>
#include <errno.h>

char *getlogin(void) {
  errno = ENOSYS;
  return NULL;
}
