// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/rappor/bloom_filter.h"

#include <stdint.h>

#include "components/rappor/byte_vector_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace rappor {

TEST(BloomFilterTest, TinyFilter) {
  BloomFilter filter(1u, 4u, 0u);

  // Size is 1 and it's initially empty
  EXPECT_EQ(1u, filter.bytes().size());
  EXPECT_EQ(0x00, filter.bytes()[0]);

  // "Test" has a self-collision, and only sets 3 bits.
  filter.SetString("Test");
  EXPECT_EQ(0x2a, filter.bytes()[0]);

  // Setting the same value shouldn't change anything.
  filter.SetString("Test");
  EXPECT_EQ(0x2a, filter.bytes()[0]);

  BloomFilter filter2(1u, 4u, 0u);
  EXPECT_EQ(0x00, filter2.bytes()[0]);
  filter2.SetString("Bar");
  EXPECT_EQ(0xa8, filter2.bytes()[0]);

  // The new string should replace the old one.
  filter.SetString("Bar");
  EXPECT_EQ(0xa8, filter.bytes()[0]);
}

TEST(BloomFilterTest, HugeFilter) {
  // Create a 500 bit filter, and use a large seed offset to see if anything
  // breaks.
  BloomFilter filter(500u, 1u, 0xabdef123);

  // Size is 500 and it's initially empty
  EXPECT_EQ(500u, filter.bytes().size());
  EXPECT_EQ(0, CountBits(filter.bytes()));

  filter.SetString("Bar");
  EXPECT_EQ(1, CountBits(filter.bytes()));

  // Adding the same value shouldn't change anything.
  filter.SetString("Bar");
  EXPECT_EQ(1, CountBits(filter.bytes()));
}

TEST(BloomFilterTest, GetBloomBitsSmall) {
  uint64_t bytes_from_get = internal::GetBloomBits(1u, 4u, 0u, "Bar");
  EXPECT_EQ(0xa8u, bytes_from_get);
}

TEST(BloomFilterTest, GetBloomBitsLarge) {
  // Make sure that a 64-bit bloom filter can set the full range of bits.
  uint64_t bytes_from_get = internal::GetBloomBits(8u, 1024u, 0u, "Bar");
  EXPECT_EQ(0xffffffffffffffffu, bytes_from_get);
}

}  // namespace rappor
