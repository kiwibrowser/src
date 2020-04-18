/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#ifndef NATIVE_CLIENT_SRC_INCLUDE_CONCURRENCY_OPS_H_
#define NATIVE_CLIENT_SRC_INCLUDE_CONCURRENCY_OPS_H_ 1


#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/portability.h"

#if NACL_WINDOWS && (NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86)
#include <intrin.h>
#include <mmintrin.h>
#endif

#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86

static INLINE void NaClWriteMemoryBarrier(void) {
#if NACL_WINDOWS
  /* Inline assembly is not available in x86-64 MSVC.  Use built-in. */
  _mm_sfence();
#else
  __asm__ __volatile__("sfence");
#endif
}

#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_arm

static INLINE void NaClWriteMemoryBarrier(void) {
  /* Note that this depends on ARMv7. */
  __asm__ __volatile__("dsb");

  /*
   * We could support ARMv6 by instead using:
   * __asm__ __volatile__("mcr p15, 0, %0, c7, c10, 5"
   *                      : : "r" (0) : "memory");
   */
}

#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_mips

static INLINE void NaClWriteMemoryBarrier(void) {
  __asm__ __volatile__("sync" : : : "memory");
}

#else

#error "Define for other architectures"

#endif


static INLINE void NaClFlushCacheForDoublyMappedCode(uint8_t *writable_addr,
                                                     uint8_t *executable_addr,
                                                     size_t size) {
#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86
  /*
   * Clearing the icache explicitly is not necessary on x86.  We could
   * call gcc's __builtin___clear_cache() on x86, where it is a no-op,
   * except that it is not available in Mac OS X's old version of gcc.
   * We simply prevent the compiler from moving loads or stores around
   * this function.
   */
  UNREFERENCED_PARAMETER(writable_addr);
  UNREFERENCED_PARAMETER(executable_addr);
  UNREFERENCED_PARAMETER(size);
#if NACL_WINDOWS
  _ReadWriteBarrier();
#else
  __asm__ __volatile__("" : : : "memory");
#endif
#elif defined(__GNUC__)
  /*
   * __clear_cache() does two things:
   *
   *  1) It flushes the write buffer for the address range.
   *     We need to do this for writable_addr.
   *  2) It clears the instruction cache for the address range.
   *     We need to do this for executable_addr.
   *
   * We do not need apply (1) to executable_addr or apply (2) to
   * writable_addr, but the Linux kernel does not expose (1) and (2)
   * as separate operations; it just provides a single syscall that
   * does both.  For background, see:
   * http://code.google.com/p/nativeclient/issues/detail?id=2443
   *
   * We use __builtin___clear_cache since __clear_cache is only available
   * with gnu extensions available.
   *
   * Casts are needed since clang's prototype for __builtin___clear_cache
   * doesn't match gcc's.
   */
  __builtin___clear_cache((char *) writable_addr,
                          (char *) writable_addr + size);
  __builtin___clear_cache((char *) executable_addr,
                          (char *) executable_addr + size);
#else
  /*
   * Give an error in case we ever target a non-gcc compiler for ARM
   * or for some other architecture that we might support in the
   * future.
   */
# error "Don't know how to clear the icache on this architecture"
#endif
}


#endif  /* NATIVE_CLIENT_SRC_INCLUDE_CONCURRENCY_OPS_H_ */
