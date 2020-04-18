/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <unistd.h>

int main(void) {
  /* Check that getpagesize() works and returns a sensible value. */
  int page_size = getpagesize();
  assert(page_size == sysconf(_SC_PAGESIZE));
  /* The page size is 64k under SFI NaCl but may be 4k under Non-SFI NaCl. */
  assert(page_size == 0x10000 || page_size == 0x1000);
  return 0;
}
