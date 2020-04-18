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

#include "gmock/gmock.h"
#include "TestFixture.h"

namespace {

using spvtest::MakeVector;
using ::testing::Eq;
using Words = std::vector<uint32_t>;

TEST(MakeVector, Samples) {
  EXPECT_THAT(MakeVector(""), Eq(Words{0}));
  EXPECT_THAT(MakeVector("a"), Eq(Words{0x0061}));
  EXPECT_THAT(MakeVector("ab"), Eq(Words{0x006261}));
  EXPECT_THAT(MakeVector("abc"), Eq(Words{0x00636261}));
  EXPECT_THAT(MakeVector("abcd"), Eq(Words{0x64636261, 0x00}));
  EXPECT_THAT(MakeVector("abcde"), Eq(Words{0x64636261, 0x0065}));
}

TEST(WordVectorPrintTo, PreservesFlagsAndFill) {
  std::stringstream s;
  s << std::setw(4) << std::oct << std::setfill('x') << 8 << " ";
  spvtest::PrintTo(spvtest::WordVector({10, 16}), &s);
  // The octal setting and fill character should be preserved
  // from before the PrintTo.
  // Width is reset after each emission of a regular scalar type.
  // So set it explicitly again.
  s << std::setw(4) << 9;

  EXPECT_THAT(s.str(), Eq("xx10 0x0000000a 0x00000010 xx11"));
}

TEST_P(RoundTripTest, Sample) {
  EXPECT_THAT(EncodeAndDecodeSuccessfully(GetParam()), Eq(GetParam()))
      << GetParam();
}

}  // anonymous namespace
