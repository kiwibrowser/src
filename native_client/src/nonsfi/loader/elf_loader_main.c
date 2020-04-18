/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "native_client/src/public/irt_core.h"
#include "native_client/src/public/nonsfi/elf_loader.h"

int main(int argc, char **argv, char **environ) {
  nacl_irt_nonsfi_allow_dev_interfaces();
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <executable> <args...>\n", argv[0]);
    return 1;
  }
  const char *nexe_filename = argv[1];
  int fd = open(nexe_filename, O_RDONLY);
  if (fd < 0) {
    fprintf(stderr, "Failed to open %s: %s\n", nexe_filename, strerror(errno));
    return 1;
  }
  uintptr_t entry = NaClLoadElfFile(fd);
  return nacl_irt_nonsfi_entry(argc - 1, argv + 1, environ,
                               (nacl_entry_func_t) entry, nacl_irt_query_core);
}
