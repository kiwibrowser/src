/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <setjmp.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/shared/platform/nacl_check.h"

#include "native_client/src/include/nacl_compiler_annotations.h"

#include "native_client/src/include/nacl/nacl_exception.h"

#include "native_client/src/trusted/service_runtime/nacl_config.h"

#define NUM_FILE_BYTES 3 * NACL_MAP_PAGESIZE

static jmp_buf g_jmp_buf;

static void test_handler(struct NaClExceptionContext *context) {
  /* We got an exception as expected. Return from the handler. */
  int rc = nacl_exception_clear_flag();
  CHECK(rc == 0);
  longjmp(g_jmp_buf, 1);
}

static void check_addr_is_unreadable(volatile char *addr) {
  int rc;

  if (getenv("RUNNING_ON_VALGRIND") != NULL) {
    fprintf(stderr, "Skipping assert_addr_is_unreadable() under Valgrind\n");
    return;
  }

  rc = nacl_exception_set_handler(test_handler);
  CHECK(rc == 0);
  if (!setjmp(g_jmp_buf)) {
    char value = *addr;
    /* If we reach here, the assertion failed. */
    fprintf(stderr, "Address %p was readable, and contained %i\n",
            (void *) addr, value);
    exit(1);
  }
  /*
   * Clean up: Unregister the exception handler so that we do not
   * accidentally return through g_jmp_buf if an exception occurs.
   */
  rc = nacl_exception_set_handler(NULL);
  CHECK(rc == 0);
}

static void check_addr_is_unwritable(volatile char *addr, char value) {
  int rc;

  if (getenv("RUNNING_ON_VALGRIND") != NULL) {
    fprintf(stderr, "Skipping assert_addr_is_unwritable() under Valgrind\n");
    return;
  }
  if (getenv("RUNNING_ON_ASAN") != NULL) {
    fprintf(stderr, "Skipping assert_addr_is_unwritable() under ASan\n");
    return;
  }

  rc = nacl_exception_set_handler(test_handler);
  CHECK(rc == 0);
  if (!setjmp(g_jmp_buf)) {
    *addr = value;
    /* If we reach here, the assertion failed. */
    fprintf(stderr, "Address %p was writable, %i was written\n",
            (void *) addr, value);
    exit(1);
  }
  /*
   * Clean up: Unregister the exception handler so that we do not
   * accidentally return through g_jmp_buf if an exception occurs.
   */
  rc = nacl_exception_set_handler(NULL);
  CHECK(rc == 0);
}

static unsigned char const g_test_data[] =
    "Test file for mmapping, less than a page in size.";

struct prot_basic_specs {
  int map_prot;
  int map_flags;
  int expected_errno; /* if expected mmap to fail */
  int target_prot;
};

int test_prot_basic(int fd, size_t map_size, void *test_spec) {
  struct prot_basic_specs *spec = (struct prot_basic_specs *) test_spec;
  void *addr;

  addr = mmap(NULL, map_size, spec->map_prot, spec->map_flags, fd, 0);
  if (MAP_FAILED == addr) {
    fprintf(stderr, "test_prot_basic: mmap failed, errno %d\n", errno);
    return 1;
  }
  if (0 != spec->expected_errno) {
    if (MAP_FAILED != addr) {
      fprintf(stderr,
              "test_prot_basic: expected mmap to fail but did not.\n");
      return 1;
    }
    if (spec->expected_errno != errno) {
      fprintf(stderr,
              "test_prot_basic: expected mmap to fail with errno %d,"
              " got %d instead.\n", spec->expected_errno, errno);
      return 1;
    }
  } else {
    if (MAP_FAILED == addr) {
      fprintf(stderr, "test_prot_basic: mmap failed, errno %d\n", errno);
      return 1;
    }
  }
  printf("test_prot_basic: mmap done\n");
  /* Change the protection to make the page accessible. */
  CHECK(0 == mprotect(addr, map_size, spec->target_prot));
  printf("test_prot_basic: mprotect good\n");
  /* Check that memory contains the expected content. */
  CHECK(0 == memcmp(addr, g_test_data, sizeof g_test_data));
  /* Unmap the memory. */
  CHECK(0 == munmap(addr, map_size));
  printf("test_prot_basic: munmap done\n");
  return 0;
}

