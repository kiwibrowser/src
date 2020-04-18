/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Stub routine for `signal' for porting support.
 */

#include <errno.h>
#include <signal.h>

_sig_func_ptr signal(int sig, _sig_func_ptr handler) {
  errno = ENOSYS;
  return SIG_ERR;
}
