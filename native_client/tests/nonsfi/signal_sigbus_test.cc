/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <fcntl.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "native_client/src/include/nacl_assert.h"
#include "native_client/src/nonsfi/linux/linux_syscall_defines.h"

static jmp_buf g_jmp_buf;

static void return_from_signal_handler(int signo) {
  /*
   * TODO(uekawa): We are sending down host value (x86 or ARM linux),
   * this needs to be converted. newlib expects SIGBUS==10.
   */
  ASSERT_EQ(signo, LINUX_SIGBUS);
  longjmp(g_jmp_buf, 42);
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Filename for small test file not specified.\n");
    _exit(1);
  }

  sighandler_t prev_handler = signal(SIGBUS, SIG_IGN);
  ASSERT_EQ(prev_handler, SIG_DFL);
  const char *small_test_file = argv[1];
  int fd = open(small_test_file, O_RDONLY);
  ASSERT_NE(-1, fd);
  void *smallfile = mmap(NULL, sysconf(_SC_PAGESIZE) * 10, PROT_READ,
                         MAP_SHARED, fd, 0);
  ASSERT_NE(MAP_FAILED, smallfile);

  prev_handler = signal(SIGBUS, return_from_signal_handler);
  ASSERT_EQ(prev_handler, SIG_IGN);

  int rc = setjmp(g_jmp_buf);
  if (rc == 0) {
    /* Try reading after the page to cause SIGBUS. */
    int value = *((volatile char *) smallfile + sysconf(_SC_PAGESIZE));

    /* Shouldn't reach here. */
    ASSERT_NE(value, 0);
    _exit(1);
  }

  ASSERT_EQ(rc, 42);
  puts("PASSED");
  return 0;
}
