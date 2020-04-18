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

#include <libaddressinput/address_field.h>

#include <sstream>

#include <gtest/gtest.h>

namespace {

using i18n::addressinput::AddressField;
using i18n::addressinput::SORTING_CODE;

TEST(AddressFieldTest, ValidEnumValue) {
  std::ostringstream oss;
  oss << SORTING_CODE;
  EXPECT_EQ("SORTING_CODE", oss.str());
}

TEST(AddressFieldTest, InvalidEnumValue) {
  std::ostringstream oss;
  oss << static_cast<AddressField>(-42);
  EXPECT_EQ("[INVALID ENUM VALUE -42]", oss.str());
}

}  // namespace
