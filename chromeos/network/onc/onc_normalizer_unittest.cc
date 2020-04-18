// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/network/onc/onc_normalizer.h"

#include <memory>

#include "base/values.h"
#include "chromeos/network/onc/onc_signature.h"
#include "chromeos/network/onc/onc_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {
namespace onc {

// Validate that an irrelevant StaticIPConfig dictionary will be removed.
TEST(ONCNormalizerTest, RemoveStaticIPConfig) {
  Normalizer normalizer(true);
  std::unique_ptr<const base::DictionaryValue> data(
      test_utils::ReadTestDictionary("settings_with_normalization.json"));

  const base::DictionaryValue* original = NULL;
  const base::DictionaryValue* expected_normalized = NULL;
  data->GetDictionary("irrelevant-staticipconfig", &original);
  data->GetDictionary("irrelevant-staticipconfig-normalized",
                      &expected_normalized);

  std::unique_ptr<base::DictionaryValue> actual_normalized =
      normalizer.NormalizeObject(&kNetworkConfigurationSignature, *original);
  EXPECT_TRUE(test_utils::Equals(expected_normalized, actual_normalized.get()));
}

// This test case is about validating valid ONC objects.
TEST(ONCNormalizerTest, NormalizeNetworkConfigurationEthernetAndVPN) {
  Normalizer normalizer(true);
  std::unique_ptr<const base::DictionaryValue> data(
      test_utils::ReadTestDictionary("settings_with_normalization.json"));

  const base::DictionaryValue* original = NULL;
  const base::DictionaryValue* expected_normalized = NULL;
  data->GetDictionary("ethernet-and-vpn", &original);
  data->GetDictionary("ethernet-and-vpn-normalized", &expected_normalized);

  std::unique_ptr<base::DictionaryValue> actual_normalized =
      normalizer.NormalizeObject(&kNetworkConfigurationSignature, *original);
  EXPECT_TRUE(test_utils::Equals(expected_normalized, actual_normalized.get()));
}

// This test case is about validating valid ONC objects.
TEST(ONCNormalizerTest, NormalizeNetworkConfigurationWifi) {
  Normalizer normalizer(true);
  std::unique_ptr<const base::DictionaryValue> data(
      test_utils::ReadTestDictionary("settings_with_normalization.json"));

  const base::DictionaryValue* original = NULL;
  const base::DictionaryValue* expected_normalized = NULL;
  data->GetDictionary("wifi", &original);
  data->GetDictionary("wifi-normalized", &expected_normalized);

  std::unique_ptr<base::DictionaryValue> actual_normalized =
      normalizer.NormalizeObject(&kNetworkConfigurationSignature, *original);
  EXPECT_TRUE(test_utils::Equals(expected_normalized, actual_normalized.get()));
}

}  // namespace onc
}  // namespace chromeos
