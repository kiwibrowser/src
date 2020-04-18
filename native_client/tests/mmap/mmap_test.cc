/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "native_client/src/include/nacl_assert.h"
#include "native_client/src/include/nacl/nacl_exception.h"


#define PRINT_HEADER 0
#define TEXT_LINE_SIZE 1024

const char *example_file;
const char *tmp_dir;

/*
 * function failed(testname, msg)
 *   print failure message and exit with a return code of -1
 */

bool failed(const char *testname, const char *msg) {
  printf("TEST FAILED: %s: %s\n", testname, msg);
  return false;
}

/*
 * function passed(testname, msg)
 *   print success message
 */

bool passed(const char *testname, const char *msg) {
  printf("TEST PASSED: %s: %s\n", testname, msg);
  return true;
}


static jmp_buf g_jmp_buf;

static void exception_handler(struct NaClExceptionContext *context) {
  /* We got an exception as expected.  Return from the handler. */
  int rc = nacl_exception_clear_flag();
  assert(rc == 0);
  longjmp(g_jmp_buf, 1);
}

static void assert_addr_is_unreadable(volatile char *addr) {
  /*
   * TODO(mseaborn): It would be better to use Valgrind annotations to
   * turn off the memory access checks temporarily.
   */
  if (getenv("RUNNING_ON_VALGRIND") != NULL) {
    fprintf(stderr, "Skipping assert_addr_is_unreadable() under Valgrind\n");
    return;
  }
  /*
   * Non-SFI Mode nonsfi_loader with host libc does not have signal
   * handler implementation.
   */
  if (!USE_NEWLIB_NONSFI_LOADER) {
    fprintf(stderr, "Skipping assert_addr_is_unreadable() under "
            "nonsfi_loader with host libc\n");
    return;
  }

  int rc = nacl_exception_set_handler(exception_handler);
  assert(rc == 0);
  if (!setjmp(g_jmp_buf)) {
    char value = *addr;
    /* If we reach here, the assertion failed. */
    fprintf(stderr, "Address %p was readable, and contained %i\n",
            /* C-style cast, because C++ casts can't cast away volatile */
            (void *) addr, value);
    exit(1);
  }
  /*
   * Clean up: Unregister the exception handler so that we do not
   * accidentally return through g_jmp_buf if an exception occurs.
   */
  rc = nacl_exception_set_handler(NULL);
  assert(rc == 0);
}

static void assert_addr_is_unwritable(volatile char *addr, char value) {
  /*
   * TODO(mseaborn): It would be better to use Valgrind annotations to
   * turn off the memory access checks temporarily.
   */
  if (getenv("RUNNING_ON_VALGRIND") != NULL) {
    fprintf(stderr, "Skipping assert_addr_is_unwritable() under Valgrind\n");
    return;
  }
  if (getenv("RUNNING_ON_ASAN") != NULL) {
    fprintf(stderr, "Skipping assert_addr_is_unwritable() under ASan\n");
    return;
  }
  /*
   * Non-SFI Mode nonsfi_loader with host libc does not have signal
   * handler implementation.
   */
  if (!USE_NEWLIB_NONSFI_LOADER) {
    fprintf(stderr, "Skipping assert_addr_is_unwritable() under "
            "nonsfi_loader with host libc\n");
    return;
  }

  int rc = nacl_exception_set_handler(exception_handler);
  assert(rc == 0);
  if (!setjmp(g_jmp_buf)) {
    *addr = value;
    /* If we reach here, the assertion failed. */
    fprintf(stderr, "Address %p was writable, %i was written\n",
            /* C-style cast, because C++ casts can't cast away volatile */
            (void *) addr, value);
    exit(1);
  }
  /*
   * Clean up: Unregister the exception handler so that we do not
   * accidentally return through g_jmp_buf if an exception occurs.
   */
  rc = nacl_exception_set_handler(NULL);
  assert(rc == 0);
}

