/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <signal.h>

int sigblock(int mask) {
  sigset_t newmask = mask;
  sigset_t oldmask;
  if (pthread_sigmask(SIG_BLOCK, &newmask, &oldmask) != 0)
    return 0;
  return oldmask;
}
