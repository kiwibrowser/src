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

#ifndef COMMON_MATH_H_
#define COMMON_MATH_H_

#include <cstddef>
#include <cstdint>
#include <cstring>

#include <limits>
#include <type_traits>

// The following are not valid for 0
uint32_t ScanForward(uint32_t bits);
uint32_t Log2(uint32_t value);
bool IsPowerOfTwo(size_t n);

bool IsPtrAligned(const void* ptr, size_t alignment);
void* AlignVoidPtr(void* ptr, size_t alignment);
bool IsAligned(uint32_t value, size_t alignment);
uint32_t Align(uint32_t value, size_t alignment);

template <typename T>
T* AlignPtr(T* ptr, size_t alignment) {
    return static_cast<T*>(AlignVoidPtr(ptr, alignment));
}

template <typename T>
const T* AlignPtr(const T* ptr, size_t alignment) {
    return static_cast<const T*>(AlignVoidPtr(const_cast<T*>(ptr), alignment));
}

template <typename destType, typename sourceType>
destType BitCast(const sourceType& source) {
    static_assert(sizeof(destType) == sizeof(sourceType), "BitCast: cannot lose precision.");
    destType output;
    std::memcpy(&output, &source, sizeof(destType));
    return output;
}

uint16_t Float32ToFloat16(float fp32);

#endif  // COMMON_MATH_H_
