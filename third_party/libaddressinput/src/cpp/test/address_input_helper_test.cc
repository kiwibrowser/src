// Copyright (C) 2013 Google Inc.
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

#include <libaddressinput/address_input_helper.h>

#include <libaddressinput/address_data.h>
#include <libaddressinput/callback.h>
#include <libaddressinput/null_storage.h>
#include <libaddressinput/preload_supplier.h>

#include <memory>
#include <string>
#include <utility>

#include <gtest/gtest.h>

#include "mock_source.h"
#include "testdata_source.h"

namespace {

using i18n::addressinput::AddressData;
using i18n::addressinput::AddressInputHelper;
using i18n::addressinput::BuildCallback;
using i18n::addressinput::MockSource;
using i18n::addressinput::NullStorage;
using i18n::addressinput::PreloadSupplier;
using i18n::addressinput::TestdataSource;

class AddressInputHelperTest : public testing::Test {
 public:
  AddressInputHelperTest(const AddressInputHelperTest&) = delete;
  AddressInputHelperTest& operator=(const AddressInputHelperTest&) = delete;

 protected:
  AddressInputHelperTest()
      // Our PreloadSupplier loads all data for a country at a time.
      : supplier_(new TestdataSource(true), new NullStorage),
        address_input_helper_(&supplier_),
        loaded_(BuildCallback(this, &AddressInputHelperTest::Loaded)) {}

  // Helper method to test FillAddress that ensures the PreloadSupplier has the
  // necessary data preloaded.
  void FillAddress(AddressData* address) {
    const std::string& region_code = address->region_code;
    if (!region_code.empty()) {
      supplier_.LoadRules(region_code, *loaded_);
    }
    address_input_helper_.FillAddress(address);
  }

 private:
  // Used to preload data that we need.
  void Loaded(bool success, const std::string&, int) { ASSERT_TRUE(success); }

