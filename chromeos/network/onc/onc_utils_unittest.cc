// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/network/onc/onc_utils.h"

#include <string>

#include "base/json/json_file_value_serializer.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "chromeos/chromeos_test_utils.h"
#include "chromeos/network/network_ui_data.h"
#include "chromeos/network/onc/onc_signature.h"
#include "chromeos/network/onc/onc_test_utils.h"
#include "chromeos/tools/variable_expander.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {

namespace {

std::unique_ptr<base::Value> ReadTestJson(const std::string& filename) {
  base::FilePath path;
  std::unique_ptr<base::Value> result;
  if (!test_utils::GetTestDataPath("network", filename, &path)) {
    NOTREACHED() << "Unable to get test file path for: " << filename;
    return result;
  }
  JSONFileValueDeserializer deserializer(path,
                                         base::JSON_ALLOW_TRAILING_COMMAS);
  std::string error_message;
  result = deserializer.Deserialize(nullptr, &error_message);
  CHECK(result != nullptr) << "Couldn't json-deserialize file: " << filename
                           << ": " << error_message;
  return result;
}

}  // namespace

namespace onc {

TEST(ONCDecrypterTest, BrokenEncryptionIterations) {
  std::unique_ptr<base::DictionaryValue> encrypted_onc =
      test_utils::ReadTestDictionary("broken-encrypted-iterations.onc");

  std::unique_ptr<base::DictionaryValue> decrypted_onc =
      Decrypt("test0000", *encrypted_onc);

  EXPECT_EQ(NULL, decrypted_onc.get());
}

TEST(ONCDecrypterTest, BrokenEncryptionZeroIterations) {
  std::unique_ptr<base::DictionaryValue> encrypted_onc =
      test_utils::ReadTestDictionary("broken-encrypted-zero-iterations.onc");

  std::string error;
  std::unique_ptr<base::DictionaryValue> decrypted_onc =
      Decrypt("test0000", *encrypted_onc);

  EXPECT_EQ(NULL, decrypted_onc.get());
}

TEST(ONCDecrypterTest, LoadEncryptedOnc) {
  std::unique_ptr<base::DictionaryValue> encrypted_onc =
      test_utils::ReadTestDictionary("encrypted.onc");
  std::unique_ptr<base::DictionaryValue> expected_decrypted_onc =
      test_utils::ReadTestDictionary("decrypted.onc");

  std::string error;
  std::unique_ptr<base::DictionaryValue> actual_decrypted_onc =
      Decrypt("test0000", *encrypted_onc);

  base::DictionaryValue emptyDict;
  EXPECT_TRUE(test_utils::Equals(expected_decrypted_onc.get(),
                                 actual_decrypted_onc.get()));
}

namespace {

const char* kLoginId = "hans";
const char* kLoginEmail = "hans@my.domain.com";

std::map<std::string, std::string> GetTestStringSubstutions() {
  std::map<std::string, std::string> substitutions;
  substitutions[::onc::substitutes::kLoginID] = kLoginId;
  substitutions[::onc::substitutes::kLoginEmail] = kLoginEmail;
  return substitutions;
}

}  // namespace

TEST(ONCStringExpansion, OpenVPN) {
  std::unique_ptr<base::DictionaryValue> vpn_onc =
      test_utils::ReadTestDictionary("valid_openvpn.onc");

  VariableExpander variable_expander(GetTestStringSubstutions());
  ExpandStringsInOncObject(kNetworkConfigurationSignature, variable_expander,
                           vpn_onc.get());

  std::string actual_expanded;
  vpn_onc->GetString("VPN.OpenVPN.Username", &actual_expanded);
  EXPECT_EQ(actual_expanded, std::string("abc ") + kLoginEmail + " def");
}

TEST(ONCStringExpansion, WiFi_EAP) {
  std::unique_ptr<base::DictionaryValue> wifi_onc =
      test_utils::ReadTestDictionary("wifi_clientcert_with_cert_pems.onc");

  VariableExpander variable_expander(GetTestStringSubstutions());
  ExpandStringsInOncObject(kNetworkConfigurationSignature, variable_expander,
                           wifi_onc.get());

  std::string actual_expanded;
  wifi_onc->GetString("WiFi.EAP.Identity", &actual_expanded);
  EXPECT_EQ(actual_expanded, std::string("abc ") + kLoginId + "@my.domain.com");
}

TEST(ONCResolveServerCertRefs, ResolveServerCertRefs) {
  std::unique_ptr<base::DictionaryValue> test_cases =
      test_utils::ReadTestDictionary(
          "network_configs_with_resolved_certs.json");

  CertPEMsByGUIDMap certs;
  certs["cert_google"] = "pem_google";
  certs["cert_webkit"] = "pem_webkit";

  for (base::DictionaryValue::Iterator it(*test_cases);
       !it.IsAtEnd(); it.Advance()) {
    SCOPED_TRACE("Test case: " + it.key());

    const base::DictionaryValue* test_case = NULL;
    it.value().GetAsDictionary(&test_case);

    const base::ListValue* networks_with_cert_refs = NULL;
    test_case->GetList("WithCertRefs", &networks_with_cert_refs);

    const base::ListValue* expected_resolved_onc = NULL;
    test_case->GetList("WithResolvedRefs", &expected_resolved_onc);

    bool expected_success = (networks_with_cert_refs->GetSize() ==
                             expected_resolved_onc->GetSize());

    std::unique_ptr<base::ListValue> actual_resolved_onc(
        networks_with_cert_refs->DeepCopy());

    bool success = ResolveServerCertRefsInNetworks(certs,
                                                   actual_resolved_onc.get());
    EXPECT_EQ(expected_success, success);
    EXPECT_TRUE(test_utils::Equals(expected_resolved_onc,
                                   actual_resolved_onc.get()));
  }
}

TEST(ONCUtils, ProxySettingsToProxyConfig) {
  std::unique_ptr<base::Value> test_data(ReadTestJson("proxy_config.json"));

  base::ListValue* tests1;
  test_data->GetAsList(&tests1);
  ASSERT_TRUE(tests1);

  std::unique_ptr<base::ListValue> list_of_tests = tests1->CreateDeepCopy();

  // Additional ONC -> ProxyConfig test cases to test fixup.
  test_data = ReadTestJson("proxy_config_from_onc.json");
  base::ListValue* tests2;
  test_data->GetAsList(&tests2);
  ASSERT_TRUE(tests2);
  for (auto iter1 = tests2->begin(); iter1 != tests2->end(); ++iter1)
    list_of_tests->Append(iter1->CreateDeepCopy());

  int index = 0;
  for (auto iter2 = list_of_tests->begin(); iter2 != list_of_tests->end();
       ++iter2, ++index) {
    SCOPED_TRACE("Test case #" + base::IntToString(index));

    base::DictionaryValue* test_case = nullptr;
    iter2->GetAsDictionary(&test_case);
    ASSERT_TRUE(test_case);

    base::DictionaryValue* onc_proxy_settings;
    test_case->GetDictionary("ONC_ProxySettings", &onc_proxy_settings);

    base::DictionaryValue* expected_proxy_config;
    test_case->GetDictionary("ProxyConfig", &expected_proxy_config);

    std::unique_ptr<base::DictionaryValue> actual_proxy_config =
        ConvertOncProxySettingsToProxyConfig(*onc_proxy_settings);
    EXPECT_TRUE(
        test_utils::Equals(expected_proxy_config, actual_proxy_config.get()));
  }
}

TEST(ONCUtils, ProxyConfigToOncProxySettings) {
  std::unique_ptr<base::Value> test_data(ReadTestJson("proxy_config.json"));

  base::ListValue* list_of_tests;
  test_data->GetAsList(&list_of_tests);
  ASSERT_TRUE(list_of_tests);

  int index = 0;
  for (base::ListValue::iterator it = list_of_tests->begin();
       it != list_of_tests->end(); ++it, ++index) {
    SCOPED_TRACE("Test case #" + base::IntToString(index));

    base::DictionaryValue* test_case;
    it->GetAsDictionary(&test_case);

    base::DictionaryValue* shill_proxy_config;
    test_case->GetDictionary("ProxyConfig", &shill_proxy_config);

    base::DictionaryValue* onc_proxy_settings;
    test_case->GetDictionary("ONC_ProxySettings", &onc_proxy_settings);

    std::unique_ptr<base::DictionaryValue> actual_proxy_settings =
        ConvertProxyConfigToOncProxySettings(
            shill_proxy_config->CreateDeepCopy());
    EXPECT_TRUE(
        test_utils::Equals(onc_proxy_settings, actual_proxy_settings.get()));
  }
}

TEST(ONCPasswordVariable, PasswordAvailable) {
  const auto wifi_onc = test_utils::ReadTestDictionary(
      "wifi_eap_ttls_with_password_variable.onc");

  EXPECT_TRUE(HasUserPasswordSubsitutionVariable(kNetworkConfigurationSignature,
                                                 wifi_onc.get()));
}

TEST(ONCPasswordVariable, PasswordNotAvailable) {
  const auto wifi_onc = test_utils::ReadTestDictionary("wifi_eap_ttls.onc");

  EXPECT_FALSE(HasUserPasswordSubsitutionVariable(
      kNetworkConfigurationSignature, wifi_onc.get()));
}

TEST(ONCPasswordVariable, PasswordHarcdoded) {
  const auto wifi_onc = test_utils::ReadTestDictionary(
      "wifi_eap_ttls_with_hardcoded_password.onc");

  EXPECT_FALSE(HasUserPasswordSubsitutionVariable(
      kNetworkConfigurationSignature, wifi_onc.get()));
}

TEST(ONCPasswordVariable, MultipleNetworksPasswordAvailable) {
  const auto network_dictionary = test_utils::ReadTestDictionary(
      "managed_toplevel_with_password_variable.onc");

  const auto network_list = std::make_unique<base::ListValue>(base::ListValue(
      network_dictionary->FindKey("NetworkConfigurations")->GetList()));

  EXPECT_TRUE(HasUserPasswordSubsitutionVariable(network_list.get()));
}

TEST(ONCPasswordVariable, MultipleNetworksPasswordNotAvailable) {
  const auto network_dictionary = test_utils::ReadTestDictionary(
      "managed_toplevel_with_no_password_variable.onc");

  const auto network_list = std::make_unique<base::ListValue>(base::ListValue(
      network_dictionary->FindKey("NetworkConfigurations")->GetList()));

  EXPECT_FALSE(HasUserPasswordSubsitutionVariable(network_list.get()));
}

}  // namespace onc
}  // namespace chromeos
