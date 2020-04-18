// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/activity_log_converter_strategy.h"

#include <memory>

#include "base/values.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "v8/include/v8.h"

namespace extensions {

class ActivityLogConverterStrategyTest : public testing::Test {
 public:
  ActivityLogConverterStrategyTest()
      : isolate_(v8::Isolate::GetCurrent()),
        handle_scope_(isolate_),
        context_(isolate_, v8::Context::New(isolate_)),
        context_scope_(context()) {}

 protected:
  void SetUp() override {
    converter_ = content::V8ValueConverter::Create();
    strategy_.reset(new ActivityLogConverterStrategy());
    converter_->SetFunctionAllowed(true);
    converter_->SetStrategy(strategy_.get());
  }

  testing::AssertionResult VerifyNull(v8::Local<v8::Value> v8_value) {
    std::unique_ptr<base::Value> value(
        converter_->FromV8Value(v8_value, context()));
    if (value->is_none())
      return testing::AssertionSuccess();
    return testing::AssertionFailure();
  }

  testing::AssertionResult VerifyBoolean(v8::Local<v8::Value> v8_value,
                                         bool expected) {
    bool out;
    std::unique_ptr<base::Value> value(
        converter_->FromV8Value(v8_value, context()));
    if (value->is_bool() && value->GetAsBoolean(&out) && out == expected)
      return testing::AssertionSuccess();
    return testing::AssertionFailure();
  }

  testing::AssertionResult VerifyInteger(v8::Local<v8::Value> v8_value,
                                         int expected) {
    int out;
    std::unique_ptr<base::Value> value(
        converter_->FromV8Value(v8_value, context()));
    if (value->is_int() && value->GetAsInteger(&out) && out == expected)
      return testing::AssertionSuccess();
    return testing::AssertionFailure();
  }

  testing::AssertionResult VerifyDouble(v8::Local<v8::Value> v8_value,
                                        double expected) {
    double out;
    std::unique_ptr<base::Value> value(
        converter_->FromV8Value(v8_value, context()));
    if (value->is_double() && value->GetAsDouble(&out) && out == expected)
      return testing::AssertionSuccess();
    return testing::AssertionFailure();
  }

  testing::AssertionResult VerifyString(v8::Local<v8::Value> v8_value,
                                        const std::string& expected) {
    std::string out;
    std::unique_ptr<base::Value> value(
        converter_->FromV8Value(v8_value, context()));
    if (value->is_string() && value->GetAsString(&out) && out == expected)
      return testing::AssertionSuccess();
    return testing::AssertionFailure();
  }

  v8::Local<v8::Context> context() const {
    return v8::Local<v8::Context>::New(isolate_, context_);
  }

  v8::Isolate* isolate_;
  v8::HandleScope handle_scope_;
  v8::Global<v8::Context> context_;
  v8::Context::Scope context_scope_;
  std::unique_ptr<content::V8ValueConverter> converter_;
  std::unique_ptr<ActivityLogConverterStrategy> strategy_;
};

TEST_F(ActivityLogConverterStrategyTest, ConversionTest) {
  const char* source = "(function() {"
      "function foo() {}"
      "return {"
        "null: null,"
        "true: true,"
        "false: false,"
        "positive_int: 42,"
        "negative_int: -42,"
        "zero: 0,"
        "double: 88.8,"
        "big_integral_double: 9007199254740992.0,"  // 2.0^53
        "string: \"foobar\","
        "empty_string: \"\","
        "dictionary: {"
          "foo: \"bar\","
          "hot: \"dog\","
        "},"
        "empty_dictionary: {},"
        "list: [ \"bar\", \"foo\" ],"
        "empty_list: [],"
        "function: (0, function() {}),"  // ensure function is anonymous
        "named_function: foo"
      "};"
      "})();";

  v8::MicrotasksScope microtasks(
      isolate_, v8::MicrotasksScope::kDoNotRunMicrotasks);
  v8::Local<v8::Script> script(
      v8::Script::Compile(context_.Get(isolate_),
                          v8::String::NewFromUtf8(isolate_, source))
          .ToLocalChecked());
  v8::Local<v8::Object> v8_object =
      script->Run(context_.Get(isolate_)).ToLocalChecked().As<v8::Object>();

  EXPECT_TRUE(VerifyString(v8_object, "[Object]"));
  EXPECT_TRUE(
      VerifyNull(v8_object->Get(v8::String::NewFromUtf8(isolate_, "null"))));
  EXPECT_TRUE(VerifyBoolean(
      v8_object->Get(v8::String::NewFromUtf8(isolate_, "true")), true));
  EXPECT_TRUE(VerifyBoolean(
      v8_object->Get(v8::String::NewFromUtf8(isolate_, "false")), false));
  EXPECT_TRUE(VerifyInteger(
      v8_object->Get(v8::String::NewFromUtf8(isolate_, "positive_int")), 42));
  EXPECT_TRUE(VerifyInteger(
      v8_object->Get(v8::String::NewFromUtf8(isolate_, "negative_int")), -42));
  EXPECT_TRUE(VerifyInteger(
      v8_object->Get(v8::String::NewFromUtf8(isolate_, "zero")), 0));
  EXPECT_TRUE(VerifyDouble(
      v8_object->Get(v8::String::NewFromUtf8(isolate_, "double")), 88.8));
  EXPECT_TRUE(VerifyDouble(
      v8_object->Get(v8::String::NewFromUtf8(isolate_, "big_integral_double")),
      9007199254740992.0));
  EXPECT_TRUE(VerifyString(
      v8_object->Get(v8::String::NewFromUtf8(isolate_, "string")), "foobar"));
  EXPECT_TRUE(VerifyString(
      v8_object->Get(v8::String::NewFromUtf8(isolate_, "empty_string")), ""));
  EXPECT_TRUE(VerifyString(
      v8_object->Get(v8::String::NewFromUtf8(isolate_, "dictionary")),
      "[Object]"));
  EXPECT_TRUE(VerifyString(
      v8_object->Get(v8::String::NewFromUtf8(isolate_, "empty_dictionary")),
      "[Object]"));
  EXPECT_TRUE(VerifyString(
      v8_object->Get(v8::String::NewFromUtf8(isolate_, "list")), "[Array]"));
  EXPECT_TRUE(VerifyString(
      v8_object->Get(v8::String::NewFromUtf8(isolate_, "empty_list")),
      "[Array]"));
  EXPECT_TRUE(VerifyString(
      v8_object->Get(v8::String::NewFromUtf8(isolate_, "function")),
      "[Function]"));
  EXPECT_TRUE(VerifyString(
      v8_object->Get(v8::String::NewFromUtf8(isolate_, "named_function")),
      "[Function foo()]"));
}

}  // namespace extensions
