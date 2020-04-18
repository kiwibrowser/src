// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include "media/filters/h264_bitstream_buffer.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media {

namespace {
const uint64_t kTestPattern = 0xfedcba0987654321;
}

class H264BitstreamBufferAppendBitsTest
    : public ::testing::TestWithParam<uint64_t> {};

// TODO(posciak): More tests!

TEST_P(H264BitstreamBufferAppendBitsTest, AppendAndVerifyBits) {
  auto b = base::MakeRefCounted<H264BitstreamBuffer>();
  uint64_t num_bits = GetParam();
  // TODO(posciak): Tests for >64 bits.
  ASSERT_LE(num_bits, 64u);
  uint64_t num_bytes = (num_bits + 7) / 8;

  b->AppendBits(num_bits, kTestPattern);
  b->FlushReg();

  EXPECT_EQ(b->BytesInBuffer(), num_bytes);

  const uint8_t* ptr = b->data();
  uint64_t got = 0;
  uint64_t expected = kTestPattern;

  if (num_bits < 64)
    expected &= ((1ull << num_bits) - 1);

  while (num_bits > 8) {
    got |= (*ptr & 0xff);
    num_bits -= 8;
    got <<= (num_bits > 8 ? 8 : num_bits);
    ptr++;
  }
  if (num_bits > 0) {
    uint64_t temp = (*ptr & 0xff);
    temp >>= (8 - num_bits);
    got |= temp;
  }
  EXPECT_EQ(got, expected) << std::hex << "0x" << got << " vs 0x" << expected;
}

INSTANTIATE_TEST_CASE_P(AppendNumBits,
                        H264BitstreamBufferAppendBitsTest,
                        ::testing::Range(static_cast<uint64_t>(1),
                                         static_cast<uint64_t>(65)));
}  // namespace media
