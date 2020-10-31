// Copyright 2020 The Dawn Authors
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

#ifndef COMMON_ITYP_BITSET_H_
#define COMMON_ITYP_BITSET_H_

#include "common/BitSetIterator.h"
#include "common/TypedInteger.h"
#include "common/UnderlyingType.h"

namespace ityp {

    // ityp::bitset is a helper class that wraps std::bitset with the restriction that
    // indices must be a particular type |Index|.
    template <typename Index, size_t N>
    class bitset : private std::bitset<N> {
        using I = UnderlyingType<Index>;
        using Base = std::bitset<N>;

        static_assert(sizeof(I) <= sizeof(size_t), "");

        constexpr bitset(const Base& rhs) : Base(rhs) {
        }

      public:
        constexpr bitset() noexcept : Base() {
        }

        constexpr bitset(unsigned long long value) noexcept : Base(value) {
        }

        constexpr bool operator[](Index i) const {
            return Base::operator[](static_cast<I>(i));
        }

        typename Base::reference operator[](Index i) {
            return Base::operator[](static_cast<I>(i));
        }

        bool test(Index i) const {
            return Base::test(static_cast<I>(i));
        }

        using Base::all;
        using Base::any;
        using Base::count;
        using Base::none;
        using Base::size;

        bool operator==(const bitset& other) const noexcept {
            return Base::operator==(static_cast<const Base&>(other));
        }

        bool operator!=(const bitset& other) const noexcept {
            return Base::operator!=(static_cast<const Base&>(other));
        }

        bitset& operator&=(const bitset& other) noexcept {
            return static_cast<bitset&>(Base::operator&=(static_cast<const Base&>(other)));
        }

        bitset& operator|=(const bitset& other) noexcept {
            return static_cast<bitset&>(Base::operator|=(static_cast<const Base&>(other)));
        }

        bitset& operator^=(const bitset& other) noexcept {
            return static_cast<bitset&>(Base::operator^=(static_cast<const Base&>(other)));
        }

        bitset operator~() const noexcept {
            return bitset(*this).flip();
        }

        bitset& set() noexcept {
            return static_cast<bitset&>(Base::set());
        }

        bitset& set(Index i, bool value = true) {
            return static_cast<bitset&>(Base::set(static_cast<I>(i), value));
        }

        bitset& reset() noexcept {
            return static_cast<bitset&>(Base::reset());
        }

        bitset& reset(Index i) {
            return static_cast<bitset&>(Base::reset(static_cast<I>(i)));
        }

        bitset& flip() noexcept {
            return static_cast<bitset&>(Base::flip());
        }

        bitset& flip(Index i) {
            return static_cast<bitset&>(Base::flip(static_cast<I>(i)));
        }

        using Base::to_string;
        using Base::to_ullong;
        using Base::to_ulong;

        friend bitset operator&(const bitset& lhs, const bitset& rhs) noexcept {
            return bitset(static_cast<const Base&>(lhs) & static_cast<const Base&>(rhs));
        }

        friend bitset operator|(const bitset& lhs, const bitset& rhs) noexcept {
            return bitset(static_cast<const Base&>(lhs) | static_cast<const Base&>(rhs));
        }

        friend bitset operator^(const bitset& lhs, const bitset& rhs) noexcept {
            return bitset(static_cast<const Base&>(lhs) ^ static_cast<const Base&>(rhs));
        }

        friend BitSetIterator<N, Index> IterateBitSet(const bitset& bitset) {
            return BitSetIterator<N, Index>(static_cast<const Base&>(bitset));
        }

        friend struct std::hash<bitset>;
    };

}  // namespace ityp

#endif  // COMMON_ITYP_BITSET_H_
