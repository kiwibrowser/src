// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/data_decoder/public/cpp/json_sanitizer.h"

#include <memory>

#include "base/bind.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/values.h"
#include "build/build_config.h"
#include "services/data_decoder/public/cpp/safe_json_parser.h"
#include "testing/gtest/include/gtest/gtest.h"

#if !defined(OS_ANDROID)
#include "services/data_decoder/public/cpp/testing_json_parser.h"
#endif

namespace data_decoder {

class JsonSanitizerTest : public ::testing::Test {
 public:
  void TearDown() override {
    // Flush any tasks from the message loop to avoid leaks.
    base::RunLoop().RunUntilIdle();
  }

 protected:
  void CheckSuccess(const std::string& json);
  void CheckError(const std::string& json);

 private:
  enum class State {
    // ERROR is a #define on Windows, so we prefix the values with STATE_.
    STATE_IDLE,
    STATE_SUCCESS,
    STATE_ERROR,
  };

  void Sanitize(const std::string& json);

  void OnSuccess(const std::string& json);
  void OnError(const std::string& error);

  base::MessageLoop message_loop_;

#if !defined(OS_ANDROID)
  TestingJsonParser::ScopedFactoryOverride factory_override_;
#endif

  std::string result_;
  std::string error_;
  State state_;

  std::unique_ptr<base::RunLoop> run_loop_;
};

void JsonSanitizerTest::CheckSuccess(const std::string& json) {
  SCOPED_TRACE(json);
  Sanitize(json);
  std::unique_ptr<base::Value> parsed = base::JSONReader::Read(json);
  ASSERT_TRUE(parsed);
  EXPECT_EQ(State::STATE_SUCCESS, state_) << "Error: " << error_;

  // The JSON parser should accept the result.
  int error_code;
  std::string error;
  std::unique_ptr<base::Value> reparsed = base::JSONReader::ReadAndReturnError(
      result_, base::JSON_PARSE_RFC, &error_code, &error);
  EXPECT_TRUE(reparsed) << "Invalid result: " << error;

  // The parsed values should be equal.
  EXPECT_TRUE(reparsed->Equals(parsed.get()));
}

void JsonSanitizerTest::CheckError(const std::string& json) {
  SCOPED_TRACE(json);
  Sanitize(json);
  EXPECT_EQ(State::STATE_ERROR, state_) << "Result: " << result_;
}

void JsonSanitizerTest::Sanitize(const std::string& json) {
  state_ = State::STATE_IDLE;
  result_.clear();
  error_.clear();
  run_loop_.reset(new base::RunLoop);
  JsonSanitizer::Sanitize(
      nullptr, json,
      base::Bind(&JsonSanitizerTest::OnSuccess, base::Unretained(this)),
      base::Bind(&JsonSanitizerTest::OnError, base::Unretained(this)));

  // We should never get a result immediately.
  EXPECT_EQ(State::STATE_IDLE, state_);
  run_loop_->Run();
}

void JsonSanitizerTest::OnSuccess(const std::string& json) {
  ASSERT_EQ(State::STATE_IDLE, state_);
  state_ = State::STATE_SUCCESS;
  result_ = json;
  run_loop_->Quit();
}

void JsonSanitizerTest::OnError(const std::string& error) {
  ASSERT_EQ(State::STATE_IDLE, state_);
  state_ = State::STATE_ERROR;
  error_ = error;
  run_loop_->Quit();
}

TEST_F(JsonSanitizerTest, Json) {
  // Valid JSON:
  CheckSuccess("{\n  \"foo\": \"bar\"\n}");
  CheckSuccess("[true]");
  CheckSuccess("[42]");
  CheckSuccess("[3.14]");
  CheckSuccess("[4.0]");
  CheckSuccess("[null]");
  CheckSuccess("[\"foo\", \"bar\"]");

  // JSON syntax errors:
  CheckError("");
  CheckError("[");
  CheckError("null");

  // Unterminated array.
  CheckError("[1,2,3,]");
}

TEST_F(JsonSanitizerTest, Nesting) {
  // 199 nested arrays are fine.
  std::string nested(199u, '[');
  nested.append(199u, ']');
  CheckSuccess(nested);

  // 200 nested arrays is too much.
  CheckError(std::string(200u, '[') + std::string(200u, ']'));
}

TEST_F(JsonSanitizerTest, Unicode) {
  // Non-ASCII characters encoded either directly as UTF-8 or escaped as UTF-16:
  CheckSuccess("[\"☃\"]");
  CheckSuccess("[\"\\u2603\"]");
  CheckSuccess("[\"😃\"]");
  CheckSuccess("[\"\\ud83d\\ude03\"]");

  // Malformed UTF-8:
  // A continuation byte outside of a sequence.
  CheckError("[\"\x80\"]");

  // A start byte that is missing a continuation byte.
  CheckError("[\"\xc0\"]");

  // An invalid byte in UTF-8.
  CheckError("[\"\xfe\"]");

  // An overlong encoding (of the letter 'A').
  CheckError("[\"\xc1\x81\"]");

  // U+D83D, a code point reserved for (high) surrogates.
  CheckError("[\"\xed\xa0\xbd\"]");

  // U+4567890, a code point outside of the valid range for Unicode.
  CheckError("[\"\xfc\x84\x95\xa7\xa2\x90\"]");

  // Malformed escaped UTF-16:
  // An unmatched high surrogate.
  CheckError("[\"\\ud83d\"]");

  // An unmatched low surrogate.
  CheckError("[\"\\ude03\"]");

  // A low surrogate followed by a high surrogate.
  CheckError("[\"\\ude03\\ud83d\"]");

  // Valid escaped UTF-16 that encodes non-characters:
  CheckError("[\"\\ufdd0\"]");
  CheckError("[\"\\ufffe\"]");
  CheckError("[\"\\ud83f\\udffe\"]");
}

}  // namespace data_decoder
