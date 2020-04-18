/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <nacl/nacl_dyncode.h>

#include "native_client/src/include/arm_sandbox.h"
#include "native_client/tests/dynamic_code_loading/dynamic_segment.h"


#if defined(__i386__) || defined(__x86_64__)
uint8_t halts = 0xf4; /* HLT */
#elif defined(__arm__)
uint32_t halts = NACL_INSTR_ARM_HALT_FILL;
#else
# error "Unknown arch"
#endif


void load_into_page(uint8_t *dest) {
  uint8_t buf[32];
  int rc;
  uint8_t *ptr;

  /* Touch the page by loading some halt instructions into it. */
  for (ptr = buf; ptr < buf + sizeof(buf); ptr += sizeof(halts)) {
    memcpy(ptr, &halts, sizeof(halts));
  }
  rc = nacl_dyncode_create(dest, buf, sizeof(buf));
  assert(rc == 0);

  /* Check that the whole page is correctly filled with halts. */
  for (ptr = dest; ptr < dest + DYNAMIC_CODE_PAGE_SIZE; ptr += sizeof(halts)) {
    if (memcmp(ptr, &halts, sizeof(halts)) != 0) {
      fprintf(stderr, "Mismatch at %p\n", (void *) ptr);
      exit(1);
    }
  }
}

int main(void) {
  uint8_t *dyncode = (uint8_t *) DYNAMIC_CODE_SEGMENT_START;
  int value;

  /*
   * Sanity check: First check that two code pages can be written and
   * read, before we check that the page inbetween is unreadable.
   */
  load_into_page(dyncode);
  load_into_page(dyncode + DYNAMIC_CODE_PAGE_SIZE * 2);

  printf("Attempting to read from unallocated dyncode page.  "
         "This should fault...\n");
  fprintf(stderr, "** intended_exit_status=untrusted_segfault\n");
  value = dyncode[DYNAMIC_CODE_PAGE_SIZE];
  printf("Failed: Dynamic code page was readable and contained the "
         "byte 0x%x.\n", value);
  return 1;
}
