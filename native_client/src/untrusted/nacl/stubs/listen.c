/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Stub routine for `listen' for porting support.
 */

#include <errno.h>

int listen(int sockfd, int backlog) {
  errno = ENOSYS;
  return -1;
}
