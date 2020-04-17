// Copyright 2017 The Dawn Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "common/Math.h"

#include "common/Assert.h"

#include <algorithm>

#if defined(DAWN_COMPILER_MSVC)
#    include <intrin.h>
#endif

uint32_t ScanForward(uint32_t bits) {
    ASSERT(bits != 0);
#if defined(DAWN_COMPILER_MSVC)
    unsigned long firstBitIndex = 0ul;
    unsigned char ret = _BitScanForward(&firstBitIndex, bits);
    ASSERT(ret != 0);
    return firstBitIndex;
#else
    return static_cast<uint32_t>(__builtin_ctz(bits));
#endif
}

uint32_t Log2(uint32_t value) {
    ASSERT(value != 0);
#if defined(DAWN_COMPILER_MSVC)
    unsigned long firstBitIndex = 0ul;
    unsigned char ret = _BitScanReverse(&firstBitIndex, value);
    ASSERT(ret != 0);
    return firstBitIndex;
#else
    return 31 - static_cast<uint32_t>(__builtin_clz(value));
#endif
}

bool IsPowerOfTwo(size_t n) {
    ASSERT(n != 0);
    return (n & (n - 1)) == 0;
}

bool IsPtrAligned(const void* ptr, size_t alignment) {
    ASSERT(IsPowerOfTwo(alignment));
    ASSERT(alignment != 0);
    return (reinterpret_cast<size_t>(ptr) & (alignment - 1)) == 0;
}

void* AlignVoidPtr(void* ptr, size_t alignment) {
    ASSERT(IsPowerOfTwo(alignment));
    ASSERT(alignment != 0);
    return reinterpret_cast<void*>((reinterpret_cast<size_t>(ptr) + (alignment - 1)) &
                                   ~(alignment - 1));
}

bool IsAligned(uint32_t value, size_t alignment) {
    ASSERT(alignment <= UINT32_MAX);
    ASSERT(IsPowerOfTwo(alignment));
    ASSERT(alignment != 0);
    uint32_t alignment32 = static_cast<uint32_t>(alignment);
    return (value & (alignment32 - 1)) == 0;
}

uint32_t Align(uint32_t value, size_t alignment) {
    ASSERT(alignment <= UINT32_MAX);
    ASSERT(IsPowerOfTwo(alignment));
    ASSERT(alignment != 0);
    uint32_t alignment32 = static_cast<uint32_t>(alignment);
    return (value + (alignment32 - 1)) & ~(alignment32 - 1);
}

uint16_t Float32ToFloat16(float fp32) {
    uint32_t fp32i = BitCast<uint32_t>(fp32);
    uint32_t sign16 = (fp32i & 0x80000000) >> 16;
    uint32_t mantissaAndExponent = fp32i & 0x7FFFFFFF;

    if (mantissaAndExponent > 0x47FFEFFF) {  // Infinity
        return static_cast<uint16_t>(sign16 | 0x7FFF);
    } else if (mantissaAndExponent < 0x38800000) {  // Denormal
        uint32_t mantissa = (mantissaAndExponent & 0x007FFFFF) | 0x00800000;
        int32_t exponent = 113 - (mantissaAndExponent >> 23);

        if (exponent < 24) {
            mantissaAndExponent = mantissa >> exponent;
        } else {
            mantissaAndExponent = 0;
        }

        return static_cast<uint16_t>(
            sign16 | (mantissaAndExponent + 0x00000FFF + ((mantissaAndExponent >> 13) & 1)) >> 13);
    } else {
        return static_cast<uint16_t>(sign16 | (mantissaAndExponent + 0xC8000000 + 0x00000FFF +
                                               ((mantissaAndExponent >> 13) & 1)) >>
                                                  13);
    }
}
