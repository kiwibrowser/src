/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <assert.h>

#include "native_client/src/include/nacl_assert.h"
#include "native_client/src/untrusted/irt/irt.h"

/*
 * This test checks that memory allocated via sysbrk() is zeroed, even
 * if it was previously allocated and deallocated.
 * See http://code.google.com/p/nativeclient/issues/detail?id=2417
 *
 * sysbrk() is deprecated, so we test the IRT interface directly
 * rather than testing any libc wrappers for it.
 */

static struct nacl_irt_memory_v0_2 irt_memory;

static void *get_break(void) {
  void *addr = NULL;
  int rc = irt_memory.sysbrk(&addr);
  ASSERT_EQ(rc, 0);
  ASSERT_NE(addr, NULL);
  return addr;
}

static void set_break(void *new_addr) {
  void *addr_copy = new_addr;
  int rc = irt_memory.sysbrk(&addr_copy);
  ASSERT_EQ(rc, 0);
  /* Check that sysbrk() does not modify the value in the success case. */
  ASSERT_EQ(addr_copy, new_addr);
}

#define NUM_WORDS 512

int main(void) {
  size_t    ix;
  int       status;

  size_t query_result = nacl_interface_query(NACL_IRT_MEMORY_v0_2,
                                             &irt_memory, sizeof(irt_memory));
  ASSERT_EQ(query_result, sizeof(irt_memory));

  /* Find the current break pointer. */
  int *alloc_start = get_break();
  fprintf(stderr, "initial break is at %p\n", (void *) alloc_start);

  /* We expect that the initial break pointer is word-aligned. */
  ASSERT_EQ((uintptr_t) alloc_start & 3, 0);

  /* Allocate some memory and fill it with data. */
  void *alloc_end = alloc_start + NUM_WORDS;
  set_break(alloc_end);
  for (ix = 0; ix < NUM_WORDS; ++ix) {
    alloc_start[ix] = 0xdeadbeef;
  }
  /* Deallocate the memory. */
  set_break(alloc_start);
  /* Allocate the memory again.  The contents should have been zeroed. */
  set_break(alloc_end);
  status = 0;
  for (ix = 0; ix < NUM_WORDS; ++ix) {
    if (0 != alloc_start[ix]) {
      fprintf(stderr, "new memory word at %zd contains 0x%04x\n",
              ix, alloc_start[ix]);
      status = 3;
    }
  }
  return status;
}
