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

#include "UnitSPIRV.h"

#include "TestFixture.h"

namespace {

using spvtest::ScopedContext;

TEST(BinaryDestroy, Null) {
  // There is no state or return value to check. Just check
  // for the ability to call the API without abnormal termination.
  spvBinaryDestroy(nullptr);
}

using BinaryDestroySomething = spvtest::TextToBinaryTest;

// Checks safety of destroying a validly constructed binary.
TEST_F(BinaryDestroySomething, Default) {
  // Use a binary object constructed by the API instead of rolling our own.
  SetText("OpSource OpenCL_C 120");
  spv_binary my_binary = nullptr;
  ASSERT_EQ(SPV_SUCCESS, spvTextToBinary(ScopedContext().context, text.str,
                                         text.length, &my_binary, &diagnostic));
  ASSERT_NE(nullptr, my_binary);
  spvBinaryDestroy(my_binary);
}

}  // anonymous namespace
