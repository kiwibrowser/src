/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <errno.h>
#include <unistd.h>

#define TEST(func)                              \
  do {                                          \
    int id;                                     \
    errno = 0;                                  \
    id = func();                                \
    assert(id == -1);                           \
    assert(errno == ENOSYS);                    \
  } while (0)

int main(void) {

  TEST(getgid);
  TEST(getegid);
  TEST(getuid);
  TEST(geteuid);

  return 0;
}
