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

#ifndef COMMON_UNDERLYINGTYPE_H_
#define COMMON_UNDERLYINGTYPE_H_

#include <type_traits>

// UnderlyingType is similar to std::underlying_type_t. It is a passthrough for already
// integer types which simplifies getting the underlying primitive type for an arbitrary
// template parameter. It includes a specialization for detail::TypedIntegerImpl which yields
// the wrapped integer type.
namespace detail {
    template <typename T, typename Enable = void>
    struct UnderlyingTypeImpl;

    template <typename I>
    struct UnderlyingTypeImpl<I, typename std::enable_if_t<std::is_integral<I>::value>> {
        using type = I;
    };

    template <typename E>
    struct UnderlyingTypeImpl<E, typename std::enable_if_t<std::is_enum<E>::value>> {
        using type = std::underlying_type_t<E>;
    };

    // Forward declare the TypedInteger impl.
    template <typename Tag, typename T>
    class TypedIntegerImpl;

    template <typename Tag, typename I>
    struct UnderlyingTypeImpl<TypedIntegerImpl<Tag, I>> {
        using type = typename UnderlyingTypeImpl<I>::type;
    };
}  // namespace detail

template <typename T>
using UnderlyingType = typename detail::UnderlyingTypeImpl<T>::type;

#endif  // COMMON_UNDERLYINGTYPE_H_
