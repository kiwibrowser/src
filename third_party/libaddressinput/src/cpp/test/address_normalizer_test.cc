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

#include <libaddressinput/address_normalizer.h>

#include <libaddressinput/address_data.h>
#include <libaddressinput/callback.h>
#include <libaddressinput/null_storage.h>
#include <libaddressinput/preload_supplier.h>

#include <memory>
#include <string>

#include <gtest/gtest.h>

#include "testdata_source.h"

namespace {

using i18n::addressinput::AddressData;
using i18n::addressinput::AddressNormalizer;
using i18n::addressinput::BuildCallback;
using i18n::addressinput::NullStorage;
using i18n::addressinput::PreloadSupplier;
using i18n::addressinput::TestdataSource;

class AddressNormalizerTest : public testing::Test {
 public:
  AddressNormalizerTest(const AddressNormalizerTest&) = delete;
  AddressNormalizerTest& operator=(const AddressNormalizerTest&) = delete;

 protected:
  AddressNormalizerTest()
      : supplier_(new TestdataSource(true), new NullStorage),
        loaded_(BuildCallback(this, &AddressNormalizerTest::OnLoaded)),
        normalizer_(&supplier_) {}

  PreloadSupplier supplier_;
  const std::unique_ptr<const PreloadSupplier::Callback> loaded_;
  const AddressNormalizer normalizer_;

 private:
  void OnLoaded(bool success, const std::string& region_code, int num_rules) {
    ASSERT_TRUE(success);
    ASSERT_FALSE(region_code.empty());
    ASSERT_LT(0, num_rules);
  }
};

TEST_F(AddressNormalizerTest, CountryWithNoLanguageNoAdminArea) {
  // This test is to make sure that Normalize would not crash for the case where
  // there is neither a language, nor an admin area listed for the rule.
  supplier_.LoadRules("IR", *loaded_);
  i18n::addressinput::AddressData address;
  address.region_code = "IR";
  address.administrative_area = "Tehran";
  normalizer_.Normalize(&address);
  EXPECT_EQ("Tehran", address.administrative_area);
}

TEST_F(AddressNormalizerTest, BrazilAdminAreaAndLocality) {
  // A country with more than two levels of data
  supplier_.LoadRules("BR", *loaded_);
  i18n::addressinput::AddressData address;
  address.region_code = "BR";
  address.administrative_area = "Maranhão";
  address.locality = "Cantanhede";
  normalizer_.Normalize(&address);
  EXPECT_EQ("MA", address.administrative_area);  // For Maranhão
  EXPECT_EQ("Cantanhede", address.locality);
}

TEST_F(AddressNormalizerTest, FrenchCanadaNameLanguageNotConsistent) {
  supplier_.LoadRules("CA", *loaded_);
  i18n::addressinput::AddressData address;
  address.language_code = "en-CA";
  address.region_code = "CA";
  address.administrative_area = "Nouveau-Brunswick";
  normalizer_.Normalize(&address);
  // Normalize will look into every available language for that region,
  // not only the supplied or the default language.
  EXPECT_EQ("NB", address.administrative_area);
}

TEST_F(AddressNormalizerTest, FrenchCanadaName) {
  supplier_.LoadRules("CA", *loaded_);
  i18n::addressinput::AddressData address;
  address.language_code = "fr-CA";
  address.region_code = "CA";
  address.administrative_area = "Nouveau-Brunswick";
  normalizer_.Normalize(&address);
  EXPECT_EQ("NB", address.administrative_area);
}

TEST_F(AddressNormalizerTest, FrenchCanadaNameLanguageNotListed) {
  supplier_.LoadRules("CA", *loaded_);
  i18n::addressinput::AddressData address;
  address.language_code = "fa-CA";
  address.region_code = "CA";
  address.administrative_area = "Colombie-Britannique";
  normalizer_.Normalize(&address);
  EXPECT_EQ("BC", address.administrative_area);
}

TEST_F(AddressNormalizerTest, CaliforniaShortNameCa) {
  supplier_.LoadRules("US", *loaded_);
  AddressData address;
  address.language_code = "en-US";
  address.region_code = "US";
  address.administrative_area = "California";
  address.locality = "Mountain View";
  normalizer_.Normalize(&address);
  EXPECT_EQ("CA", address.administrative_area);
}

TEST_F(AddressNormalizerTest, CountryWithNonStandardData) {
  // This test is to make sure that Normalize would not crash for the case where
  // the data is not standard and key--language does not exist.
  supplier_.LoadRules("HK", *loaded_);
  i18n::addressinput::AddressData address;
  address.region_code = "HK";
  address.administrative_area = "香港島";
  normalizer_.Normalize(&address);
  EXPECT_EQ("香港島", address.administrative_area);
}

TEST_F(AddressNormalizerTest, GangwonLatinNameStaysUnchanged) {
  supplier_.LoadRules("KR", *loaded_);
  AddressData address;
  address.language_code = "ko-Latn";
  address.region_code = "KR";
  address.administrative_area = "Gangwon";
  normalizer_.Normalize(&address);
  EXPECT_EQ("Gangwon", address.administrative_area);
}

TEST_F(AddressNormalizerTest, GangwonKoreanName) {
  supplier_.LoadRules("KR", *loaded_);
  AddressData address;
  address.language_code = "ko-KR";
  address.region_code = "KR";
  address.administrative_area = u8"강원";
  normalizer_.Normalize(&address);
  EXPECT_EQ(u8"강원도", address.administrative_area);
}

TEST_F(AddressNormalizerTest, DontSwitchLatinScriptForUnknownLanguage) {
  supplier_.LoadRules("KR", *loaded_);
  AddressData address;
  address.region_code = "KR";
  address.administrative_area = "Gangwon";
  normalizer_.Normalize(&address);
  EXPECT_EQ("Gangwon", address.administrative_area);
}

TEST_F(AddressNormalizerTest, DontSwitchLocalScriptForUnknownLanguage) {
  supplier_.LoadRules("KR", *loaded_);
  AddressData address;
  address.region_code = "KR";
  address.administrative_area = u8"강원";
  normalizer_.Normalize(&address);
  EXPECT_EQ(u8"강원도", address.administrative_area);
}

}  // namespace
