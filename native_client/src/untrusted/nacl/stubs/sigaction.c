/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Stub routine for `sigaction' for porting support.
 */

#include <errno.h>
#include <signal.h>

int sigaction(int signum, const struct sigaction *act,
              struct sigaction *oldact) {
  errno = ENOSYS;
  return -1;
}
