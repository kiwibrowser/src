/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include "native_client/src/untrusted/irt/irt.h"

const size_t k64Kbytes = 64 * 1024;
const int kAnonymousFiledesc = -1;

/*
 * Check that the old version of the memory interface is
 * a prefix of the new version.
 */
void test_memory_interface_prefix(void) {
  struct nacl_irt_memory_v0_1 m1;
  struct nacl_irt_memory_v0_2 m2;
  struct nacl_irt_memory m3;
  void *addr;
  int rc;

  rc = nacl_interface_query(NACL_IRT_MEMORY_v0_1, &m1, sizeof m1);
  assert(rc == sizeof m1);

  rc = nacl_interface_query(NACL_IRT_MEMORY_v0_2, &m2, sizeof m2);
  assert(rc == sizeof m2);

  rc = nacl_interface_query(NACL_IRT_MEMORY_v0_3, &m3, sizeof m3);
  assert(rc == sizeof m3);

  /* Verify that v0.1 mmap ignores PROT_EXEC  */
  addr = 0;
  rc = m1.mmap(&addr,
               k64Kbytes,
               PROT_READ | PROT_WRITE | PROT_EXEC,
               MAP_PRIVATE | MAP_ANONYMOUS,
               kAnonymousFiledesc,
               0);
  /* Return value is actually new address and not a negative return code.  */
  assert(0xffff0000u > (uint32_t)rc);


  /* Verify that v0.2 mmap does not ignore PROT_EXEC  */
  addr = 0;
  rc = m2.mmap(&addr,
               k64Kbytes,
               PROT_READ | PROT_WRITE | PROT_EXEC,
               MAP_PRIVATE | MAP_ANONYMOUS,
               kAnonymousFiledesc,
               0);
  assert(rc = -EINVAL);

  /* mmap is different, everything else should be the same.  */
  m1.mmap = m2.mmap;
  assert(memcmp(&m1, &m2, sizeof m1) == 0);

  /* v0.3 is the same as v0.2, but with the deprecated sysbrk() removed. */
  assert(m3.mmap == m2.mmap);
  assert(m3.munmap == m2.munmap);
  assert(m3.mprotect == m2.mprotect);
}

int main(void) {
  test_memory_interface_prefix();

  return 0;
}
