/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * dup and dup2 functions.
 */

#include <errno.h>
#include <unistd.h>

#include "native_client/src/untrusted/nacl/nacl_irt.h"

int dup(int fd) {
  int newfd;
  int error = __libnacl_irt_fdio.dup(fd, &newfd);
  if (error) {
    errno = error;
    return -1;
  }
  return newfd;
}

int dup2(int oldfd, int newfd) {
  int error = __libnacl_irt_fdio.dup2(oldfd, newfd);
  if (error) {
    errno = error;
    return -1;
  }
  return newfd;
}
