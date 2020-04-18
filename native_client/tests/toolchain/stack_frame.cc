/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This tests execises __builtin_dwarf_cfa()
 *
 * NOTE: because of fun pointer casting we need to disable -pedantic.
 * NOTE: because of aggressive inlining we need to disable -O2.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unwind.h>
#include "native_client/tests/toolchain/utils.h"

int main(int argc, char* argv[]);
const int MAGIC_MARKER = 0x73537353;
const int NUM_ITERS = 5;


int PointerDelta(void* a, void* b) {
  return (char*) a - (char*) b;
}


void DumpMemory(const unsigned char* cp, int n) {
  for (int i = 0; i < n; ++i) {
    if (i % 8 == 0) printf("%p: %08x  %08x  ", cp, *(int*)cp, *(int*)(cp+4));
    printf("%02x ", *cp);
    ++cp;
    if (i % 8 == 7) printf("\n");
  }
}


void* GetReturnAddress(void* frame_end) {
#if defined(__native_client__)

#if defined(__arm__)
  return ((void**)frame_end)[-1];
#elif defined(__mips__)
  return ((void**)frame_end)[-1];
#elif defined(__i386__)
  return ((void**)frame_end)[-1];
#elif defined(__x86_64__)
  /* NOTE: a call pushes 64 bits but we only care about the first 32 */
  return ((void**)frame_end)[-2];
#else
#error "unknown arch"
#endif

#else /* !defined(__native_client__) */
// NOTE: we also want to compile this file with local compilers like so
// g++ tests/toolchain/stack_frame.cc -m32
// g++ tests/toolchain/stack_frame.cc -m64
// arm-none-linux-gnueabihf-g++
//   tests/toolchain/stack_frame.cc
//   -Wl,-T -Wl,toolchain/linux_x86_linux_arm/arm_trusted/ld_script_arm_trusted
#if defined(__arm__)
  return ((void**)frame_end)[-1];
#elif defined(__i386__)
  return ((void**)frame_end)[-1];
#elif defined(__x86_64__)
  return ((void**)frame_end)[-1];
#else
#error "unknown arch"
#endif

#endif
}


void recurse(int n, unsigned char* old_cfa) {
  int i;
  int array[16];
  unsigned char* cfa = (unsigned char*) __builtin_dwarf_cfa();
  int* start = &array[0];
  int* end = &array[16];
  int frame_size = PointerDelta(old_cfa, cfa);
  void* return_address = GetReturnAddress(old_cfa);
  for (i = 0;  i < 16; ++i) {
    array[i] = MAGIC_MARKER;
  }

  /* NOTE: we dump the frame for this invocation at the beginning of the next */
  printf("frame [%p, %p[\n", cfa, old_cfa);
  printf("framesize %d\n", frame_size);
  printf("return %p\n", return_address);
  DumpMemory(cfa, frame_size);

  // TODO(sehr): change those to 16
  ASSERT(frame_size % 8 == 0, "ERRRO: bad frame size");
  ASSERT((int) cfa % 8 == 0, "ERRRO: bad frame pointer");

  if (n == NUM_ITERS) {
    // main()'s stackframe may be non-standard due to the startup code
  } else if (n == NUM_ITERS - 1) {
    // first stack frame for recurse() - return address inside main()
    ASSERT(FUNPTR2PTR(main) < return_address,
           "ERROR: return address is not within main()");
  } else {
    // recurse() calling itself
    ASSERT(FUNPTR2PTR(recurse) < return_address &&
           return_address < FUNPTR2PTR(main),
           "ERROR: return address is not within recurse()");
  }

  if (n == 0) {
    return;
  }

  printf("========================\n");
  printf("recurse level %d\n", n);
  printf("array %p %p\n", start, end);

  recurse(n - 1, cfa);
  /* NOTE: this print statement also prevents this function
   * from tail recursing into itself.
   * On gcc this behavior can also be controlled using
   *   -foptimize-sibling-calls
   */
  printf("recurse <- %d\n", n);
}


int main(int argc, char* argv[]) {
  printf("&main: %p\n", FUNPTR2PTR(main));
  printf("&recurse: %p\n", FUNPTR2PTR(recurse));
  ASSERT(FUNPTR2PTR(recurse) < FUNPTR2PTR(main),
         "ERROR: this test assumes that main() follows recurse()\n");

  unsigned char* cfa = (unsigned char*) __builtin_dwarf_cfa();
  recurse(NUM_ITERS, cfa);
  return 55;
}
