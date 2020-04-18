/*
 * Copyright 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

#include "native_client/src/include/nacl_assert.h"
#include "native_client/src/public/linux_syscalls/sys/syscall.h"

void test_getpid() {
  puts("test_getpid");
  int syscall_result = syscall(__NR_getpid);
  int pid = getpid();

  ASSERT_EQ(syscall_result, pid);
}

void test_mmap() {
  puts("test_mmap");

  const int kAllocateSize = 4096;

  int mmap_result = syscall(__NR_mmap2,
                            /* addr */ 0,
                            /* length */ kAllocateSize,
                            /* prot=PROT_READ */ 1,
                            /* flags=MAP_PRIVATE | MAP_ANON */ 0x22,
                            /* fd */ -1,
                            /* pgoffset */ 0);
  ASSERT_NE(mmap_result, -1);
  munmap((void *) mmap_result, kAllocateSize);
}

int main(int argc, char *argv[]) {
  test_getpid();
  test_mmap();
  puts("PASSED");
  return 0;
}
