// Copyright (C) 2014 Google Inc.
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

#include "format_element.h"

#include <libaddressinput/address_field.h>

#include <sstream>

#include <gtest/gtest.h>

namespace {

using i18n::addressinput::FormatElement;
using i18n::addressinput::SORTING_CODE;

TEST(FormatElementTest, StreamFunctionNewline) {
  std::ostringstream oss;
  oss << FormatElement();
  EXPECT_EQ("Newline", oss.str());
}

TEST(FormatElementTest, StreamFunctionLiteral) {
  std::ostringstream oss;
  oss << FormatElement("Text");
  EXPECT_EQ("Literal: Text", oss.str());
}

TEST(FormatElementTest, StreamFunctionField) {
  std::ostringstream oss;
  oss << FormatElement(SORTING_CODE);
  EXPECT_EQ("Field: SORTING_CODE", oss.str());
}

TEST(FormatElementTest, IsNewline) {
  EXPECT_TRUE(FormatElement().IsNewline());
  EXPECT_FALSE(FormatElement(" ").IsNewline());
  EXPECT_FALSE(FormatElement(SORTING_CODE).IsNewline());
}

TEST(FormatElementTest, IsField) {
  EXPECT_FALSE(FormatElement().IsField());
  EXPECT_FALSE(FormatElement(" ").IsField());
  EXPECT_TRUE(FormatElement(SORTING_CODE).IsField());
}

}  // namespace
