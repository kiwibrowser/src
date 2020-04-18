/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_WTF_BYTE_SWAP_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_WTF_BYTE_SWAP_H_

#include <stdint.h>
#include "build/build_config.h"
#include "third_party/blink/renderer/platform/wtf/compiler.h"
#include "third_party/blink/renderer/platform/wtf/cpu.h"

#if defined(COMPILER_MSVC)
#include <stdlib.h>
#endif

namespace WTF {

inline uint32_t Wswap32(uint32_t x) {
  return ((x & 0xffff0000) >> 16) | ((x & 0x0000ffff) << 16);
}

#if defined(COMPILER_MSVC)

ALWAYS_INLINE uint64_t Bswap64(uint64_t x) {
  return _byteswap_uint64(x);
}
ALWAYS_INLINE uint32_t Bswap32(uint32_t x) {
  return _byteswap_ulong(x);
}
ALWAYS_INLINE uint16_t Bswap16(uint16_t x) {
  return _byteswap_ushort(x);
}

#else

ALWAYS_INLINE uint64_t Bswap64(uint64_t x) {
  return __builtin_bswap64(x);
}
ALWAYS_INLINE uint32_t Bswap32(uint32_t x) {
  return __builtin_bswap32(x);
}
ALWAYS_INLINE uint16_t Bswap16(uint16_t x) {
  return __builtin_bswap16(x);
}

#endif

}  // namespace WTF

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_WTF_BYTE_SWAP_H_
