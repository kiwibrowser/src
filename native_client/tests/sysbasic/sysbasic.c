/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl tests for simple syscalls not using newlib
 */

#define CHECK_ERRNO 1

#include "native_client/src/trusted/service_runtime/include/sys/nacl_syscalls.h"

/*
 * These must come after <sys/nacl_syscalls.h> when using nacl-newlib
 * otherwise we get errors about conflicting types.  This is a bug in
 * nacl-newlib.
 */
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

/* NOTE: defining CHECK_ERRNO pulls in some newlib magic (reent.h, etc.) */
#if defined(CHECK_ERRNO)
#include <errno.h>
#endif


int mystrlen(const char* s) {
  int count = 0;
  while(*s++) ++count;
  return count;
}

#define myprint(s) write(1, s, mystrlen(s))

void hextochar(int n, char buffer[9]) {
  int i;
  buffer[8] = 0;

  for (i=0; i < 8; ++i) {
    int nibble = 0xf & (n >> (4 * (7 - i)));
    if (nibble <= 9) {
      buffer[i] = nibble + '0';
    } else {
      buffer[i] = nibble - 10 + 'A';
    }
  }
}

void PrintInt(int i) {
  char buffer[16];

  hextochar(i, buffer);
  myprint(buffer);
  myprint("\n");
}

void Error(const char* message) {
  myprint("ERROR: ");
  myprint(message);
  _exit(-1);
}

#if defined(CHECK_ERRNO)

void CheckErrno(int expected) {
  char buffer[16];

  myprint("\nerrno\n");
  hextochar(errno, buffer);
  myprint(buffer);
  myprint("\n");
  if (expected != errno) Error("bad errno value\n");
}

#else

void CheckErrno(int expected) {
  expected = expected;
}

#endif

const int kExitOk = 69;
const int kMmapBase     = 0xa00000;
const int kMmapSize     = 0x10000;
const int kInvalidFileDescriptor = 100;

/*
 * This is defined by the linker as the address of the end of our data segment.
 * That's where the break starts out by default.
 */
extern char end;

int main(void) {
  /*
   * Round up to the end of the page that's our last initial data page.
   * Then add 10MB for good measure to be out of the way of any allocations
   * that might have been done before we got here.
   */
  const int sysbrkBase = ((((intptr_t) &end) + 0xffff) & -0x10000) + (10 << 20);
  void *map_result;
  int  i;

  CheckErrno(0);

  myprint("\nnull_syscall()\n");
  null_syscall();

  myprint("\nsysbreak()\n");
  i = (int) sysbrk(0);
  PrintInt(i);
  if (0 == i) Error("bad sysbrk() value\n");

  if (sysbrkBase < i) {
    Error("INTERNAL ERROR: sysbrkBase too small\n");
  }

  myprint("\nsysbrk()\n");
  i = (int) sysbrk((void *) sysbrkBase);
  PrintInt(i);
  if (sysbrkBase != i) Error("bad sysbrk() value\n");

  myprint("\nmmap()\n");
  i = (int) mmap(0, kMmapSize,
                 PROT_READ, MAP_ANONYMOUS | MAP_SHARED,
                 -1, 0);
  PrintInt(i);
  if (-1 == i) Error("bad mmap() value\n");

  myprint("\nmunmap()\n");
  i = (int) munmap((void *)i, kMmapSize);
  PrintInt(i);
  if (0 != i) Error("bad munmap() value\n");

  myprint("\nmmap()\n");
  map_result = mmap((void *)kMmapBase, kMmapSize,
                    PROT_READ, MAP_ANONYMOUS | MAP_SHARED,
                    -1, 0);
  i = (int) map_result;
  PrintInt(i);
  /*
   * It is an ERROR to expect i to be kMapBase.  Since the mmap
   * syscall were not made with MAP_FIXED, the kernel is free to use
   * the provided address as a hint -- or to ignore it altogether.
   * Exactly what behavior we implement may vary from release to
   * release -- thus, the only reasonable value to compare this with
   * is MAP_FAILED.
   */
  if (MAP_FAILED == map_result) Error("bad: mmap() failed\n");

  myprint("\nmunmap()\n");
  i = (int) munmap(map_result, kMmapSize);
  PrintInt(i);
  if (0 != i) Error("bad munmap() value\n");

  CheckErrno(0);

  /* Some expect failures */
  myprint("\nclose()\n");
  i = (int) close(kInvalidFileDescriptor);
  PrintInt(i);
  if (~0 != i) Error("bad close value\n");
  CheckErrno(EBADF);

  myprint("\nclock()\n");
  i = clock();
  PrintInt(i);
  if (0 > i) Error("bad clock value\n");

  myprint("before _exit()\n");
  _exit(kExitOk);

  return 0;
}