struct prot_access_specs {
  int map_prot;
  int map_flags;
  int map_offset;
};

int test_prot_access(int fd, size_t map_size, void *test_spec) {
  struct prot_access_specs *spec = (struct prot_access_specs *) test_spec;
  char *addr;

  addr = (char *) mmap(NULL,
                       map_size,
                       spec->map_prot,
                       spec->map_flags,
                       fd,
                       spec->map_offset);
  if (MAP_FAILED == addr) {
    fprintf(stderr, "test_prot_access: mmap failed, errno %d\n", errno);
    return 1;
  }
  printf("test_prot_access: mmap done\n");
  /* Change the protection to make the page unreadable. */
  CHECK(0 == mprotect(addr, map_size, PROT_NONE));
  check_addr_is_unreadable(addr);
  check_addr_is_unreadable(addr + 0x1000);
  check_addr_is_unreadable(addr + 0x10000);
  check_addr_is_unreadable(addr + 0x20000);
  /* Change the protection to make the page accessible again. */
  CHECK(0 == mprotect(addr, map_size, PROT_READ | PROT_WRITE));
  addr[0] = '5';
  /* Change the protection to make the page read-only. */
  CHECK(0 == mprotect(addr, map_size, PROT_READ));
  check_addr_is_unwritable(addr, '9');
  CHECK('5' == addr[0]);
  printf("test_prot_access: mprotect good\n");
  /* We can still munmap() the memory. */
  CHECK(0 == munmap(addr, map_size));
  printf("test_prot_access: munmap done\n");
  return 0;
}

struct prot_unmap_specs {
  int map_prot;
  int map_flags;
  off_t unmap_offset;
};

int test_prot_unmap(int fd, size_t map_size, void *test_spec) {
  struct prot_unmap_specs *spec = (struct prot_unmap_specs *) test_spec;
  char *addr;

  addr = (char *) mmap(NULL, map_size, spec->map_prot, spec->map_flags, fd, 0);
  if (MAP_FAILED == addr) {
    fprintf(stderr, "test_prot_unmap: mmap failed, errno %d\n", errno);
    return 1;
  }
  printf("test_prot_unmap: mmap done\n");
  /* Change the protection to make the page unreadable. */
  CHECK(0 == mprotect(addr, spec->unmap_offset, PROT_NONE));
  printf("test_prot_unmap: mprotect done\n");
  /* Unmap the memory. */
  CHECK(0 == munmap(addr, spec->unmap_offset));
  CHECK(0 == munmap(addr + spec->unmap_offset, map_size - spec->unmap_offset));
  printf("test_prot_unmap: munmap done\n");
  return 0;
}

struct prot_unmapped_specs {
  int map_prot;
  int map_flags;
  off_t unmap_offset;
};

int test_prot_unmapped(int fd, size_t map_size, void *test_spec) {
  struct prot_unmapped_specs *spec = (struct prot_unmapped_specs *) test_spec;
  char *addr;

  addr = (char *) mmap(NULL, map_size, spec->map_prot, spec->map_flags, fd, 0);
  if (MAP_FAILED == addr) {
    fprintf(stderr, "test_prot_unmapped: mmap failed, errno %d\n", errno);
    return 1;
  }
  printf("test_prot_unmapped: mmap done\n");
  /* Now unmap the region so that we know the memory is unmapped. */
  CHECK(0 == munmap(addr + spec->unmap_offset, map_size - spec->unmap_offset));
  printf("test_prot_unmapped: munmap done\n");
  /* Try to change the region protection which should fail. */
  CHECK(0 != mprotect(addr, map_size, PROT_READ | PROT_WRITE));
  CHECK(EACCES == errno);
  printf("test_prot_unmapped: mprotect good\n");
  return 0;
}

struct prot_none_specs {
  int map_prot;
  int map_flags;
};

/*
 * Verify that mprotect() changes the virtual address protection.
 * Tests for http://code.google.com/p/nativeclient/issues/detail?id=3354
 */
