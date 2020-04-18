// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/libaddressinput/chromium/addressinput_util.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/libaddressinput/src/cpp/include/libaddressinput/address_data.h"

namespace autofill {
namespace addressinput {

using ::i18n::addressinput::AddressData;

TEST(AddressinputUtilTest, AddressRequiresRegionCode) {
  AddressData address;
  EXPECT_FALSE(HasAllRequiredFields(address));
}

TEST(AddressinputUtilTest, UsRequiresState) {
  AddressData address;
  address.region_code = "US";
  address.postal_code = "90291";
  // Leave state empty.
  address.locality = "Los Angeles";
  address.address_line.push_back("340 Main St.");
  EXPECT_FALSE(HasAllRequiredFields(address));
}

TEST(AddressinputUtilTest, CompleteAddressReturnsTrue) {
  AddressData address;
  address.region_code = "US";
  address.postal_code = "90291";
  address.administrative_area = "CA";
  address.locality = "Los Angeles";
  address.address_line.push_back("340 Main St.");
  EXPECT_TRUE(HasAllRequiredFields(address));
}

}  // namespace addressinput
}  // namespace autofill
