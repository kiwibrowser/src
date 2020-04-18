/*
 * Copyright (c) 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <sys/mman.h>

#include <cstdlib>
#include <cstring>

#include "native_client/tests/syscalls/test.h"

namespace {
// Use 64Kb regions in the memory tests.  This is usually a whole number of
// pages.  The 32Kb size is used for certain other failure modes.
const size_t k64Kbytes = 64 * 1024;
const size_t k32Kbytes = 32 * 1024;

// |MAP_ANONYMOUS| expects a filedesc of -1.  While this is not strictly true
// on all systems, portable code should work this way.
const int kAnonymousFiledesc = -1;

// When |MAP_ANONYMOUS| is not specified, any negative file desc should report
// EBADF.
const int kBadFiledesc = -3;

// Try to mmap a 0-length region.  This is expected to fail.
int TestZeroLengthRegion() {
  START_TEST("TestZeroLengthRegion");
  void* mmap_ptr = mmap(NULL,
                        0,
                        PROT_READ,
                        MAP_PRIVATE | MAP_ANONYMOUS,
                        kAnonymousFiledesc,
                        0);
  EXPECT(MAP_FAILED == mmap_ptr);
  EXPECT(EINVAL == errno);
  END_TEST();
}

// Hand in a bad file descriptor, this test is successful if mmap() fails.
// The |MAP_ANONYMOUS| flag is deliberately not used for this test, because
// we are trying to map an actual (but bad) filedesc.
int TestBadFiledesc() {
  START_TEST("TestBadFiledesc");
  void* mmap_ptr = mmap(NULL,
                        k64Kbytes,
                        PROT_READ,
                        MAP_PRIVATE,
                        kBadFiledesc,
                        0);
  EXPECT(MAP_FAILED == mmap_ptr);
  EXPECT(EBADF == errno);
  END_TEST();
}

// Verify that mmap does not fail if a bad hint address is passed, but
// |MMAP_FIXED| is not specified.
int TestMmapBadHint() {
  START_TEST("TestMmapBadHint");
  void* bad_hint = (void *) 0x123;
  void* mmap_ptr = mmap(bad_hint,
                        k64Kbytes,
                        PROT_READ,
                        MAP_PRIVATE | MAP_ANONYMOUS,
                        kAnonymousFiledesc,
                        0);
  EXPECT(MAP_FAILED != mmap_ptr);
  EXPECT(mmap_ptr != bad_hint);
  EXPECT(munmap(mmap_ptr, k64Kbytes) == 0);
  END_TEST();
}

// Verify that mmap does fail if a bad hint address is passed and
// |MMAP_FIXED| is specified.
int TestMmapBadHintFixed() {
  START_TEST("TestMmapBadHintFixed");
  void* bad_hint = (void *) 0x123;
  void* mmap_ptr = mmap(bad_hint,
                        k64Kbytes,
                        PROT_READ,
                        MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED,
                        kAnonymousFiledesc,
                        0);
  EXPECT(MAP_FAILED == mmap_ptr);
  END_TEST();
}

// Test mmap() and munmap(), since these often to go together.  Tries to mmap
// a 64 Kb region of memory and then tests to make sure that the pages have all
// been 0-filled.
int TestMmapMunmap() {
  START_TEST("TestMmapMunmap");
  void* mmap_ptr = mmap(NULL,
                        k64Kbytes,
                        PROT_READ,
                        MAP_PRIVATE | MAP_ANONYMOUS,
                        kAnonymousFiledesc,
                        0);
  EXPECT(MAP_FAILED != mmap_ptr);
  // Create a zero-filled region on the heap for comparison with the mmaped
  // region.
  void* zeroes = std::malloc(k64Kbytes);
  std::memset(zeroes, 0, k64Kbytes);
  EXPECT(std::memcmp(mmap_ptr, zeroes, k64Kbytes) == 0);
  // Attempt to release the mapped memory.
  EXPECT(munmap(mmap_ptr, k64Kbytes) == 0);
  std::free(zeroes);
  END_TEST();
}

// munmap() of text (executable) pages should fail.
int TestMunmapText() {
  START_TEST("TestMunmapText");
  // Text pages start at 64K for the syscall trampolines.  If this first call
  // to munmap() succeeds, we will get a segfault on the next trampoline usage.
  // In this case, sel_ldr will return a recognizable error code.
  int rv = munmap(reinterpret_cast<void*>(k64Kbytes), k64Kbytes);
  // Note: if the munmap succeeds, this code is probably not running any more.
  EXPECT(rv == -1);
  EXPECT(EINVAL == errno);
  // Untrusted code text starts at 128K.  If this second call to munmap()
  // succeeds, there will be a rapid segfault and sel_ldr will return a
  // recognizable error code.
  rv = munmap(reinterpret_cast<void*>(k64Kbytes*2), k64Kbytes);
  // Note: if the munmap succeeds, this code is probably not running any more.
  EXPECT(rv == -1);
  EXPECT(EINVAL == errno);
  END_TEST();
}

// Verify that mmap into the NULL pointer guard page will fail.  This uses the
// |MAP_FIXED| flag to try an force mmap() to pin the region at NULL.
int TestMmapNULL() {
  START_TEST("TestMmapNULL");
  void *mmap_ptr = mmap(NULL,
                        k64Kbytes,
                        PROT_READ,
                        MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED,
                        kAnonymousFiledesc,
                        0);
  EXPECT(MAP_FAILED == mmap_ptr);
  EXPECT(EINVAL == errno);
  END_TEST();
}

// Attempt to allocate 32Kb, starting at 32Kb.  This should fail.
int TestMmap32k() {
  START_TEST("TestMmap32k");
  void *mmap_ptr = mmap(reinterpret_cast<void*>(k32Kbytes),
                        k32Kbytes,
                        PROT_READ,
                        MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED,
                        kAnonymousFiledesc,
                        0);
  EXPECT(MAP_FAILED == mmap_ptr);
  EXPECT(EINVAL == errno);
  END_TEST();
}

// Verify that mmap() with the |MAP_FIXED| flag and a non-page-aligned address
// will fail.
int TestMmapNonPageAligned() {
  START_TEST("TestMmapNonPageAligned");
  // Reserve some legal (page-aligned) address space in which to perform the
  // test.
  char* local_heap = reinterpret_cast<char*>(mmap(NULL,
                                                  k64Kbytes,
                                                  PROT_NONE,
                                                  MAP_PRIVATE | MAP_ANONYMOUS,
                                                  kAnonymousFiledesc,
                                                  0));
  EXPECT(MAP_FAILED != local_heap);

  void* unaligned_ptr = mmap(static_cast<void*>(local_heap + 0x100),
                             k64Kbytes,
                             PROT_READ,
                             MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED,
                             kAnonymousFiledesc,
                             0);
  EXPECT(MAP_FAILED == unaligned_ptr);
  EXPECT(EINVAL == errno);
  END_TEST();
}
}  // namespace

// Run through the complete sequence of memory tests.  Sets the exit code to
// the number of failed tests.  Exit code 0 means all passed.
int main() {
  int fail_count = 0;
  fail_count += TestZeroLengthRegion();
  fail_count += TestBadFiledesc();
  fail_count += TestMmapBadHint();
  fail_count += TestMmapBadHintFixed();
  fail_count += TestMmapMunmap();
  fail_count += TestMunmapText();
  fail_count += TestMmapNULL();
  fail_count += TestMmap32k();
  fail_count += TestMmapNonPageAligned();
  return fail_count;
}
