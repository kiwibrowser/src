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

#include "spirv_endian.h"

#include <cstring>

enum {
  I32_ENDIAN_LITTLE = 0x03020100ul,
  I32_ENDIAN_BIG = 0x00010203ul,
};

// This constant value allows the detection of the host machine's endianness.
// Accessing it through the "value" member is valid due to C++11 section 3.10
// paragraph 10.
static const union {
  unsigned char bytes[4];
  uint32_t value;
} o32_host_order = {{0, 1, 2, 3}};

#define I32_ENDIAN_HOST (o32_host_order.value)

uint32_t spvFixWord(const uint32_t word, const spv_endianness_t endian) {
  if ((SPV_ENDIANNESS_LITTLE == endian && I32_ENDIAN_HOST == I32_ENDIAN_BIG) ||
      (SPV_ENDIANNESS_BIG == endian && I32_ENDIAN_HOST == I32_ENDIAN_LITTLE)) {
    return (word & 0x000000ff) << 24 | (word & 0x0000ff00) << 8 |
           (word & 0x00ff0000) >> 8 | (word & 0xff000000) >> 24;
  }

  return word;
}

uint64_t spvFixDoubleWord(const uint32_t low, const uint32_t high,
                          const spv_endianness_t endian) {
  return (uint64_t(spvFixWord(high, endian)) << 32) | spvFixWord(low, endian);
}

spv_result_t spvBinaryEndianness(spv_const_binary binary,
                                 spv_endianness_t* pEndian) {
  if (!binary->code || !binary->wordCount) return SPV_ERROR_INVALID_BINARY;
  if (!pEndian) return SPV_ERROR_INVALID_POINTER;

  uint8_t bytes[4];
  memcpy(bytes, binary->code, sizeof(uint32_t));

  if (0x03 == bytes[0] && 0x02 == bytes[1] && 0x23 == bytes[2] &&
      0x07 == bytes[3]) {
    *pEndian = SPV_ENDIANNESS_LITTLE;
    return SPV_SUCCESS;
  }

  if (0x07 == bytes[0] && 0x23 == bytes[1] && 0x02 == bytes[2] &&
      0x03 == bytes[3]) {
    *pEndian = SPV_ENDIANNESS_BIG;
    return SPV_SUCCESS;
  }

  return SPV_ERROR_INVALID_BINARY;
}

bool spvIsHostEndian(spv_endianness_t endian) {
  return ((SPV_ENDIANNESS_LITTLE == endian) &&
          (I32_ENDIAN_LITTLE == I32_ENDIAN_HOST)) ||
         ((SPV_ENDIANNESS_BIG == endian) &&
          (I32_ENDIAN_BIG == I32_ENDIAN_HOST));
}
