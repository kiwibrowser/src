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

#ifndef LIBSPIRV_TEXT_H_
#define LIBSPIRV_TEXT_H_

#include <string>

#include "operand.h"
#include "spirv-tools/libspirv.h"
#include "spirv_constant.h"

typedef enum spv_literal_type_t {
  SPV_LITERAL_TYPE_INT_32,
  SPV_LITERAL_TYPE_INT_64,
  SPV_LITERAL_TYPE_UINT_32,
  SPV_LITERAL_TYPE_UINT_64,
  SPV_LITERAL_TYPE_FLOAT_32,
  SPV_LITERAL_TYPE_FLOAT_64,
  SPV_LITERAL_TYPE_STRING,
  SPV_FORCE_32_BIT_ENUM(spv_literal_type_t)
} spv_literal_type_t;

typedef struct spv_literal_t {
  spv_literal_type_t type;
  union value_t {
    int32_t i32;
    int64_t i64;
    uint32_t u32;
    uint64_t u64;
    float f;
    double d;
  } value;
  std::string str;  // Special field for literal string.
} spv_literal_t;

// Converts the given text string to a number/string literal and writes the
// result to *literal. String literals must be surrounded by double-quotes ("),
// which are then stripped.
spv_result_t spvTextToLiteral(const char* text, spv_literal_t* literal);

#endif  // LIBSPIRV_TEXT_H_