static void assert_page_is_allocated(void *addr) {
  const int kPageSize = getpagesize();
  assert(((uintptr_t) addr & (kPageSize - 1)) == 0);
  /*
   * Try mapping at addr without MAP_FIXED.  If something is already
   * mapped there, the system will pick another address.  Otherwise,
   * we will get the address we asked for.
   */
  void *result = mmap(addr, kPageSize, PROT_READ | PROT_WRITE,
                      MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  assert(result != MAP_FAILED);
  assert(result != addr);
  int rc = munmap(result, kPageSize);
  assert(rc == 0);
}

/*
 * function test*()
 *
 *   Simple tests follow below.  Each test may call one or more
 *   of the functions above.  They all have a boolean return value
 *   to indicate success (all tests passed) or failure (one or more
 *   tests failed)  Order matters - the parent should call
 *   test1() before test2(), and so on.
 */

bool test1() {
  int size = 64 * 1024;  /* we need 64K */
  void *zeroes;
  // test simple mmap
  void *res = NULL;
  int rv;

  printf("test1\n");

  res = mmap(res, size, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
  if (0 >= res) /* define MAP_FAILED */
    return false;
  printf("mmap done\n");
  zeroes = malloc(size);
  memset(zeroes, 0, size);
  if (memcmp(res, zeroes, size)) {
    printf("memcmp failed\n");
    return false;
  }

  rv = munmap(res, 1024);
  if (rv != 0) {
    printf("munmap failed\n");
    return false;
  }
  printf("munmap good\n");
  return true;
}



/*
 *   Verify that munmap of executable text pages will fail.
 */

bool test2() {
  int rv;

  printf("test2\n");

  if (NONSFI_MODE) {
    /*
     * Unmapping SFI-NaCl's text page would succeed in non-SFI
     * mode. We skip this test case.
     */
    printf("test2 skipped\n");
    return true;
  }

  /* text starts at 64K */
  rv = munmap(reinterpret_cast<void*>(1<<16), (size_t) (1<<16));

  /*
   * if the munmap succeeds, we probably won't be able to continue to
   * run....
   */
  printf("munmap returned %d\n", rv);

  if (-1 == rv && EINVAL == errno) {
    printf("munmap good (failed as expected)\n");
    return true;
  }
  printf("munmap should not have succeeded, or failed with wrong error\n");
  return false;
}

/*
 *   Verify that mmap into the NULL pointer guard page will fail.
 */

bool test3() {
  void  *res;

  printf("test3\n");
  if (NONSFI_MODE) {
    /*
     * This test checks a security property of the NaCl TCB.  However, when
     * calling Linux's mmap() syscall directly, we can't necessarily expect
     * a specific error value for this case.
     */
    printf("test3 skipped\n");
    return true;
  }

  res = mmap(static_cast<void*>(0), (size_t) (1 << 16),
             PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, 0, 0);
  printf("res = %p\n", res);
  if (MAP_FAILED == res) {
    printf("errno = %d\n", errno);
  }
  if (MAP_FAILED == res && EINVAL == errno) {
    printf("mmap okay\n");
    return true;
  }
  printf("mmap should not have succeeded, or failed with wrong error\n");
  return false;
}

/*
 *   Verify that mmap/MAP_FIXED with a non-page-aligned address will fail.
 */

bool test4() {
  printf("test4\n");
  /* First reserve some address space in which to perform the experiment. */
  char *alloc = (char *) mmap(NULL, 1 << 16, PROT_NONE,
                              MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (MAP_FAILED == alloc) {
    printf("mmap failed\n");
    return false;
  }

  void *res = mmap((void *) (alloc + 0x100), 1 << 16, PROT_READ,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
  if (MAP_FAILED == res && EINVAL == errno) {
    printf("mmap gave an error as expected\n");
    return true;
  }
  printf("mmap should not have succeeded, or failed with wrong error\n");
  return false;
}

/*
 *   Verify that munmap() leaves virtual addresses inaccessible.
 */

bool test_munmap() {
  printf("test_munmap\n");
  /*
   * Note that this test could fail if it were run concurrently with
   * other tests in the same process, because other threads might
   * mmap() pages at the address we munmap().
   *
   * Note that, on Windows, NaCl's munmap() has different code paths
   * for anonymous and file-backed mappings.  This test case only
   * covers the anonymous case.  The file-backed case is covered by
   * test_mmap_end_of_file().
   */
  size_t map_size = 0x20000;
  char *addr = (char *) mmap(NULL, map_size, PROT_READ | PROT_WRITE,
                             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  assert(addr != MAP_FAILED);
  int rc = munmap(addr, map_size);
  assert(rc == 0);
  assert_addr_is_unreadable(addr);
  assert_addr_is_unreadable(addr + 0x1000);
  assert_addr_is_unreadable(addr + 0x10000);
  /* Test that munmap() is idempotent. */
  rc = munmap(addr, map_size);
  assert(rc == 0);
  return true;
}

bool test_mmap_zero_size() {
  /*
   * This test fails under Non-SFI Mode under ARM QEMU, because ARM QEMU
   * handles this case incorrectly.
   */
  if (NONSFI_MODE)
    return true;

  void *addr = mmap(NULL, 0, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  ASSERT_EQ(addr, MAP_FAILED);
  ASSERT_EQ(errno, EINVAL);

  /* Test behaviour when rounding up the size overflows. */
  addr = mmap(NULL, ~(size_t) 0, PROT_READ | PROT_WRITE,
              MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  ASSERT_EQ(addr, MAP_FAILED);
  ASSERT_EQ(errno, EINVAL);

  return true;
}

bool test_munmap_zero_size() {
  /* Allocate a valid address to test munmap() with. */
  size_t map_size = 0x10000;
  char *addr = (char *) mmap(NULL, map_size, PROT_READ | PROT_WRITE,
                             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  assert(addr != MAP_FAILED);

  int rc = munmap(addr, 0);
  ASSERT_EQ(rc, -1);
  ASSERT_EQ(errno, EINVAL);

  /* Test behaviour when rounding up the size overflows. */
  rc = munmap(addr, ~(size_t) 0);
  ASSERT_EQ(rc, -1);
  ASSERT_EQ(errno, EINVAL);

  /* Clean up. */
  rc = munmap(addr, map_size);
  ASSERT_EQ(rc, 0);
  return true;
}

/*
 *   Verify that mprotect() changes the virtual address protection.
 */

bool test_mprotect() {
  printf("test_mprotect\n");
  /*
   * Note that, on Windows, NaCl's mprotect() has different code paths
   * for anonymous and file-backed mappings.  This test case only
   * covers the anonymous case.
   */
  size_t map_size = 0x20000;
  char *addr = (char *) mmap(NULL, map_size, PROT_READ | PROT_WRITE,
                             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  assert(addr != MAP_FAILED);
  printf("mmap done\n");
  /* Change the protection to make the page unreadable. */
  int rc = mprotect(addr, map_size, PROT_NONE);
  assert(rc == 0);
  assert_addr_is_unreadable(addr);
  assert_addr_is_unreadable(addr + 0x1000);
  assert_addr_is_unreadable(addr + 0x10000);
  /* Change the protection to make the page accessible again. */
  rc = mprotect(addr, map_size, PROT_READ | PROT_WRITE);
  assert(rc == 0);
  addr[0] = '5';
  /* Change the protection to make the page read-only. */
  rc = mprotect(addr, map_size, PROT_READ);
  assert(rc == 0);
  assert_addr_is_unwritable(addr, '9');
  assert('5' == addr[0]);
  printf("mprotect good\n");
  /* We can still munmap() the memory. */
  rc = munmap(addr, map_size);
  assert(rc == 0);
  return true;
}

bool test_mprotect_offset() {
  printf("test_mprotect_offset\n");
  /*
   * Note that, on Windows, NaCl's mprotect() has different code paths
   * for anonymous and file-backed mappings.  This test case only
   * covers the anonymous case.
   */
  size_t map_size = 0x40000;
  volatile char *addr = (char *) mmap(NULL, map_size, PROT_READ | PROT_WRITE,
                                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  assert(addr != MAP_FAILED);
  printf("mmap done\n");
  /* Change the protection to make the pages unreadable. */
  int rc = mprotect((char *) addr + 0x20000, 0x20000, PROT_NONE);
  assert(rc == 0);
  addr[0] = '5';
  assert('5' == addr[0]);
  assert_addr_is_unreadable(addr + 0x20000);
  assert_addr_is_unreadable(addr + 0x30000);
  /* Change the protection to make the pages read-only. */
  rc = mprotect((char *) addr, 0x20000, PROT_READ);
  assert(rc == 0);
  assert_addr_is_unwritable(addr, '9');
  assert('5' == addr[0]);
  /* Change the protection to make the pages accessible again. */
  rc = mprotect((char *) addr + 0x10000, 0x20000, PROT_READ | PROT_WRITE);
  assert(rc == 0);
  *(addr + 0x20000) = '7';
  printf("mprotect good\n");
  /* We can still munmap() the memory. */
  rc = munmap((char *) addr, map_size);
  assert(rc == 0);
  return true;
}

/*
 *   Verify that mprotect() fails when changing protection of unmapped
 *   memory region.
 */

bool test_mprotect_unmapped_memory() {
  printf("test_mprotect_unmapped_memory\n");
  /*
   * Note that, on Windows, NaCl's mprotect() has different code paths
   * for anonymous and file-backed mappings.  This test case only
   * covers the anonymous case.
   */
  size_t map_size = 0x20000;
  char *addr = (char *) mmap(NULL, map_size, PROT_READ | PROT_WRITE,
                             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  assert(addr != MAP_FAILED);
  printf("mmap done\n");
  /* Unmap the mapped memory region. */
  int rc = munmap(addr, map_size);
  assert(rc == 0);
  printf("munmap done\n");
  /* Change the protection to make the page unreadable. */
  rc = mprotect(addr, map_size, PROT_NONE);
  const int kExpectedErrno = NONSFI_MODE ? ENOMEM : EACCES;
  if (-1 == rc && kExpectedErrno == errno) {
    printf("mprotect good (failed as expected)\n");
    return true;
  }
  printf("mprotect should not have succeeded, or failed with wrong error\n");
  return false;
}


/*
 *   Verify that the last page in a file can be mmapped when the file's
 *   size is not a multiple of the page size.
 *   Tests for http://code.google.com/p/nativeclient/issues/detail?id=836
 */

bool test_mmap_end_of_file() {
  printf("test_mmap_end_of_file\n");
  int fd = open(example_file, O_RDONLY);
  if (fd < 0) {
    printf("open() failed\n");
    return false;
  }
  size_t map_size = 0x20000;
  /*
   * First, map an address range as readable+writable, in order to
   * check that these mappings are properly overwritten by the second
   * mmap() call.
   */
  char *alloc = (char *) mmap(NULL, map_size, PROT_READ | PROT_WRITE,
                              MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  assert(alloc != MAP_FAILED);
  char *addr = (char *) mmap((void *) alloc, map_size, PROT_READ,
                             MAP_PRIVATE | MAP_FIXED, fd, 0);
  assert(addr == alloc);
  int rc = close(fd);
  if (rc != 0) {
    printf("close() failed\n");
    return false;
  }
  /* To avoid line ending issues, this test file contains no newlines. */
  const char *expected_data =
    "Test file for mmapping, less than a page in size.";
  if (memcmp(alloc, expected_data, strlen(expected_data)) != 0) {
    printf("Unexpected contents: %s\n", alloc);
    return false;
  }
  /* The first 4k page should be readable. */
  for (size_t i = strlen(expected_data); i < 0x1000; i++) {
    if (alloc[i] != 0) {
      printf("Unexpected padding byte: %i\n", alloc[i]);
      return false;
    }
  }
  /*
   * Addresses beyond the first 4k should not be readable.  This is
   * one case where we expose a 4k page size rather than a 64k page
   * size.  Windows forces us to expose a mixture of 4k and 64k page
   * sizes for end-of-file mappings.
   * See http://code.google.com/p/nativeclient/issues/detail?id=824
   */
  assert_addr_is_unreadable(alloc + 0x1000);
  assert_addr_is_unreadable(alloc + 0x2000);
  assert_addr_is_unreadable(alloc + 0x10000);
  assert_addr_is_unreadable(alloc + 0x11000);
  assert_page_is_allocated(alloc);
  assert_page_is_allocated(alloc + 0x10000);
  rc = munmap(alloc, map_size);
  if (rc != 0) {
    printf("munmap() failed\n");
    return false;
  }
  /* This is similar to test_munmap(), but it covers the file-backed case. */
  assert_addr_is_unreadable(alloc);
  assert_addr_is_unreadable(alloc + 0x1000);
  assert_addr_is_unreadable(alloc + 0x2000);
  assert_addr_is_unreadable(alloc + 0x10000);
  assert_addr_is_unreadable(alloc + 0x11000);
  return true;
}

/*
 *   Verify mmap with file offsets works properly.
 */

bool test_mmap_offset() {
  printf("test_mmap_offset\n");

  /*
   * Prepare a file which is filled with raw integer values. These
   * integer values are their offsets in the file.
   */
  char tmp_filename[PATH_MAX] = {};
  snprintf(tmp_filename, PATH_MAX - 1, "%s/test.txt", tmp_dir);
  int fd = open(tmp_filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  ASSERT_GE(fd, 0);
  const int map_size = 0x10000;
  for (int i = 0; i < map_size * 2; i += sizeof(i)) {
    ssize_t written = write(fd, &i, sizeof(i));
    ASSERT_EQ(written, sizeof(i));
  }
  int rc = close(fd);
  ASSERT_EQ(rc, 0);

  fd = open(tmp_filename, O_RDONLY);
  ASSERT_GE(fd, 0);

  /* A valid mmap call with an offset specified. */
  int file_offset = 0x10000;
  char *addr = (char *) mmap(NULL, map_size, PROT_READ,
                             MAP_PRIVATE, fd, file_offset);
  ASSERT_NE(addr, MAP_FAILED);
  for (int i = 0; i < map_size; i += sizeof(i)) {
    int expected = i + file_offset;
    int actual = *(int *) (addr + i);
    ASSERT_EQ(expected, actual);
  }
  rc = munmap(addr, map_size);
  ASSERT_EQ(rc, 0);

  /* An invalid offset is specified to mmap. */
  file_offset = 0x100;
  addr = (char *) mmap(NULL, map_size, PROT_READ,
                       MAP_PRIVATE, fd, file_offset);
  ASSERT_MSG(addr == MAP_FAILED && EINVAL == errno,
             "mmap should not have succeeded, or failed with wrong error");

  rc = close(fd);
  ASSERT_EQ(rc, 0);

  printf("mmap with offset good\n");
  return true;
}

/*
 * function testSuite()
 *
 *   Run through a complete sequence of file tests.
 *
 * returns true if all tests succeed.  false if one or more fail.
 */

bool testSuite() {
  bool ret = true;
  // The order of executing these tests matters!
  ret &= test1();
  ret &= test2();
  ret &= test3();
  ret &= test4();

  ret &= test_munmap();
  ret &= test_mmap_zero_size();
  ret &= test_munmap_zero_size();
  ret &= test_mprotect();
  ret &= test_mprotect_offset();
  ret &= test_mprotect_unmapped_memory();
  ret &= test_mmap_end_of_file();
  ret &= test_mmap_offset();

  return ret;
}

/*
 * main entry point.
 *
 * run all tests and call system exit with appropriate value
 *   0 - success, all tests passed.
 *  -1 - one or more tests failed.
 */

int main(const int argc, const char *argv[]) {
  bool passed;

  if (argc != 3) {
    printf("Error: Expected test file and temp dir args\n");
    return 1;
  }
  example_file = argv[1];
  tmp_dir = argv[2];

  // run the full test suite
  passed = testSuite();

  if (passed) {
    printf("All tests PASSED\n");
    exit(0);
  }
  printf("One or more tests FAILED\n");
  exit(-1);
}
