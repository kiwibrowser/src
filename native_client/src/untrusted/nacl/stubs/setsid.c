/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Stub routine for `setsid' for porting support.
 */

#include <errno.h>
#include <unistd.h>

pid_t setsid() {
  errno = ENOSYS;
  return (pid_t) -1;
}
