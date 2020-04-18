/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <errno.h>
#include <unistd.h>

#include "native_client/src/include/nacl_assert.h"

int main(void) {
  int pid = getpid();
  ASSERT_EQ(pid, -1);
  ASSERT_EQ(errno, ENOSYS);
  return 0;
}
