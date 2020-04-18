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

#include <libaddressinput/region_data.h>

#include <cstddef>
#include <string>

#include <gtest/gtest.h>

namespace {

using i18n::addressinput::RegionData;

TEST(RegionDataTest, NoParentByDefault) {
  static const std::string kEmpty;
  RegionData region(kEmpty);
  EXPECT_FALSE(region.has_parent());
}

TEST(RegionDataTest, NoSubRegionsByDefault) {
  static const std::string kEmpty;
  RegionData region(kEmpty);
  EXPECT_TRUE(region.sub_regions().empty());
}

TEST(RegionDataTest, SubRegionGetsParent) {
  static const std::string kEmpty;
  RegionData region(kEmpty);
  region.AddSubRegion(kEmpty, kEmpty);
  ASSERT_EQ(1U, region.sub_regions().size());
  ASSERT_TRUE(region.sub_regions()[0] != nullptr);
  EXPECT_EQ(&region, &region.sub_regions()[0]->parent());
}

}  // namespace
