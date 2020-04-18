/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <errno.h>

#include <nacl/nacl_dyncode.h>
#include "native_client/tests/dynamic_code_loading/dynamic_segment.h"


/*
 * This test checks that it is being run in the context of dynamic
 * loading being disabled.
 */

int main(void) {
  void *dest = (void *) DYNAMIC_CODE_SEGMENT_START;
  char buf[1];
  int rc = nacl_dyncode_create(dest, buf, 0);
  assert(rc == -1);
  assert(errno == EINVAL);
  return 0;
}
