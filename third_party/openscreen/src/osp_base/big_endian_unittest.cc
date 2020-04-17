// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp_base/big_endian.h"

#include "third_party/googletest/src/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

namespace openscreen {
namespace {

// Tests that ReadBigEndian() correctly imports values from various offsets in
// memory.
TEST(BigEndianTest, ReadValues) {
  const uint8_t kInput[] = {
      0,    1,    2,    3,    4,    5,    6,    7,    8,    9,    0xa,
      0xb,  0xc,  0xd,  0xe,  0xf,  0xff, 0xff, 0xfe, 0xff, 0xff, 0xff,
      0xff, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe,
  };

  EXPECT_EQ(UINT8_C(0x05), ReadBigEndian<uint8_t>(kInput + 5));
  EXPECT_EQ(UINT8_C(0xff), ReadBigEndian<uint8_t>(kInput + 16));
  EXPECT_EQ(7, ReadBigEndian<int8_t>(kInput + 7));
  EXPECT_EQ(-1, ReadBigEndian<int8_t>(kInput + 17));

  EXPECT_EQ(UINT16_C(0x0001), ReadBigEndian<uint16_t>(kInput));
  EXPECT_EQ(UINT16_C(0x0102), ReadBigEndian<uint16_t>(kInput + 1));
  EXPECT_EQ(UINT16_C(0x0203), ReadBigEndian<uint16_t>(kInput + 2));
  EXPECT_EQ(-1, ReadBigEndian<int16_t>(kInput + 16));
  EXPECT_EQ(-2, ReadBigEndian<int16_t>(kInput + 17));

  EXPECT_EQ(UINT32_C(0x03040506), ReadBigEndian<uint32_t>(kInput + 3));
  EXPECT_EQ(UINT32_C(0x04050607), ReadBigEndian<uint32_t>(kInput + 4));
  EXPECT_EQ(UINT32_C(0x05060708), ReadBigEndian<uint32_t>(kInput + 5));
  EXPECT_EQ(-1, ReadBigEndian<int32_t>(kInput + 19));
  EXPECT_EQ(-2, ReadBigEndian<int32_t>(kInput + 20));

  EXPECT_EQ(UINT64_C(0x0001020304050607), ReadBigEndian<uint64_t>(kInput));
  EXPECT_EQ(UINT64_C(0x0102030405060708), ReadBigEndian<uint64_t>(kInput + 1));
  EXPECT_EQ(UINT64_C(0x0203040506070809), ReadBigEndian<uint64_t>(kInput + 2));
  EXPECT_EQ(-1, ReadBigEndian<int64_t>(kInput + 24));
  EXPECT_EQ(-2, ReadBigEndian<int64_t>(kInput + 25));
}

// Tests that WriteBigEndian() correctly writes-out values to various offsets in
// memory. This test assumes ReadBigEndian() is working, using it to verify that
// WriteBigEndian() is working.
TEST(BigEndianTest, WriteValues) {
  uint8_t scratch[16];

  WriteBigEndian<uint8_t>(0x07, scratch);
  EXPECT_EQ(UINT8_C(0x07), ReadBigEndian<uint8_t>(scratch));
  WriteBigEndian<uint8_t>(0xf0, scratch + 1);
  EXPECT_EQ(UINT8_C(0xf0), ReadBigEndian<uint8_t>(scratch + 1));
  WriteBigEndian<int8_t>(23, scratch + 2);
  EXPECT_EQ(23, ReadBigEndian<int8_t>(scratch + 2));
  WriteBigEndian<int8_t>(-25, scratch + 3);
  EXPECT_EQ(-25, ReadBigEndian<int8_t>(scratch + 3));

  WriteBigEndian<uint16_t>(0x0102, scratch);
  EXPECT_EQ(UINT16_C(0x0102), ReadBigEndian<uint16_t>(scratch));
  WriteBigEndian<uint16_t>(0x0304, scratch + 1);
  EXPECT_EQ(UINT16_C(0x0304), ReadBigEndian<uint16_t>(scratch + 1));
  WriteBigEndian<uint16_t>(0x0506, scratch + 2);
  EXPECT_EQ(UINT16_C(0x0506), ReadBigEndian<uint16_t>(scratch + 2));
  WriteBigEndian<int16_t>(42, scratch + 3);
  EXPECT_EQ(42, ReadBigEndian<int16_t>(scratch + 3));
  WriteBigEndian<int16_t>(-1, scratch + 4);
  EXPECT_EQ(-1, ReadBigEndian<int16_t>(scratch + 4));
  WriteBigEndian<int16_t>(-2, scratch + 5);
  EXPECT_EQ(-2, ReadBigEndian<int16_t>(scratch + 5));

  WriteBigEndian<uint32_t>(UINT32_C(0x03040506), scratch);
  EXPECT_EQ(UINT32_C(0x03040506), ReadBigEndian<uint32_t>(scratch));
  WriteBigEndian<uint32_t>(UINT32_C(0x0708090a), scratch + 1);
  EXPECT_EQ(UINT32_C(0x0708090a), ReadBigEndian<uint32_t>(scratch + 1));
  WriteBigEndian<uint32_t>(UINT32_C(0x0b0c0d0e), scratch + 2);
  EXPECT_EQ(UINT32_C(0x0b0c0d0e), ReadBigEndian<uint32_t>(scratch + 2));
  WriteBigEndian<int32_t>(42, scratch + 3);
  EXPECT_EQ(42, ReadBigEndian<int32_t>(scratch + 3));
  WriteBigEndian<int32_t>(-1, scratch + 4);
  EXPECT_EQ(-1, ReadBigEndian<int32_t>(scratch + 4));
  WriteBigEndian<int32_t>(-2, scratch + 5);
  EXPECT_EQ(-2, ReadBigEndian<int32_t>(scratch + 5));

  WriteBigEndian<uint64_t>(UINT64_C(0x0f0e0d0c0b0a0908), scratch);
  EXPECT_EQ(UINT64_C(0x0f0e0d0c0b0a0908), ReadBigEndian<uint64_t>(scratch));
  WriteBigEndian<uint64_t>(UINT64_C(0x0708090a0b0c0d0e), scratch + 1);
  EXPECT_EQ(UINT64_C(0x0708090a0b0c0d0e), ReadBigEndian<uint64_t>(scratch + 1));
  WriteBigEndian<uint64_t>(UINT64_C(0x99aa88bb77cc66dd), scratch + 2);
  EXPECT_EQ(UINT64_C(0x99aa88bb77cc66dd), ReadBigEndian<uint64_t>(scratch + 2));
  WriteBigEndian<int64_t>(42, scratch + 3);
  EXPECT_EQ(42, ReadBigEndian<int64_t>(scratch + 3));
  WriteBigEndian<int64_t>(-1, scratch + 4);
  EXPECT_EQ(-1, ReadBigEndian<int64_t>(scratch + 4));
  WriteBigEndian<int64_t>(-2, scratch + 5);
  EXPECT_EQ(-2, ReadBigEndian<int64_t>(scratch + 5));
}

TEST(BigEndianReaderTest, ConstructWithValidBuffer) {
  uint8_t data[64];
  BigEndianReader reader(data, sizeof(data));

  EXPECT_EQ(reader.begin(), data);
  EXPECT_EQ(reader.current(), data);
  EXPECT_EQ(reader.end(), data + 64);
  EXPECT_EQ(reader.offset(), size_t(0));
  EXPECT_EQ(reader.remaining(), size_t(64));
  EXPECT_EQ(reader.length(), size_t(64));
}

TEST(BigEndianReaderTest, SkipLessThanRemaining) {
  uint8_t data[64];
  BigEndianReader reader(data, sizeof(data));

  EXPECT_TRUE(reader.Skip(16));

  EXPECT_EQ(reader.begin(), data);
  EXPECT_EQ(reader.current(), data + 16);
  EXPECT_EQ(reader.end(), data + 64);
  EXPECT_EQ(reader.offset(), size_t(16));
  EXPECT_EQ(reader.remaining(), size_t(48));
  EXPECT_EQ(reader.length(), size_t(64));
}

TEST(BigEndianReaderTest, SkipMoreThanRemaining) {
  uint8_t data[64];
  BigEndianReader reader(data, sizeof(data));

  EXPECT_TRUE(reader.Skip(16));
  EXPECT_FALSE(reader.Skip(64));

  // Check that failed Skip does not modify any pointers or offsets.
  EXPECT_EQ(reader.begin(), data);
  EXPECT_EQ(reader.current(), data + 16);
  EXPECT_EQ(reader.end(), data + 64);
  EXPECT_EQ(reader.offset(), size_t(16));
  EXPECT_EQ(reader.remaining(), size_t(48));
  EXPECT_EQ(reader.length(), size_t(64));
}

TEST(BigEndianReaderTest, ConstructWithZeroLengthBuffer) {
  uint8_t data[8];
  BigEndianReader reader(data, 0);

  EXPECT_EQ(reader.begin(), data);
  EXPECT_EQ(reader.current(), data);
  EXPECT_EQ(reader.end(), data);
  EXPECT_EQ(reader.offset(), size_t(0));
  EXPECT_EQ(reader.remaining(), size_t(0));
  EXPECT_EQ(reader.length(), size_t(0));

  EXPECT_FALSE(reader.Skip(1));
}

TEST(BigEndianReaderTest, ReadValues) {
  uint8_t data[17] = {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8,
                      0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF, 0x10};
  BigEndianReader reader(data, sizeof(data));

  uint8_t buffer[2];
  EXPECT_TRUE(reader.ReadBytes(sizeof(buffer), buffer));
  EXPECT_EQ(buffer[0], UINT8_C(0x0));
  EXPECT_EQ(buffer[1], UINT8_C(0x1));

  uint8_t u8;
  EXPECT_TRUE(reader.Read<uint8_t>(&u8));
  EXPECT_EQ(u8, UINT8_C(0x2));

  uint16_t u16;
  EXPECT_TRUE(reader.Read<uint16_t>(&u16));
  EXPECT_EQ(u16, UINT16_C(0x0304));

  uint32_t u32;
  EXPECT_TRUE(reader.Read<uint32_t>(&u32));
  EXPECT_EQ(u32, UINT32_C(0x05060708));

  uint64_t u64;
  EXPECT_TRUE(reader.Read<uint64_t>(&u64));
  EXPECT_EQ(u64, UINT64_C(0x090A0B0C0D0E0F10));

  EXPECT_EQ(reader.begin(), data);
  EXPECT_EQ(reader.current(), data + 17);
  EXPECT_EQ(reader.end(), data + 17);
  EXPECT_EQ(reader.offset(), size_t(17));
  EXPECT_EQ(reader.remaining(), size_t(0));
  EXPECT_EQ(reader.length(), size_t(17));
}

TEST(BigEndianReaderTest, RespectLength) {
  uint8_t data[8];
  BigEndianReader reader(data, sizeof(data));

  // 8 left
  EXPECT_FALSE(reader.Skip(9));
  EXPECT_TRUE(reader.Skip(1));

  // 7 left
  uint64_t u64;
  EXPECT_FALSE(reader.Read<uint64_t>(&u64));
  EXPECT_TRUE(reader.Skip(4));

  // 3 left
  uint32_t u32;
  EXPECT_FALSE(reader.Read<uint32_t>(&u32));
  EXPECT_TRUE(reader.Skip(2));

  // 1 left
  uint16_t u16;
  EXPECT_FALSE(reader.Read<uint16_t>(&u16));

  uint8_t buffer[2];
  EXPECT_FALSE(reader.ReadBytes(2, buffer));
  EXPECT_TRUE(reader.Skip(1));

  // 0 left
  uint8_t u8;
  EXPECT_FALSE(reader.Read<uint8_t>(&u8));

  EXPECT_EQ(reader.begin(), data);
  EXPECT_EQ(reader.current(), data + 8);
  EXPECT_EQ(reader.end(), data + 8);
  EXPECT_EQ(reader.offset(), size_t(8));
  EXPECT_EQ(reader.remaining(), size_t(0));
  EXPECT_EQ(reader.length(), size_t(8));
}

TEST(BigEndianWriterTest, ConstructWithValidBuffer) {
  uint8_t data[64];
  BigEndianWriter writer(data, sizeof(data));

  EXPECT_EQ(writer.begin(), data);
  EXPECT_EQ(writer.current(), data);
  EXPECT_EQ(writer.end(), data + 64);
  EXPECT_EQ(writer.offset(), size_t(0));
  EXPECT_EQ(writer.remaining(), size_t(64));
  EXPECT_EQ(writer.length(), size_t(64));
}

TEST(BigEndianWriterTest, SkipLessThanRemaining) {
  uint8_t data[64];
  BigEndianWriter writer(data, sizeof(data));

  EXPECT_TRUE(writer.Skip(16));

  EXPECT_EQ(writer.begin(), data);
  EXPECT_EQ(writer.current(), data + 16);
  EXPECT_EQ(writer.end(), data + 64);
  EXPECT_EQ(writer.offset(), size_t(16));
  EXPECT_EQ(writer.remaining(), size_t(48));
  EXPECT_EQ(writer.length(), size_t(64));
}

TEST(BigEndianWriterTest, SkipMoreThanRemaining) {
  uint8_t data[64];
  BigEndianWriter writer(data, sizeof(data));

  EXPECT_TRUE(writer.Skip(16));
  EXPECT_FALSE(writer.Skip(64));

  // Check that failed Skip does not modify any pointers or offsets.
  EXPECT_EQ(writer.begin(), data);
  EXPECT_EQ(writer.current(), data + 16);
  EXPECT_EQ(writer.end(), data + 64);
  EXPECT_EQ(writer.offset(), size_t(16));
  EXPECT_EQ(writer.remaining(), size_t(48));
  EXPECT_EQ(writer.length(), size_t(64));
}

TEST(BigEndianWriterTest, ConstructWithZeroLengthBuffer) {
  uint8_t data[8];
  BigEndianWriter writer(data, 0);

  EXPECT_EQ(writer.begin(), data);
  EXPECT_EQ(writer.current(), data);
  EXPECT_EQ(writer.end(), data);
  EXPECT_EQ(writer.offset(), size_t(0));
  EXPECT_EQ(writer.remaining(), size_t(0));
  EXPECT_EQ(writer.length(), size_t(0));

  EXPECT_FALSE(writer.Skip(1));
}

TEST(BigEndianWriterTest, WriteValues) {
  uint8_t expected[17] = {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8,
                          0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF, 0x10};

  uint8_t data[17];
  memset(data, 0xFF, sizeof(data));
  BigEndianWriter writer(data, sizeof(data));

  uint8_t buffer[] = {0x0, 0x1};
  EXPECT_TRUE(writer.WriteBytes(buffer, sizeof(buffer)));
  EXPECT_TRUE(writer.Write<uint8_t>(UINT8_C(0x2)));
  EXPECT_TRUE(writer.Write<uint16_t>(UINT16_C(0x0304)));
  EXPECT_TRUE(writer.Write<uint32_t>(UINT32_C(0x05060708)));
  EXPECT_TRUE(writer.Write<uint64_t>(UINT64_C(0x090A0B0C0D0E0F10)));
  EXPECT_THAT(data, testing::ElementsAreArray(expected));

  EXPECT_EQ(writer.begin(), data);
  EXPECT_EQ(writer.current(), data + 17);
  EXPECT_EQ(writer.end(), data + 17);
  EXPECT_EQ(writer.offset(), size_t(17));
  EXPECT_EQ(writer.remaining(), size_t(0));
  EXPECT_EQ(writer.length(), size_t(17));
}

TEST(BigEndianWriterTest, RespectLength) {
  uint8_t data[8];
  BigEndianWriter writer(data, sizeof(data));

  // 8 left
  EXPECT_FALSE(writer.Skip(9));
  EXPECT_TRUE(writer.Skip(1));

  // 7 left
  EXPECT_FALSE(writer.Write<uint64_t>(0));
  EXPECT_TRUE(writer.Skip(4));

  // 3 left
  EXPECT_FALSE(writer.Write<uint32_t>(0));
  EXPECT_TRUE(writer.Skip(2));

  // 1 left
  EXPECT_FALSE(writer.Write<uint16_t>(0));

  uint8_t buffer[2];
  EXPECT_FALSE(writer.WriteBytes(buffer, 2));
  EXPECT_TRUE(writer.Skip(1));

  // 0 left
  EXPECT_FALSE(writer.Write<uint8_t>(0));
  EXPECT_EQ(0u, writer.remaining());

  EXPECT_EQ(writer.begin(), data);
  EXPECT_EQ(writer.current(), data + 8);
  EXPECT_EQ(writer.end(), data + 8);
  EXPECT_EQ(writer.offset(), size_t(8));
  EXPECT_EQ(writer.remaining(), size_t(0));
  EXPECT_EQ(writer.length(), size_t(8));
}

}  // namespace
}  // namespace openscreen
