// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp_base/error.h"

#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

namespace {

class Dummy {
 public:
  Dummy() = default;
  explicit Dummy(const std::string& x) : message(x) {}
  ~Dummy() = default;

  std::string message;
};

}  // namespace

namespace openscreen {

TEST(ErrorTest, TestDefaultError) {
  Error error;
  EXPECT_EQ(error, Error::None());
  EXPECT_EQ(error.message(), "");
}

TEST(ErrorTest, TestNonDefaultError) {
  Error error(Error::Code::kCborParsing, "Parse error");
  EXPECT_EQ(error.code(), Error::Code::kCborParsing);
  EXPECT_EQ(error.message(), "Parse error");

  Error error2(error);
  EXPECT_EQ(error2.code(), Error::Code::kCborParsing);
  EXPECT_EQ(error2.message(), "Parse error");

  Error error3 = error2;
  EXPECT_EQ(error, error2);
  EXPECT_EQ(error, error3);
  EXPECT_EQ(error2, error3);

  Error default_error;
  EXPECT_FALSE(error == default_error);
  EXPECT_FALSE(error2 == default_error);
  EXPECT_FALSE(error3 == default_error);

  Error error4(std::move(error2));
  Error error5 = std::move(error3);
  EXPECT_EQ(error, error4);
  EXPECT_EQ(error, error5);
}

TEST(ErrorOrTest, ErrorOrWithError) {
  ErrorOr<Dummy> error_or1(Error(Error::Code::kCborParsing, "Parse Error"));
  ErrorOr<Dummy> error_or2(Error::Code::kCborParsing);
  ErrorOr<Dummy> error_or3(Error::Code::kCborParsing, "Parse Error Again");

  EXPECT_FALSE(error_or1);
  EXPECT_FALSE(error_or1.is_value());
  EXPECT_TRUE(error_or1.is_error());
  EXPECT_EQ(error_or1.error().code(), Error::Code::kCborParsing);
  EXPECT_EQ(error_or1.error().message(), "Parse Error");

  EXPECT_FALSE(error_or2);
  EXPECT_FALSE(error_or2.is_value());
  EXPECT_TRUE(error_or2.is_error());
  EXPECT_EQ(error_or2.error().code(), Error::Code::kCborParsing);
  EXPECT_EQ(error_or2.error().message(), "");

  EXPECT_FALSE(error_or3);
  EXPECT_FALSE(error_or3.is_value());
  EXPECT_TRUE(error_or3.is_error());
  EXPECT_EQ(error_or3.error().code(), Error::Code::kCborParsing);
  EXPECT_EQ(error_or3.error().message(), "Parse Error Again");

  ErrorOr<Dummy> error_or4(std::move(error_or1));
  ErrorOr<Dummy> error_or5 = std::move(error_or3);

  EXPECT_FALSE(error_or4);
  EXPECT_FALSE(error_or4.is_value());
  EXPECT_TRUE(error_or4.is_error());
  EXPECT_EQ(error_or4.error().code(), Error::Code::kCborParsing);
  EXPECT_EQ(error_or4.error().message(), "Parse Error");

  EXPECT_FALSE(error_or5);
  EXPECT_FALSE(error_or5.is_value());
  EXPECT_TRUE(error_or5.is_error());
  EXPECT_EQ(error_or5.error().code(), Error::Code::kCborParsing);
  EXPECT_EQ(error_or5.error().message(), "Parse Error Again");
}

TEST(ErrorOrTest, ErrorOrWithValue) {
  ErrorOr<Dummy> error_or1(Dummy("Winterfell"));
  ErrorOr<Dummy> error_or2(Dummy("Riverrun"));

  EXPECT_TRUE(error_or1);
  EXPECT_TRUE(error_or1.is_value());
  EXPECT_FALSE(error_or1.is_error());
  EXPECT_EQ(error_or1.value().message, "Winterfell");
  EXPECT_EQ(error_or1.error(), Error::None());

  EXPECT_TRUE(error_or2);
  EXPECT_TRUE(error_or2.is_value());
  EXPECT_FALSE(error_or2.is_error());
  EXPECT_EQ(error_or2.value().message, "Riverrun");
  EXPECT_EQ(error_or2.error(), Error::None());

  ErrorOr<Dummy> error_or3(std::move(error_or1));
  ErrorOr<Dummy> error_or4 = std::move(error_or2);

  EXPECT_TRUE(error_or3);
  EXPECT_TRUE(error_or3.is_value());
  EXPECT_FALSE(error_or3.is_error());
  EXPECT_EQ(error_or3.value().message, "Winterfell");
  EXPECT_EQ(error_or3.error(), Error::None());

  EXPECT_TRUE(error_or4);
  EXPECT_TRUE(error_or4.is_value());
  EXPECT_FALSE(error_or4.is_error());
  EXPECT_EQ(error_or4.value().message, "Riverrun");
  EXPECT_EQ(error_or4.error(), Error::None());

  Dummy value = error_or4.MoveValue();
  EXPECT_EQ(value.message, "Riverrun");
}

}  // namespace openscreen
