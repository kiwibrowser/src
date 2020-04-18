/*
 * Copyright 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Stub routine for `tcsetattr' for porting support.
 */

#include <errno.h>

struct termios;

int tcsetattr(int fd, int optional_actions, const struct termios *termios_p) {
  errno = ENOSYS;
  return -1;
}
