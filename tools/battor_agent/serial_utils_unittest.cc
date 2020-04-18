// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/battor_agent/serial_utils.h"

#include "testing/gtest/include/gtest/gtest.h"

using namespace testing;

namespace battor {

TEST(SerialUtilsTest, ByteVectorToStringLengthZero) {
  EXPECT_EQ("", ByteVectorToString(std::vector<uint8_t>()));
}

TEST(SerialUtilsTest, ByteVectorToStringLengthOne) {
  EXPECT_EQ("0x41", ByteVectorToString(std::vector<uint8_t>({'A'})));
}

TEST(SerialUtilsTest, ByteVectorToStringLengthTwo) {
  EXPECT_EQ("0x41 0x4a", ByteVectorToString(std::vector<uint8_t>({'A', 'J'})));
}

TEST(SerialUtilsTest, CharArrayToStringLengthOne) {
  const char arr[] = {'A'};
  EXPECT_EQ("0x41", CharArrayToString(arr, sizeof(arr)));
}

TEST(SerialUtilsTest, CharArrayToStringLengthTwo) {
  const char arr[] = {'A', 'J'};
  EXPECT_EQ("0x41 0x4a", CharArrayToString(arr, sizeof(arr)));
}

}  // namespace battor