int test_prot_none(int fd, size_t map_size, void *test_spec) {
  static unsigned char const expected_data[] =
    "Changed test data, less than a page in size.";
  struct prot_unmap_specs *spec = (struct prot_unmap_specs *) test_spec;
  char *addr;

  addr = mmap(NULL, map_size, spec->map_prot, spec->map_flags, fd, 0);
  if (MAP_FAILED == addr) {
    fprintf(stderr, "test_prot_none: mmap failed, errno %d\n", errno);
    return 1;
  }
  printf("test_prot_none: mmap done\n");
  /* Change the memory content. */
  memcpy(addr, expected_data, sizeof expected_data);
  /* Check that memory contains the modified content. */
  CHECK(0 == memcmp(addr, expected_data, sizeof expected_data));
  /* Check that the rest of the region is inaccessible. */
  check_addr_is_unreadable(addr + 0x1000);
  check_addr_is_unreadable(addr + 0x10000);
  printf("test_prot_none: memcpy good\n");
  /* Change the protection to make the page unreadable. */
  CHECK(0 == mprotect(addr, map_size, PROT_NONE));
  check_addr_is_unreadable(addr);
  check_addr_is_unreadable(addr + 0x1000);
  check_addr_is_unreadable(addr + 0x10000);
  /* Change the protection to make the page accessible again. */
  CHECK(0 == mprotect(addr, map_size, PROT_READ | PROT_WRITE));
  /* Check that the modified content is still there. */
  CHECK(0 == memcmp(addr, expected_data, sizeof expected_data));
  /* Check that the rest of the region is still inaccessible. */
  check_addr_is_unreadable(addr + 0x1000);
  check_addr_is_unreadable(addr + 0x10000);
  printf("test_prot_none: mprotect good\n");
  /* Unmap the memory. */
  CHECK(0 == munmap(addr, map_size));
  printf("test_prot_none: munmap done\n");
  return 0;
}

struct prot_mapping_specs {
  size_t map_offset;
};

