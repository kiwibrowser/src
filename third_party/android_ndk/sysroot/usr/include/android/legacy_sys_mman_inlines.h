/*
 * Copyright (C) 2017 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#pragma once

#include <sys/cdefs.h>

#if __ANDROID_API__ < __ANDROID_API_L__

#include <errno.h>
#include <sys/mman.h>
#include <unistd.h>

__BEGIN_DECLS

/*
 * While this was never an inline, this function alone has caused most of the
 * bug reports related to _FILE_OFFSET_BITS=64. Providing an inline for it
 * should allow a lot more code to build with _FILE_OFFSET_BITS=64 when
 * targeting pre-L.
 */
static __inline void* mmap64(void* __addr, size_t __size, int __prot, int __flags, int __fd,
                             off64_t __offset) __RENAME(mmap64);
static __inline void* mmap64(void* __addr, size_t __size, int __prot, int __flags, int __fd,
                             off64_t __offset) {
  const int __mmap2_shift = 12; // 2**12 == 4096
  if (__offset < 0 || (__offset & ((1UL << __mmap2_shift) - 1)) != 0) {
    errno = EINVAL;
    return MAP_FAILED;
  }

  // prevent allocations large enough for `end - start` to overflow
  size_t __rounded = __BIONIC_ALIGN(__size, PAGE_SIZE);
  if (__rounded < __size || __rounded > PTRDIFF_MAX) {
    errno = ENOMEM;
    return MAP_FAILED;
  }

  extern void* __mmap2(void* __addr, size_t __size, int __prot, int __flags, int __fd,
                       size_t __offset);
  return __mmap2(__addr, __size, __prot, __flags, __fd, __offset >> __mmap2_shift);
}

__END_DECLS

#endif  /* __ANDROID_API__ < __ANDROID_API_L__ */
