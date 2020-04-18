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

namespace {

// A sampling of word counts.  Covers extreme points well, and all bit
// positions, and some combinations of bit positions.
const uint16_t kSampleWordCounts[] = {
    0,   1,   2,   3,    4,    8,    16,   32,    64,    127,    128,
    256, 511, 512, 1024, 2048, 4096, 8192, 16384, 32768, 0xfffe, 0xffff};

// A sampling of opcode values.  Covers the lower values well, a few samples
// around the number of core instructions (as of this writing), and some
// higher values.
const uint16_t kSampleOpcodes[] = {0,   1,   2,    3,      4,     100,
                                   300, 305, 1023, 0xfffe, 0xffff};

TEST(OpcodeMake, Samples) {
  for (auto wordCount : kSampleWordCounts) {
    for (auto opcode : kSampleOpcodes) {
      uint32_t word = 0;
      word |= uint32_t(opcode);
      word |= uint32_t(wordCount) << 16;
      EXPECT_EQ(word, spvOpcodeMake(wordCount, SpvOp(opcode)));
    }
  }
}

}  // anonymous namespace
