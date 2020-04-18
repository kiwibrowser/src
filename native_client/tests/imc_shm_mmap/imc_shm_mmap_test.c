/*
 * Copyright (c) 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>

#include "native_client/src/public/imc_syscalls.h"
#include "native_client/src/public/imc_types.h"
#include "native_client/tests/inbrowser_test_runner/test_runner.h"

int verbosity = 0;
int fail_count = 0;

void fail(char const *msg) {
  fail_count++;
  printf("FAIL: %s\n", msg);
}

void failed(char const *msg) {
  printf("TEST FAILED: %s\n", msg);
}

void passed(char const *msg) {
  printf("TEST PASSED: %s\n", msg);
}

/*
 * tests return error counts for a quick sum.
 */

int imc_shm_mmap(size_t   region_size,
                 int      prot,
                 int      flags,
                 int      off,
                 int      expect_creation_failure,
                 int      map_view)
{
  int   d;
  void  *addr;
  char  *mem;
  int   i;
  int   diff_count;

  if (verbosity > 0) {
    printf("imc_shm_mmap(0x%x, 0x%x, 0x%x, 0x%x, %d, %d)\n",
           region_size, prot, flags, off,
           expect_creation_failure, map_view);
  }
  if (verbosity > 1) {
    printf("basic imc mem obj creation\n");
  }
  d = imc_mem_obj_create(region_size);
  if (expect_creation_failure) {
    if (-1 != d) {
      fprintf(stderr,
              ("imc_shm_mmap: expected failure to create"
               " IMC shm object, but got %d\n"),
              d);
      (void) close(d);
      return 1;
    }
    return 0;
  }
  if (-1 == d) {
    fprintf(stderr,
            ("imc_shm_mmap: could not create an IMC shm object"
             " of size %d (0x%x)\n"),
            region_size, region_size);
    return 1;
  }
  if (verbosity > 1) {
    printf("basic mmap functionality\n");
  }
  addr = mmap((void *) 0, region_size, prot, flags, d, off);
  if (MAP_FAILED == addr) {
    fprintf(stderr, "imc_shm_mmap: mmap failed, errno %d\n", errno);
    return 1;
  }

  /* Check that the region is zero-initialised. */
  mem = (char *) addr;
  diff_count = 0;
  if (verbosity > 1) {
    printf("initial zero content check\n");
  }
  for (i = 0; i < region_size; ++i) {
    if (0 != mem[i]) {
      if (i >= verbosity) {
        printf("imc_shm_mmap: check_contents, byte %d not zero\n", i);
      }
      ++diff_count;
    }
  }
  if (0 != diff_count) {
    munmap(addr, region_size);
    (void) close(d);
    return diff_count;
  }

  if (map_view && 0 != (prot & PROT_WRITE)) {
    void  *addr2;
    int   i;
    int   diff_count;
    char  *mem1;
    char  *mem2;

    if (verbosity > 1) {
      printf("coherent mapping test\n");
    }

    addr2 = mmap((void *) 0, region_size, prot, flags, d, off);
    if (MAP_FAILED == addr2) {
      fprintf(stderr,
              "imc_shm_mmap: 2nd map view mapping failed, errno %d\n", errno);
      return 1;
    }
    for (diff_count = 0, mem1 = (char *) addr, mem2 = (char *) addr2, i = 0;
         i < region_size; ++i) {
      mem1[i] = i;
      if ((0xff & i) != (0xff & mem2[i])) {
        if (i >= verbosity) {
          printf(("imc_shm_mmap: map_view: write to byte %d not coherent"
                  " wrote 0x%02x, got 0x%02x\n"), i, i, mem2[i]);
        }
        ++diff_count;
      }
    }
    if (0 != diff_count) {
      fprintf(stderr, "imc_shm_mmap: map_view: incoherent mapping!\n");
      return diff_count;
    }
  }

  if (0 != close(d)) {
    fprintf(stderr, "imc_shm_mmap: close on IMC shm desc failed\n");
    return 1;
  }
  return 0;
}

void test_map_private_is_not_supported(void) {
  int fd;
  void *addr;
  int rc;

  printf("Test MAP_PRIVATE on shared memory FD...\n");
  fd = imc_mem_obj_create(0x10000);
  if (fd < 0) {
    fail("imc_mem_obj_create() failed");
    return;
  }
  addr = mmap(NULL, 0x10000, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
  if (addr != MAP_FAILED) {
    fail("mmap() succeeded unexpectedly");
    return;
  }
  rc = close(fd);
  assert(rc == 0);
}

int TestMain(void) {
  fail_count += imc_shm_mmap(65536, PROT_READ|PROT_WRITE, MAP_SHARED,
                             0, 0, 0);
  fail_count += imc_shm_mmap(4096, PROT_READ|PROT_WRITE, MAP_SHARED,
                             0, 1, 0);
  fail_count += imc_shm_mmap(1, PROT_READ|PROT_WRITE, MAP_SHARED,
                             0, 1, 0);
  fail_count += imc_shm_mmap(0x20000, PROT_READ|PROT_WRITE, MAP_SHARED,
                             0, 0, 0);
  fail_count += imc_shm_mmap(0x30000, PROT_READ, MAP_SHARED,
                             0, 0, 0);

  fail_count += imc_shm_mmap(65536, PROT_READ|PROT_WRITE, MAP_SHARED,
                             0, 0, 1);
  fail_count += imc_shm_mmap(0x20000, PROT_READ|PROT_WRITE, MAP_SHARED,
                             0, 0, 1);
  fail_count += imc_shm_mmap(0x30000, PROT_READ, MAP_SHARED,
                             0, 0, 1);
  fail_count += imc_shm_mmap(0x100000, PROT_READ|PROT_WRITE, MAP_SHARED,
                             0, 0, 1);

  /*
   * TODO(mseaborn): This triggers a LOG_FATAL error.  Enable this
   * when it returns a normal error from the syscall.  See
   * http://code.google.com/p/nativeclient/issues/detail?id=724
   */
  /* test_map_private_is_not_supported(); */

  printf("imc_shm_mmap: %d failures\n", fail_count);
  if (0 == fail_count) passed("imc_shm_mmap: all sizes\n");
  else failed("imc_shm_mmap: some test(s) failed\n");

  return fail_count;
}

int main(int ac, char **av) {
  int opt;

  while (EOF != (opt = getopt(ac, av, "v"))) {
    switch (opt) {
      case 'v':
        ++verbosity;
        break;
      default:
        fprintf(stderr, "Usage: sel_ldr -- imc_shm_mmap_test.nexe [-v]\n");
        return 1;
    }
  }

  return RunTests(TestMain);
}
