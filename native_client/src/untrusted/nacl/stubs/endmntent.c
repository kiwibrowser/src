/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Stub routine for `endmntent' for porting support.
 */

#include <errno.h>
#include <stdio.h>

int endmntent(FILE *fp) {
  errno = ENOSYS;
  return -1;
}
