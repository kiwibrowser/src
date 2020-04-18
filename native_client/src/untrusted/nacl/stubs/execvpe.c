/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Stub routine for `execvpe' for porting support.
 */

#include <errno.h>
#include <unistd.h>

int execvpe(const char *file, char *const argv[], char *const envp[]) {
  errno = ENOSYS;
  return -1;
}