int test_prot_mapping(int fd, size_t map_size, void *test_spec) {
  struct prot_mapping_specs *spec = (struct prot_mapping_specs *) test_spec;
  size_t region_size = map_size + spec->map_offset;
  char *alloc;
  char *view1;
  char *view2;

  /* Allocate memory to make sure we have enough contiguous space. */
  alloc = (char *) mmap(NULL, region_size, PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  CHECK(alloc != MAP_FAILED);
  /* Now create both mappings adjacent to each other. */
  view1 = (char *) mmap(alloc, map_size, PROT_READ | PROT_WRITE,
                        MAP_SHARED | MAP_FIXED, fd, /* offset*/ 0);
  CHECK(view1 == alloc);
  view2 = (char *) mmap(alloc + spec->map_offset, map_size, PROT_READ,
                        MAP_PRIVATE | MAP_FIXED, fd, /* offset */ 0);
  CHECK(view2 == alloc + spec->map_offset);
  printf("test_prot_view: mmap done\n");
  view1[0] = '5';
  CHECK('5' == view1[0]);
  check_addr_is_unwritable(view2, '9');
  /* Make both regions inaccessible. */
  CHECK(0 == mprotect(view2, map_size, PROT_READ | PROT_WRITE));
  view2[0] = '7';
  CHECK('5' == view1[0]);
  CHECK('7' == view2[0]);
  printf("test_prot_view: mprotect good\n");
  /* Check that we can still unmap both regions. */
  CHECK(0 == munmap(alloc, region_size));
  printf("test_prot_view: munmap done\n");
  return 0;
}

int test_prot_ronly_rw(int fd, size_t map_size, void *test_spec) {
  char *addr;
  UNREFERENCED_PARAMETER(test_spec);

  addr = (char *) mmap(NULL, map_size, PROT_READ, MAP_SHARED, fd, 0);
  CHECK(addr != MAP_FAILED);
  printf("test_prot_ronly_rw: mmap done\n");
  /* Try to make the region read/write which should fail. */
  CHECK(0 != mprotect(addr, map_size, PROT_READ | PROT_WRITE));
  CHECK(EACCES == errno);
  printf("test_prot_ronly_rw: mprotect good\n");
  /* Check that we can still unmap both regions. */
  CHECK(0 == munmap(addr, map_size));
  printf("test_prot_ronly_rw: munmap done\n");
  return 0;
}

struct test_params {
  char const *test_name;
  int (*test_func)(int fd, size_t map_size, void *test_specifics);
  int open_flags;
  int file_mode;
  size_t file_size;
  size_t map_size;
  unsigned char const *test_data_start;
  size_t test_data_size;
  void *test_specifics;
};

#define __ALIGN_MASK(x, mask) (((x) + (mask)) & ~(mask))
#define ALIGN(x, n) __ALIGN_MASK(x, (__typeof__(x))(n) - 1)
#define IS_ALIGNED(x, n) (((x) & ((__typeof__(x))(n) - 1)) == 0)

int test_data_create(int fd, size_t num_bytes) {
  static int buffer[1024];
  size_t nbytes;
  size_t written = 0;
  ssize_t result;

  CHECK(IS_ALIGNED(num_bytes, 4));

  memset(buffer, 0, sizeof buffer);
  while (written < num_bytes) {
    nbytes = num_bytes - written;
    if (nbytes > sizeof buffer) {
      nbytes = sizeof buffer;
    }
    result = write(fd, buffer, nbytes);
    if (result != nbytes) {
      return -1;
    }
    written += nbytes;
  }
  return 0;
}

int test_file_create(char const *pathname, struct test_params *params) {
  int fd;
  struct stat stbuf;
  off_t off;
  size_t nbytes;
  ssize_t result;

  if (-1 == (fd = open(pathname,
                       O_WRONLY | O_CREAT | O_TRUNC,
                       params->file_mode))) {
    fprintf(stderr,
            "mmap_prot_test: could not open test file, errno %d\n",
            errno);
    exit(1);
  }
  if (0 != test_data_create(fd, params->file_size)) {
    fprintf(stderr,
            "mmap_prot_test: could not write test data, errno %d\n",
            errno);
    exit(1);
  }
  if (-1 == fstat(fd, &stbuf)) {
    fprintf(stderr, "mmap_prot_test: fstat failed, errno %d\n", errno);
    exit(1);
  }
  if (stbuf.st_size != params->file_size) {
    fprintf(stderr,
            "mmap_prot_test: file size incorrect:"
            " should be %d, actual %d\n",
            (int) params->file_size, (int) stbuf.st_size);
    exit(1);
  }
  if (NULL != params->test_data_start) {
    off = lseek(fd, 0LL, 0);
    if (off < 0) {
      fprintf(stderr,
              "mmap_prot_test: could not seek, errno %d\n",
              errno);
      exit(1);
    }
    nbytes = params->test_data_size;
    result = write(fd, params->test_data_start, nbytes);
    if (result < 0) {
      fprintf(stderr,
              "mmap_prot_test: could not write test data, errno %d\n",
              errno);
      exit(1);
    }
    if ((size_t) result != nbytes) {
      fprintf(stderr,
              "mmap_prot_test: error while writing test data, errno %d\n",
              errno);
      exit(1);
    }
  }
  CHECK(0 == close(fd));
  if (-1 == (fd = open(pathname, params->open_flags, 0777))) {
    fprintf(stderr,
            "mmap_prot_test: failed to reopen test file, errno %d\n",
            errno);
    exit(1);
  }
  return fd;
}

void test_file_close(int d) {
  CHECK(0 == close(d));
}

struct prot_basic_specs prot_basic_none_private = {
  .map_prot = PROT_NONE,
  .map_flags = MAP_PRIVATE,
  .expected_errno = 0,
  .target_prot = PROT_READ,
};

struct prot_basic_specs prot_basic_none_shared = {
  .map_prot = PROT_NONE,
  .map_flags = MAP_SHARED,
  .expected_errno = 0,
  .target_prot = PROT_READ,
};

struct prot_access_specs prot_access_private = {
  .map_prot = PROT_READ | PROT_WRITE,
  .map_flags = MAP_PRIVATE,
  .map_offset = 0x0,
};

struct prot_access_specs prot_access_shared = {
  .map_prot = PROT_READ | PROT_WRITE,
  .map_flags = MAP_SHARED,
  .map_offset = 0x0,
};

struct prot_access_specs prot_access_offset = {
  .map_prot = PROT_READ | PROT_WRITE,
  .map_flags = MAP_PRIVATE,
  .map_offset = NACL_MAP_PAGESIZE,
};

struct prot_unmap_specs prot_unmap_private = {
  .map_prot = PROT_READ | PROT_WRITE,
  .map_flags = MAP_PRIVATE,
  .unmap_offset = NACL_MAP_PAGESIZE,
};

struct prot_unmapped_specs prot_unmapped_full = {
  .map_prot = PROT_READ | PROT_WRITE,
  .map_flags = MAP_PRIVATE,
  .unmap_offset = 0,
};

struct prot_unmapped_specs prot_unmapped_partial = {
  .map_prot = PROT_READ | PROT_WRITE,
  .map_flags = MAP_PRIVATE,
  .unmap_offset = NACL_MAP_PAGESIZE,
};

struct prot_none_specs prot_none_private = {
  .map_prot = PROT_READ | PROT_WRITE,
  .map_flags = MAP_PRIVATE,
};

struct prot_mapping_specs prot_mapping_adjacent = {
  .map_offset = NUM_FILE_BYTES,
};

struct prot_mapping_specs prot_mapping_overlapped = {
  .map_offset = NACL_MAP_PAGESIZE,
};

struct test_params tests[] = {
  {
    .test_name = "PROT_NONE, MAP_PRIVATE, change to accessible",
    .test_func = test_prot_basic,
    .open_flags = O_RDONLY,
    .file_mode = 0666,
    .file_size = ALIGN(sizeof g_test_data, 4),
    .map_size = NUM_FILE_BYTES,
    .test_data_start = g_test_data,
    .test_data_size = sizeof g_test_data,
    .test_specifics = &prot_basic_none_private,
  }, {
    .test_name = "PROT_NONE, MAP_SHARED, change to accessible",
    .test_func = test_prot_basic,
    .open_flags = O_RDONLY,
    .file_mode = 0666,
    .file_size = ALIGN(sizeof g_test_data, 4),
    .map_size = NUM_FILE_BYTES,
    .test_data_start = g_test_data,
    .test_data_size = sizeof g_test_data,
    .test_specifics = &prot_basic_none_shared,
  }, {
    .test_name = "MAP_PRIVATE access test",
    .test_func = test_prot_access,
    .open_flags = O_RDWR,
    .file_mode = 0666,
    .file_size = NUM_FILE_BYTES,
    .map_size = NUM_FILE_BYTES,
    .test_data_start = NULL,
    .test_data_size = 0,
    .test_specifics = &prot_access_private,
  }, {
    .test_name = "MAP_SHARED access test",
    .test_func = test_prot_access,
    .open_flags = O_RDWR,
    .file_mode = 0666,
    .file_size = NUM_FILE_BYTES,
    .map_size = NUM_FILE_BYTES,
    .test_data_start = NULL,
    .test_data_size = 0,
    .test_specifics = &prot_access_shared,
  }, {
    .test_name = "MAP_PRIVATE access (non-aligned) test",
    .test_func = test_prot_access,
    .open_flags = O_RDWR,
    .file_mode = 0666,
    .file_size = 0x21000,
    .map_size = NUM_FILE_BYTES,
    .test_data_start = NULL,
    .test_data_size = 0,
    .test_specifics = &prot_access_private,
  }, {
    .test_name = "MAP_PRIVATE offset access test",
    .test_func = test_prot_access,
    .open_flags = O_RDWR,
    .file_mode = 0666,
    .file_size = NUM_FILE_BYTES,
    .map_size = NUM_FILE_BYTES,
    .test_data_start = NULL,
    .test_data_size = 0,
    .test_specifics = &prot_access_offset,
  }, {
    .test_name = "MAP_PRIVATE unmap test",
    .test_func = test_prot_unmap,
    .open_flags = O_RDWR,
    .file_mode = 0666,
    .file_size = NUM_FILE_BYTES,
    .map_size = NUM_FILE_BYTES,
    .test_data_start = NULL,
    .test_data_size = 0,
    .test_specifics = &prot_unmap_private,
  }, {
    .test_name = "Unmapped (fully) protection test",
    .test_func = test_prot_unmapped,
    .open_flags = O_RDWR,
    .file_mode = 0666,
    .file_size = NUM_FILE_BYTES,
    .map_size = NUM_FILE_BYTES,
    .test_data_start = NULL,
    .test_data_size = 0,
    .test_specifics = &prot_unmapped_full,
  }, {
    .test_name = "Unmapped (partially) protection test",
    .test_func = test_prot_unmapped,
    .open_flags = O_RDWR,
    .file_mode = 0666,
    .file_size = NUM_FILE_BYTES,
    .map_size = NUM_FILE_BYTES,
    .test_data_start = NULL,
    .test_data_size = 0,
    .test_specifics = &prot_unmapped_partial,
  }, {
    .test_name = "PROT_NONE read/write test",
    .test_func = test_prot_none,
    .open_flags = O_RDWR,
    .file_mode = 0666,
    .file_size = ALIGN(sizeof g_test_data, 4),
    .map_size = NUM_FILE_BYTES,
    .test_data_start = g_test_data,
    .test_data_size = sizeof g_test_data,
    .test_specifics = &prot_none_private,
  }, {
    .test_name = "PROT_NONE read-only test",
    .test_func = test_prot_none,
    .open_flags = O_RDONLY,
    .file_mode = 0666,
    .file_size = ALIGN(sizeof g_test_data, 4),
    .map_size = NUM_FILE_BYTES,
    .test_data_start = g_test_data,
    .test_data_size = sizeof g_test_data,
    .test_specifics = &prot_none_private,
  }, {
    .test_name = "Adjacent Mapping Test",
    .test_func = test_prot_mapping,
    .open_flags = O_RDWR,
    .file_mode = 0666,
    .file_size = ALIGN(sizeof g_test_data, 4),
    .map_size = NUM_FILE_BYTES,
    .test_data_start = g_test_data,
    .test_data_size = sizeof g_test_data,
    .test_specifics = &prot_mapping_adjacent,
  }, {
    .test_name = "Overlapped Mapping Test",
    .test_func = test_prot_mapping,
    .open_flags = O_RDWR,
    .file_mode = 0666,
    .file_size = ALIGN(sizeof g_test_data, 4),
    .map_size = NUM_FILE_BYTES,
    .test_data_start = g_test_data,
    .test_data_size = sizeof g_test_data,
    .test_specifics = &prot_mapping_overlapped,
  }, {
    .test_name = "PROT_READ, MAP_SHARED, O_RDONLY, change to PROT_WRITE",
    .test_func = test_prot_ronly_rw,
    .open_flags = O_RDONLY,
    .file_mode = 0666,
    .file_size = NUM_FILE_BYTES,
    .map_size = NUM_FILE_BYTES,
    .test_data_start = NULL,
    .test_data_size = 0,
    .test_specifics = NULL,
  },
};

int main(int argc, char **argv) {
  static char const *test_file_dir = "/tmp/nacl_mmap_prot_test";
  int fd;
  size_t err_count;
  size_t test_errors;
  size_t ix;
  int opt;
  int num_runs = 1;
  int test_run;

  while (EOF != (opt = getopt(argc, argv, "c:t:"))) {
    switch (opt) {
      case 'c':
        num_runs = atoi(optarg);
        break;
      case 't':
        test_file_dir = optarg;
        break;
      default:
        fprintf(stderr,
                "Usage: mmap_prot_test [-c run_count]\n"
                "                      [-t test_temporary_dir]\n");
        exit(1);
    }
  }

  err_count = 0;
  for (test_run = 0; test_run < num_runs; ++test_run) {
    printf("Test run %d\n\n", test_run);
    for (ix = 0; ix < NACL_ARRAY_SIZE(tests); ++ix) {
      char test_file_name[PATH_MAX];
      printf("%s\n", tests[ix].test_name);

      snprintf(test_file_name, sizeof test_file_name,
                "%s/f%d.%u", test_file_dir, test_run, ix);
      fd = test_file_create(test_file_name, &tests[ix]);
      test_errors = (*tests[ix].test_func)(fd,
                                           tests[ix].map_size,
                                           tests[ix].test_specifics);
      printf("%s\n", (0 == test_errors) ? "PASS" : "FAIL");
      err_count += test_errors;
      test_file_close(fd);
    }
  }

  return (err_count > 0) ? -1 : 0;
}
