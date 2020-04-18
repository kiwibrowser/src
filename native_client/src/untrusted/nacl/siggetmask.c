/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <signal.h>

int siggetmask(void) {
  sigset_t oldmask;
  if (pthread_sigmask(SIG_SETMASK, NULL, &oldmask) != 0)
    return 0;
  return oldmask;
}