  PreloadSupplier supplier_;
  const AddressInputHelper address_input_helper_;
  const std::unique_ptr<const PreloadSupplier::Callback> loaded_;
};

TEST_F(AddressInputHelperTest, AddressWithMissingPostalCode) {
  AddressData address;
  address.region_code = "CX";
  address.administrative_area = "WA";

  // There is only one postal code for Christmas Island
  AddressData expected = address;
  expected.postal_code = "6798";
  FillAddress(&address);
  EXPECT_EQ(expected, address);
}

TEST_F(AddressInputHelperTest, AddressWithPostalCodeMatchingAdmin) {
  AddressData address;
  address.region_code = "US";
  address.postal_code = "58098";
  // Other data should be left alone.
  address.address_line.push_back("10 High St");

  // North Dakota has post codes starting with 58.
  AddressData expected = address;
  expected.administrative_area = "ND";
  FillAddress(&address);
  EXPECT_EQ(expected, address);

  address.administrative_area = "CA";  // Override the admin area.
  // Now, since the admin area was already filled in, we don't fix it, even
  // though it was correct.
  expected.administrative_area = "CA";
  FillAddress(&address);
  EXPECT_EQ(expected, address);
}

TEST_F(AddressInputHelperTest, AddressWithPostalCodeMatchingLowerLevel) {
  AddressData address;
  address.region_code = "TW";
  address.postal_code = "53012";

  // This matches 二水鄉 - Ershuei Township.
  AddressData expected = address;
  // This locality is in 彰化縣 - Changhua County.
  expected.administrative_area = u8"彰化縣";
  expected.locality = u8"二水鄉";
  FillAddress(&address);
  EXPECT_EQ(expected, address);

  // Override the admin area.
  address.administrative_area = "Already filled in";
  expected.administrative_area = "Already filled in";
  address.locality = "";
  // However, the locality will still be filled in.
  FillAddress(&address);
  EXPECT_EQ(expected, address);
}

TEST_F(AddressInputHelperTest, AddressWithPostalCodeMatchingLowerLevelLatin) {
  AddressData address;
  address.region_code = "TW";
  address.postal_code = "53012";
  address.language_code = "zh-Latn";

  // This matches 二水鄉 - Ershuei Township.
  AddressData expected = address;
  // This locality is in 彰化縣 - Changhua County.
  expected.locality = "Ershuei Township";
  expected.administrative_area = "Changhua County";
  FillAddress(&address);
  EXPECT_EQ(expected, address);

  // Override the admin area.
  address.administrative_area = "Already filled in";
  expected.administrative_area = "Already filled in";
  address.locality = "";
  // However, the locality will still be filled in.
  FillAddress(&address);
  EXPECT_EQ(expected, address);
}

TEST_F(AddressInputHelperTest, AddressWithPostalCodeMatchingDependentLocality) {
  AddressData address;
  address.region_code = "KR";
  // This matches Danwon-gu district.
  address.postal_code = "425-111";

  AddressData expected = address;
  // The province is Gyeonggi - 경기도.
  expected.administrative_area = u8"경기도";
  // The city is Ansan-si - 안산시.
  expected.locality = u8"안산시";
  // The district is Danwon-gu - 단원구
  expected.dependent_locality = u8"단원구";

  FillAddress(&address);
  EXPECT_EQ(expected, address);

  AddressData address_ko_latn;
  address_ko_latn.region_code = "KR";
  address_ko_latn.postal_code = "425-111";
  address_ko_latn.language_code = "ko-Latn";

  expected = address_ko_latn;
  // The province is Gyeonggi - 경기도.
  expected.administrative_area = "Gyeonggi";
  // The city is Ansan-si - 안산시.
  expected.locality = "Ansan-si";
  // The district is Danwon-gu - 단원구
  expected.dependent_locality = "Danwon-gu";

  FillAddress(&address_ko_latn);
  EXPECT_EQ(expected, address_ko_latn);
}

TEST_F(AddressInputHelperTest, AddressWithPostalCodeMatchingMultipleValues) {
  AddressData address;
  address.region_code = "KR";
  // This matches Wando-gun and Ganjin-gun, both in Jeonnam province.
  address.postal_code = "527-111";

  AddressData expected = address;
  // The province, Jeonnam - 전라남도 - is known, but we have several locality
  // matches so none of them are populated.
  expected.administrative_area = u8"전라남도";
  FillAddress(&address);
  EXPECT_EQ(expected, address);
}

TEST_F(AddressInputHelperTest, AddressWithInvalidPostalCode) {
  AddressData address;
  address.postal_code = "970";
  address.region_code = "US";

  // We don't expect any changes, since the postal code couldn't be determined
  // as valid.
  AddressData expected = address;
  FillAddress(&address);
  EXPECT_EQ(expected, address);
}

TEST_F(AddressInputHelperTest, AddressWithNoPostalCodeValidation) {
  AddressData address;
  address.postal_code = "123";
  address.region_code = "GA";

  // We don't expect any changes, since the postal code couldn't be determined
  // as valid - we have no information about postal codes in Gabon (or even that
  // they are in use).
  AddressData expected = address;
  FillAddress(&address);
  EXPECT_EQ(expected, address);
}

TEST_F(AddressInputHelperTest, AddressWithInvalidOrMissingRegionCode) {
  AddressData address;
  address.postal_code = "XXX";
  address.administrative_area = "YYY";

  // We don't expect any changes, since there was no region code.
  AddressData expected = address;
  FillAddress(&address);
  EXPECT_EQ(expected, address);

  address.region_code = "XXXX";
  expected.region_code = "XXXX";
  // Again, nothing should change.
  FillAddress(&address);
  EXPECT_EQ(expected, address);
}

TEST_F(AddressInputHelperTest, RegionWithUnusedAdminAreaNames) {
  AddressData address;
  address.region_code = "CH";
  address.postal_code = "1111";
  address.language_code = "de-CH";

  // Administrative area should not be filled because it's not used. Locality
  // should not be filled because there's no data for it.
  AddressData expected = address;
  FillAddress(&address);
  EXPECT_EQ(expected, address);
}

class AddressInputHelperMockDataTest : public testing::Test {
 public:
  AddressInputHelperMockDataTest(
      const AddressInputHelperMockDataTest&) = delete;
  AddressInputHelperMockDataTest& operator=(
      const AddressInputHelperMockDataTest&) = delete;

