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

#include <libaddressinput/region_data_builder.h>

#include <libaddressinput/callback.h>
#include <libaddressinput/null_storage.h>
#include <libaddressinput/preload_supplier.h>
#include <libaddressinput/region_data.h>

#include <memory>
#include <string>

#include <gtest/gtest.h>

#include "testdata_source.h"

namespace {

using i18n::addressinput::BuildCallback;
using i18n::addressinput::NullStorage;
using i18n::addressinput::PreloadSupplier;
using i18n::addressinput::RegionData;
using i18n::addressinput::RegionDataBuilder;
using i18n::addressinput::TestdataSource;

class RegionDataBuilderTest : public testing::Test {
 public:
  RegionDataBuilderTest(const RegionDataBuilderTest&) = delete;
  RegionDataBuilderTest& operator=(const RegionDataBuilderTest&) = delete;

 protected:
  RegionDataBuilderTest()
      : supplier_(new TestdataSource(true),
                  new NullStorage),
        builder_(&supplier_),
        loaded_callback_(BuildCallback(this, &RegionDataBuilderTest::OnLoaded)),
        best_language_() {}

  PreloadSupplier supplier_;
  RegionDataBuilder builder_;
  const std::unique_ptr<const PreloadSupplier::Callback> loaded_callback_;
  std::string best_language_;

 private:
  void OnLoaded(bool success, const std::string& region_code, int num_rules) {
    ASSERT_TRUE(success);
    ASSERT_FALSE(region_code.empty());
    ASSERT_LT(0, num_rules);
    ASSERT_TRUE(supplier_.IsLoaded(region_code));
  }
};

TEST_F(RegionDataBuilderTest, BuildUsRegionTree) {
  supplier_.LoadRules("US", *loaded_callback_);
  const RegionData& tree = builder_.Build("US", "en-US", &best_language_);
  EXPECT_FALSE(tree.sub_regions().empty());
}

TEST_F(RegionDataBuilderTest, BuildCnRegionTree) {
  supplier_.LoadRules("CN", *loaded_callback_);
  const RegionData& tree = builder_.Build("CN", "zh-Hans", &best_language_);
  ASSERT_FALSE(tree.sub_regions().empty());
  EXPECT_FALSE(tree.sub_regions().front()->sub_regions().empty());
}

TEST_F(RegionDataBuilderTest, BuildChRegionTree) {
  supplier_.LoadRules("CH", *loaded_callback_);
  const RegionData& tree = builder_.Build("CH", "de-CH", &best_language_);
  // Although "CH" has information for its administrative divisions, the
  // administrative area field is not used, which results in an empty tree of
  // sub-regions.
  EXPECT_TRUE(tree.sub_regions().empty());
}

TEST_F(RegionDataBuilderTest, BuildZwRegionTree) {
  supplier_.LoadRules("ZW", *loaded_callback_);
  const RegionData& tree = builder_.Build("ZW", "en-ZW", &best_language_);
  EXPECT_TRUE(tree.sub_regions().empty());
}

TEST_F(RegionDataBuilderTest, UsTreeHasStateAbbreviationsAndNames) {
  supplier_.LoadRules("US", *loaded_callback_);
  const RegionData& tree = builder_.Build("US", "en-US", &best_language_);
  EXPECT_EQ("en", best_language_);
  ASSERT_FALSE(tree.sub_regions().empty());
  EXPECT_EQ("AL", tree.sub_regions().front()->key());
  EXPECT_EQ("Alabama", tree.sub_regions().front()->name());
}

TEST_F(RegionDataBuilderTest,
       KrWithKoLatnLanguageHasKoreanKeysAndLatinScriptNames) {
  supplier_.LoadRules("KR", *loaded_callback_);
  const RegionData& tree = builder_.Build("KR", "ko-Latn", &best_language_);
  EXPECT_EQ("ko-Latn", best_language_);
  ASSERT_FALSE(tree.sub_regions().empty());
  EXPECT_EQ(u8"강원도", tree.sub_regions().front()->key());
  EXPECT_EQ("Gangwon", tree.sub_regions().front()->name());
}

TEST_F(RegionDataBuilderTest, KrWithKoKrLanguageHasKoreanKeysAndNames) {
  supplier_.LoadRules("KR", *loaded_callback_);
  const RegionData& tree = builder_.Build("KR", "ko-KR", &best_language_);
  EXPECT_EQ("ko", best_language_);
  ASSERT_FALSE(tree.sub_regions().empty());
  EXPECT_EQ(u8"강원도", tree.sub_regions().front()->key());
  EXPECT_EQ(u8"강원", tree.sub_regions().front()->name());
}

}  // namespace
