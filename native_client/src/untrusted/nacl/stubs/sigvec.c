/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Stub routine for `sigvec' for porting support.
 */

#include <errno.h>

struct sigvec;

int sigvec(int sig, struct sigvec *vec, struct sigvec *ovec) {
  errno = ENOSYS;
  return -1;
}
