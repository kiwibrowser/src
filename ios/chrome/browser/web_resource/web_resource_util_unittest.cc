// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/web_resource/web_resource_util.h"

#include <string>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/run_loop.h"
#include "base/strings/stringprintf.h"
#include "base/test/scoped_task_environment.h"
#include "base/values.h"
#include "ios/web/public/web_thread.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

namespace web_resource {

class WebResourceUtilTest : public PlatformTest {
 protected:
  WebResourceUtilTest() : error_called_(false), success_called_(false) {}
  ~WebResourceUtilTest() override {}

  WebResourceService::SuccessCallback GetSuccessCallback() {
    return base::Bind(&WebResourceUtilTest::OnParseSuccess,
                      base::Unretained(this));
  }

  WebResourceService::ErrorCallback GetErrorCallback() {
    return base::Bind(&WebResourceUtilTest::OnParseError,
                      base::Unretained(this));
  }

  // Called on success.
  void OnParseSuccess(std::unique_ptr<base::Value> value) {
    success_called_ = true;
    value_ = std::move(value);
  }

  // Called on error.
  void OnParseError(const std::string& error) {
    error_called_ = true;
    error_ = error;
  }

  void FlushBackgroundTasks() {
    // The function is not synchronous, callbacks are not called before flushing
    // the tasks.
    EXPECT_FALSE(success_called_);
    EXPECT_FALSE(error_called_);

    scoped_task_environment_.RunUntilIdle();
  }

  base::test::ScopedTaskEnvironment scoped_task_environment_;
  std::string error_;
  std::unique_ptr<base::Value> value_;
  bool error_called_;
  bool success_called_;
};

TEST_F(WebResourceUtilTest, Success) {
  const std::string kExpectedKey("foo");
  const std::string kExpectedValue("bar");
  std::string json = base::StringPrintf("{\"%s\":\"%s\"}", kExpectedKey.c_str(),
                                        kExpectedValue.c_str());
  GetIOSChromeParseJSONCallback().Run(json, GetSuccessCallback(),
                                      GetErrorCallback());

  FlushBackgroundTasks();

  // The success callback is called with the reference value.
  EXPECT_FALSE(error_called_);
  EXPECT_TRUE(success_called_);

  base::DictionaryValue* dictionary = nullptr;
  ASSERT_TRUE(value_->GetAsDictionary(&dictionary));
  EXPECT_EQ(1u, dictionary->size());
  base::Value* actual_value = nullptr;
  ASSERT_TRUE(dictionary->Get(kExpectedKey, &actual_value));
  std::string actual_value_as_string;
  EXPECT_TRUE(actual_value->GetAsString(&actual_value_as_string));
  EXPECT_EQ(kExpectedValue, actual_value_as_string);
}

// Only DictionartValues are expected.
TEST_F(WebResourceUtilTest, UnexpectedValue) {
  GetIOSChromeParseJSONCallback().Run("foo", GetSuccessCallback(),
                                      GetErrorCallback());

  FlushBackgroundTasks();

  // The error callback is called.
  EXPECT_TRUE(error_called_);
  EXPECT_FALSE(success_called_);
  EXPECT_FALSE(error_.empty());
}

// Empty data is not expected.
TEST_F(WebResourceUtilTest, EmptyValue) {
  GetIOSChromeParseJSONCallback().Run(std::string(), GetSuccessCallback(),
                                      GetErrorCallback());

  FlushBackgroundTasks();

  // The error callback is called.
  EXPECT_TRUE(error_called_);
  EXPECT_FALSE(success_called_);
  EXPECT_FALSE(error_.empty());
}

// Wrong syntax.
TEST_F(WebResourceUtilTest, SyntaxError) {
  GetIOSChromeParseJSONCallback().Run("%$[", GetSuccessCallback(),
                                      GetErrorCallback());

  FlushBackgroundTasks();

  // The error callback is called.
  EXPECT_TRUE(error_called_);
  EXPECT_FALSE(success_called_);
  EXPECT_FALSE(error_.empty());
}

}  // namespace web_resource
