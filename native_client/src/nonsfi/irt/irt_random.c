/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/public/nonsfi/irt_random.h"
#include "native_client/src/untrusted/nacl/nacl_random.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static int g_urandom_fd = -1;

void nonsfi_set_urandom_fd(int fd) {
  g_urandom_fd = fd;
}

/*
 * Note: This library provides the nacl_secure_random_init for testing purposes.
 */
int nacl_secure_random_init(void) {
  return 0;  /* Success */
}

int nacl_secure_random(void *buf, size_t count, size_t *nread) {
  if (g_urandom_fd < 0) {
    /*
     * If the fd for /dev/urandom is not initialized, try to open it.
     * This happens on testing, or in nonsfi_loader. Abort on failure.
     */
    g_urandom_fd = open("/dev/urandom", O_RDONLY);
    if (g_urandom_fd < 0)
      abort();
  }

  int result = read(g_urandom_fd, buf, count);
  if (result < 0)
    return errno;
  *nread = result;
  return 0;
}
