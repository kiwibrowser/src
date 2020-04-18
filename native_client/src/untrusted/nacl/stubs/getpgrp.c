/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Stub routine for `getpgrp' for porting support.
 */

#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

pid_t getpgrp(void) {
  errno = ENOSYS;
  return -1;
}
