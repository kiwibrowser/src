/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nacl/nacl_dyncode.h>

#include "native_client/src/trusted/service_runtime/nacl_config.h"
#include "native_client/tests/dynamic_code_loading/dynamic_segment.h"


/* Test that we can't write to the dynamic code area. */

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage: write_to_dyncode <alloc_dest_first>\n");
    return 1;
  }
  int alloc_dest_first = atoi(argv[1]);

  void (*func)(void);
  uintptr_t code_ptr = (uintptr_t) DYNAMIC_CODE_SEGMENT_START;

  if (alloc_dest_first) {
    char code_buf[32];
    uint32_t halt_val = NACL_HALT_WORD;
    for (int i = 0; i < sizeof(code_buf); i += NACL_HALT_LEN) {
      memcpy(code_buf + i, &halt_val, NACL_HALT_LEN);
    }
    int rc = nacl_dyncode_create((void *) code_ptr, code_buf, sizeof(code_buf));
    assert(rc == 0);
  }

  fprintf(stdout, "This should fault...\n");
  fflush(stdout);
#if defined(__i386__) || defined(__x86_64__)
  *(uint8_t *) code_ptr = 0xc3; /* RET */
#elif defined(__arm__)
  *(uint32_t *) code_ptr = 0xe12fff1e; /* BX LR */
#else
# error Unknown architecture
#endif

  fprintf(stdout, "We're still running. This is wrong.\n");
  fprintf(stdout, "Now try executing the code we wrote...\n");

  /* Double cast required to stop gcc complaining. */
  func = (void (*)(void)) (uintptr_t) code_ptr;
  func();
  fprintf(stdout, "We managed to run the code. This is bad.\n");
  return 1;
}
