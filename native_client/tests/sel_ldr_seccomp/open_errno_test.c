/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>

#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/trusted/seccomp_bpf/seccomp_bpf.h"

int main(void) {
  int fd;

  CHECK(0 == NaClInstallBpfFilter());

  fd = open("non_existent_file", O_RDONLY);
  CHECK(fd < 0);
  CHECK(errno == EACCES);

  return 0;
}
