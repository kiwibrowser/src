// Copyright (c) 2015-2016 The Khronos Group Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and/or associated documentation files (the
// "Materials"), to deal in the Materials without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Materials, and to
// permit persons to whom the Materials are furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Materials.
//
// MODIFICATIONS TO THIS FILE MAY MEAN IT NO LONGER ACCURATELY REFLECTS
// KHRONOS STANDARDS. THE UNMODIFIED, NORMATIVE VERSIONS OF KHRONOS
// SPECIFICATIONS AND HEADER INFORMATION ARE LOCATED AT
//    https://www.khronos.org/registry/
//
// THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.

#ifndef LIBSPIRV_UTIL_BITUTILS_H_
#define LIBSPIRV_UTIL_BITUTILS_H_

#include <cstdint>
#include <cstring>

namespace spvutils {

// Performs a bitwise copy of source to the destination type Dest.
template <typename Dest, typename Src>
Dest BitwiseCast(Src source) {
  Dest dest;
  static_assert(sizeof(source) == sizeof(dest),
                "BitwiseCast: Source and destination must have the same size");
  std::memcpy(&dest, &source, sizeof(dest));
  return dest;
}

// SetBits<T, First, Num> returns an integer of type <T> with bits set
// for position <First> through <First + Num - 1>, counting from the least
// significant bit. In particular when Num == 0, no positions are set to 1.
// A static assert will be triggered if First + Num > sizeof(T) * 8, that is,
// a bit that will not fit in the underlying type is set.
template <typename T, size_t First = 0, size_t Num = 0>
struct SetBits {
  static_assert(First < sizeof(T) * 8,
                "Tried to set a bit that is shifted too far.");
  const static T get = (T(1) << First) | SetBits<T, First + 1, Num - 1>::get;
};

template <typename T, size_t Last>
struct SetBits<T, Last, 0> {
  const static T get = T(0);
};

// This is all compile-time so we can put our tests right here.
static_assert(SetBits<uint32_t, 0, 0>::get == uint32_t(0x00000000),
              "SetBits failed");
static_assert(SetBits<uint32_t, 0, 1>::get == uint32_t(0x00000001),
              "SetBits failed");
static_assert(SetBits<uint32_t, 31, 1>::get == uint32_t(0x80000000),
              "SetBits failed");
static_assert(SetBits<uint32_t, 1, 2>::get == uint32_t(0x00000006),
              "SetBits failed");
static_assert(SetBits<uint32_t, 30, 2>::get == uint32_t(0xc0000000),
              "SetBits failed");
static_assert(SetBits<uint32_t, 0, 31>::get == uint32_t(0x7FFFFFFF),
              "SetBits failed");
static_assert(SetBits<uint32_t, 0, 32>::get == uint32_t(0xFFFFFFFF),
              "SetBits failed");
static_assert(SetBits<uint32_t, 16, 16>::get == uint32_t(0xFFFF0000),
              "SetBits failed");

static_assert(SetBits<uint64_t, 0, 1>::get == uint64_t(0x0000000000000001LL),
              "SetBits failed");
static_assert(SetBits<uint64_t, 63, 1>::get == uint64_t(0x8000000000000000LL),
              "SetBits failed");
static_assert(SetBits<uint64_t, 62, 2>::get == uint64_t(0xc000000000000000LL),
              "SetBits failed");
static_assert(SetBits<uint64_t, 31, 1>::get == uint64_t(0x0000000080000000LL),
              "SetBits failed");
static_assert(SetBits<uint64_t, 16, 16>::get == uint64_t(0x00000000FFFF0000LL),
              "SetBits failed");

}  // namespace spvutils

#endif  // LIBSPIRV_UTIL_BITUTILS_H_
