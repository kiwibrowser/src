/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


/*
 * Stub routine for `getrusage' for porting support.
 */

#include <sys/time.h>
#include <sys/resource.h>
#include <errno.h>

int getrusage(int who, struct rusage *usage) {
  errno = ENOSYS;
  return -1;
}
