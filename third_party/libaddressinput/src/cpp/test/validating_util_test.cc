// Copyright (C) 2013 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "validating_util.h"

#include <string>

#include <gtest/gtest.h>

#define ITOA_HELPER(i) #i
#define ITOA(i) ITOA_HELPER(i)

#define DATA "{'foo': 'bar'}"
#define TIMESTAMP 1388001600
#define TIMESTAMP_HALF_MONTH_AGO 1386705600
#define TIMESTAMP_TWO_MONTHS_AGO 1382817600
#define CHECKSUM "dd63dafcbd4d5b28badfcaf86fb6fcdb"

namespace {

using i18n::addressinput::ValidatingUtil;

// The data being wrapped.
const char kUnwrappedData[] = DATA;

// The timestamp for the wrapped data.
const time_t kTimestamp = TIMESTAMP;

// The checksum and data together.
const char kChecksummedData[] = "checksum=" CHECKSUM "\n"
                                DATA;

// "Randomly" corrupted checksummed data. The "m" in "checksum" is capitalized.
const char kCorruptedChecksummedData[] = "checksuM=" CHECKSUM "\n"
                                         DATA;

// The checksum in the middle of data.
const char kChecksumInMiddle[] = DATA "\n"
                                 "checksum=" CHECKSUM "\n"
                                 DATA;

// The file as it is stored on disk.
const char kWrappedData[] = "timestamp=" ITOA(TIMESTAMP) "\n"
                            "checksum=" CHECKSUM "\n"
                            DATA;

// "Randomly" corrupted file. The "p" in "timestamp" is capitalized.
const char kCorruptedWrappedData[] = "timestamP=" ITOA(TIMESTAMP) "\n"
                                     "checksum=" CHECKSUM "\n"
                                     DATA;

// The timestamp in the middle of data.
const char kTimestampInMiddle[] = DATA "\n"
                                  "timestamp=" ITOA(TIMESTAMP) "\n"
                                  DATA;

// A recent timestamp and data together.
const char kTimestampHalfMonthAgo[] =
    "timestamp=" ITOA(TIMESTAMP_HALF_MONTH_AGO) "\n"
    DATA;

// A stale timestamp and data together.
const char kTimestampTwoMonthsAgo[] =
    "timestamp=" ITOA(TIMESTAMP_TWO_MONTHS_AGO) "\n"
    DATA;

TEST(ValidatingUtilTest, UnwrapChecksum_CorruptedData) {
  std::string data(kCorruptedChecksummedData);
  EXPECT_FALSE(ValidatingUtil::UnwrapChecksum(&data));
}

TEST(ValidatingUtilTest, UnwrapChecksum_EmptyString) {
  std::string data;
  EXPECT_FALSE(ValidatingUtil::UnwrapChecksum(&data));
}

TEST(ValidatingUtilTest, UnwrapChecksum_GarbageData) {
  std::string data("garbage");
  EXPECT_FALSE(ValidatingUtil::UnwrapChecksum(&data));
}

TEST(ValidatingUtilTest, UnwrapChecksum_InMiddle) {
  std::string data(kChecksumInMiddle);
  EXPECT_FALSE(ValidatingUtil::UnwrapChecksum(&data));
}

TEST(ValidatingUtilTest, UnwrapChecksum) {
  std::string data(kChecksummedData);
  EXPECT_TRUE(ValidatingUtil::UnwrapChecksum(&data));
  EXPECT_EQ(kUnwrappedData, data);
}

TEST(ValidatingUtilTest, UnwrapTimestamp_CorruptedData) {
  std::string data(kCorruptedWrappedData);
  EXPECT_FALSE(ValidatingUtil::UnwrapTimestamp(&data, kTimestamp));
}

TEST(ValidatingUtilTest, UnwrapTimestamp_EmptyString) {
  std::string data;
  EXPECT_FALSE(ValidatingUtil::UnwrapTimestamp(&data, kTimestamp));
}

TEST(ValidatingUtilTest, UnwrapTimestamp_GarbageData) {
  std::string data("garbage");
  EXPECT_FALSE(ValidatingUtil::UnwrapTimestamp(&data, kTimestamp));
}

TEST(ValidatingUtilTest, UnwrapTimestamp_InMiddle) {
  std::string data(kTimestampInMiddle);
  EXPECT_FALSE(ValidatingUtil::UnwrapTimestamp(&data, kTimestamp));
}

TEST(ValidatingUtilTest, UnwrapTimestamp_Recent) {
  std::string data(kTimestampHalfMonthAgo);
  EXPECT_TRUE(ValidatingUtil::UnwrapTimestamp(&data, kTimestamp));
  EXPECT_EQ(kUnwrappedData, data);
}

TEST(ValidatingUtilTest, UnwrapTimestamp_Stale) {
  std::string data(kTimestampTwoMonthsAgo);
  EXPECT_FALSE(ValidatingUtil::UnwrapTimestamp(&data, kTimestamp));
}

TEST(ValidatingUtilTest, UnwrapTimestamp) {
  std::string data(kWrappedData);
  EXPECT_TRUE(ValidatingUtil::UnwrapTimestamp(&data, kTimestamp));
  EXPECT_EQ(kChecksummedData, data);
}

TEST(ValidatingUtilTest, Wrap) {
  std::string data = kUnwrappedData;
  ValidatingUtil::Wrap(kTimestamp, &data);
  EXPECT_EQ(kWrappedData, data);
}

TEST(ValidatingUtilTest, WrapUnwrapIt) {
  std::string data = kUnwrappedData;
  ValidatingUtil::Wrap(kTimestamp, &data);
  EXPECT_TRUE(ValidatingUtil::UnwrapTimestamp(&data, kTimestamp));
  EXPECT_EQ(kChecksummedData, data);
  EXPECT_TRUE(ValidatingUtil::UnwrapChecksum(&data));
  EXPECT_EQ(kUnwrappedData, data);
}

}  // namespace
