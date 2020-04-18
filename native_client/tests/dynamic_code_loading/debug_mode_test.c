/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <errno.h>
#include <string.h>

#include <nacl/nacl_dyncode.h>

#include "native_client/tests/dynamic_code_loading/dynamic_segment.h"
#include "native_client/tests/dynamic_code_loading/templates.h"


/* This test checks that the validator check is disabled in debug mode. */

int main(void) {
  void *dest = (void *) DYNAMIC_CODE_SEGMENT_START;
  char buf[32];
  int rc;

  /*
   * This data won't pass the validators, but in debug mode it can be
   * loaded anyway.
   */
  memset(buf, 0, sizeof(buf));
  assert(sizeof(buf) >= &invalid_code_end - &invalid_code);
  memcpy(buf, &invalid_code, &invalid_code_end - &invalid_code);

  rc = nacl_dyncode_create(dest, buf, sizeof(buf));
  assert(rc == 0);

  return 0;
}
