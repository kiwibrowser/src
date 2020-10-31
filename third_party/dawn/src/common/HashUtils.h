// Copyright 2018 The Dawn Authors
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

#ifndef COMMON_HASHUTILS_H_
#define COMMON_HASHUTILS_H_

#include "common/Platform.h"
#include "common/TypedInteger.h"
#include "common/ityp_bitset.h"

#include <bitset>
#include <functional>

// Wrapper around std::hash to make it a templated function instead of a functor. It is marginally
// nicer, and avoids adding to the std namespace to add hashing of other types.
template <typename T>
size_t Hash(const T& value) {
    return std::hash<T>()(value);
}

// Add hashing of TypedIntegers
template <typename Tag, typename T>
size_t Hash(const TypedInteger<Tag, T>& value) {
    return Hash(static_cast<T>(value));
}

// When hashing sparse structures we want to iteratively build a hash value with only parts of the
// data. HashCombine "hashes" together an existing hash and hashable values.
//
// Example usage to compute the hash of a mask and values corresponding to the mask:
//
//    size_t hash = Hash(mask):
//    for (uint32_t i : IterateBitSet(mask)) { HashCombine(&hash, hashables[i]); }
//    return hash;
template <typename T>
void HashCombine(size_t* hash, const T& value) {
#if defined(DAWN_PLATFORM_64_BIT)
    const size_t offset = 0x9e3779b97f4a7c16;
#elif defined(DAWN_PLATFORM_32_BIT)
    const size_t offset = 0x9e3779b9;
#else
#    error "Unsupported platform"
#endif
    *hash ^= Hash(value) + offset + (*hash << 6) + (*hash >> 2);
}

template <typename T, typename... Args>
void HashCombine(size_t* hash, const T& value, const Args&... args) {
    HashCombine(hash, value);
    HashCombine(hash, args...);
}

// Workaround a bug between clang++ and libstdlibc++ by defining our own hashing for bitsets.
// When _GLIBCXX_DEBUG is enabled libstdc++ wraps containers into debug containers. For bitset this
// means what is normally std::bitset is defined as std::__cxx1988::bitset and is replaced by the
// debug version of bitset.
// When hashing, std::hash<std::bitset> proxies the call to std::hash<std::__cxx1998::bitset> and
// fails on clang because the latter tries to access the private _M_getdata member of the bitset.
// It looks like it should work because the non-debug bitset declares
//
//     friend struct std::hash<bitset> // bitset is the name of the class itself
//
// which should friend std::hash<std::__cxx1998::bitset> but somehow doesn't work on clang.
#if defined(_GLIBCXX_DEBUG)
template <size_t N>
size_t Hash(const std::bitset<N>& value) {
    constexpr size_t kWindowSize = sizeof(unsigned long long);

    std::bitset<N> bits = value;
    size_t hash = 0;
    for (size_t processedBits = 0; processedBits < N; processedBits += kWindowSize) {
        HashCombine(&hash, bits.to_ullong());
        bits >>= kWindowSize;
    }

    return hash;
}
#endif

namespace std {
    template <typename Index, size_t N>
    struct hash<ityp::bitset<Index, N>> {
      public:
        size_t operator()(const ityp::bitset<Index, N>& value) const {
            return Hash(static_cast<const std::bitset<N>&>(value));
        }
    };
}  // namespace std

#endif  // COMMON_HASHUTILS_H_
