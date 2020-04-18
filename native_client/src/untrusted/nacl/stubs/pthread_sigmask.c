/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Stub routine for `pthread_sigmask' for porting support.
 */

#include <errno.h>
#include <signal.h>

int pthread_sigmask(int how, const sigset_t *set, sigset_t *oldset) {
  return ENOSYS;
}