 protected:
  AddressInputHelperMockDataTest()
      : source_(new MockSource),
        // Our PreloadSupplier loads all data for a country at a time.
        supplier_(source_, new NullStorage),
        address_input_helper_(&supplier_),
        loaded_(BuildCallback(this, &AddressInputHelperMockDataTest::Loaded)) {}

  // Helper method to test FillAddress that ensures the PreloadSupplier has the
  // necessary data preloaded.
  void FillAddress(AddressData* address) {
    const std::string& region_code = address->region_code;
    if (!region_code.empty()) {
      supplier_.LoadRules(region_code, *loaded_);
    }
    address_input_helper_.FillAddress(address);
  }

  MockSource* const source_;

 private:
  // Our mock source we assume will always succeed.
  void Loaded(bool success, const std::string&, int) { ASSERT_TRUE(success); }

  PreloadSupplier supplier_;
  const AddressInputHelper address_input_helper_;
  const std::unique_ptr<const PreloadSupplier::Callback> loaded_;
};

TEST_F(AddressInputHelperMockDataTest,
       PostalCodeSharedAcrossDifferentHierarchies) {
  // Note that this data is in the format of data that would be returned from
  // the aggregate server.
  source_->data_.insert(std::make_pair(
      // We use KR since we need a country where we format all fields down to
      // dependent locality, or the hierarchy won't be loaded.
      "data/KR",
      R"({"data/KR": )"
      // The top-level ZIP expression must be present for sub-key matches to be
      // evaluated.
      R"({"id":"data/KR", "sub_keys":"A~B", "zip":"\\d{5}"}, )"
      R"("data/KR/A": )"
      R"({"id":"data/KR/A", "sub_keys":"A1"}, )"
      R"("data/KR/A/A1": )"
      R"({"id":"data/KR/A/A1", "zip":"1"}, )"
      R"("data/KR/B": )"
      R"({"id":"data/KR/B", "sub_keys":"B1"}, )"
      R"("data/KR/B/B1": )"
      R"({"id":"data/KR/B/B1", "zip":"12"}})"));

  AddressData address;
  address.region_code = "KR";
  address.postal_code = "12345";
  address.administrative_area = "";

  AddressData expected = address;
  FillAddress(&address);
  // Nothing should have changed, since the ZIP code matches both of the cities,
  // and they aren't even in the same state.
  EXPECT_EQ(expected, address);
}

TEST_F(AddressInputHelperMockDataTest,
       PostalCodeSharedAcrossDifferentHierarchiesSameState) {
  // Create data where one state matches the ZIP code, but the other doesn't:
  // within the state which does, multiple cities and sub-cities match. The only
  // thing we can be certain of is therefore the state.
  source_->data_.insert(std::make_pair(
      // We use KR since we need a country where we format all fields down to
      // dependent locality, or the hierarchy won't be loaded.
      "data/KR",
      R"({"data/KR": )"
      // The top-level ZIP expression must be present for sub-key matches to be
      // evaluated.
      R"({"id":"data/KR", "sub_keys":"A~B", "zip":"\\d{5}"}, )"
      R"("data/KR/A": )"
      R"({"id":"data/KR/A", "sub_keys":"A1~A2"}, )"
      R"("data/KR/A/A1": )"
      R"({"id":"data/KR/A/A1", "sub_keys":"A1a", "zip":"1"}, )"
      // This key matches the ZIP code.
      R"("data/KR/A/A1/A1a": )"
      R"({"id":"data/KR/A/A1/A1a", "zip":"12"}, )"
      R"("data/KR/A/A2": )"
      R"({"id":"data/KR/A/A2", "sub_keys":"A2a", "zip":"1"}, )"
      // This key, also in state A, but in city A2, matches the ZIP code.
      R"("data/KR/A/A2/A2a": )"
      R"({"id":"data/KR/A/A2/A2a", "zip":"123"}, )"
      // This key, in state B, does not match the ZIP code.
      R"("data/KR/B": )"
      R"({"id":"data/KR/B", "zip":"2"}})"));

  AddressData address;
  address.region_code = "KR";
  address.postal_code = "12345";
  address.administrative_area = "";

  AddressData expected = address;
  expected.administrative_area = "A";
  FillAddress(&address);
  // The ZIP code matches multiple city districts and cities; but only one
  // state, so we fill this in.
  EXPECT_EQ(expected, address);
}

}  // namespace
