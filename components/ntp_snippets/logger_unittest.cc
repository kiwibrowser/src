// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_snippets/logger.h"

#include "base/location.h"
#include "base/strings/string_number_conversions.h"
#include "components/ntp_snippets/features.h"
#include "components/variations/variations_params_manager.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ntp_snippets {

namespace {

const int kMaxItemsCount = 1000;

}  // namespace

class LoggerTest : public ::testing::Test {
 public:
  LoggerTest() = default;
  ~LoggerTest() override = default;

  void EnableFeature() {
    params_manager_.SetVariationParamsWithFeatureAssociations(
        kContentSuggestionsDebugLog.name,
        {{"max_items_count", base::IntToString(kMaxItemsCount)}},
        {kContentSuggestionsDebugLog.name});
  }

 private:
  variations::testing::VariationParamsManager params_manager_;
};

TEST_F(LoggerTest, ShouldNotLogWhenNotEnabled) {
  Logger logger;
  logger.Log(FROM_HERE, /*message=*/"test");
  EXPECT_EQ("", logger.GetHumanReadableLog());
}

TEST_F(LoggerTest, ShouldLogInRightOrder) {
  EnableFeature();
  Logger logger;

  logger.Log(FROM_HERE, /*message=*/"first");
  logger.Log(FROM_HERE, /*message=*/"second");

  const std::string log = logger.GetHumanReadableLog();
  ASSERT_NE(std::string::npos, log.find("first"));
  ASSERT_NE(std::string::npos, log.find("second"));
  EXPECT_LT(log.find("first"), log.find("second"));
}

TEST_F(LoggerTest, ShouldNotResetLogWhenDumping) {
  EnableFeature();
  Logger logger;

  logger.Log(FROM_HERE, /*message=*/"first");
  logger.Log(FROM_HERE, /*message=*/"second");

  const std::string first_call = logger.GetHumanReadableLog();
  const std::string second_call = logger.GetHumanReadableLog();
  EXPECT_EQ(first_call, second_call);
}

TEST_F(LoggerTest, ShouldLimitLogItems) {
  EnableFeature();
  Logger logger;

  logger.Log(FROM_HERE, /*message=*/"first_event");
  for (int i = 0; i < kMaxItemsCount - 1; ++i) {
    logger.Log(FROM_HERE, /*message=*/"other_event");
  }
  // The first event still should be in the log.
  ASSERT_NE(std::string::npos,
            logger.GetHumanReadableLog().find("first_event"));

  logger.Log(FROM_HERE, /*message=*/"other_event");
  // The first event should not be in the log anymore.
  EXPECT_EQ(std::string::npos,
            logger.GetHumanReadableLog().find("first_event"));
}

}  // namespace ntp_snippets
