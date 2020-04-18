// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/common/save_password_progress_logger.h"

#include <stddef.h>

#include <limits>

#include "base/bind.h"
#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "components/autofill/core/common/password_form.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using base::UTF8ToUTF16;

namespace autofill {

namespace {

const char kTestString[] = "Message";  // Corresponds to STRING_MESSAGE.

class TestLogger : public SavePasswordProgressLogger {
 public:
  bool LogsContainSubstring(const std::string& substring) {
    return accumulated_log_.find(substring) != std::string::npos;
  }

  std::string accumulated_log() { return accumulated_log_; }

 private:
  void SendLog(const std::string& log) override {
    accumulated_log_.append(log);
  }

  std::string accumulated_log_;
};

};  // namespace

TEST(SavePasswordProgressLoggerTest, LogPasswordForm) {
  TestLogger logger;
  PasswordForm form;
  form.action = GURL("http://example.org/verysecret?verysecret");
  form.password_element = UTF8ToUTF16("pwdelement");
  form.password_value = UTF8ToUTF16("verysecret");
  form.username_value = UTF8ToUTF16("verysecret");
  logger.LogPasswordForm(SavePasswordProgressLogger::STRING_MESSAGE, form);
  SCOPED_TRACE(testing::Message() << "Log string = ["
                                  << logger.accumulated_log() << "]");
  EXPECT_TRUE(logger.LogsContainSubstring(kTestString));
  EXPECT_TRUE(logger.LogsContainSubstring("pwdelement"));
  EXPECT_TRUE(logger.LogsContainSubstring("http://example.org"));
  EXPECT_FALSE(logger.LogsContainSubstring("verysecret"));
}

TEST(SavePasswordProgressLoggerTest, LogPasswordFormElementID) {
  // Test filtering element IDs.
  TestLogger logger;
  PasswordForm form;
  const std::string kHTMLInside("Username <script> element");
  const std::string kHTMLInsideExpected("Username__script__element");
  const std::string kIPAddressInside("y128.0.0.1Y");
  const std::string kIPAddressInsideExpected("y128_0_0_1Y");
  const std::string kSpecialCharsInside("X@#a$%B&*c()D;:e+!x");
  const std::string kSpecialCharsInsideExpected("X__a__B__c__D__e__x");
  form.username_element = UTF8ToUTF16(kHTMLInside);
  form.password_element = UTF8ToUTF16(kIPAddressInside);
  form.new_password_element = UTF8ToUTF16(kSpecialCharsInside);
  logger.LogPasswordForm(SavePasswordProgressLogger::STRING_MESSAGE, form);
  SCOPED_TRACE(testing::Message() << "Log string = ["
                                  << logger.accumulated_log() << "]");
  EXPECT_TRUE(logger.LogsContainSubstring(kTestString));
  EXPECT_FALSE(logger.LogsContainSubstring(kHTMLInside));
  EXPECT_TRUE(logger.LogsContainSubstring(kHTMLInsideExpected));
  EXPECT_FALSE(logger.LogsContainSubstring(kIPAddressInside));
  EXPECT_TRUE(logger.LogsContainSubstring(kIPAddressInsideExpected));
  EXPECT_FALSE(logger.LogsContainSubstring(kSpecialCharsInside));
  EXPECT_TRUE(logger.LogsContainSubstring(kSpecialCharsInsideExpected));
}

TEST(SavePasswordProgressLoggerTest, LogHTMLForm) {
  TestLogger logger;
  logger.LogHTMLForm(SavePasswordProgressLogger::STRING_MESSAGE,
                     "form_name",
                     GURL("http://example.org/verysecret?verysecret"));
  SCOPED_TRACE(testing::Message() << "Log string = ["
                                  << logger.accumulated_log() << "]");
  EXPECT_TRUE(logger.LogsContainSubstring(kTestString));
  EXPECT_TRUE(logger.LogsContainSubstring("form_name"));
  EXPECT_TRUE(logger.LogsContainSubstring("http://example.org"));
  EXPECT_FALSE(logger.LogsContainSubstring("verysecret"));
}

TEST(SavePasswordProgressLoggerTest, LogURL) {
  TestLogger logger;
  logger.LogURL(SavePasswordProgressLogger::STRING_MESSAGE,
                GURL("http://example.org/verysecret?verysecret"));
  SCOPED_TRACE(testing::Message() << "Log string = ["
                                  << logger.accumulated_log() << "]");
  EXPECT_TRUE(logger.LogsContainSubstring(kTestString));
  EXPECT_TRUE(logger.LogsContainSubstring("http://example.org"));
  EXPECT_FALSE(logger.LogsContainSubstring("verysecret"));
}

TEST(SavePasswordProgressLoggerTest, LogBooleanTrue) {
  TestLogger logger;
  logger.LogBoolean(SavePasswordProgressLogger::STRING_MESSAGE, true);
  SCOPED_TRACE(testing::Message() << "Log string = ["
                                  << logger.accumulated_log() << "]");
  EXPECT_TRUE(logger.LogsContainSubstring(kTestString));
  EXPECT_TRUE(logger.LogsContainSubstring("true"));
}

TEST(SavePasswordProgressLoggerTest, LogBooleanFalse) {
  TestLogger logger;
  logger.LogBoolean(SavePasswordProgressLogger::STRING_MESSAGE, false);
  SCOPED_TRACE(testing::Message() << "Log string = ["
                                  << logger.accumulated_log() << "]");
  EXPECT_TRUE(logger.LogsContainSubstring(kTestString));
  EXPECT_TRUE(logger.LogsContainSubstring("false"));
}

TEST(SavePasswordProgressLoggerTest, LogSignedNumber) {
  TestLogger logger;
  int signed_number = -12345;
  logger.LogNumber(SavePasswordProgressLogger::STRING_MESSAGE, signed_number);
  SCOPED_TRACE(testing::Message() << "Log string = ["
                                  << logger.accumulated_log() << "]");
  EXPECT_TRUE(logger.LogsContainSubstring(kTestString));
  EXPECT_TRUE(logger.LogsContainSubstring("-12345"));
}

TEST(SavePasswordProgressLoggerTest, LogUnsignedNumber) {
  TestLogger logger;
  size_t unsigned_number = 654321;
  logger.LogNumber(SavePasswordProgressLogger::STRING_MESSAGE, unsigned_number);
  SCOPED_TRACE(testing::Message() << "Log string = ["
                                  << logger.accumulated_log() << "]");
  EXPECT_TRUE(logger.LogsContainSubstring(kTestString));
  EXPECT_TRUE(logger.LogsContainSubstring("654321"));
}

TEST(SavePasswordProgressLoggerTest, LogMessage) {
  TestLogger logger;
  logger.LogMessage(SavePasswordProgressLogger::STRING_MESSAGE);
  SCOPED_TRACE(testing::Message() << "Log string = ["
                                  << logger.accumulated_log() << "]");
  EXPECT_TRUE(logger.LogsContainSubstring(kTestString));
}

// Test that none of the strings associated to string IDs contain the '.'
// character.
TEST(SavePasswordProgressLoggerTest, NoFullStops) {
  for (int id = 0; id < SavePasswordProgressLogger::STRING_MAX; ++id) {
    TestLogger logger;
    logger.LogMessage(static_cast<SavePasswordProgressLogger::StringID>(id));
    EXPECT_FALSE(logger.LogsContainSubstring("."))
        << "Log string = [" << logger.accumulated_log() << "]";
  }
}

}  // namespace autofill
