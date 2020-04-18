/*
 * Copyright (c) 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "native_client/src/untrusted/irt/irt.h"
#include "native_client/tests/dynamic_code_loading/dynamic_segment.h"

/*
 * This is a regression test for:
 *      https://code.google.com/p/nativeclient/issues/detail?id=4109
 * The case is that an mmap with PROT_EXEC,MAP_FIXED was done before
 * any use of the allocate_code_data interface.  The bug is that the
 * allocate_code_data implementation fails to notice the previous
 * mmap, and tries to hand out an address range that overlaps what
 * was already used via mmap.
 */

static void try_map(const char *which, int fd, void *address, size_t size) {
  void *mapped = mmap(address, size,
                      PROT_READ|PROT_EXEC, MAP_PRIVATE|MAP_FIXED,
                      fd, 0);
  if (mapped == MAP_FAILED) {
    fprintf(stderr, "%s: mmap at %#08x: %s\n",
            which, (uintptr_t) address, strerror(errno));
    exit(1);
  }
  if (mapped != address) {
    fprintf(stderr, "%s: mmap MAP_FIXED returned %#08x instead of %#08x\n",
            which, (uintptr_t) mapped, (uintptr_t) address);
    exit(1);
  }
}

int main(int argc, char **argv) {
  const size_t pagesize = getpagesize();

  if (argc != 2) {
    fprintf(stderr, "Usage: %s FILENAME\n", argv[0]);
    return 2;
  }

  const char *filename = argv[1];
  int fd = open(filename, O_RDONLY);
  if (fd < 0) {
    fprintf(stderr, "Cannot open '%s': %s\n", filename, strerror(errno));
    return 2;
  }

  /*
   * This should match the IRT's g_dynamic_text_start and be the first
   * address that allocate_code_data wants to give out.
   */
  void *address = (void *) DYNAMIC_CODE_SEGMENT_START;

  /*
   * Do a PROT_EXEC, MAP_FIXED mapping first thing, before any
   * uses of the allocate_code_data interface.
   */
  try_map("first", fd, address, pagesize);

  struct nacl_irt_code_data_alloc alloc;
  int rc = nacl_interface_query(NACL_IRT_CODE_DATA_ALLOC_v0_1,
                                &alloc, sizeof alloc);
  assert(rc == sizeof alloc);

  uintptr_t alloc_addr;
  rc = alloc.allocate_code_data(0, pagesize, 0, 0, &alloc_addr);
  assert(rc == 0);

  if (alloc_addr == (uintptr_t) address) {
    fprintf(stderr,
            "allocate_code_data returned address already used by mmap!\n");
    return 1;
  }

  try_map("second", fd, (void *) alloc_addr, pagesize);

  return 0;
}
