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

#ifndef LIBSPIRV_SPIRV_DEFINITION_H_
#define LIBSPIRV_SPIRV_DEFINITION_H_

#include <cstdint>

#include "spirv-tools/libspirv.h"

#define spvIsInBitfield(value, bitfield) ((value) == ((value)&bitfield))

// A bit mask representing a set of capabilities.
// Currently there are 60 distinct capabilities, so 64 bits
// should be enough.
typedef uint64_t spv_capability_mask_t;

// Transforms spv::Capability into a mask for use in bitfields.  Should really
// be a constexpr inline function, but some important versions of MSVC don't
// support that yet.  Different from SPV_BIT, which doesn't guarantee 64-bit
// values.
#define SPV_CAPABILITY_AS_MASK(capability) \
  (spv_capability_mask_t(1) << (capability))

// Applies f to every capability present in a mask.
namespace libspirv {
template <typename Functor>
inline void ForEach(spv_capability_mask_t capabilities, Functor f) {
  for (int c = 0; capabilities; ++c, capabilities >>= 1)
    if (capabilities & 1) f(static_cast<SpvCapability>(c));
}
}  // end namespace libspirv

typedef struct spv_header_t {
  uint32_t magic;
  uint32_t version;
  uint32_t generator;
  uint32_t bound;
  uint32_t schema;               // NOTE: Reserved
  const uint32_t* instructions;  // NOTE: Unfixed pointer to instruciton stream
} spv_header_t;

#endif  // LIBSPIRV_SPIRV_DEFINITION_H_
