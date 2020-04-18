/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <signal.h>
#include <errno.h>

int sigprocmask(int how, const sigset_t *set, sigset_t *oldset) {
  return pthread_sigmask(how, set, oldset);
}
