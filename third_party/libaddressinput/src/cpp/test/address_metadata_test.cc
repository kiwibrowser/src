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

#include <libaddressinput/address_metadata.h>

#include <libaddressinput/address_field.h>

#include <gtest/gtest.h>

namespace {

using i18n::addressinput::IsFieldRequired;
using i18n::addressinput::IsFieldUsed;

using i18n::addressinput::COUNTRY;
using i18n::addressinput::ADMIN_AREA;
using i18n::addressinput::DEPENDENT_LOCALITY;

TEST(AddressMetadataTest, IsFieldRequiredCountry) {
  EXPECT_TRUE(IsFieldRequired(COUNTRY, "US"));
  EXPECT_TRUE(IsFieldRequired(COUNTRY, "CH"));
  EXPECT_TRUE(IsFieldRequired(COUNTRY, "rrr"));
}

TEST(AddressMetadataTest, IsUsedRequiredCountry) {
  EXPECT_TRUE(IsFieldUsed(COUNTRY, "US"));
  EXPECT_TRUE(IsFieldUsed(COUNTRY, "CH"));
  EXPECT_TRUE(IsFieldUsed(COUNTRY, "rrr"));
}

TEST(AddressMetadataTest, IsFieldRequiredAdminAreaUS) {
  EXPECT_TRUE(IsFieldRequired(ADMIN_AREA, "US"));
}

TEST(AddressMetadataTest, IsFieldRequiredAdminAreaAT) {
  EXPECT_FALSE(IsFieldRequired(ADMIN_AREA, "AT"));
}

TEST(AddressMetadataTest, IsFieldRequiredAdminAreaSU) {
  // Unsupported region.
  EXPECT_FALSE(IsFieldRequired(ADMIN_AREA, "SU"));
}

TEST(AddressMetadataTest, IsFieldUsedDependentLocalityUS) {
  EXPECT_FALSE(IsFieldUsed(DEPENDENT_LOCALITY, "US"));
}

TEST(AddressMetadataTest, IsFieldUsedDependentLocalityCN) {
  EXPECT_TRUE(IsFieldUsed(DEPENDENT_LOCALITY, "CN"));
}

TEST(AddressMetadataTest, IsFieldUsedDependentLocalitySU) {
  // Unsupported region.
  EXPECT_FALSE(IsFieldUsed(DEPENDENT_LOCALITY, "SU"));
}

}  // namespace
